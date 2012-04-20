/*!	\file PrefPanelTerminals.cp
	\brief Implements the Terminals panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#include "PrefPanelTerminals.h"
#include <UniversalDefines.h>

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
#include "SpacingConstants.r"

// application includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "GenericPanelTabs.h"
#include "Panel.h"
#include "Preferences.h"
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
HIViewID const	idMyLabelScrollSpeedFast		= { 'LScF', 0/* ID */ };
HIViewID const	idMyHelpTextTweaks				= { 'TwkH', 0/* ID */ };

// The following cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMy_DataBrowserPropertyIDTweakIsEnabled		= 'TwOn',
	kMy_DataBrowserPropertyIDTweakName			= 'TwNm'
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
struct My_EmulationTweaksDataBrowserCallbacks
{
	My_EmulationTweaksDataBrowserCallbacks		();
	~My_EmulationTweaksDataBrowserCallbacks		();
	
	DataBrowserCallbacks	_listCallbacks;
};

/*!
Implements the “Emulation” tab.
*/
struct My_TerminalsPanelEmulationUI
{
	My_TerminalsPanelEmulationUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref								panel;			//!< the panel using this UI
	Float32									idealWidth;		//!< best size in pixels
	Float32									idealHeight;	//!< best size in pixels
	My_EmulationTweaksDataBrowserCallbacks	listCallbacks;
	HIViewWrap								mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	static OSStatus
	receiveFieldChanged		(EventHandlerCallRef, EventRef, void*);
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	void
	saveFieldPreferences	(Preferences_ContextRef);
	
	void
	setAnswerBack	(CFStringRef);
	
	void
	setAnswerBackFromEmulator	(Terminal_Emulator);
	
	void
	setEmulator		(Terminal_Emulator, Boolean = true);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	void
	setEmulationTweaksDataBrowserColumnWidths	();
	
private:
	CarbonEventHandlerWrap				_menuCommandsHandler;			//!< responds to menu selections
	CarbonEventHandlerWrap				_fieldAnswerBackInputHandler;	//!< saves field settings when they change
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
	
	static OSStatus
	receiveFieldChanged		(EventHandlerCallRef, EventRef, void*);
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	void
	saveFieldPreferences	(Preferences_ContextRef);
	
	void
	setColumns		(UInt16);
	
	void
	setRows		(UInt16);
	
	void
	setScrollbackCustomizationEnabled	(Boolean);
	
	void
	setScrollbackRows	(UInt32);
	
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

OSStatus	accessDataBrowserItemData	(ControlRef, DataBrowserItemID, DataBrowserPropertyID,
										 DataBrowserItemDataRef, Boolean);
Boolean		compareDataBrowserItems		(ControlRef, DataBrowserItemID, DataBrowserItemID, DataBrowserPropertyID);

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
	
	
	tabList.push_back(PrefPanelTerminals_NewEmulationPane());
	tabList.push_back(PrefPanelTerminals_NewScreenPane());
	tabList.push_back(PrefPanelTerminals_NewOptionsPane());
	
	if (UIStrings_Copy(kUIStrings_PrefPanelTerminalsCategoryName, nameCFString).ok())
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
		if (UIStrings_Copy(kUIStrings_PrefPanelTerminalsEmulationTabName, nameCFString).ok())
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
		if (UIStrings_Copy(kUIStrings_PrefPanelTerminalsOptionsTabName, nameCFString).ok())
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
		if (UIStrings_Copy(kUIStrings_PrefPanelTerminalsScreenTabName, nameCFString).ok())
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


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.0)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewScreenPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalScreenColumns);
	tagList.push_back(kPreferences_TagTerminalScreenRows);
	tagList.push_back(kPreferences_TagTerminalScreenScrollbackRows);
	tagList.push_back(kPreferences_TagTerminalScreenScrollbackType);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewScreenPaneTagSet


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_EmulationTweaksDataBrowserCallbacks structure.

