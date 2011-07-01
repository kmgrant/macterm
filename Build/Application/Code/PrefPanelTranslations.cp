/*###############################################################

	PrefPanelTranslations.cp
	
	MacTerm
		© 1998-2011 by Kevin Grant.
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
#include <CocoaBasic.h>
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
#include "SpacingConstants.r"

// application includes
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
namespace {

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanels.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyLabelBaseTranslationTable			= { 'LBTT', 0/* ID */ };
HIViewID const	idMyLabelBaseTranslationTableSelection	= { 'SBTT', 0/* ID */ };
HIViewID const	idMyDataBrowserBaseTranslationTable		= { 'Tran', 0/* ID */ };
HIViewID const	idMyLabelOptions						= { 'LOpt', 0/* ID */ };
HIViewID const	idMyCheckBoxUseBackupFont				= { 'XUBF', 0/* ID */ };
HIViewID const	idMyButtonBackupFontName				= { 'UFNm', 0/* ID */ };
HIViewID const	idMyHelpTextBackupFontTest				= { 'HFTs', 0/* ID */ };

// The following cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMy_DataBrowserPropertyIDBaseCharacterSet		= 'Base'
};

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
struct My_TranslationsDataBrowserCallbacks
{
	My_TranslationsDataBrowserCallbacks		();
	~My_TranslationsDataBrowserCallbacks	();
	
	DataBrowserCallbacks	_listCallbacks;
};

/*!
Implements the user interface of the panel - only
created when the panel directs for this to happen.
*/
struct My_TranslationsPanelUI
{
public:
	My_TranslationsPanelUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref								_panel;
	My_TranslationsDataBrowserCallbacks		_listCallbacks;
	HIViewWrap								_mainView;
	CarbonEventHandlerWrap					_buttonCommandsHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap					_fontPanelHandler;			//!< invoked when the font panel changes
	CarbonEventHandlerWrap					_viewClickHandler;			//!< invoked when a tab is clicked
	CommonEventHandlers_HIViewResizer		_containerResizer;			//!< invoked when the panel is resized
	Boolean									_disableMonitorRoutine;		//!< a hack to ignore update spam from buggy OS calls
	
	void
	loseFocus	();
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	setEncoding		(CFStringEncoding, Boolean = true);
	
	void
	setFontName		(ConstStringPtr);

protected:
	void
	assignAccessibilityRelationships ();
	
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
	
	void
	setFontEnabled	(Boolean);
};
typedef My_TranslationsPanelUI*		My_TranslationsPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_TranslationsPanelData
{
	My_TranslationsPanelData ();
	
	Panel_Ref					_panel;			//!< the panel this data is for
	My_TranslationsPanelUI*		_interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef		_dataModel;		//!< source of initializations and target of changes
};
typedef My_TranslationsPanelData*	My_TranslationsPanelDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus			accessDataBrowserItemData		(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
													 DataBrowserItemDataRef, Boolean);
Boolean				compareDataBrowserItems			(HIViewRef, DataBrowserItemID, DataBrowserItemID, DataBrowserPropertyID);
void				deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
void				disposePanel					(Panel_Ref, void*);
void				monitorDataBrowserItems			(HIViewRef, DataBrowserItemID, DataBrowserItemNotification);
SInt32				panelChanged					(Panel_Ref, Panel_Message, void*);
OSStatus			receiveFontChange				(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveHICommand				(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveViewHit					(EventHandlerCallRef, EventRef, void*);
void				setDataBrowserColumnWidths		(My_TranslationsPanelUIPtr);

} // anonymous namespace

#pragma mark Variables
namespace {

Float32		gIdealPanelWidth = 0.0;
Float32		gIdealPanelHeight = 0.0;

} // anonymous namespace



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
										kConstantsRegistry_IconServicesIconPrefPanelTranslations);
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

(4.0)
*/
Preferences_TagSetRef
PrefPanelTranslations_NewTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagTextEncodingIANAName);
	tagList.push_back(kPreferences_TagTextEncodingID);
	tagList.push_back(kPreferences_TagBackupFontName);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_TranslationsDataBrowserCallbacks structure.

