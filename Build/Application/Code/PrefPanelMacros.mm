/*!	\file PrefPanelMacros.mm
	\brief Implements the Macros panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#import "PrefPanelMacros.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <iostream>
#import <sstream>
#import <vector>

// Unix includes
#import <strings.h>

// Mac includes
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
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
#import "MacroManager.h"
#import "Panel.h"
#import "Preferences.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

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
HIViewID const	idMyPopUpMenuMacroKeyType				= { 'MKTy', 0/* ID */ };
HIViewID const	idMyFieldMacroKeyCharacter				= { 'MKCh', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierCommand		= { 'McMC', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierControl		= { 'McML', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierOption		= { 'McMO', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierShift		= { 'McMS', 0/* ID */ };
HIViewID const	idMyPopUpMenuMacroAction				= { 'MMTy', 0/* ID */ };
HIViewID const	idMyFieldMacroText						= { 'McAc', 0/* ID */ };
HIViewID const	idMyButtonInsertControlKey				= { 'EMTC', 0/* ID */ };
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
	
	void
	getKeyTypeAndCharacterCodeFromPref	(MacroManager_KeyID, UInt32&, UniChar&);
	
	void
	getPrefFromKeyTypeAndCharacterCode	(UInt32, UniChar, MacroManager_KeyID&);
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef, Preferences_Index);
	
	static OSStatus
	receiveFieldChanged		(EventHandlerCallRef, EventRef, void*);
	
	void
	resetDisplay ();
	
	void
	saveFieldPreferences	(Preferences_ContextRef, UInt32);
	
	Boolean
	saveKeyTypeAndCharacterPreferences	(Preferences_ContextRef, UInt32);
	
	void
	setAction	(UInt32);
	
	void
	setDataBrowserColumnWidths ();
	
	void
	setKeyModifiers		(UInt32);
	
	void
	setKeyType		(UInt32);
	
	void
	setOrdinaryKeyCharacter		(UniChar);
	
	void
	setOrdinaryKeyFieldEnabled	(Boolean);
	
	Panel_Ref							panel;						//!< the panel using this UI
	Float32								idealWidth;					//!< best size in pixels
	Float32								idealHeight;				//!< best size in pixels
	My_MacrosDataBrowserCallbacks		listCallbacks;				//!< used to provide data for the list
	HIViewWrap							mainView;
	UInt32								keyType;					//!< cache of selected menu command ID

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);

private:
	CarbonEventHandlerWrap				_menuCommandsHandler;		//!< responds to menu selections
	CarbonEventHandlerWrap				_fieldKeyCharInputHandler;	//!< saves field settings when they change
	CarbonEventHandlerWrap				_fieldContentsInputHandler;	//!< saves field settings when they change
	CommonEventHandlers_HIViewResizer	_containerResizer;			//!< invoked when the panel is resized
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
	switchDataModel		(Preferences_ContextRef, Preferences_Index);
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_MacrosPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef	dataModel;		//!< source of initializations and target of changes
	Preferences_Index		currentIndex;	//!< which index (one-based) in the data model to use for macro-specific settings
};
typedef My_MacrosPanelData*		My_MacrosPanelDataPtr;

} // anonymous namespace


/*!
Private properties.
*/
@interface PrefPanelMacros_ViewManager () //{

// accessors
	@property (strong) PrefPanelMacros_MacroEditorViewManager*
	macroEditorViewManager;

@end //}


/*!
Private properties.
*/
@interface PrefPanelMacros_MacroEditorViewManager () //{

@end //}


/*!
The private class interface.
*/
@interface PrefPanelMacros_MacroEditorViewManager (PrefPanelMacros_MacroEditorViewManagerInternal) //{

// new methods
	- (void)
	configureForIndex:(Preferences_Index)_;

// accessors
	@property (readonly) NSArray*
	primaryDisplayBindingKeys;

@end //}