(3.1)
*/
My_EmulationTweaksDataBrowserCallbacks::
My_EmulationTweaksDataBrowserCallbacks ()
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
}// My_EmulationTweaksDataBrowserCallbacks default constructor


/*!
Tears down a My_EmulationTweaksDataBrowserCallbacks structure.

(3.1)
*/
My_EmulationTweaksDataBrowserCallbacks::
~My_EmulationTweaksDataBrowserCallbacks ()
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
}// My_EmulationTweaksDataBrowserCallbacks destructor


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
panel						(inPanel),
idealWidth					(0.0),
idealHeight					(0.0),
listCallbacks				(),
mainView					(createContainerView(inPanel, inOwningWindow)
								<< HIViewWrap_AssertExists),
_menuCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
_fieldAnswerBackInputHandler(GetControlEventTarget(HIViewWrap(idMyFieldAnswerBackMessage, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_containerResizer			(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
								My_TerminalsPanelEmulationUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_menuCommandsHandler.isInstalled());
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
	
	// create the emulation tweaks list; insert appropriate columns
	{
		HIViewWrap							listDummy(idMyDataBrowserHacks, inOwningWindow);
		HIViewRef							emulationTweaksList = nullptr;
		DataBrowserTableViewColumnIndex		columnNumber = 0;
		DataBrowserListViewColumnDesc		columnInfo;
		Rect								bounds;
		UIStrings_Result					stringResult = kUIStrings_ResultOK;
		
		
		GetControlBounds(listDummy, &bounds);
		
		// NOTE: here, the original variable is being *replaced* with the data browser, as
		// the original user pane was only needed for size definition
		error = CreateDataBrowserControl(inOwningWindow, &bounds, kDataBrowserListView, &emulationTweaksList);
		assert_noerr(error);
		error = SetControlID(emulationTweaksList, &idMyDataBrowserHacks);
		assert_noerr(error);
		
		bzero(&columnInfo, sizeof(columnInfo));
		
		// set defaults for all columns, then override below
		columnInfo.propertyDesc.propertyID = '----';
		columnInfo.propertyDesc.propertyType = kDataBrowserTextType;
		columnInfo.propertyDesc.propertyFlags = kDataBrowserDefaultPropertyFlags | kDataBrowserListViewSortableColumn;
		columnInfo.headerBtnDesc.version = kDataBrowserListViewLatestHeaderDesc;
		columnInfo.headerBtnDesc.minimumWidth = 200; // arbitrary
		columnInfo.headerBtnDesc.maximumWidth = 600; // arbitrary
		columnInfo.headerBtnDesc.titleOffset = 0; // arbitrary
		columnInfo.headerBtnDesc.titleString = nullptr;
		columnInfo.headerBtnDesc.initialOrder = 0;
		columnInfo.headerBtnDesc.btnFontStyle.flags = kControlUseJustMask;
		columnInfo.headerBtnDesc.btnFontStyle.just = teFlushDefault;
		columnInfo.headerBtnDesc.btnContentInfo.contentType = kControlContentTextOnly;
		
		// create checkbox column
		columnInfo.headerBtnDesc.titleString = CFSTR("");
		columnInfo.headerBtnDesc.minimumWidth = 32; // arbitrary
		columnInfo.headerBtnDesc.maximumWidth = 32; // arbitrary
		columnInfo.propertyDesc.propertyID = kMy_DataBrowserPropertyIDTweakIsEnabled;
		columnInfo.propertyDesc.propertyType = kDataBrowserCheckboxType;
		columnInfo.propertyDesc.propertyFlags |= kDataBrowserPropertyIsMutable;
		error = AddDataBrowserListViewColumn(emulationTweaksList, &columnInfo, columnNumber++);
		assert_noerr(error);
		
		// create tweak name column
		stringResult = UIStrings_Copy(kUIStrings_PrefPanelTerminalsListHeaderTweakName,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.headerBtnDesc.minimumWidth = 200; // arbitrary
			columnInfo.headerBtnDesc.maximumWidth = 600; // arbitrary
			columnInfo.propertyDesc.propertyID = kMy_DataBrowserPropertyIDTweakName;
			columnInfo.propertyDesc.propertyType = kDataBrowserTextType;
			columnInfo.propertyDesc.propertyFlags &= ~kDataBrowserPropertyIsMutable;
			error = AddDataBrowserListViewColumn(emulationTweaksList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// automatically adjust initial widths of variable-sized columns
		this->setEmulationTweaksDataBrowserColumnWidths();
		
		// insert as many rows as there are preferences tags for terminal emulation tweaks
		{
			DataBrowserItemID const		kTweakTags[] =
										{
											kPreferences_TagVT100FixLineWrappingBug,
											kPreferences_TagXTerm256ColorsEnabled,
											kPreferences_TagXTermBackgroundColorEraseEnabled,
											kPreferences_TagXTermColorEnabled,
											kPreferences_TagXTermGraphicsEnabled,
											kPreferences_TagXTermWindowAlterationEnabled
										};
			
			
			error = AddDataBrowserItems(emulationTweaksList, kDataBrowserNoItem/* parent item */,
										sizeof(kTweakTags) / sizeof(DataBrowserItemID), kTweakTags/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */);
			assert_noerr(error);
		}
		
		// attach data that would not be specifiable in a NIB
		error = SetDataBrowserCallbacks(emulationTweaksList, &this->listCallbacks._listCallbacks);
		assert_noerr(error);
		
		// initialize sort column
		error = SetDataBrowserSortProperty(emulationTweaksList, kMy_DataBrowserPropertyIDTweakName);
		assert_noerr(error);
		
		// set other nice things (most can be set in a NIB someday)
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		if (FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)DataBrowserChangeAttributes(emulationTweaksList,
													FUTURE_SYMBOL(1 << 1, kDataBrowserAttributeListViewAlternatingRowColors)/* attributes to set */,
													0/* attributes to clear */);
		}
	#endif
		(OSStatus)SetDataBrowserListViewUsePlainBackground(emulationTweaksList, false);
		(OSStatus)SetDataBrowserHasScrollBars(emulationTweaksList, false/* horizontal */, true/* vertical */);
		
		// attach panel to data browser
		error = SetControlProperty(emulationTweaksList, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeOwningPanel,
									sizeof(inPanel), &inPanel);
		assert_noerr(error);
		
		error = HIViewAddSubview(result, emulationTweaksList);
		assert_noerr(error);
	}
	
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
			 void*			inContext)
{
	HIWindowRef const				kPanelWindow = HIViewGetWindow(inContainer);
	My_TerminalsPanelEmulationUI*	dataPtr = REINTERPRET_CAST(inContext, My_TerminalsPanelEmulationUI*);
	HIViewWrap						viewWrap;
	
	
	// INCOMPLETE
	viewWrap = HIViewWrap(idMyFieldAnswerBackMessage, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyDataBrowserHacks, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyHelpTextTweaks, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	
	dataPtr->setEmulationTweaksDataBrowserColumnWidths();
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
			My_TerminalsPanelEmulationDataPtr	panelDataPtr = REINTERPRET_CAST(inDataPtr, My_TerminalsPanelEmulationDataPtr);
			
			
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
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			My_TerminalsPanelEmulationDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelEmulationDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
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
	
	case kPanel_MessageGetUsefulResizeAxes: // request for panel to return the directions in which resizing makes sense
		result = kPanel_ResponseResizeHorizontal;
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
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::TERMINAL);
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
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// INCOMPLETE
		
		// set emulation type
		{
			Terminal_Emulator		emulatorType = kTerminal_EmulatorVT100;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalEmulatorType, sizeof(emulatorType),
														&emulatorType, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setEmulator(emulatorType, false/* set answer-back */);
			}
		}
		
		// set answer-back message
		{
			CFStringRef		messageCFString = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalAnswerBackMessage,
														sizeof(messageCFString), &messageCFString, true/* search defaults too */,
														&actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAnswerBack(messageCFString);
			}
		}
		
		// set emulation options
		{
			DataBrowserItemID const		kUpdatedItems = { kDataBrowserNoItem };
			HIViewWrap					dataBrowser(idMyDataBrowserHacks, HIViewGetWindow(this->mainView));
			
			
			(OSStatus)UpdateDataBrowserItems(dataBrowser, kDataBrowserNoItem/* parent */,
												sizeof(kUpdatedItems) / sizeof(DataBrowserItemID), &kUpdatedItems,
												kDataBrowserItemNoProperty/* pre-sort property */,
												kMy_DataBrowserPropertyIDTweakIsEnabled/* updated property */);
		}
	}
}// My_TerminalsPanelEmulationUI::readPreferences


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the fields in this panel by saving
their preferences when new text arrives.