(3.1)
*/
My_TranslationsDataBrowserCallbacks::
My_TranslationsDataBrowserCallbacks ()
{
	// set up all the callbacks needed for the data browser
	this->_listCallbacks.version = kDataBrowserLatestCallbacks;
	if (noErr != InitDataBrowserCallbacks(&this->_listCallbacks))
	{
		// fallback
		bzero(&this->_listCallbacks, sizeof(this->_listCallbacks));
		this->_listCallbacks.version = kDataBrowserLatestCallbacks;
	}
	this->_listCallbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
	assert(nullptr != this->_listCallbacks.u.v1.itemDataCallback);
	this->_listCallbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(compareDataBrowserItems);
	assert(nullptr != this->_listCallbacks.u.v1.itemCompareCallback);
	this->_listCallbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
	assert(nullptr != this->_listCallbacks.u.v1.itemNotificationCallback);
}// My_TranslationsDataBrowserCallbacks default constructor


/*!
Tears down a My_TranslationsDataBrowserCallbacks structure.

(3.1)
*/
My_TranslationsDataBrowserCallbacks::
~My_TranslationsDataBrowserCallbacks ()
{
	// dispose callbacks
	if (nullptr != this->_listCallbacks.u.v1.itemDataCallback)
	{
		DisposeDataBrowserItemDataUPP(this->_listCallbacks.u.v1.itemDataCallback),
			this->_listCallbacks.u.v1.itemDataCallback = nullptr;
	}
	if (nullptr != this->_listCallbacks.u.v1.itemCompareCallback)
	{
		DisposeDataBrowserItemCompareUPP(this->_listCallbacks.u.v1.itemCompareCallback),
			this->_listCallbacks.u.v1.itemCompareCallback = nullptr;
	}
	if (nullptr != this->_listCallbacks.u.v1.itemNotificationCallback)
	{
		DisposeDataBrowserItemNotificationUPP(this->_listCallbacks.u.v1.itemNotificationCallback),
			this->_listCallbacks.u.v1.itemNotificationCallback = nullptr;
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
_panel(nullptr),
_interfacePtr(nullptr),
_dataModel(nullptr)
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
_panel					(inPanel),
_listCallbacks			(),
_mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_fontPanelHandler		(GetControlEventTarget(HIViewWrap(idMyButtonBackupFontName, inOwningWindow)), receiveFontChange,
							CarbonEventSetInClass(CarbonEventClass(kEventClassFont), kEventFontPanelClosed, kEventFontSelection),
							this/* user data */),
_viewClickHandler		(CarbonEventUtilities_ReturnViewTarget(this->_mainView), receiveViewHit,
							CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
							this/* user data */),
_containerResizer		(_mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
							kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
							deltaSizePanelContainerHIView, this/* context */)
{
	assert(_buttonCommandsHandler.isInstalled());
	assert(_fontPanelHandler.isInstalled());
	assert(_viewClickHandler.isInstalled());
	assert(_containerResizer.isInstalled());
	
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
		error = SetDataBrowserCallbacks(baseTableList, &this->_listCallbacks._listCallbacks);
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
		
		// attach panel to data browser
		error = SetControlProperty(baseTableList, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeOwningPanel,
									sizeof(inPanel), &inPanel);
		assert_noerr(error);
		
		error = HIViewAddSubview(result, baseTableList);
		assert_noerr(error);
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
Deselects any font buttons that are active, in response to
a focus-lost event.

(3.1)
*/
void
My_TranslationsPanelUI::
loseFocus ()
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->_panel);
	HIViewWrap			fontNameButton(idMyButtonBackupFontName, kOwningWindow);
	
	
	SetControl32BitValue(fontNameButton, kControlCheckBoxUncheckedValue);
}// My_TranslationsPanelUI::loseFocus


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_TranslationsPanelUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// IMPORTANT: the tags read here should be in sync with those
		// returned by PrefPanelTranslations_NewTagSet()
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// select character set
		{
			CFStringEncoding	selectedEncoding = TextTranslation_ContextReturnEncoding
													(inSettings, kCFStringEncodingUTF8/* fallback */);
			
			
			this->setEncoding(selectedEncoding);
		}
		
		// set backup font
		{
			Str255		fontName;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagBackupFontName, sizeof(fontName),
														fontName, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setFontName(fontName);
			}
			else
			{
				this->setFontName("\p");
			}
		}
	}
}// My_TranslationsPanelUI::readPreferences