#pragma mark Internal Method Prototypes
namespace {

OSStatus	accessDataBrowserItemData			(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
												 DataBrowserItemDataRef, Boolean);
Boolean		compareDataBrowserItems				(HIViewRef, DataBrowserItemID, DataBrowserItemID,
												 DataBrowserPropertyID);
void		deltaSizePanelContainerHIView		(HIViewRef, Float32, Float32, void*);
void		disposePanel						(Panel_Ref, void*);
void		monitorDataBrowserItems				(HIViewRef, DataBrowserItemID, DataBrowserItemNotification);
OSStatus	receiveHICommand					(EventHandlerCallRef, EventRef, void*);

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
		if (UIStrings_Copy(kUIStrings_PrefPanelMacrosCategoryName, nameCFString).ok())
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


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelMacros_NewTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	for (UInt16 i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
	{
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, i));
	}
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewTagSet


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
					 Preferences_Index			inNewIndex)
{
	if (nullptr != inNewContext) this->dataModel = inNewContext;
	if (0 != inNewIndex) this->currentIndex = inNewIndex;
	if (nullptr != this->interfacePtr)
	{
		this->interfacePtr->resetDisplay();
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
keyType						(0),
_menuCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
_fieldKeyCharInputHandler	(HIViewGetEventTarget(HIViewWrap(idMyFieldMacroKeyCharacter, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldContentsInputHandler	(HIViewGetEventTarget(HIViewWrap(idMyFieldMacroText, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_containerResizer			(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
										kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
								deltaSizePanelContainerHIView, this/* context */)
{
	assert(_menuCommandsHandler.isInstalled());
	assert(_fieldContentsInputHandler.isInstalled());
	assert(_containerResizer.isInstalled());
	
	this->setDataBrowserColumnWidths();
	
	// hack in Yosemite edit-text font for now (will later transition to Cocoa)
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyFieldMacroKeyCharacter, inOwningWindow), kThemeSystemFont);
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyFieldMacroText, inOwningWindow), kThemeSystemFont);
}// My_MacrosPanelUI 2-argument constructor


/*!
Tears down a My_MacrosPanelUI structure.

(3.1)
*/
My_MacrosPanelUI::
~My_MacrosPanelUI ()
{
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
		
		
		bzero(&containerBounds, sizeof(containerBounds));
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
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDMacroNumber;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// create name column
		stringResult = UIStrings_Copy(kUIStrings_PrefPanelMacrosListHeaderName,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDMacroName;
			columnInfo.propertyDesc.propertyFlags |= kDataBrowserListViewTypeSelectColumn;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// insert as many rows as there are macros in a set
		error = AddDataBrowserItems(macrosList, kDataBrowserNoItem/* parent item */, kMacroManager_MaximumMacroSetSize,
									nullptr/* IDs */, kDataBrowserItemNoProperty/* pre-sort property */);
		assert_noerr(error);
		
		// attach data that would not be specifiable in a NIB
		error = SetDataBrowserCallbacks(macrosList, &this->listCallbacks.listCallbacks);
		assert_noerr(error);
		
		// initialize sort column
		error = SetDataBrowserSortProperty(macrosList, kMyDataBrowserPropertyIDMacroNumber);
		assert_noerr(error);
		
		// set other nice things (most can be set in a NIB someday)
		UNUSED_RETURN(OSStatus)DataBrowserChangeAttributes(macrosList,
															kDataBrowserAttributeListViewAlternatingRowColors/* attributes to set */,
															0/* attributes to clear */);
		UNUSED_RETURN(OSStatus)SetDataBrowserListViewUsePlainBackground(macrosList, false);
		UNUSED_RETURN(OSStatus)SetDataBrowserTableViewHiliteStyle(macrosList, kDataBrowserTableViewFillHilite);
		UNUSED_RETURN(OSStatus)SetDataBrowserHasScrollBars(macrosList, false/* horizontal */, true/* vertical */);
		error = SetDataBrowserSelectionFlags(macrosList, kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
		assert_noerr(error);
		{
			DataBrowserPropertyFlags	flags = 0;
			
			
			error = GetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDMacroName, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserPropertyIsMutable;
				UNUSED_RETURN(OSStatus)SetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDMacroName, flags);
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
This utility can tell you what values to pass to
setKeyType() and setOrdinaryKeyFieldEnabled(),
respectively, for the given key ID (which is the
value that is actually saved in Preferences).  The
key type and character code will both be zero if
there is an error.

See also getPrefFromKeyTypeAndCharacterCode(), whose
implementation should mirror this one.

(4.0)
*/
void
My_MacrosPanelUI::
getKeyTypeAndCharacterCodeFromPref	(MacroManager_KeyID		inKeyID,
									 UInt32&				outKeyType,
									 UniChar&				outCharacterCodeOrZeroForVirtualKey)
{
	UInt16 const	kCharacterOrKeyCode = MacroManager_KeyIDKeyCode(inKeyID);
	Boolean const	kIsVirtualKey = MacroManager_KeyIDIsVirtualKey(inKeyID);
	
	
	if (false == kIsVirtualKey)
	{
		outKeyType = kCommandSetMacroKeyTypeOrdinaryChar;
		outCharacterCodeOrZeroForVirtualKey = kCharacterOrKeyCode;
	}
	else
	{
		// virtual key code
		outCharacterCodeOrZeroForVirtualKey = 0;
		switch (kCharacterOrKeyCode)
		{
		case 0x33:
			outKeyType = kCommandSetMacroKeyTypeBackwardDelete;
			break;
		
		case 0x75:
			outKeyType = kCommandSetMacroKeyTypeForwardDelete;
			break;
		
		case 0x73:
			outKeyType = kCommandSetMacroKeyTypeHome;
			break;
		
		case 0x77:
			outKeyType = kCommandSetMacroKeyTypeEnd;
			break;
		
		case 0x74:
			outKeyType = kCommandSetMacroKeyTypePageUp;
			break;
		
		case 0x79:
			outKeyType = kCommandSetMacroKeyTypePageDown;
			break;
		
		case 0x7E:
			outKeyType = kCommandSetMacroKeyTypeUpArrow;
			break;
		
		case 0x7D:
			outKeyType = kCommandSetMacroKeyTypeDownArrow;
			break;
		
		case 0x7B:
			outKeyType = kCommandSetMacroKeyTypeLeftArrow;
			break;
		
		case 0x7C:
			outKeyType = kCommandSetMacroKeyTypeRightArrow;
			break;
		
		case 0x47:
			outKeyType = kCommandSetMacroKeyTypeClear;
			break;
		
		case 0x35:
			outKeyType = kCommandSetMacroKeyTypeEscape;
			break;
		
		case 0x24:
			outKeyType = kCommandSetMacroKeyTypeReturn;
			break;
		
		case 0x4C:
			outKeyType = kCommandSetMacroKeyTypeEnter;
			break;
		
		case 0x7A:
			outKeyType = kCommandSetMacroKeyTypeF1;
			break;
		
		case 0x78:
			outKeyType = kCommandSetMacroKeyTypeF2;
			break;
		
		case 0x63:
			outKeyType = kCommandSetMacroKeyTypeF3;
			break;
		
		case 0x76:
			outKeyType = kCommandSetMacroKeyTypeF4;
			break;
		
		case 0x60:
			outKeyType = kCommandSetMacroKeyTypeF5;
			break;
		
		case 0x61:
			outKeyType = kCommandSetMacroKeyTypeF6;
			break;
		
		case 0x62:
			outKeyType = kCommandSetMacroKeyTypeF7;
			break;
		
		case 0x64:
			outKeyType = kCommandSetMacroKeyTypeF8;
			break;
		
		case 0x65:
			outKeyType = kCommandSetMacroKeyTypeF9;
			break;
		
		case 0x6D:
			outKeyType = kCommandSetMacroKeyTypeF10;
			break;
		
		case 0x67:
			outKeyType = kCommandSetMacroKeyTypeF11;
			break;
		
		case 0x69:
			outKeyType = kCommandSetMacroKeyTypeF13;
			break;
		
		case 0x6A:
			outKeyType = kCommandSetMacroKeyTypeF16;
			break;
		
		case 0x6B:
			outKeyType = kCommandSetMacroKeyTypeF14;
			break;
		
		case 0x6F:
			outKeyType = kCommandSetMacroKeyTypeF12;
			break;
		
		case 0x71:
			outKeyType = kCommandSetMacroKeyTypeF15;
			break;
		
		default:
			// ???
			outKeyType = 0;
			outCharacterCodeOrZeroForVirtualKey = 0;
			break;
		}
	}
}// My_MacrosPanelUI::getKeyTypeAndCharacterCodeFromPref


/*!
This utility can tell you the equivalent preference
setting for a given combination of command ID (also
known as key type) and, for ordinary keys, a character
code.  Pass zero for the character code if it is a
special key, since this is described by the key type.
The key ID will be zero if there is an error.

You typically use this to help convert UI settings
into a code for storing in preferences.

See also getKeyTypeAndCharacterCodeFromPref(), whose
implementation should mirror this one.

(4.0)
*/
void
My_MacrosPanelUI::
getPrefFromKeyTypeAndCharacterCode	(UInt32					inKeyType,
									 UniChar				inCharacterCodeIfApplicable,
									 MacroManager_KeyID&	outKeyID)
{
	// Virtual key codes are not well documented!  But they
	// can be found in old Inside Macintosh books.
	switch (inKeyType)
	{
	case kCommandSetMacroKeyTypeOrdinaryChar:
		outKeyID = MacroManager_MakeKeyID(false/* is virtual key */, inCharacterCodeIfApplicable);
		break;
	
	case kCommandSetMacroKeyTypeBackwardDelete:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x33);
		break;
	
	case kCommandSetMacroKeyTypeForwardDelete:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x75);
		break;
	
	case kCommandSetMacroKeyTypeHome:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x73);
		break;
	
	case kCommandSetMacroKeyTypeEnd:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x77);
		break;
	
	case kCommandSetMacroKeyTypePageUp:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x74);
		break;
	
	case kCommandSetMacroKeyTypePageDown:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x79);
		break;
	
	case kCommandSetMacroKeyTypeUpArrow:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x7E);
		break;
	
	case kCommandSetMacroKeyTypeDownArrow:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x7D);
		break;
	
	case kCommandSetMacroKeyTypeLeftArrow:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x7B);
		break;
	
	case kCommandSetMacroKeyTypeRightArrow:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x7C);
		break;
	
	case kCommandSetMacroKeyTypeClear:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x47);
		break;
	
	case kCommandSetMacroKeyTypeEscape:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x35);
		break;
	
	case kCommandSetMacroKeyTypeReturn:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x24);
		break;
	
	case kCommandSetMacroKeyTypeEnter:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x4C);
		break;
	
	case kCommandSetMacroKeyTypeF1:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x7A);
		break;
	
	case kCommandSetMacroKeyTypeF2:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x78);
		break;
	
	case kCommandSetMacroKeyTypeF3:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x63);
		break;
	
	case kCommandSetMacroKeyTypeF4:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x76);
		break;
	
	case kCommandSetMacroKeyTypeF5:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x60);
		break;
	
	case kCommandSetMacroKeyTypeF6:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x61);
		break;
	
	case kCommandSetMacroKeyTypeF7:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x62);
		break;
	
	case kCommandSetMacroKeyTypeF8:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x64);
		break;
	
	case kCommandSetMacroKeyTypeF9:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x65);
		break;
	
	case kCommandSetMacroKeyTypeF10:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x6D);
		break;
	
	case kCommandSetMacroKeyTypeF11:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x67);
		break;
	
	case kCommandSetMacroKeyTypeF12:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x6F);
		break;
	
	case kCommandSetMacroKeyTypeF13:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x69);
		break;
	
	case kCommandSetMacroKeyTypeF14:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x6B);
		break;
	
	case kCommandSetMacroKeyTypeF15:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x71);
		break;
	
	case kCommandSetMacroKeyTypeF16:
		outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, 0x6A);
		break;
	
	default:
		// ???
		outKeyID = 0;
		break;
	}
}// My_MacrosPanelUI::getPrefFromKeyTypeAndCharacterCode


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
				UNUSED_RETURN(OSStatus)SetDataBrowserSelectedItems(dataBrowser, 1/* number of items */, &kFirstItem, kDataBrowserItemsAssign);
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
					 Preferences_Index			inOneBasedIndex)
{
	assert(0 != inOneBasedIndex);
	if (nullptr != inSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set key type
		{
			MacroManager_KeyID		macroKey = 0;
			
			
			prefsResult = Preferences_ContextGetData
							(inSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, inOneBasedIndex),
								sizeof(macroKey), &macroKey, false/* search defaults too */);
			if (kPreferences_ResultOK == prefsResult)
			{
				UInt32		prefKeyType = 0;
				UniChar		prefKeyChar = 0;
				
				
				getKeyTypeAndCharacterCodeFromPref(macroKey, prefKeyType, prefKeyChar);
				this->setKeyType(prefKeyType);
				this->setOrdinaryKeyCharacter(prefKeyChar);
			}
			else
			{
				Console_Warning(Console_WriteLine, "unable to find existing macro key setting");
				this->setKeyType(kCommandSetMacroKeyTypeOrdinaryChar);
				this->setOrdinaryKeyCharacter(0);
			}
		}
		
		// set modifier buttons
		{
			UInt32		modifiers = 0;
			
			
			prefsResult = Preferences_ContextGetData
							(inSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, inOneBasedIndex),
								sizeof(modifiers), &modifiers, false/* search defaults too */);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setKeyModifiers(modifiers);
			}
			else
			{
				Console_Warning(Console_WriteLine, "unable to find existing modifier key settings");
				this->setKeyModifiers(0);
			}
		}
		
		// set action
		{
			MacroManager_Action		actionPerformed = kMacroManager_ActionSendTextProcessingEscapes;
			
			
			prefsResult = Preferences_ContextGetData
							(inSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, inOneBasedIndex),
								sizeof(actionPerformed), &actionPerformed, false/* search defaults too */);
			if (kPreferences_ResultOK != prefsResult)
			{
				// initialize; do not fall back on defaults because there is
				// probably no correlation between the actions that happen
				// to be used on default macros, and the content of other sets!
				actionPerformed = kMacroManager_ActionSendTextProcessingEscapes;
			}
			
			// update UI
			{
				UInt32		commandForAction = MacroManager_CommandForAction(actionPerformed);
				
				
				this->setAction(commandForAction);
			}
		}
		
		// set action text
		{
			CFStringRef		actionCFString = nullptr;
			
			
			prefsResult = Preferences_ContextGetData
							(inSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, inOneBasedIndex),
								sizeof(actionCFString), &actionCFString, false/* search defaults too */);
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
OSStatus
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
Clears or resets the user interface elements of the panel
to reflect a lack of preferences.  This is useful just
prior to loading in a specific set of preferences, and is
important for predictable results.