(3.1)
*/
OSStatus
My_TerminalsPanelEmulationUI::
receiveFieldChanged		(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_TerminalsPanelEmulationUI*	emulationInterfacePtr = REINTERPRET_CAST(inMyTerminalsPanelUIPtr, My_TerminalsPanelEmulationUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with preferences
	{
		My_TerminalsPanelEmulationDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(emulationInterfacePtr->panel),
																			My_TerminalsPanelEmulationDataPtr);
		
		
		emulationInterfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
	}
	
	return result;
}// My_TerminalsPanelEmulationUI::receiveFieldChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons and menus in this panel.

(3.1)
*/
OSStatus
My_TerminalsPanelEmulationUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_TerminalsPanelEmulationUI*	emulationInterfacePtr = REINTERPRET_CAST(inMyTerminalsPanelUIPtr, My_TerminalsPanelEmulationUI*);
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
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandSetEmulatorNone:
			case kCommandSetEmulatorVT100:
			case kCommandSetEmulatorVT102:
			case kCommandSetEmulatorVT220:
			case kCommandSetEmulatorXTermOriginal:
				{
					Terminal_Emulator					newEmulator = kTerminal_EmulatorDumb;
					My_TerminalsPanelEmulationDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(emulationInterfacePtr->panel),
																					My_TerminalsPanelEmulationDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					switch (received.commandID)
					{
					case kCommandSetEmulatorNone:
						newEmulator = kTerminal_EmulatorDumb;
						break;
					
					case kCommandSetEmulatorVT100:
						newEmulator = kTerminal_EmulatorVT100;
						break;
					
					case kCommandSetEmulatorVT102:
						newEmulator = kTerminal_EmulatorVT102;
						break;
					
					case kCommandSetEmulatorVT220:
						newEmulator = kTerminal_EmulatorVT220;
						break;
					
					case kCommandSetEmulatorXTermOriginal:
						newEmulator = kTerminal_EmulatorXTerm256Color;
						{
							// assume that by default the user will want all XTerm-related tweaks
							// enabled when using XTerm as the base type (they can still be disabled
							// individually if the user so desires)
							Boolean		flagValue = true;
							
							
							(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTerm256ColorsEnabled,
																			sizeof(flagValue), &flagValue);
							(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermBackgroundColorEraseEnabled,
																			sizeof(flagValue), &flagValue);
							(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermColorEnabled,
																			sizeof(flagValue), &flagValue);
							(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermGraphicsEnabled,
																			sizeof(flagValue), &flagValue);
							(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermWindowAlterationEnabled,
																			sizeof(flagValue), &flagValue);
							emulationInterfacePtr->readPreferences(dataPtr->dataModel);
						}
						break;
					
					default:
						// ???
						break;
					}
					emulationInterfacePtr->setEmulator(newEmulator);
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTerminalEmulatorType,
																sizeof(newEmulator), &newEmulator);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "failed to set terminal emulator");
					}
					
					// only when setting from the menu, resync the answerback message
					{
						CFStringRef const	kAnswerBackMessage = Terminal_EmulatorReturnDefaultName(newEmulator);
						
						
						prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTerminalAnswerBackMessage,
																	sizeof(kAnswerBackMessage), &kAnswerBackMessage);
						if ((nullptr == kAnswerBackMessage) || (kPreferences_ResultOK != prefsResult))
						{
							Console_Warning(Console_WriteLine, "failed to set terminal answer-back message");
						}
					}
					
					result = noErr; // event is handled
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
}// My_TerminalsPanelEmulationUI::receiveHICommand


