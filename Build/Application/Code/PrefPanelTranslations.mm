/*!	\file PrefPanelTranslations.mm
	\brief Implements the Translations panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#import "PrefPanelTranslations.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <BoundName.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <DialogAdjust.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <IconManager.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>

// application includes
#import "AppResources.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "TextTranslation.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



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
	setFontName		(CFStringRef);

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


/*!
Implements an object wrapper for translation tables, that allows
them to be easily inserted into user interface elements without
losing less user-friendly information about each encoding.
*/
@interface PrefPanelTranslations_TableInfo : BoundName_Object //{
{
@private
	CFStringEncoding	encodingType;
}

// initializers
	- (id)
	initWithEncodingType:(CFStringEncoding)_
	description:(NSString*)_;

// accessors; see "Translation Tables" array controller in the NIB, for key names
	- (CFStringEncoding)
	encodingType;
	- (void)
	setEncodingType:(CFStringEncoding)_;

@end //}

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

/*!
The private class interface.
*/
@interface PrefPanelTranslations_ViewManager (PrefPanelTranslations_ViewManagerInternal) @end

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
		if (UIStrings_Copy(kUIStrings_PrefPanelTranslationsCategoryName, nameCFString).ok())
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
_fontPanelHandler		(HIViewGetEventTarget(HIViewWrap(idMyButtonBackupFontName, inOwningWindow)), receiveFontChange,
							CarbonEventSetInClass(CarbonEventClass(kEventClassFont), kEventFontPanelClosed, kEventFontSelection),
							this/* user data */),