(4.0)
*/
void
My_MacrosPanelUI::
resetDisplay ()
{
	HIViewWrap		macrosListContainer(idMyDataBrowserMacroSetList, HIViewGetWindow(this->mainView));
	assert(macrosListContainer.exists());
	
	
	// reset the list
	UNUSED_RETURN(OSStatus)UpdateDataBrowserItems(macrosListContainer, kDataBrowserNoItem/* parent item */,
													0/* number of IDs */, nullptr/* IDs */,
													kDataBrowserItemNoProperty/* pre-sort property */,
													kMyDataBrowserPropertyIDMacroName);
	
	// reset the selected macro’s items
	{
		Preferences_ContextRef	factoryDefaults = nullptr;
		Preferences_Result		prefsResult = Preferences_GetFactoryDefaultsContext(&factoryDefaults);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			readPreferences(factoryDefaults, 1/* index */);
		}
	}
}// My_MacrosPanelUI::resetDisplay


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
		
		
		// set key; this is also technically invoked when the key type menu
		// changes, since it is jointly dependent on the field setting
		this->saveKeyTypeAndCharacterPreferences(inoutSettings, inOneBasedIndex);
		
		// set action text
		{
			CFStringRef		actionCFString = nullptr;
			
			
			GetControlTextAsCFString(HIViewWrap(idMyFieldMacroText, kOwningWindow), actionCFString);
			if (nullptr != actionCFString)
			{
				prefsResult = Preferences_ContextSetData
								(inoutSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, inOneBasedIndex),
									sizeof(actionCFString), &actionCFString);
				if (kPreferences_ResultOK != prefsResult)
				{
					Console_Warning(Console_WriteValue, "failed to set action text of macro with index", inOneBasedIndex);
				}
				CFRelease(actionCFString), actionCFString = nullptr;
			}
		}
	}
}// My_MacrosPanelUI::saveFieldPreferences


/*!
Since the key type can be “ordinary character”, which
must then take into account the text of a separate field,
this special method exists to save both at once.

Returns true only if the save was successful.

(4.0)
*/
Boolean
My_MacrosPanelUI::
saveKeyTypeAndCharacterPreferences	(Preferences_ContextRef		inoutSettings,
									 UInt32						inOneBasedIndex)
{
	Boolean		result = false;
	
	
	assert(0 != inOneBasedIndex);
	if (nullptr != inoutSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		MacroManager_KeyID		keyID = 0;
		UniChar					keyChar = 0;
		
		
		// for ordinary keys, read the character typed in the text field
		if (kCommandSetMacroKeyTypeOrdinaryChar == this->keyType)
		{
			CFStringRef		ordinaryCharCFString = nullptr;
			
			
			GetControlTextAsCFString(HIViewWrap(idMyFieldMacroKeyCharacter, kOwningWindow), ordinaryCharCFString);
			if (nullptr != ordinaryCharCFString)
			{
				if (0 != CFStringGetLength(ordinaryCharCFString))
				{
					keyChar = CFStringGetCharacterAtIndex(ordinaryCharCFString, 0);
					if (1 != CFStringGetLength(ordinaryCharCFString))
					{
						Console_Warning(Console_WriteLine, "more than one character entered for macro key, using only the first character");
					}
				}
				else
				{
					// do not warn about this case because right now there is
					// no other way to simply say “macro wants no key mapping”
					//Console_Warning(Console_WriteLine, "ordinary character requested for macro but no character was entered");
				}
				CFRelease(ordinaryCharCFString), ordinaryCharCFString = nullptr;
			}
		}
		
		getPrefFromKeyTypeAndCharacterCode(this->keyType, keyChar, keyID);
		prefsResult = Preferences_ContextSetData
						(inoutSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, inOneBasedIndex),
							sizeof(keyID), &keyID);
		if (kPreferences_ResultOK == prefsResult)
		{
			result = true;
		}
		else
		{
			Console_Warning(Console_WriteValuePair, "failed to set key equivalent of macro with index, error",
							inOneBasedIndex, prefsResult);
		}
	}
	return result;
}// My_MacrosPanelUI::saveKeyTypeAndCharacterPreferences