/*!
Updates the text encoding display based on the given setting.

An option to avoid updating the data browser exists in case
this call is inside an actual data browser callback.

(3.1)
*/
void
My_TranslationsPanelUI::
setEncoding		(CFStringEncoding	inEncoding,
				 Boolean			inUpdateDataBrowser)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->_panel);
	
	
	SetControlTextWithCFString(HIViewWrap(idMyLabelBaseTranslationTableSelection, kOwningWindow),
								CFStringGetNameOfEncoding(inEncoding)); // LOCALIZE THIS
	if (inUpdateDataBrowser)
	{
		HIViewRef const		kDataBrowser = HIViewWrap(idMyDataBrowserBaseTranslationTable, kOwningWindow);
		UInt16 const		kEncodingIndex = TextTranslation_ReturnCharacterSetIndex(inEncoding);
		DataBrowserItemID	itemList[] = { STATIC_CAST(kEncodingIndex, DataBrowserItemID) };
		OSStatus			error = noErr;
		
		
		// there is a very stupid data browser bug where “assign” doesn’t seem to mean
		// “assign”; one must first explicitly clear the selection, then set a new one;
		// HOWEVER, this triggers an even stupider bug where the monitoring routine is
		// spammed as if dozens of items were selected in turn, so a flag is set to
		// prevent that callback from doing anything during this phase (sigh...)
		this->_disableMonitorRoutine = true;
		error = SetDataBrowserSelectedItems(kDataBrowser, 0/* number of items */, nullptr, kDataBrowserItemsAssign);
		this->_disableMonitorRoutine = false;
		error = SetDataBrowserSelectedItems(kDataBrowser, 1/* number of items */, itemList, kDataBrowserItemsAssign);
		error = RevealDataBrowserItem(kDataBrowser, itemList[0]/* item to reveal */,
										kMy_DataBrowserPropertyIDBaseCharacterSet,
										kDataBrowserRevealAndCenterInView);
	}
}// My_TranslationsPanelUI::setEncoding


/*!
Enables or disables font-related views.

Do not use this directly; it is an indirect effect of
using setFontName() with a font string that could be
empty.

(3.1)
*/
void
My_TranslationsPanelUI::
setFontEnabled		(Boolean	inIsBackupFont)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->_panel);
	HIViewWrap			viewWrap;
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxUseBackupFont, kOwningWindow), BooleanToCheckBoxValue(inIsBackupFont));
	viewWrap = HIViewWrap(idMyButtonBackupFontName, kOwningWindow);
	viewWrap << HIViewWrap_SetActiveState(inIsBackupFont);
	viewWrap = HIViewWrap(idMyHelpTextBackupFontTest, kOwningWindow);
	viewWrap << HIViewWrap_SetActiveState(inIsBackupFont);
}// My_TranslationsPanelUI::setFontEnabled


