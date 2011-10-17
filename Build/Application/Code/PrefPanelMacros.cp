/*!	\file PrefPanelMacros.cp
	\brief Implements the Macros panel of Preferences.
*/
/*###############################################################

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

#include "PrefPanelMacros.h"
#include <UniversalDefines.h>

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
#include "SpacingConstants.r"

// application includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Keypads.h"
#include "MacroManager.h"
#include "Panel.h"
#include "Preferences.h"
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
_fieldKeyCharInputHandler	(GetControlEventTarget(HIViewWrap(idMyFieldMacroKeyCharacter, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldContentsInputHandler	(GetControlEventTarget(HIViewWrap(idMyFieldMacroText, inOwningWindow)), receiveFieldChanged,
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
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderName,
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
					 Preferences_Index			inOneBasedIndex)
{
	assert(0 != inOneBasedIndex);
	if (nullptr != inSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// set key type
		{
			MacroManager_KeyID		macroKey = 0;
			
			
			prefsResult = Preferences_ContextGetData
							(inSettings, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, inOneBasedIndex),
								sizeof(macroKey), &macroKey, false/* search defaults too */, &actualSize);
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
								sizeof(modifiers), &modifiers, false/* search defaults too */, &actualSize);
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
								sizeof(actionPerformed), &actionPerformed, false/* search defaults too */, &actualSize);
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
								sizeof(actionCFString), &actionCFString, false/* search defaults too */, &actualSize);
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
	(OSStatus)UpdateDataBrowserItems(macrosListContainer, kDataBrowserNoItem/* parent item */,
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
					Console_Warning(Console_WriteLine, "ordinary character requested for macro, but no character was entered");
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
	(OSStatus)DialogUtilities_SetPopUpItemByCommand(viewWrap, inSetMacroActionCommandID);
	if (inSetMacroActionCommandID != kCommandSetMacroActionEnterText)
	{
		(OSStatus)DeactivateControl(HIViewWrap(idMyButtonInsertControlKey, kOwningWindow));
	}
	else
	{
		(OSStatus)ActivateControl(HIViewWrap(idMyButtonInsertControlKey, kOwningWindow));
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
Set the state of all user interface elements that represent
the selected modifier keys of the current macro.

The specified integer uses the same bit flags as events do.

(3.1)
*/
void
My_MacrosPanelUI::
setKeyModifiers		(UInt32		inModifiers)
{
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierCommand, HIViewGetWindow(this->mainView)),
							(inModifiers & cmdKey) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierControl, HIViewGetWindow(this->mainView)),
							(inModifiers & controlKey) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierOption, HIViewGetWindow(this->mainView)),
							(inModifiers & optionKey) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(HIViewWrap(idMyButtonInvokeWithModifierShift, HIViewGetWindow(this->mainView)),
							(inModifiers & shiftKey) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
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
	(OSStatus)DialogUtilities_SetPopUpItemByCommand(viewWrap, inKeyCommandID);
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
			{
				Preferences_Index	macroIndex = STATIC_CAST(inItemID, Preferences_Index);
				CFStringRef			nameCFString = nullptr;
				Preferences_Result	prefsResult = kPreferences_ResultOK;
				size_t				actualSize = 0;
				
				
				prefsResult = Preferences_ContextGetData
								(panelDataPtr->dataModel,
									Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex),
									sizeof(nameCFString), &nameCFString, false/* search defaults too */, &actualSize);
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
				SInt32				macroIndex1 = STATIC_CAST(inItemOne, SInt32);
				SInt32				macroIndex2 = STATIC_CAST(inItemTwo, SInt32);
				Preferences_Result	prefsResult = kPreferences_ResultOK;
				size_t				actualSize = 0;
				
				
				// ignore results, the strings are checked below
				prefsResult = Preferences_ContextGetData
								(panelDataPtr->dataModel,
									Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex1),
									sizeof(string1), &string1, false/* search defaults too */, &actualSize);
				prefsResult = Preferences_ContextGetData
								(panelDataPtr->dataModel,
									Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, macroIndex2),
									sizeof(string2), &string2, false/* search defaults too */, &actualSize);
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
						if (allModifiers & cmdKey) allModifiers &= ~cmdKey;
						else allModifiers |= cmdKey;
						break;
					
					case kCommandSetMacroKeyModifierControl:
						if (allModifiers & controlKey) allModifiers &= ~controlKey;
						else allModifiers |= controlKey;
						break;
					
					case kCommandSetMacroKeyModifierOption:
						if (allModifiers & optionKey) allModifiers &= ~optionKey;
						else allModifiers |= optionKey;
						break;
					
					case kCommandSetMacroKeyModifierShift:
						if (allModifiers & shiftKey) allModifiers &= ~shiftKey;
						else allModifiers |= shiftKey;
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
					(Boolean)interfacePtr->saveKeyTypeAndCharacterPreferences(dataPtr->dataModel, dataPtr->currentIndex);
					
					result = noErr;
				}
				break;
			
			case kCommandSetMacroActionEnterText:
			case kCommandSetMacroActionEnterTextVerbatim:
			case kCommandSetMacroActionOpenURL:
			case kCommandSetMacroActionNewWindowCommand:
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
						Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, nullptr);
						
						// remove the selected state from the button
						SetControl32BitValue(buttonInsertControlKey, kControlCheckBoxUncheckedValue);
					}
					else
					{
						// show the control keys palette and target the button
						Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(window));
						
						// try to avoid confusing the user by removing all focus in this window
						// (to emphasize that the focus is in the palette)
						(OSStatus)ClearKeyboardFocus(window);
						
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
			
			case kCommandKeypadControlTilde:
				insertedCFString = CFSTR("\\036");
				break;
			
			case kCommandKeypadControlQuestionMark:
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
						(OSStatus)ClearKeyboardFocus(window);
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

// BELOW IS REQUIRED NEWLINE TO END FILE