/*!
Updates the views that represent the current macro action,
based on the given set-macro-action command ID.

Note that MacroManager_CommandForAction() can be used to
find the command ID for an action type.

(4.0)
*/
void
My_MacrosPanelUI::
setAction		(UInt32		inSetMacroActionCommandID)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			viewWrap;
	
	
	viewWrap = HIViewWrap(idMyPopUpMenuMacroAction, kOwningWindow);
	UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(viewWrap, inSetMacroActionCommandID);
	if ((inSetMacroActionCommandID != kCommandSetMacroActionEnterTextWithSub) &&
		(inSetMacroActionCommandID != kCommandSetMacroActionFindTextWithSub))
	{
		UNUSED_RETURN(OSStatus)DeactivateControl(HIViewWrap(idMyButtonInsertControlKey, kOwningWindow));
	}
	else
	{
		UNUSED_RETURN(OSStatus)ActivateControl(HIViewWrap(idMyButtonInsertControlKey, kOwningWindow));
	}
}// My_MacrosPanelUI::setAction


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
	
	
	UNUSED_RETURN(Rect*)GetControlBounds(macrosListContainer, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		availableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */ - 6/* double the inset focus ring width */;
		UInt16		integerWidth = 0;
		
		
		// leave number column fixed-size
		{
			integerWidth = 42; // arbitrary
			UNUSED_RETURN(OSStatus)SetDataBrowserTableViewNamedColumnWidth
									(macrosListContainer, kMyDataBrowserPropertyIDMacroNumber, integerWidth);
			availableWidth -= integerWidth;
		}
		
		// give all remaining space to the title
		UNUSED_RETURN(OSStatus)SetDataBrowserTableViewNamedColumnWidth
								(macrosListContainer, kMyDataBrowserPropertyIDMacroName, availableWidth);
	}
}// My_MacrosPanelUI::setDataBrowserColumnWidths


/*!
Set the state of all user interface elements that represent
the selected modifier keys of the current macro.

The specified integer uses "MacroManager_ModifierKeyMask"
bit flags.

(4.1)
*/
void
My_MacrosPanelUI::
setKeyModifiers		(UInt32		inModifiers)
{
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierCommand, HIViewGetWindow(this->mainView)),
							(inModifiers & kMacroManager_ModifierKeyMaskCommand) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierControl, HIViewGetWindow(this->mainView)),
							(inModifiers & kMacroManager_ModifierKeyMaskControl) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierOption, HIViewGetWindow(this->mainView)),
							(inModifiers & kMacroManager_ModifierKeyMaskOption) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierShift, HIViewGetWindow(this->mainView)),
							(inModifiers & kMacroManager_ModifierKeyMaskShift) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
}// My_MacrosPanelUI::setKeyModifiers


/*!
Updates the views that represent the current macro key,
based on the given key type.

This calls setOrdinaryKeyFieldEnabled() automatically.

(3.1)
*/
void
My_MacrosPanelUI::
setKeyType		(UInt32		inKeyCommandID)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			viewWrap;
	
	
	viewWrap = HIViewWrap(idMyPopUpMenuMacroKeyType, kOwningWindow);
	UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(viewWrap, inKeyCommandID);
	this->keyType = inKeyCommandID;
	if (kCommandSetMacroKeyTypeOrdinaryChar == inKeyCommandID)
	{
		this->setOrdinaryKeyFieldEnabled(true);
	}
	else
	{
		viewWrap = HIViewWrap(idMyFieldMacroKeyCharacter, kOwningWindow);
		SetControlTextWithCFString(viewWrap, CFSTR(""));
		this->setOrdinaryKeyFieldEnabled(false);
	}
}// My_MacrosPanelUI::setKeyType


/*!
Fills in the field for ordinary keys.  Normally you should
do this in tandem with a call to setKeyType() where the
type is "kCommandSetMacroKeyTypeOrdinaryChar".

(4.0)
*/
void
My_MacrosPanelUI::
setOrdinaryKeyCharacter		(UniChar	inCharacter)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			viewWrap;
	CFRetainRelease		charCFString(CFStringCreateWithCharacters(kCFAllocatorDefault, &inCharacter, 1),
										true/* is retained */);
	
	
	viewWrap = HIViewWrap(idMyFieldMacroKeyCharacter, kOwningWindow);
	if ((charCFString.exists()) && (0 != inCharacter)/* ignore zeroes from past errors */)
	{
		SetControlTextWithCFString(viewWrap, charCFString.returnCFStringRef());
	}
	else
	{
		SetControlTextWithCFString(viewWrap, CFSTR(""));
	}
}// My_MacrosPanelUI::setOrdinaryKeyCharacter