/*!
Saves every text field in the panel to the data model.
It is necessary to treat fields specially because they
do not have obvious state changes (as, say, buttons do);
they might need saving when focus is lost or the window
is closed, etc.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings)
{
	if (nullptr != inoutSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set answer-back message
		{
			CFStringRef		messageCFString = nullptr;
			
			
			GetControlTextAsCFString(HIViewWrap(idMyFieldAnswerBackMessage, kOwningWindow), messageCFString);
			if (nullptr != messageCFString)
			{
				prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagTerminalAnswerBackMessage,
															sizeof(messageCFString), &messageCFString);
				if (kPreferences_ResultOK != prefsResult)
				{
					Console_Warning(Console_WriteLine, "failed to set answer-back message");
				}
				CFRelease(messageCFString), messageCFString = nullptr;
			}
		}
	}
}// My_TerminalsPanelEmulationUI::saveFieldPreferences


/*!
Updates the answer-back message display with the given string.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
setAnswerBack	(CFStringRef	inMessage)
{
	HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControlTextWithCFString(HIViewWrap(idMyFieldAnswerBackMessage, kOwningWindow), inMessage);
}// My_TerminalsPanelEmulationUI::setAnswerBack


/*!
Updates the answer-back message display based on the given
type of emulator.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
setAnswerBackFromEmulator		(Terminal_Emulator		inEmulator)
{
	CFStringRef const	kAnswerBackMessage = Terminal_EmulatorReturnDefaultName(inEmulator);
	
	
	setAnswerBack(kAnswerBackMessage);
}// My_TerminalsPanelEmulationUI::setAnswerBackFromEmulator


/*!
Sets the widths of the data browser columns
proportionately based on the total width.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
setEmulationTweaksDataBrowserColumnWidths ()
{
	HIViewWrap	dataBrowser(idMyDataBrowserHacks, HIViewGetWindow(mainView));
	Rect		containerRect;
	
	
	(Rect*)GetControlBounds(dataBrowser, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		availableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */;
		UInt16		integerWidth = 0;
		
		
		// subtract the fixed-size checkbox column from the total width
		(OSStatus)GetDataBrowserTableViewNamedColumnWidth
					(dataBrowser, kMy_DataBrowserPropertyIDTweakIsEnabled, &integerWidth);
		availableWidth -= integerWidth;
		
		// set other column to use remaining space
		{
			integerWidth = availableWidth;
			(OSStatus)SetDataBrowserTableViewNamedColumnWidth
						(dataBrowser, kMy_DataBrowserPropertyIDTweakName, integerWidth);
			//availableWidth -= integerWidth;
		}
	}
}// My_TerminalsPanelEmulationUI::setEmulationTweaksDataBrowserColumnWidths