_viewClickHandler		(HIViewGetEventTarget(this->_mainView), receiveViewHit,
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
		
		
		bzero(&containerBounds, sizeof(containerBounds));
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
		stringResult = UIStrings_Copy(kUIStrings_PrefPanelTranslationsListHeaderBaseTable,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMy_DataBrowserPropertyIDBaseCharacterSet;
			error = AddDataBrowserListViewColumn(baseTableList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// hide the heading; it is not useful
		UNUSED_RETURN(OSStatus)SetDataBrowserListViewHeaderBtnHeight(baseTableList, 0);
		
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
		UNUSED_RETURN(OSStatus)DataBrowserChangeAttributes(baseTableList,
															kDataBrowserAttributeListViewAlternatingRowColors/* attributes to set */,
															0/* attributes to clear */);
		UNUSED_RETURN(OSStatus)SetDataBrowserListViewUsePlainBackground(baseTableList, false);
		UNUSED_RETURN(OSStatus)SetDataBrowserTableViewHiliteStyle(baseTableList, kDataBrowserTableViewFillHilite);
		UNUSED_RETURN(OSStatus)SetDataBrowserHasScrollBars(baseTableList, false/* horizontal */, true/* vertical */);
		UNUSED_RETURN(OSStatus)SetDataBrowserSelectionFlags(baseTableList, kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
		
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
			CFStringRef		fontName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagBackupFontName, sizeof(fontName),
														&fontName, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setFontName(fontName);
			}
			else
			{
				this->setFontName(CFSTR(""));
			}
			
			if (nullptr != fontName)
			{
				CFRelease(fontName), fontName = nullptr;
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
setFontName		(CFStringRef	inFontName)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->_panel);
	HIViewWrap			fontButton(idMyButtonBackupFontName, kOwningWindow);
	
	
	SetControlTitleWithCFString(fontButton, inFontName);
	if (CFStringGetLength(inFontName) > 0)
	{
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
	Boolean							proceed = false;
	
	
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
					CFStringRef		fontNameCFString = CFStringCreateWithPascalString
														(kCFAllocatorDefault, fontName, kCFStringEncodingMacRoman);
					
					
					prefsResult = Preferences_ContextSetData(panelDataPtr->_dataModel, kPreferences_TagBackupFontName,
																sizeof(fontNameCFString), &fontNameCFString);
					if (kPreferences_ResultOK != prefsResult)
					{
						result = eventNotHandledErr;
					}
					else
					{
						// success!
						interfacePtr->setFontName(fontNameCFString);
					}
					
					if (nullptr != fontNameCFString)
					{
						CFRelease(fontNameCFString), fontNameCFString = nullptr;
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
					CFStringRef				fontName = nullptr;
					Boolean					releaseFontName = true;
					size_t					actualSize = 0;
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextGetData(panelDataPtr->_dataModel, kPreferences_TagBackupFontName,
																sizeof(fontName), &fontName, false/* search defaults too */,
																&actualSize);
					if (kPreferences_ResultOK != prefsResult)
					{
						// not found; set an arbitrary default
						fontName = CFSTR("Monaco");
						releaseFontName = false;
					}
					
					bzero(&fontInfo, sizeof(fontInfo));
					fontInfo.version = kFontSelectionQDStyleVersionZero;
					{
						Str255		fontNamePascalString;
						
						
						if (CFStringGetPascalString(fontName, fontNamePascalString, sizeof(fontNamePascalString),
													kCFStringEncodingMacRoman))
						{
							fontInfo.instance.fontFamily = FMGetFontFamilyFromName(fontNamePascalString);
						}
					}
					fontInfo.instance.fontStyle = normal;
					fontInfo.size = 12; // arbitrary; required by the panel but not used
					fontInfo.hasColor = false;
					// apparently this API can return paramErr even though it
					// successfully sets the desired font information...
					UNUSED_RETURN(OSStatus)SetFontInfoForSelection
											(kFontSelectionQDType, 1/* number of styles */, &fontInfo, HIViewGetEventTarget(buttonHit));
					if (1)
					{
						// show the font panel, even if it could not be initialized properly
						SetControl32BitValue(HIViewWrap(idMyButtonBackupFontName, HIViewGetWindow(buttonHit)), kControlCheckBoxCheckedValue);
						if (false == FPIsFontPanelVisible())
						{
							result = FPShowHideFontPanel();
						}
					}
					
					if ((releaseFontName) && (nullptr != fontName))
					{
						CFRelease(fontName), fontName = nullptr;
					}
				}
				break;
			
			case kCommandUseBackupFont:
				{
					CFStringRef				fontName = nullptr;
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					if (kControlCheckBoxCheckedValue == GetControl32BitValue(buttonHit))
					{
						// arbitrary - TEMPORARY, should probably read this from somewhere
						fontName = CFSTR("Monaco");
					}
					else
					{
						fontName = CFSTR("");
					}
					
					// update preferences and the display
					prefsResult = Preferences_ContextSetData(panelDataPtr->_dataModel, kPreferences_TagBackupFontName,
																sizeof(fontName), &fontName);
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
	
	
	UNUSED_RETURN(Rect*)GetControlBounds(baseTableListContainer, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		kAvailableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */;
		Float32		calculatedWidth = 0;
		UInt16		totalWidthSoFar = 0;
		
		
		// give all remaining space to the base table column
		calculatedWidth = kAvailableWidth - totalWidthSoFar;
		UNUSED_RETURN(OSStatus)SetDataBrowserTableViewNamedColumnWidth
								(baseTableListContainer, kMy_DataBrowserPropertyIDBaseCharacterSet,
									STATIC_CAST(calculatedWidth, UInt16));
	}
}// setDataBrowserColumnWidths

} // anonymous namespace


@implementation PrefPanelTranslations_TableInfo


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithEncodingType:(CFStringEncoding)		anEncoding
description:(NSString*)						aDescription
{
	self = [super initWithBoundName:aDescription];
	if (nil != self)
	{
		[self setEncodingType:anEncoding];
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

(4.0)
*/
- (CFStringEncoding)
encodingType
{
	return encodingType;
}
- (void)
setEncodingType:(CFStringEncoding)		anEncoding
{
	encodingType = anEncoding;
}// setEncodingType:


@end // PrefPanelTranslations_TableInfo


@implementation PrefPanelTranslations_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelTranslationsCocoa" delegate:self context:nullptr];
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
	[translationTables release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
backupFontEnabled
{
	return (NO == [[self backupFontFamilyName] isEqualToString:@""]);
}
- (void)
setBackupFontEnabled:(BOOL)		aFlag
{
	if ([self backupFontEnabled] != aFlag)
	{
		if (aFlag)
		{
			// the default is arbitrary (TEMPORARY; should probably read this from somewhere else)
			NSString*	defaultFont = [[NSFont userFixedPitchFontOfSize:[NSFont systemFontSize]] familyName];
			
			
			if (nil == defaultFont)
			{
				defaultFont = @"Monaco";
			}
			[self setBackupFontFamilyName:defaultFont];
		}
		else
		{
			[self setBackupFontFamilyName:@""];
		}
	}
}// setBackupFontEnabled:


/*!
Accessor.

(4.1)
*/
- (NSString*)
backupFontFamilyName
{
	NSString*	result = @"";
	
	
	if (Preferences_ContextIsValid(self->activeContext))
	{
		CFStringRef			fontName = nullptr;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self->activeContext, kPreferences_TagBackupFontName,
																		sizeof(fontName), &fontName, false/* search defaults too */);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(fontName, NSString*);
			[result autorelease];
		}
	}
	return result;
}
- (void)
setBackupFontFamilyName:(NSString*)		aString
{
	if (Preferences_ContextIsValid(self->activeContext))
	{
		CFStringRef			fontName = BRIDGE_CAST(aString, CFStringRef);
		BOOL				saveOK = NO;
		Preferences_Result	prefsResult = Preferences_ContextSetData(self->activeContext, kPreferences_TagBackupFontName,
																		sizeof(fontName), &fontName);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			saveOK = YES;
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save backup font preference");
		}
	}
}// setBackupFontFamilyName:


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
translationTableIndexes
{
	NSIndexSet*		result = [NSIndexSet indexSetWithIndex:0];
	
	
	if (Preferences_ContextIsValid(self->activeContext))
	{
		CFStringEncoding	encodingID = TextTranslation_ContextReturnEncoding(self->activeContext, kCFStringEncodingInvalidId);
		
		
		if (kCFStringEncodingInvalidId != encodingID)
		{
			UInt16		index = TextTranslation_ReturnCharacterSetIndex(encodingID);
			
			
			if (0 == index)
			{
				// not found
				Console_Warning(Console_WriteValue, "saved encoding is not available on the current system", encodingID);
			}
			else
			{
				// translate from one-based to zero-based
				result = [NSIndexSet indexSetWithIndex:(index - 1)];
			}
		}
	}
	return result;
}
- (void)
setTranslationTableIndexes:(NSIndexSet*)	indexes
{
	if (Preferences_ContextIsValid(self->activeContext))
	{
		BOOL	saveOK = NO;
		
		
		[self willChangeValueForKey:@"translationTableIndexes"];
		
		if ([indexes count] > 0)
		{
			// translate from zero-based to one-based
			UInt16		index = (1 + [indexes firstIndex]);
			
			
			// update preferences
			if (TextTranslation_ContextSetEncoding(self->activeContext, TextTranslation_ReturnIndexedCharacterSet(index),
													false/* via copy */))
			{
				saveOK = YES;
			}
			
			// scroll the list to reveal the current selection
			[self->translationTableView scrollRowToVisible:[indexes firstIndex]];
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save translation table preference");
		}
		
		[self didChangeValueForKey:@"translationTableIndexes"];
	}
}// setTranslationTableIndexes:


/*!
Accessor.

(4.1)
*/
- (NSArray*)
translationTables
{
	return [[translationTables retain] autorelease];
}


#pragma mark Actions


/*!
Displays a font panel for editing the backup font.

(4.1)
*/
- (IBAction)
performBackupFontSelection:(id)		sender
{
	[[NSFontPanel sharedFontPanel] makeKeyAndOrderFront:sender];
}// performBackupFontSelection:


#pragma mark NSFontPanel


/*!
Updates the “backup font” preference based on the user’s
selection in the system’s Fonts panel.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	NSFont*		oldFont = [NSFont fontWithName:[self backupFontFamilyName] size:[NSFont systemFontSize]];
	NSFont*		newFont = [sender convertFont:oldFont];
	
	
	[self setBackupFontFamilyName:[newFont familyName]];
}// changeFont:


#pragma mark NSKeyValueObservingCustomization


/*!
Returns true for keys that manually notify observers
(through "willChangeValueForKey:", etc.).

(4.1)
*/
+ (BOOL)
automaticallyNotifiesObserversForKey:(NSString*)	theKey
{
	BOOL	result = YES;
	SEL		flagSource = NSSelectorFromString([[self class] selectorNameForKeyChangeAutoNotifyFlag:theKey]);
	
	
	if (NULL != class_getClassMethod([self class], flagSource))
	{
		// See selectorToReturnKeyChangeAutoNotifyFlag: for more information on the form of the selector.
		result = [[self performSelector:flagSource] boolValue];
	}
	else
	{
		result = [super automaticallyNotifiesObserversForKey:theKey];
	}
	return result;
}// automaticallyNotifiesObserversForKey:


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
	UInt16		characterSetCount = TextTranslation_ReturnCharacterSetCount();
	
	
	self->translationTables = [[NSMutableArray alloc] initWithCapacity:characterSetCount];
	self->activeContext = nullptr; // set later
	
	// build a table of available text encodings
	[self willChangeValueForKey:@"translationTables"];
	for (UInt16 i = 1; i <= characterSetCount; ++i)
	{
		CFStringEncoding	encodingID = TextTranslation_ReturnIndexedCharacterSet(i);
		NSString*			encodingName = BRIDGE_CAST(CFStringGetNameOfEncoding(encodingID), NSString*);
		
		
		[self->translationTables addObject:[[[PrefPanelTranslations_TableInfo alloc]
												initWithEncodingType:encodingID description:encodingName]
											autorelease]];
	}
	[self didChangeValueForKey:@"translationTables"];
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
	assert(nil != translationTableView);
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
	*outIdealSize = [[self managedView] frame].size;
}// panelViewManager:requestingIdealSize:


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
	self->activeContext = REINTERPRET_CAST(newDataSet, Preferences_ContextRef);
	
	// invoke every accessor with itself, which has the effect of
	// reading the current preference values (of the now-current
	// new context) and then updating the display based on them
	[self setBackupFontEnabled:[self backupFontEnabled]];
	[self setBackupFontFamilyName:[self backupFontFamilyName]];
	[self setTranslationTableIndexes:[self translationTableIndexes]];
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
	return [NSImage imageNamed:@"IconForPrefPanelTranslations"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Translations";
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
	return NSLocalizedStringFromTable(@"Translations", @"PrefPanelTranslations", @"the name of this panel");
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
	return kPanel_ResizeConstraintBothAxes;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::TRANSLATION;
}// preferencesClass


@end // PrefPanelTranslations_ViewManager


@implementation PrefPanelTranslations_ViewManager (PrefPanelTranslations_ViewManagerInternal)


@end // PrefPanelTranslations_ViewManager (PrefPanelTranslations_ViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