/*!
Enables or disables the views that display a completely
arbitrary key character (as opposed to a special key
type such as “page down”).

Normally, you should use setKeyType().

(3.1)
*/
void
My_MacrosPanelUI::
setOrdinaryKeyFieldEnabled	(Boolean	inIsEnabled)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			viewWrap;
	
	
	viewWrap = HIViewWrap(idMyFieldMacroKeyCharacter, kOwningWindow);
	viewWrap << HIViewWrap_SetActiveState(inIsEnabled);
}// My_MacrosPanelUI::setOrdinaryKeyFieldEnabled


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
OSStatus
accessDataBrowserItemData	(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus				result = noErr;
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
	
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kDataBrowserItemIsEditableProperty:
			result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
			break;
		
		case kMyDataBrowserPropertyIDMacroName:
			// return the text string for the macro name
			if (nullptr == panelDataPtr)
			{
				result = errDataBrowserItemNotFound;
			}
			else
			{
				Preferences_Index	macroIndex = STATIC_CAST(inItemID, Preferences_Index);
				CFStringRef			nameCFString = nullptr;
				Preferences_Result	prefsResult = kPreferences_ResultOK;
				
				
				prefsResult = Preferences_ContextGetData
								(panelDataPtr->dataModel,
									Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex),
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
		
		case kMyDataBrowserPropertyIDMacroNumber:
			// return the macro number as a string
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
		case kMyDataBrowserPropertyIDMacroName:
			// user has changed the macro name; update the macro in memory
			if (nullptr == panelDataPtr)
			{
				result = errDataBrowserItemNotFound;
			}
			else
			{
				CFStringRef		newName = nullptr;
				
				
				result = GetDataBrowserItemDataText(inItemData, &newName);
				if (noErr == result)
				{
					// fix macro name
					Preferences_Index	macroIndex = STATIC_CAST(inItemID, Preferences_Index);
					Preferences_Result	prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextSetData
									(panelDataPtr->dataModel,
										Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex),
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
		
		case kMyDataBrowserPropertyIDMacroNumber:
			// read-only
			result = paramErr;
			break;
		
		default:
			// ???
			result = errDataBrowserPropertyNotSupported;
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
compareDataBrowserItems		(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	Boolean					result = false;
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
	
	switch (inSortProperty)
	{
	case kMyDataBrowserPropertyIDMacroName:
		{
			CFStringRef		string1 = nullptr;
			CFStringRef		string2 = nullptr;
			
			
			if (nullptr != panelDataPtr)
			{
				SInt32		macroIndex1 = STATIC_CAST(inItemOne, SInt32);
				SInt32		macroIndex2 = STATIC_CAST(inItemTwo, SInt32);
				
				
				// ignore results, the strings are checked below
				UNUSED_RETURN(Preferences_Result)Preferences_ContextGetData
													(panelDataPtr->dataModel,
														Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex1),
														sizeof(string1), &string1, false/* search defaults too */);
				UNUSED_RETURN(Preferences_Result)Preferences_ContextGetData
													(panelDataPtr->dataModel,
														Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex2),
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
Responds to changes in the data browser.

(3.1)
*/
void
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
				Console_Warning(Console_WriteLine, "unexpected problem determining selected macro!");
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


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons and menus in this panel.

(3.1)
*/
OSStatus
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
			HIWindowRef		window = HIViewGetWindow(interfacePtr->mainView);
			CFStringRef		insertedCFString = nullptr;
			
			
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandSetMacroKeyModifierCommand:
			case kCommandSetMacroKeyModifierControl:
			case kCommandSetMacroKeyModifierOption:
			case kCommandSetMacroKeyModifierShift:
				{
					UInt32					allModifiers = 0;
					My_MacrosPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																		My_MacrosPanelDataPtr);
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextGetData
									(dataPtr->dataModel, Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedMacroKeyModifiers, dataPtr->currentIndex),
										sizeof(allModifiers), &allModifiers, false/* search defaults */);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to find existing modifier key settings");
					}
					
					switch (received.commandID)
					{
					case kCommandSetMacroKeyModifierCommand:
						if (allModifiers & kMacroManager_ModifierKeyMaskCommand)
						{
							allModifiers &= ~kMacroManager_ModifierKeyMaskCommand;
						}
						else
						{
							allModifiers |= kMacroManager_ModifierKeyMaskCommand;
						}
						break;
					
					case kCommandSetMacroKeyModifierControl:
						if (allModifiers & kMacroManager_ModifierKeyMaskControl)
						{
							allModifiers &= ~kMacroManager_ModifierKeyMaskControl;
						}
						else
						{
							allModifiers |= kMacroManager_ModifierKeyMaskControl;
						}
						break;
					
					case kCommandSetMacroKeyModifierOption:
						if (allModifiers & kMacroManager_ModifierKeyMaskOption)
						{
							allModifiers &= ~kMacroManager_ModifierKeyMaskOption;
						}
						else
						{
							allModifiers |= kMacroManager_ModifierKeyMaskOption;
						}
						break;
					
					case kCommandSetMacroKeyModifierShift:
						if (allModifiers & kMacroManager_ModifierKeyMaskShift)
						{
							allModifiers &= ~kMacroManager_ModifierKeyMaskShift;
						}
						else
						{
							allModifiers |= kMacroManager_ModifierKeyMaskShift;
						}
						break;
					
					default:
						// ???
						break;
					}
					
					// update button states
					interfacePtr->setKeyModifiers(allModifiers);
					
					// save preferences
					prefsResult = Preferences_ContextSetData
									(dataPtr->dataModel, Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedMacroKeyModifiers, dataPtr->currentIndex),
										sizeof(allModifiers), &allModifiers);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to save modifier key settings");
					}
					result = noErr;
				}
				break;
			
			case kCommandSetMacroKeyTypeOrdinaryChar:
			case kCommandSetMacroKeyTypeBackwardDelete:
			case kCommandSetMacroKeyTypeForwardDelete:
			case kCommandSetMacroKeyTypeHome:
			case kCommandSetMacroKeyTypeEnd:
			case kCommandSetMacroKeyTypePageUp:
			case kCommandSetMacroKeyTypePageDown:
			case kCommandSetMacroKeyTypeUpArrow:
			case kCommandSetMacroKeyTypeDownArrow:
			case kCommandSetMacroKeyTypeLeftArrow:
			case kCommandSetMacroKeyTypeRightArrow:
			case kCommandSetMacroKeyTypeClear:
			case kCommandSetMacroKeyTypeEscape:
			case kCommandSetMacroKeyTypeReturn:
			case kCommandSetMacroKeyTypeEnter:
			case kCommandSetMacroKeyTypeF1:
			case kCommandSetMacroKeyTypeF2:
			case kCommandSetMacroKeyTypeF3:
			case kCommandSetMacroKeyTypeF4:
			case kCommandSetMacroKeyTypeF5:
			case kCommandSetMacroKeyTypeF6:
			case kCommandSetMacroKeyTypeF7:
			case kCommandSetMacroKeyTypeF8:
			case kCommandSetMacroKeyTypeF9:
			case kCommandSetMacroKeyTypeF10:
			case kCommandSetMacroKeyTypeF11:
			case kCommandSetMacroKeyTypeF12:
			case kCommandSetMacroKeyTypeF13:
			case kCommandSetMacroKeyTypeF14:
			case kCommandSetMacroKeyTypeF15:
			case kCommandSetMacroKeyTypeF16:
				{
					My_MacrosPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																		My_MacrosPanelDataPtr);
					
					
					// update menu state
					interfacePtr->setKeyType(received.commandID);
					
					// save preferences
					UNUSED_RETURN(Boolean)interfacePtr->saveKeyTypeAndCharacterPreferences(dataPtr->dataModel, dataPtr->currentIndex);
					
					result = noErr;
				}
				break;
			
			case kCommandSetMacroActionEnterTextWithSub:
			case kCommandSetMacroActionEnterTextVerbatim:
			case kCommandSetMacroActionFindTextWithSub:
			case kCommandSetMacroActionFindTextVerbatim:
			case kCommandSetMacroActionOpenURL:
			case kCommandSetMacroActionNewWindowCommand:
			case kCommandSetMacroActionSelectWindow:
				{
					My_MacrosPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																		My_MacrosPanelDataPtr);
					MacroManager_Action		actionForCommand = MacroManager_ActionForCommand(received.commandID);
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					// update menu state
					interfacePtr->setAction(received.commandID);
					
					// save preferences
					prefsResult = Preferences_ContextSetData
									(dataPtr->dataModel, Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedMacroAction, dataPtr->currentIndex),
										sizeof(actionForCommand), &actionForCommand);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to save action setting");
					}
					
					result = noErr;
				}
				break;
			
			case kCommandEditMacroTextWithControlKeys:
				{
					HIViewWrap		buttonInsertControlKey(idMyButtonInsertControlKey, window);
					
					
					if (kControlCheckBoxCheckedValue == GetControl32BitValue(buttonInsertControlKey))
					{
						// stop sending keypad control keys to the field
						Keypads_RemoveEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(window));
						
						// remove the selected state from the button
						SetControl32BitValue(buttonInsertControlKey, kControlCheckBoxUncheckedValue);
					}
					else
					{
						// show the control keys palette and target the button
						Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(window));
						
						// try to avoid confusing the user by removing all focus in this window
						// (to emphasize that the focus is in the palette)
						UNUSED_RETURN(OSStatus)ClearKeyboardFocus(window);
						
						// give the button a selected state
						SetControl32BitValue(buttonInsertControlKey, kControlCheckBoxCheckedValue);
					}
				}
				break;
			
			case kCommandKeypadControlAtSign:
				insertedCFString = CFSTR("\\000");
				break;
			
			case kCommandKeypadControlA:
				insertedCFString = CFSTR("\\001");
				break;
			
			case kCommandKeypadControlB:
				insertedCFString = CFSTR("\\002");
				break;
			
			case kCommandKeypadControlC:
				insertedCFString = CFSTR("\\003");
				break;
			
			case kCommandKeypadControlD:
				insertedCFString = CFSTR("\\004");
				break;
			
			case kCommandKeypadControlE:
				insertedCFString = CFSTR("\\005");
				break;
			
			case kCommandKeypadControlF:
				insertedCFString = CFSTR("\\006");
				break;
			
			case kCommandKeypadControlG:
				insertedCFString = CFSTR("\\007");
				break;
			
			case kCommandKeypadControlH:
				insertedCFString = CFSTR("\\010");
				break;
			
			case kCommandKeypadControlI:
				insertedCFString = CFSTR("\\011");
				break;
			
			case kCommandKeypadControlJ:
				insertedCFString = CFSTR("\\012");
				break;
			
			case kCommandKeypadControlK:
				insertedCFString = CFSTR("\\013");
				break;
			
			case kCommandKeypadControlL:
				insertedCFString = CFSTR("\\014");
				break;
			
			case kCommandKeypadControlM:
				insertedCFString = CFSTR("\\015");
				break;
			
			case kCommandKeypadControlN:
				insertedCFString = CFSTR("\\016");
				break;
			
			case kCommandKeypadControlO:
				insertedCFString = CFSTR("\\017");
				break;
			
			case kCommandKeypadControlP:
				insertedCFString = CFSTR("\\020");
				break;
			
			case kCommandKeypadControlQ:
				insertedCFString = CFSTR("\\021");
				break;
			
			case kCommandKeypadControlR:
				insertedCFString = CFSTR("\\022");
				break;
			
			case kCommandKeypadControlS:
				insertedCFString = CFSTR("\\023");
				break;
			
			case kCommandKeypadControlT:
				insertedCFString = CFSTR("\\024");
				break;
			
			case kCommandKeypadControlU:
				insertedCFString = CFSTR("\\025");
				break;
			
			case kCommandKeypadControlV:
				insertedCFString = CFSTR("\\026");
				break;
			
			case kCommandKeypadControlW:
				insertedCFString = CFSTR("\\027");
				break;
			
			case kCommandKeypadControlX:
				insertedCFString = CFSTR("\\030");
				break;
			
			case kCommandKeypadControlY:
				insertedCFString = CFSTR("\\031");
				break;
			
			case kCommandKeypadControlZ:
				insertedCFString = CFSTR("\\032");
				break;
			
			case kCommandKeypadControlLeftSquareBracket:
				//insertedCFString = CFSTR("\\033"); // could use this, but "\e" is friendlier
				insertedCFString = CFSTR("\\e");
				break;
			
			case kCommandKeypadControlBackslash:
				insertedCFString = CFSTR("\\034");
				break;
			
			case kCommandKeypadControlRightSquareBracket:
				insertedCFString = CFSTR("\\035");
				break;
			
			case kCommandKeypadControlCaret:
				insertedCFString = CFSTR("\\036");
				break;
			
			case kCommandKeypadControlUnderscore:
				insertedCFString = CFSTR("\\037");
				break;
			
			default:
				break;
			}
			
			// if a control key in the palette was clicked, insert the
			// equivalent escaped octal sequence into the macro text field
			if (nullptr != insertedCFString)
			{
				HIViewWrap		macroTextField(idMyFieldMacroText, window);
				CFStringRef		fieldText = nullptr;
				
				
				GetControlTextAsCFString(macroTextField, fieldText);
				if (nullptr != fieldText)
				{
					CFRetainRelease		newText(CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* maximum size */, fieldText));
					
					
					if (newText.exists())
					{
						CFStringAppend(newText.returnCFMutableStringRef(), insertedCFString);
						SetControlTextWithCFString(macroTextField, newText.returnCFStringRef());
						
						// try to avoid confusing the user by removing all focus in this window
						// (to emphasize that the focus is in the palette)
						UNUSED_RETURN(OSStatus)ClearKeyboardFocus(window);
					}
				}
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