/*!
Updates the emulator display based on the given type.
Changing this value should usually set a new default
for the answer-back message.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
setEmulator		(Terminal_Emulator		inEmulator,
				 Boolean				inSynchronizeAnswerBackMessage)
{
	HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	switch (inEmulator)
	{
	case kTerminal_EmulatorVT102:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
														kCommandSetEmulatorVT102);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kTerminal_EmulatorVT220:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
														kCommandSetEmulatorVT220);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kTerminal_EmulatorXTermOriginal:
	case kTerminal_EmulatorXTermColor:
	case kTerminal_EmulatorXTerm256Color:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
														kCommandSetEmulatorXTermOriginal);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kTerminal_EmulatorDumb:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
														kCommandSetEmulatorNone);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kTerminal_EmulatorVT100:
	default:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
														kCommandSetEmulatorVT100);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	}
}// My_TerminalsPanelEmulationUI::setEmulator


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
	
	case kPanel_MessageGetUsefulResizeAxes: // request for panel to return the directions in which resizing makes sense
		result = kPanel_ResponseResizeHorizontal;
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
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::TERMINAL);
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
	
	case kPanel_MessageFocusFirst: // notification that the first logical view should become focused
		{
			My_TerminalsPanelScreenDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_TerminalsPanelScreenDataPtr);
			HIWindowRef						owningWindow = HIViewGetWindow(panelDataPtr->interfacePtr->mainView);
			HIViewWrap						columnsField(idMyFieldColumns, owningWindow);
			
			
			DialogUtilities_SetKeyboardFocus(columnsField);
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
	
	case kPanel_MessageGetUsefulResizeAxes: // request for panel to return the directions in which resizing makes sense
		result = kPanel_ResponseResizeNotNeeded;
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
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::TERMINAL);
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
		// IMPORTANT: the tags read here should be in sync with those
		// returned by PrefPanelTerminals_NewScreenPaneTagSet()
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// set columns
		{
			UInt16		dimension = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalScreenColumns, sizeof(dimension),
														&dimension, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setColumns(dimension);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read screen columns");
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
			else
			{
				Console_Warning(Console_WriteLine, "failed to read screen rows");
			}
		}
		
		// set scrollback rows
		{
			UInt32		dimension = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalScreenScrollbackRows, sizeof(dimension),
														&dimension, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setScrollbackRows(dimension);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read scrollback rows");
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
			else
			{
				Console_Warning(Console_WriteLine, "failed to read scrollback type");
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
OSStatus
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
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons and menus in this panel.

(3.1)
*/
OSStatus
My_TerminalsPanelScreenUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
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
			case kCommandSetScrollbackTypeFixed:
			case kCommandSetScrollbackTypeUnlimited:
			case kCommandSetScrollbackTypeDistributed:
				{
					Terminal_ScrollbackType				newScrollbackType = kTerminal_ScrollbackTypeDisabled;
					My_TerminalsPanelEmulationDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(screenInterfacePtr->panel),
																					My_TerminalsPanelEmulationDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					switch (received.commandID)
					{
					case kCommandSetScrollbackTypeDisabled:
						newScrollbackType = kTerminal_ScrollbackTypeDisabled;
						break;
					
					case kCommandSetScrollbackTypeFixed:
						newScrollbackType = kTerminal_ScrollbackTypeFixed;
						break;
					
					case kCommandSetScrollbackTypeUnlimited:
						newScrollbackType = kTerminal_ScrollbackTypeUnlimited;
						break;
					
					case kCommandSetScrollbackTypeDistributed:
						newScrollbackType = kTerminal_ScrollbackTypeDistributed;
						break;
					
					default:
						// ???
						break;
					}
					screenInterfacePtr->setScrollbackType(newScrollbackType);
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTerminalScreenScrollbackType,
																sizeof(newScrollbackType), &newScrollbackType);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "failed to set screen scrollback type");
					}
					
					result = noErr; // event is handled
				}
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
}// My_TerminalsPanelScreenUI::receiveHICommand


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
				Console_Warning(Console_WriteLine, "failed to set screen columns");
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
				Console_Warning(Console_WriteLine, "failed to set screen rows");
			}
		}
		
		// set scrollback rows
		{
			UInt32		dimension = 0;
			size_t		stringLength = 0;
			
			
			// INCOMPLETE - take the current units into account!!!
			GetControlNumericalText(HIViewWrap(idMyFieldScrollback, kOwningWindow), &dummyInteger, &stringLength);
			if (dummyInteger < 0)
			{
				dummyInteger = 0; // arbitrary
			}
			dimension = STATIC_CAST(dummyInteger, UInt32);
			
			prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagTerminalScreenScrollbackRows,
														sizeof(dimension), &dimension);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "failed to set screen scrollback");
			}
			
			// the Preferences module will special-case the value 0 and
			// choose the Default value, instead; short-cut that here,
			// and implicitly switch to Off mode
			if ((0 == dimension) && (0 != stringLength))
			{
				Commands_ExecuteByIDUsingEvent(kCommandSetScrollbackTypeDisabled);
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
setScrollbackRows	(UInt32		inDimension)
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
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
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
}// My_TerminalsPanelScreenUI::setScrollbackType


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X to obtain or
modify data for the specified list.