/*!
Updates the font name display based on the given setting.

(3.1)
*/
void
My_TranslationsPanelUI::
setFontName		(ConstStringPtr		inFontName)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->_panel);
	HIViewWrap			fontButton(idMyButtonBackupFontName, kOwningWindow);
	
	
	SetControlTitle(fontButton, inFontName);
	if (PLstrlen(inFontName) > 0)
	{
		ControlFontStyleRec		fontStyle;
		
		
		bzero(&fontStyle, sizeof(fontStyle));
		fontStyle.flags = kControlUseFontMask;
		fontStyle.font = FMGetFontFamilyFromName(inFontName);
		SetControlFontStyle(fontButton, &fontStyle);
		
		this->setFontEnabled(true);
	}
	else
	{
		this->setFontEnabled(false);
	}
}// My_TranslationsPanelUI::setFontName


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
OSStatus
accessDataBrowserItemData	(HIViewRef					UNUSED_ARGUMENT(inDataBrowser),
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
			result = SetDataBrowserItemDataBooleanValue(inItemData, false/* is editable */);
			break;
		
		case kMy_DataBrowserPropertyIDBaseCharacterSet:
			// return the text string for the character set name
			{
				CFStringEncoding	thisEncoding = TextTranslation_ReturnIndexedCharacterSet(inItemID);
				CFStringRef			tableNameCFString = nullptr;
				
				
			#if 0
				// LOCALIZE THIS
				tableNameCFString = CFStringGetNameOfEncoding(thisEncoding);
			#else
				tableNameCFString = CocoaBasic_ReturnStringEncodingLocalizedName(thisEncoding);
			#endif
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
Boolean
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
void
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
void
disposePanel	(Panel_Ref	UNUSED_ARGUMENT(inPanel),
				 void*		inDataPtr)
{
	My_TranslationsPanelDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, My_TranslationsPanelDataPtr);
	
	
	// destroy UI, if present
	if (nullptr != dataPtr->_interfacePtr) delete dataPtr->_interfacePtr;
	
	delete dataPtr, dataPtr = nullptr;
}// disposePanel


/*!
Responds to changes in the data browser.  Currently,
most of the possible messages are ignored, but this
is used to determine when to update the panel views.

(3.1)
*/
void
monitorDataBrowserItems		(HIViewRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	My_TranslationsPanelDataPtr		panelDataPtr = nullptr;
	Panel_Ref						owningPanel = nullptr;
	UInt32							actualSize = 0;
	OSStatus						getPropertyError = noErr;
	Boolean							proceed = true;
	
	
	getPropertyError = GetControlProperty(inDataBrowser, AppResources_ReturnCreatorCode(),
											kConstantsRegistry_ControlPropertyTypeOwningPanel,
											sizeof(owningPanel), &actualSize, &owningPanel);
	if (noErr == getPropertyError)
	{
		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(owningPanel),
										My_TranslationsPanelDataPtr);
	}
	
	if (nullptr != panelDataPtr)
	{
		proceed = (false == panelDataPtr->_interfacePtr->_disableMonitorRoutine);
	}
	
	if (proceed)
	{
		switch (inMessage)
		{
		case kDataBrowserItemSelected:
			if (kDataBrowserNoItem != inItemID)
			{
				Preferences_Result		prefsResult = kPreferences_ResultOK;
				CFStringEncoding		newEncoding = TextTranslation_ReturnIndexedCharacterSet(inItemID);
				CFStringRef				newEncodingName = CFStringConvertEncodingToIANACharSetName(newEncoding);
				
				
				assert_noerr(getPropertyError);
				
				// save preferences
				prefsResult = Preferences_ContextSetData(panelDataPtr->_dataModel, kPreferences_TagTextEncodingID,
															sizeof(newEncoding), &newEncoding);
				if (kPreferences_ResultOK != prefsResult)
				{
					Console_Warning(Console_WriteLine, "failed to save encoding ID");
				}
				prefsResult = Preferences_ContextSetData(panelDataPtr->_dataModel, kPreferences_TagTextEncodingIANAName,
															sizeof(newEncodingName), &newEncodingName);
				if (kPreferences_ResultOK != prefsResult)
				{
					Console_Warning(Console_WriteLine, "failed to save encoding IANA name");
				}
				
				// update the panel views to match the newly-selected item
				panelDataPtr->_interfacePtr->setEncoding(newEncoding, false/* update data browser */);
			}
			break;
		
		default:
			// not all messages are supported
			break;
		}
	}
}// monitorDataBrowserItems


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
												kConstantsRegistry_PrefPanelDescriptorTranslations, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_TranslationsPanelDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_TranslationsPanelDataPtr);
			HIWindowRef const*				windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			panelDataPtr->_interfacePtr = new My_TranslationsPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->_interfacePtr);
			setDataBrowserColumnWidths(panelDataPtr->_interfacePtr);
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
			My_TranslationsPanelDataPtr			panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TranslationsPanelDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::TRANSLATION);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext)
			{
				panelDataPtr->_dataModel = defaultContext; // must be in sync for certain callbacks
				panelDataPtr->_interfacePtr->readPreferences(defaultContext); // reset to known state first
			}
			panelDataPtr->_dataModel = newContext;
			panelDataPtr->_interfacePtr->readPreferences(newContext);
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
Handles "kEventFontPanelClosed" and "kEventFontSelection" of
"kEventClassFont" for the font and size of the panel.