#pragma mark -
@implementation PrefPanelMacros_MacroInfo //{


@synthesize preferencesIndex = _preferencesIndex;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithIndex:(Preferences_Index)	anIndex
{
	self = [super init];
	if (nil != self)
	{
		assert(anIndex >= 1);
		self->_preferencesIndex = anIndex;
	}
	return self;
}// initWithIndex:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
macroIndexLabel
{
	return [[NSNumber numberWithUnsignedInteger:self.preferencesIndex] stringValue];
}// macroIndexLabel


/*!
Accessor.

(4.1)
*/
- (NSString*)
macroName
{
	NSString*				result = @"";
	assert(0 != self.preferencesIndex);
	Preferences_Tag const	macroNameIndexedTag = Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedMacroName, self.preferencesIndex);
	CFStringRef				nameCFString = nullptr;
	Preferences_Result		prefsResult = Preferences_ContextGetData
											(self.currentContext, macroNameIndexedTag,
												sizeof(nameCFString), &nameCFString,
												false/* search defaults */);
	
	
	if (kPreferences_ResultBadVersionDataNotAvailable == prefsResult)
	{
		// this is possible for indexed tags, as not all indexes
		// in the range may have data defined; ignore
		//Console_Warning(Console_WriteValue, "failed to retrieve macro name preference, error", prefsResult);
	}
	else if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "failed to retrieve macro name preference, error", prefsResult);
	}
	else
	{
		result = BRIDGE_CAST(nameCFString, NSString*);
	}
	
	return result;
}
- (void)
setMacroName:(NSString*)	aMacroName
{
	assert(0 != self.preferencesIndex);
	Preferences_Tag const	macroNameIndexedTag = Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedMacroName, self.preferencesIndex);
	CFStringRef				asCFStringRef = BRIDGE_CAST(aMacroName, CFStringRef);
	Preferences_Result		prefsResult = Preferences_ContextSetData
											(self.currentContext, macroNameIndexedTag,
												sizeof(asCFStringRef), &asCFStringRef);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "failed to set macro name preference, error", prefsResult);
	}
}// setMacroName:


#pragma mark GenericPanelNumberedList_ListItemHeader


/*!
Return strong reference to user interface string representing
numbered index in list.

Returns the "macroIndexLabel".

(4.1)
*/
- (NSString*)
numberedListIndexString
{
	return self.macroIndexLabel;
}// numberedListIndexString


/*!
Return or update user interface string for name of item in list.

Accesses the "macroName".

(4.1)
*/
- (NSString*)
numberedListItemName
{
	return self.macroName;
}
- (void)
setNumberedListItemName:(NSString*)		aName
{
	self.macroName = aName;
}// setNumberedListItemName:


@end //}


#pragma mark -
@implementation PrefPanelMacros_ViewManager //{


@synthesize macroEditorViewManager = _macroEditorViewManager;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self.macroEditorViewManager = [[[PrefPanelMacros_MacroEditorViewManager alloc] init] autorelease];
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Macros"
										localizedName:NSLocalizedStringFromTable(@"Macros", @"PrefPanelMacros",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelMacros"]
										master:self detailViewManager:self.macroEditorViewManager];
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
- (PrefPanelMacros_MacroInfo*)
selectedMacroInfo
{
	PrefPanelMacros_MacroInfo*		result = nil;
	NSUInteger						currentIndex = [self.listItemHeaderIndexes firstIndex];
	
	
	if (NSNotFound != currentIndex)
	{
		result = [self.listItemHeaders objectAtIndex:currentIndex];
	}
	
	return result;
}// selectedMacroInfo


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
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:1] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:2] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:3] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:4] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:5] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:6] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:7] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:8] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:9] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:10] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:11] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:12] autorelease],
						];
	
	
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
	aViewManager.headingTitleForNameColumn = NSLocalizedStringFromTable(@"Macro Name", @"PrefPanelMacros", @"the title for the item name column");
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
			for (PrefPanelMacros_MacroInfo* eachInfo in aViewManager.listItemHeaders)
			{
				[eachInfo setCurrentContext:asContext];
			}
		}
	}
}// numberedListViewManager:didChangeFromDataSet:toDataSet:


@end //} PrefPanelMacros_ViewManager


#pragma mark -
@implementation PrefPanelMacros_ActionValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionSendTextVerbatim
																description:NSLocalizedStringFromTable
																			(@"Enter Text Verbatim", @"PrefPanelMacros"/* table */,
																				@"macro action: enter text verbatim")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionSendTextProcessingEscapes
																description:NSLocalizedStringFromTable
																			(@"Enter Text with Substitutions", @"PrefPanelMacros"/* table */,
																				@"macro action: enter text with substitutions")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionHandleURL
																description:NSLocalizedStringFromTable
																			(@"Open URL", @"PrefPanelMacros"/* table */,
																				@"macro action: open URL")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionNewWindowWithCommand
																description:NSLocalizedStringFromTable
																			(@"New Window with Command", @"PrefPanelMacros"/* table */,
																				@"macro action: new window with command")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionSelectMatchingWindow
																description:NSLocalizedStringFromTable
																			(@"Select Window by Title", @"PrefPanelMacros"/* table */,
																				@"macro action: select window by title")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionFindTextVerbatim
																description:NSLocalizedStringFromTable
																			(@"Find in Local Terminal Verbatim", @"PrefPanelMacros"/* table */,
																				@"macro action: find in local terminal verbatim")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionFindTextProcessingEscapes
																description:NSLocalizedStringFromTable
																			(@"Find in Local Terminal with Substitutions", @"PrefPanelMacros"/* table */,
																				@"macro action: find in local terminal with substitutions")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:0L/* set externally later */
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt32
											valueDescriptorArray:descriptorArray];
	if (nil != self)
	{
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
	[super dealloc];
}// dealloc


@end //} PrefPanelMacros_ActionValue