(3.1)
*/
OSStatus
accessDataBrowserItemData	(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	My_TerminalsPanelEmulationDataPtr	panelDataPtr = nullptr;
	Panel_Ref							owningPanel = nullptr;
	OSStatus							result = noErr;
	
	
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
											My_TerminalsPanelEmulationDataPtr);
		}
	}
	
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kDataBrowserItemIsEditableProperty:
			result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
			break;
		
		case kMy_DataBrowserPropertyIDTweakIsEnabled:
			// return whether or not the checkbox should be on; the item ID is a
			// Preferences_Tag whose value type is Boolean, so the method for
			// determining enabled state is always the same: use Preferences
			assert(nullptr != panelDataPtr);
			{
				Preferences_Tag const	kPrefTag = STATIC_CAST(inItemID, Preferences_Tag);
				Preferences_Result		prefsResult = kPreferences_ResultOK;
				Boolean					flag = false;
				
				
				prefsResult = Preferences_ContextGetData(panelDataPtr->dataModel, kPrefTag, sizeof(flag), &flag,
															true/* search defaults */);
				if (kPreferences_ResultOK == prefsResult)
				{
					result = SetDataBrowserItemDataButtonValue(inItemData, (flag) ? kThemeButtonOn : kThemeButtonOff);
				}
			}
			break;
		
		case kMy_DataBrowserPropertyIDTweakName:
			// return the text string for the name of this tweak
			{
				UIStrings_Result	stringResult = kUIStrings_ResultOK;
				CFStringRef			preferenceDescription = nullptr;
				
				
				// the item ID is a preferences tag, but for simplicity this is
				// also guaranteed to be a valid string identifier
				stringResult = UIStrings_Copy(STATIC_CAST(inItemID, UIStrings_PreferencesWindowCFString), preferenceDescription);
				if (stringResult.ok())
				{
					result = SetDataBrowserItemDataText(inItemData, preferenceDescription);
					CFRelease(preferenceDescription), preferenceDescription = nullptr;
				}
				else
				{
					result = paramErr;
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
		case kMy_DataBrowserPropertyIDTweakIsEnabled:
			// toggle checkbox, save preference
			{
				ThemeButtonValue	newValue = kThemeButtonOff;
				
				
				result = GetDataBrowserItemDataButtonValue(inItemData, &newValue);
				if (noErr == result)
				{
					assert(nullptr != panelDataPtr);
					Preferences_Tag const	kPrefTag = STATIC_CAST(inItemID, Preferences_Tag);
					Boolean const			kFlag = (kThemeButtonOff != newValue);
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextSetData(panelDataPtr->dataModel, kPrefTag, sizeof(kFlag), &kFlag);
					if (kPreferences_ResultOK != prefsResult)
					{
						result = paramErr;
					}
				}
			}
			break;
		
		case kMy_DataBrowserPropertyIDTweakName:
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
	case kMy_DataBrowserPropertyIDTweakIsEnabled:
		result = (inItemOne < inItemTwo);
		break;
	
	case kMy_DataBrowserPropertyIDTweakName:
		result = (inItemOne < inItemTwo);
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// compareDataBrowserItems

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