(3.1)
*/
OSStatus
receiveFontChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyTranslationsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_TranslationsPanelUI*			interfacePtr = REINTERPRET_CAST(inMyTranslationsPanelUIPtr, My_TranslationsPanelUI*);
	My_TranslationsPanelDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->_panel),
																	My_TranslationsPanelDataPtr);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassFont);
	assert((kEventKind == kEventFontPanelClosed) || (kEventKind == kEventFontSelection));
	switch (kEventKind)
	{
	case kEventFontPanelClosed:
		// user has closed the panel; clear focus
		interfacePtr->loseFocus();
		break;
	
	case kEventFontSelection:
		// user has accepted font changes in some way...update views
		// and internal preferences
		{
			Preferences_Result	prefsResult = kPreferences_ResultOK;
			FMFontFamily		fontFamily = kInvalidFontFamily;
			OSStatus			error = noErr;
			
			
			result = noErr; // initially...
			
			// determine the font, if possible
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamFMFontFamily, typeFMFontFamily, fontFamily);
			if (noErr == error)
			{
				Str255		fontName;
				
				
				error = FMGetFontFamilyName(fontFamily, fontName);
				if (noErr != error) result = eventNotHandledErr;
				else
				{
					prefsResult = Preferences_ContextSetData(panelDataPtr->_dataModel, kPreferences_TagBackupFontName,
																sizeof(fontName), fontName);
					if (kPreferences_ResultOK != prefsResult)
					{
						result = eventNotHandledErr;
					}
					else
					{
						// success!
						interfacePtr->setFontName(fontName);
					}
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// receiveFontChange


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyTranslationsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_TranslationsPanelUI*		interfacePtr = REINTERPRET_CAST(inMyTranslationsPanelUIPtr, My_TranslationsPanelUI*);
	My_TranslationsPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->_panel),
															My_TranslationsPanelDataPtr);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommandExtended	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if ((noErr == result) && (received.attributes & kHICommandFromControl))
		{
			HIViewRef	buttonHit = received.source.control;
			
			
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandEditBackupFont:
				// select the button that was hit, and transmit font information
				{
					FontSelectionQDStyle	fontInfo;
					Str255					fontName;
					size_t					actualSize = 0;
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextGetData(panelDataPtr->_dataModel, kPreferences_TagBackupFontName,
																sizeof(fontName), fontName, false/* search defaults too */, &actualSize);
					if (kPreferences_ResultOK != prefsResult)
					{
						// not found; set an arbitrary default
						PLstrcpy(fontName, "\pMonaco");
					}
					
					bzero(&fontInfo, sizeof(fontInfo));
					fontInfo.version = kFontSelectionQDStyleVersionZero;
					fontInfo.instance.fontFamily = FMGetFontFamilyFromName(fontName);
					fontInfo.instance.fontStyle = normal;
					fontInfo.size = 12; // arbitrary; required by the panel but not used
					fontInfo.hasColor = false;
					// apparently this API can return paramErr even though it
					// successfully sets the desired font information...
					(OSStatus)SetFontInfoForSelection(kFontSelectionQDType, 1/* number of styles */, &fontInfo,
													// NOTE: This API is misdeclared in older headers, the last argument is supposed to
													// be an event target.  It is bastardized into HIObjectRef form for older compiles.
													#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
														GetControlEventTarget(buttonHit)
													#else
														REINTERPRET_CAST(buttonHit, HIObjectRef)
													#endif
													);
					if (1)
					{
						// show the font panel, even if it could not be initialized properly
						SetControl32BitValue(HIViewWrap(idMyButtonBackupFontName, HIViewGetWindow(buttonHit)), kControlCheckBoxCheckedValue);
						if (false == FPIsFontPanelVisible())
						{
							result = FPShowHideFontPanel();
						}
					}
				}
				break;
			
			case kCommandUseBackupFont:
				{
					Str255					fontName;
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					if (kControlCheckBoxCheckedValue == GetControl32BitValue(buttonHit))
					{
						// arbitrary - TEMPORARY, should probably read this from somewhere
						PLstrcpy(fontName, "\pMonaco");
					}
					else
					{
						PLstrcpy(fontName, "\p");
					}
					
					// update preferences and the display
					prefsResult = Preferences_ContextSetData(panelDataPtr->_dataModel, kPreferences_TagBackupFontName,
																sizeof(fontName), fontName);
					panelDataPtr->_interfacePtr->setFontName(fontName);
					
					// completely handled
					result = noErr;
				}
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


/*!
Handles "kEventControlClick" of "kEventClassControl"
for this preferences panel.

(3.1)
*/
OSStatus
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
void
setDataBrowserColumnWidths	(My_TranslationsPanelUIPtr		inInterfacePtr)
{
	Rect			containerRect;
	HIViewWrap		baseTableListContainer(idMyDataBrowserBaseTranslationTable, HIViewGetWindow(inInterfacePtr->_mainView));
	assert(baseTableListContainer.exists());
	
	
	(Rect*)GetControlBounds(baseTableListContainer, &containerRect);
	
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