#pragma mark -
@implementation PrefPanelMacros_InvokeWithValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	Boolean const	kFlagIsVirtualKey = true;
	Boolean const	kFlagIsRealKey = false;
	
	
	// create a sentinel object (exact values do not matter, the only
	// thing that is ever used is the pointer identity); this allows
	// the interface to decide when to return a key-character binding
	self.placeholderDescriptor = [[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsRealKey, 0/* exact value is never used */), UInt32)
															description:NSLocalizedStringFromTable
																		(@"Ordinary Character", @"PrefPanelMacros"/* table */,
																			@"key: ordinary character")]
									autorelease];
	
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									self.placeholderDescriptor,
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x33), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌫ Backward Delete", @"PrefPanelMacros"/* table */,
																				@"key: backward delete")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x75), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌦ Forward Delete", @"PrefPanelMacros"/* table */,
																				@"key: forward delete")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x73), UInt32)
																description:NSLocalizedStringFromTable
																			(@"↖︎ Home", @"PrefPanelMacros"/* table */,
																				@"key: home")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x77), UInt32)
																description:NSLocalizedStringFromTable
																			(@"↘︎ End", @"PrefPanelMacros"/* table */,
																				@"key: end")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x74), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇞ Page Up", @"PrefPanelMacros"/* table */,
																				@"key: page up")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x79), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇟ Page Down", @"PrefPanelMacros"/* table */,
																				@"key: page down")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x7E), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇡ Up Arrow", @"PrefPanelMacros"/* table */,
																				@"key: up arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x7D), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇣ Down Arrow", @"PrefPanelMacros"/* table */,
																				@"key: down arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x7B), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇠ Left Arrow", @"PrefPanelMacros"/* table */,
																				@"key: left arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x7C), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇢ Right Arrow", @"PrefPanelMacros"/* table */,
																				@"key: right arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x47), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌧ Clear", @"PrefPanelMacros"/* table */,
																				@"key: clear")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x35), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⎋ Escape", @"PrefPanelMacros"/* table */,
																				@"key: escape")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x24), UInt32)
																description:NSLocalizedStringFromTable
																			(@"↩︎ Return", @"PrefPanelMacros"/* table */,
																				@"key: return")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x4C), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌤ Enter", @"PrefPanelMacros"/* table */,
																				@"key: enter")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x7A), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F1 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F1")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x78), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F2 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F2")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x63), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F3 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F3")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x76), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F4 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F4")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x60), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F5 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F5")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x61), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F6 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F6")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x62), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F7 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F7")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x64), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F8 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F8")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x65), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F9 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F9")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x6D), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F10 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F10")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x67), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F11 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F11")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x6F), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F12 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F12")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x69), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F13 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F13")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x6B), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F14 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F14")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x71), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F15 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F15")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, 0x6A), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F16 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F16")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:0L/* set externally later */
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt32
											valueDescriptorArray:descriptorArray];
	if (nil != self)
	{
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
currentOrdinaryCharacter
{
	PreferenceValue_IntegerDescriptor*		asDesc = STATIC_CAST(self.currentValueDescriptor, PreferenceValue_IntegerDescriptor*);
	NSString*								result = @"";
	
	
	// most values map to specific virtual keys; if the current
	// setting is the “ordinary character” entry however, then
	// a non-empty value should be returned for this field
	if (self.placeholderDescriptor == asDesc)
	{
		// read the preferences directly to find an appropriate string
		Preferences_Tag			keyTag = self.preferencesTag; // set externally
		UInt32					intResult = 0;
		Preferences_Result		prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, keyTag,
																			sizeof(intResult), &intResult,
																			true/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to read macro ordinary-key preference; error", prefsResult);
		}
		else
		{
			MacroManager_KeyID		asKeyID = STATIC_CAST(intResult, MacroManager_KeyID);
			
			
			if (false == MacroManager_KeyIDIsVirtualKey(asKeyID))
			{
				// current preference contains a real-key mapping;
				// return the corresponding character as a string
				unichar		stringData[] = { MacroManager_KeyIDKeyCode(asKeyID) };
				
				
				result = [NSString stringWithCharacters:stringData length:1];
			}
		}
	}
	
	return result;
}
- (void)
setCurrentOrdinaryCharacter:(NSString*)		aKeyString
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentOrdinaryCharacter"];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self willChangeValueForKey:@"isOrdinaryCharacter"];
	
	if (nil == aKeyString)
	{
		[self setNilPreferenceValue];
	}
	else if (aKeyString.length > 1)
	{
		Console_Warning(Console_WriteValueCFString, "ignoring character mapping with length greater than 1", BRIDGE_CAST(aKeyString, CFStringRef));
	}
	else
	{
		// TEMPORARY; this setting is not really portable across
		// different languages and keyboard types; at some point
		// a character code will not be representable
		unichar					characterCode = [aKeyString characterAtIndex:0];
		Preferences_Tag			keyTag = self.preferencesTag; // set externally
		MacroManager_KeyID		keyID = MacroManager_MakeKeyID(false/* is virtual key */, characterCode);
		UInt32					intValue = STATIC_CAST(keyID, UInt32);
		Preferences_Result		prefsResult = Preferences_ContextSetData(self.prefsMgr.currentContext, keyTag,
																			sizeof(intValue), &intValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set macro ordinary-key preference; error", prefsResult);
		}
		
		// update the descriptor (represented by a menu item selection)
		// to indicate that the macro maps to an ordinary key; the
		// associated integer value MUST match the preference setting
		// to prevent future bindings from clearing the setting
		STATIC_CAST(self.placeholderDescriptor, PreferenceValue_IntegerDescriptor*).describedIntegerValue = intValue;
		self.currentValueDescriptor = self.placeholderDescriptor;
	}
	
	[self didChangeValueForKey:@"isOrdinaryCharacter"];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didChangeValueForKey:@"currentOrdinaryCharacter"];
	[self didSetPreferenceValue];
}// setCurrentOrdinaryCharacter:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isOrdinaryCharacter
{
	BOOL	result = (NO == [[self currentOrdinaryCharacter] isEqualToString:@""]);
	
	
	return result;
}// isOrdinaryCharacter


#pragma mark NSKeyValueObserving

/*!
Specifies the names of properties that trigger updates
to the given property.  (Called by the Key-Value Coding
mechanism as needed.)

Usually this can be achieved by sending update messages
for multiple properties from a single set-method.  This
approach is used when the set-method is not easily
accessible.

(4.1)
*/
+ (NSSet*)
keyPathsForValuesAffectingValueForKey:(NSString*)	aKey
{
	NSSet*	result = nil;
	
	
	// return other properties that (if changed) should cause
	// the specified property to be updated
	if ([aKey isEqualToString:@"isOrdinaryCharacter"])
	{
		// since the superclass implements "currentValueDescriptor",
		// it is easier to set up the relationship here (this causes
		// the ordinary-character field to be enabled/disabled as
		// the menu selection changes to/from “Ordinary Character”)
		result = [NSSet setWithObjects:@"currentValueDescriptor", nil];
	}
	else
	{
		result = [super keyPathsForValuesAffectingValueForKey:aKey];
	}
	
	return result;
}// keyPathsForValuesAffectingValueForKey:


@end //} PrefPanelMacros_InvokeWithValue


#pragma mark -
@implementation PrefPanelMacros_ModifiersValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskCommand
																description:NSLocalizedStringFromTable
																			(@"⌘", @"PrefPanelMacros"/* table */,
																				@"Command key modifier")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskOption
																description:NSLocalizedStringFromTable
																			(@"⌥", @"PrefPanelMacros"/* table */,
																				@"Option key modifier")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskControl
																description:NSLocalizedStringFromTable
																			(@"⌃", @"PrefPanelMacros"/* table */,
																				@"Control key modifier")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskShift
																description:NSLocalizedStringFromTable
																			(@"⇧", @"PrefPanelMacros"/* table */,
																				@"Shift key modifier")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:0L/* set externally later */
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt32
											valueDescriptorArray:descriptorArray];
	if (nil != self)
	{
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
	[super dealloc];
}// dealloc


@end //} PrefPanelMacros_ModifiersValue


#pragma mark -
@implementation PrefPanelMacros_MacroEditorViewManager //{


@synthesize bindControlKeyPad = _bindControlKeyPad;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelMacrosCocoa" delegate:self context:nullptr];
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
	[prefsMgr release];
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
Displays the Control Keys palette to choose a key that
can be transformed into an appropriate substitution
sequence (for instance, inserting "\004" for a ⌃D).

(4.1)
*/
- (IBAction)
performInsertControlKeyCharacter:(id)	sender
{
#pragma unused(sender)
	// toggle display of Control Keys palette (while enabled,
	// responder invokes "controlKeypadSentCharacterCode:")
	if (self.bindControlKeyPad)
	{
		Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self);
	}
	else
	{
		Keypads_RemoveResponder(kKeypads_WindowTypeControlKeys, self);
	}
}// performInsertControlKeyCharacter:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_ActionValue*)
macroAction
{
	return [self->byKey objectForKey:@"macroAction"];
}// macroAction


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
macroContents
{
	return [self->byKey objectForKey:@"macroContents"];
}// macroContents


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_InvokeWithValue*)
macroKey
{
	return [self->byKey objectForKey:@"macroKey"];
}// macroKey


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_ModifiersValue*)
macroModifiers
{
	return [self->byKey objectForKey:@"macroModifiers"];
}// macroModifiers


#pragma mark Keypads_SetResponder() Interface


/*!
Received when the Control Keys keypad is hidden by the
user while this class instance is the current keypad
responder (as set by Keypads_SetResponder()).

This handles the event by updating the button state.

(4.1)
*/
- (void)
controlKeypadHidden
{
	self.bindControlKeyPad = NO;
}// controlKeypadHidden


/*!
Received when the Control Keys keypad is used to select
a control key while this class instance is the current
keypad responder (as set by Keypads_SetResponder()).

This handles the event by inserting an appropriate macro
key sequence into the content field.  If the field is
currently being edited then the actively-edited string
is changed instead.

(4.1)
*/
- (void)
controlKeypadSentCharacterCode:(NSNumber*)	asciiChar
{
	// NOTE: the base string cannot be "self.macroContents.stringValue"
	// because the corresponding text field might be in editing mode
	// (and the binding would not be up-to-date); therefore the base
	// string comes directly from the field itself
	NSString*	originalContents = self->contentsField.stringValue;
	NSString*	newContents = nil;
	NSUInteger	charValue = [asciiChar unsignedIntegerValue];
	
	
	// insert the corresponding key value into the field
	if (0x08 == charValue)
	{
		// this is the sequence for “backspace”, which has a more
		// convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\b", originalContents];
	}
	else if (0x09 == charValue)
	{
		// this is the sequence for “horizontal tab”, which has a
		// more convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\t", originalContents];
	}
	else if (0x0D == charValue)
	{
		// this is the sequence for “carriage return”, which has
		// a more convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\r", originalContents];
	}
	else if (0x1B == charValue)
	{
		// this is the sequence for “escape”, which has a more
		// convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\e", originalContents];
	}
	else
	{
		// generate an escape sequence of the form recognized by
		// the Macro Manager’s “text with substitutions” parser
		char const*			leadingDigits = ((charValue < 8) ? "00" : "0");
		std::ostringstream	stringStream;
		std::string			stringValue;
		
		
		stringStream << std::oct << charValue;
		stringValue = stringStream.str();
		newContents = [NSString stringWithFormat:@"%@\\%s%s", originalContents, leadingDigits, stringValue.c_str()];
	}
	
	self.macroContents.stringValue = newContents;
}// controlKeypadSentCharacterCode:


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
#pragma unused(aViewManager)
	assert(nil != byKey);
	assert(nil != prefsMgr);
	assert(nil != contentsField);
	
	// remember frame from XIB (it might be changed later)
	self->idealSize = [aContainerView frame].size;
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	// NOTE: These can be changed via callback to a different combination
	// of context and index.
	[self configureForIndex:1];
	
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
#pragma unused(aViewManager)
	if (kPanel_VisibilityDisplayed != aVisibility)
	{
		// if the Control Keys palette was displayed to help edit macros,
		// remove the binding when the panel disappears (otherwise the
		// user could click a key later and inadvertently change a macro
		// in the invisible panel)
		Keypads_RemoveResponder(kKeypads_WindowTypeControlKeys, self);
	}
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
	//GenericPanelNumberedList_DataSet*	oldStructPtr = REINTERPRET_CAST(oldDataSet, GenericPanelNumberedList_DataSet*);
	GenericPanelNumberedList_DataSet*	newStructPtr = REINTERPRET_CAST(newDataSet, GenericPanelNumberedList_DataSet*);
	Preferences_ContextRef				asPrefsContext = REINTERPRET_CAST(newStructPtr->parentPanelDataSetOrNull, Preferences_ContextRef);
	Preferences_Index					asIndex = STATIC_CAST(1 + newStructPtr->selectedListItem, Preferences_Index); // index is one-based
	
	
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
			// user has selected an entirely different preferences collection
			self.macroAction.prefsMgr.currentContext = asPrefsContext;
			self.macroContents.prefsMgr.currentContext = asPrefsContext;
			self.macroKey.prefsMgr.currentContext = asPrefsContext;
			self.macroModifiers.prefsMgr.currentContext = asPrefsContext;
		}
		// user may or may not have selected a different macro
		// (the response is currently the same either way)
		[self configureForIndex:asIndex];
		
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
	return [NSImage imageNamed:@"IconForPrefPanelMacros"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Macros";
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
	return NSLocalizedStringFromTable(@"Macros", @"PrefPanelMacros", @"the name of this panel");
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
	return Quills::Prefs::MACRO_SET;
}// preferencesClass


@end //} PrefPanelMacros_MacroEditorViewManager


#pragma mark -
@implementation PrefPanelMacros_MacroEditorViewManager (PrefPanelMacros_MacroEditorViewManagerInternal) //{


#pragma mark New Methods


/*!
Creates and/or configures index-dependent internal objects used
for value bindings.

NOTE:	This does NOT trigger any will/did callbacks because it
		is typically called in places where these are already
		being called.  Callbacks are necessary to ensure that
		user interface elements are updated, for instance.

(4.1)
*/
- (void)
configureForIndex:(Preferences_Index)	anIndex
{
	if (nil == self.macroAction)
	{
		[self->byKey setObject:[[[PrefPanelMacros_ActionValue alloc]
									initWithContextManager:self->prefsMgr] autorelease]
						forKey:@"macroAction"];
	}
	self.macroAction.preferencesTag = Preferences_ReturnTagVariantForIndex
										(kPreferences_TagIndexedMacroAction, anIndex);
	
	if (nil == self.macroContents)
	{
		[self->byKey setObject:[[[PreferenceValue_String alloc]
									initWithPreferencesTag:Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedMacroContents, anIndex)
															contextManager:self->prefsMgr] autorelease]
						forKey:@"macroContents"];
	}
	self.macroContents.preferencesTag = Preferences_ReturnTagVariantForIndex
											(kPreferences_TagIndexedMacroContents, anIndex);
	
	if (nil == self.macroKey)
	{
		[self->byKey setObject:[[[PrefPanelMacros_InvokeWithValue alloc]
									initWithContextManager:self->prefsMgr] autorelease]
						forKey:@"macroKey"];
	}
	self.macroKey.preferencesTag = Preferences_ReturnTagVariantForIndex
									(kPreferences_TagIndexedMacroKey, anIndex);
	
	if (nil == self.macroModifiers)
	{
		[self->byKey setObject:[[[PrefPanelMacros_ModifiersValue alloc]
									initWithContextManager:self->prefsMgr] autorelease]
						forKey:@"macroModifiers"];
	}
	self.macroModifiers.preferencesTag = Preferences_ReturnTagVariantForIndex
											(kPreferences_TagIndexedMacroKeyModifiers, anIndex);
}// configureForIndex:


#pragma mark Accessors


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"macroAction", @"macroContents", @"macroKey", @"macroModifiers"];
}// primaryDisplayBindingKeys


@end //} PrefPanelMacros_MacroEditorViewManager (PrefPanelMacros_MacroEditorViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
