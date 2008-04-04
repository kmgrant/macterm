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

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "StringResources.h"
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
	kMyDataBrowserPropertyIDInvokeWith			= 'Keys',
	kMyDataBrowserPropertyIDMacroName			= 'Name',
	kMyDataBrowserPropertyIDAction				= 'ToDo',
	kMyDataBrowserPropertyIDContents			= 'Text'
};

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanelMacros.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyUserPaneMacroSetList				= { 'MLst', 0/* ID */ };
HIViewID const	idMyDataBrowserMacroSetList				= { 'McDB', 0/* ID */ };
HIViewID const	idMyRadioButtonInvokeWithCommandKeypad	= { 'Inv1', 0/* ID */ };
HIViewID const	idMyRadioButtonInvokeWithFunctionKeys	= { 'Inv2', 0/* ID */ };
HIViewID const	idMyCheckBoxMacrosMenuVisible			= { 'McMn', 0/* ID */ };
HIViewID const	idMyFieldMacrosMenuName					= { 'MMNm', 0/* ID */ };
HIViewID const	idMyLabelSelectedMacro					= { 'LSMc', 0/* ID */ };
HIViewID const	idMySeparatorSelectedMacro				= { 'SSMc', 0/* ID */ };
HIViewID const	idMyLabelMacroName						= { 'LMNm', 0/* ID */ };
HIViewID const	idMyFieldMacroName						= { 'FMNm', 0/* ID */ };
HIViewID const	idMyLabelMacroAction					= { 'LMTy', 0/* ID */ };
HIViewID const	idMyPopUpMenuMacroAction				= { 'MMTy', 0/* ID */ };
HIViewID const	idMyFieldMacroText						= { 'MTxt', 0/* ID */ };

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
	readPreferences		(Preferences_ContextRef);
	
	My_MacrosDataBrowserCallbacks		listCallbacks;				//!< used to provide data for the list
	HIViewWrap							mainView;
	CommonEventHandlers_HIViewResizer	containerResizer;			//!< invoked when the panel is resized
	ListenerModel_ListenerRef			macroSetChangeListener;		//!< invoked when macros change externally
	ListenerModel_ListenerRef			preferenceChangeListener;	//!< notified when certain user preferences are changed

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
};
typedef My_MacrosPanelUI*	My_MacrosPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_MacrosPanelData
{
	My_MacrosPanelData ();
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_MacrosPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef	dataModel;		//!< source of initializations and target of changes
};
typedef My_MacrosPanelData*		My_MacrosPanelDataPtr;

} // anonymous namespace

#pragma mark Variables
namespace {

Float32		gIdealPanelWidth = 0.0;
Float32		gIdealPanelHeight = 0.0;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

pascal OSStatus		accessDataBrowserItemData			(ControlRef, DataBrowserItemID, DataBrowserPropertyID,
														 DataBrowserItemDataRef, Boolean);
pascal Boolean		compareDataBrowserItems				(ControlRef, DataBrowserItemID, DataBrowserItemID,
														 DataBrowserPropertyID);
void				deltaSizePanelContainerHIView		(HIViewRef, Float32, Float32, void*);
void				disposePanel						(Panel_Ref, void*);
void				macroSetChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
pascal void			monitorDataBrowserItems				(ControlRef, DataBrowserItemID, DataBrowserItemNotification);
SInt32				panelChanged						(Panel_Ref, Panel_Message, void*);
void				preferenceChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				refreshDisplay						(My_MacrosPanelUIPtr);
void				setDataBrowserColumnWidths			(My_MacrosPanelUIPtr);

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
	Panel_Ref		result = Panel_New(panelChanged);
	
	
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
dataModel(nullptr)
{
}// My_MacrosPanelData default constructor


/*!
Initializes a My_MacrosPanelUI structure.

(3.1)
*/
My_MacrosPanelUI::
My_MacrosPanelUI	(Panel_Ref		inPanel,
					 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
listCallbacks				(),
mainView					(createContainerView(inPanel, inOwningWindow)
								<< HIViewWrap_AssertExists),
containerResizer			(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
										kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
								deltaSizePanelContainerHIView, this/* context */),
macroSetChangeListener		(nullptr)
{
	assert(containerResizer.isInstalled());
	
	setDataBrowserColumnWidths(this);
	
	// now that the views exist, it is safe to monitor macro activity
	this->macroSetChangeListener = ListenerModel_NewStandardListener(macroSetChanged, this/* context */);
	assert(nullptr != this->macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeActiveSetPlanned, this->macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeActiveSet, this->macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeContents, this->macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeMode, this->macroSetChangeListener);
	
	this->preferenceChangeListener = ListenerModel_NewStandardListener(preferenceChanged, this/* context */);
	assert(nullptr != this->preferenceChangeListener);
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring
						(this->preferenceChangeListener, kPreferences_TagMacrosMenuVisible,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
	}
}// My_MacrosPanelUI 2-argument constructor


/*!
Tears down a My_MacrosPanelUI structure.

(3.1)
*/
My_MacrosPanelUI::
~My_MacrosPanelUI ()
{
	// remove event handlers
	Macros_StopMonitoring(kMacros_ChangeActiveSetPlanned, this->macroSetChangeListener);
	Macros_StopMonitoring(kMacros_ChangeActiveSet, this->macroSetChangeListener);
	Macros_StopMonitoring(kMacros_ChangeContents, this->macroSetChangeListener);
	Macros_StopMonitoring(kMacros_ChangeMode, this->macroSetChangeListener);
	ListenerModel_ReleaseListener(&this->macroSetChangeListener);
	Preferences_StopMonitoring(this->preferenceChangeListener, kPreferences_TagMacrosMenuVisible);
	ListenerModel_ReleaseListener(&this->preferenceChangeListener);
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
		
		// create name column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderName,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
		// until macros have editable names, omit this column
		#if 1
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDMacroName;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
		#endif
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// create key column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderInvokeWith,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			// this column does not follow the default minimum and maximum width rules set above
			UInt16 const	kDefaultMinWidth = columnInfo.headerBtnDesc.minimumWidth;
			UInt16 const	kDefaultMaxWidth = columnInfo.headerBtnDesc.maximumWidth;
			
			
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDInvokeWith;
			columnInfo.headerBtnDesc.minimumWidth = kMy_MaxMacroKeyColumnWidthInPixels;
			columnInfo.headerBtnDesc.maximumWidth = kMy_MaxMacroKeyColumnWidthInPixels;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
			
			columnInfo.headerBtnDesc.minimumWidth = kDefaultMinWidth;
			columnInfo.headerBtnDesc.maximumWidth = kDefaultMaxWidth;
		}
		
		// create action column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderAction,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			// this column does not follow the default minimum and maximum width rules set above
			UInt16 const	kDefaultMinWidth = columnInfo.headerBtnDesc.minimumWidth;
			UInt16 const	kDefaultMaxWidth = columnInfo.headerBtnDesc.maximumWidth;
			
			
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDAction;
			columnInfo.headerBtnDesc.minimumWidth = kMy_MaxMacroActionColumnWidthInPixels;
			columnInfo.headerBtnDesc.maximumWidth = kMy_MaxMacroActionColumnWidthInPixels;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
			
			columnInfo.headerBtnDesc.minimumWidth = kDefaultMinWidth;
			columnInfo.headerBtnDesc.maximumWidth = kDefaultMaxWidth;
		}
		
		// create contents column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderContents,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDContents;
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
		{
			Boolean const	kFrameFlag = false;
			
			
			(OSStatus)SetControlData(macrosList, kControlEntireControl, kControlDataBrowserIncludesFrameAndFocusTag,
										sizeof(kFrameFlag), &kFrameFlag);
		}
		
		// initialize sort column
		error = SetDataBrowserSortProperty(macrosList, kMyDataBrowserPropertyIDInvokeWith);
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
		{
			DataBrowserPropertyFlags	flags = 0;
			
			
			error = GetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDMacroName, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserPropertyIsMutable;
				error = SetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDMacroName, flags);
			}
			error = GetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDContents, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserPropertyIsMutable;
				error = SetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDContents, flags);
			}
		}
		
		error = HIViewAddSubview(result, macrosList);
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
	
	return result;
}// My_MacrosPanelUI::createContainerView


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_MacrosPanelUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// read each macro
		// UNIMPLEMENTED
	}
}// My_MacrosPanelUI::readPreferences


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
pascal OSStatus
accessDataBrowserItemData	(ControlRef					inDataBrowser,
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
		
		case kMyDataBrowserPropertyIDInvokeWith:
			// return the text string for the key equivalent
			{
				MacroManager_InvocationMethod const		kMacroKeys = Macros_ReturnMode();
				
				
				switch (inItemID)
				{
				case 1:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-0") : CFSTR("F1"));
					break;
				
				case 2:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-1") : CFSTR("F2"));
					break;
				
				case 3:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-2") : CFSTR("F3"));
					break;
				
				case 4:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-3") : CFSTR("F4"));
					break;
				
				case 5:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-4") : CFSTR("F5"));
					break;
				
				case 6:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-5") : CFSTR("F6"));
					break;
				
				case 7:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-6") : CFSTR("F7"));
					break;
				
				case 8:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-7") : CFSTR("F8"));
					break;
				
				case 9:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-8") : CFSTR("F9"));
					break;
				
				case 10:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-9") : CFSTR("F10"));
					break;
				
				case 11:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-=") : CFSTR("F11"));
					break;
				
				case 12:
					result = SetDataBrowserItemDataText
								(inItemData, (kMacroKeys == kMacroManager_InvocationMethodCommandDigit)
												? CFSTR("Command-/") : CFSTR("F12"));
					break;
				
				default:
					// ???
					break;
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDMacroName:
			// return the text string for the macro name
			// UNIMPLEMENTED
			break;
		
		case kMyDataBrowserPropertyIDAction:
			// return text string for how the macro interprets its contents
			// UNIMPLEMENTED
			break;
		
		case kMyDataBrowserPropertyIDContents:
			// return the text string for the macro text
			// UNIMPLEMENTED
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
		case kMyDataBrowserPropertyIDInvokeWith:
			// read-only
			result = paramErr;
			break;
		
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
		
		case kMyDataBrowserPropertyIDAction:
			// user has changed what the macro does; update the command in memory
			{
				SInt32		selectedItem = 0;
				
				
				result = GetDataBrowserItemDataValue(inItemData, &selectedItem);
				if (noErr == result)
				{
					// change purpose of macro - UNIMPLEMENTED
					result = noErr;
				}
			}
			result = paramErr;
			break;
		
		case kMyDataBrowserPropertyIDContents:
			// user has changed the macro text; update the macro in memory
			{
				CFStringRef		newText = nullptr;
				
				
				result = GetDataBrowserItemDataText(inItemData, &newText);
				if (noErr == result)
				{
					// fix macro contents - UNIMPLEMENTED
					result = noErr;
				}
			}
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
compareDataBrowserItems		(ControlRef					inDataBrowser,
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	CFStringRef		string1 = nullptr;
	CFStringRef		string2 = nullptr;
	Boolean			result = false;
	
	
	switch (inSortProperty)
	{
	case kMyDataBrowserPropertyIDInvokeWith:
		// UNIMPLEMENTED
		break;
	
	case kMyDataBrowserPropertyIDMacroName:
		// UNIMPLEMENTED
		break;
	
	case kMyDataBrowserPropertyIDAction:
		// UNIMPLEMENTED
		break;
	
	case kMyDataBrowserPropertyIDContents:
		// UNIMPLEMENTED
		break;
	
	default:
		// ???
		break;
	}
	
	// check for nullptr, because CFStringCompare() will not deal with it
	if ((nullptr == string1) && (nullptr != string2)) result = true;
	else if ((nullptr == string1) || (nullptr == string2)) result = false;
	else
	{
		result = (kCFCompareLessThan == CFStringCompare(string1, string2,
														kCFCompareCaseInsensitive | kCFCompareLocalized/* options */));
	}
	
	return result;
}// compareDataBrowserItems


/*!
Adjusts the data browser columns in the “Macros”
preference panel to fill the new data browser
dimensions.

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
		HIWindowRef const		kPanelWindow = GetControlOwner(inView);
		My_MacrosPanelUIPtr		interfacePtr = REINTERPRET_CAST(inContext, My_MacrosPanelUIPtr);
		HIViewWrap				viewWrap(idMyDataBrowserMacroSetList, kPanelWindow);
		
		
		assert(viewWrap.exists());
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		
		setDataBrowserColumnWidths(interfacePtr);
		
		viewWrap = HIViewWrap(idMyFieldMacroText, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		viewWrap = HIViewWrap(idMyPopUpMenuMacroAction, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		viewWrap = HIViewWrap(idMyLabelMacroAction, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		viewWrap = HIViewWrap(idMyFieldMacroName, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		viewWrap = HIViewWrap(idMyLabelMacroName, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		viewWrap = HIViewWrap(idMySeparatorSelectedMacro, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		viewWrap = HIViewWrap(idMyLabelSelectedMacro, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
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
		refreshDisplay(interfacePtr);
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
		refreshDisplay(interfacePtr);
		break;
	
	case kMacros_ChangeMode:
		// If the macro keys change, the displayed content will change
		// also; re-sort the list (which will automatically find out
		// the new macro keys being used), then redraw the list.
		// NOTE: A future efficiency improvement may be to only
		//       refresh the part of the display showing macro keys.
		{
			HIViewWrap		radioButtonInvokeWithCommandKeys(idMyRadioButtonInvokeWithCommandKeypad,
																GetControlOwner(interfacePtr->mainView));
			HIViewWrap		radioButtonInvokeWithFunctionKeys(idMyRadioButtonInvokeWithFunctionKeys,
																GetControlOwner(interfacePtr->mainView));
			assert(radioButtonInvokeWithCommandKeys.exists());
			assert(radioButtonInvokeWithFunctionKeys.exists());
			
			
			refreshDisplay(interfacePtr);
			if (kMacroManager_InvocationMethodCommandDigit == Macros_ReturnMode())
			{
				SetControl32BitValue(radioButtonInvokeWithCommandKeys, kControlRadioButtonCheckedValue);
				SetControl32BitValue(radioButtonInvokeWithFunctionKeys, kControlRadioButtonUncheckedValue);
			}
			else
			{
				SetControl32BitValue(radioButtonInvokeWithCommandKeys, kControlRadioButtonUncheckedValue);
				SetControl32BitValue(radioButtonInvokeWithFunctionKeys, kControlRadioButtonCheckedValue);
			}
		}
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
monitorDataBrowserItems		(ControlRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	switch (inMessage)
	{
	case kDataBrowserItemSelected:
		{
			DataBrowserTableViewRowIndex	rowIndex = 0;
			OSStatus						error = noErr;
			
			
			// update the “selected macro” fields to match the newly-selected item
			// UNIMPLEMENTED
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
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.0)
*/
SInt32
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
	
	case kPanel_MessageGetGrowBoxLook: // request for panel to return its preferred appearance for the window grow box
		result = kPanel_ResponseGrowBoxOpaque;
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
			panelDataPtr->dataModel = newContext;
			panelDataPtr->interfacePtr->readPreferences(newContext);
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
}// panelChanged


/*!
Invoked whenever a monitored preference value is changed
(see PrefPanelMacros_New() to see which preferences are
monitored).  This routine responds by ensuring that HIView
states are up to date for the changed preference.

(3.1)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inEventContextPtr,
					 void*					inMyMacrosPanelUIPtr)
{
	//Preferences_ChangeContext*		contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	My_MacrosPanelUIPtr				interfacePtr = REINTERPRET_CAST(inMyMacrosPanelUIPtr, My_MacrosPanelUIPtr);
	size_t							actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagMacrosMenuVisible:
		{
			Boolean		isVisible = false;
			
			
			unless (Preferences_GetData(kPreferences_TagMacrosMenuVisible,
										sizeof(isVisible), &isVisible,
										&actualSize) == kPreferences_ResultOK)
			{
				isVisible = false; // assume a value, if preference can’t be found
			}
			
			// update user interface
			{
				HIViewWrap		checkBox(idMyCheckBoxMacrosMenuVisible, GetControlOwner(interfacePtr->mainView));
				HIViewWrap		textField(idMyFieldMacrosMenuName, GetControlOwner(interfacePtr->mainView));
				assert(checkBox.exists());
				assert(textField.exists());
				
				
				SetControlValue(checkBox, BooleanToCheckBoxValue(isVisible));
				if (isVisible)
				{
					(OSStatus)ActivateControl(textField);
				}
				else
				{
					(OSStatus)DeactivateControl(textField);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Redraws the macro display.  Use this when you have caused
it to change in some way (by spawning an editor dialog box
or by switching tabs, etc.).

(3.0)
*/
void
refreshDisplay		(My_MacrosPanelUIPtr	inInterfacePtr)
{
	HIViewWrap		macrosListContainer(idMyDataBrowserMacroSetList, GetControlOwner(inInterfacePtr->mainView));
	assert(macrosListContainer.exists());
	
	
	(OSStatus)UpdateDataBrowserItems(macrosListContainer, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDInvokeWith);
	(OSStatus)UpdateDataBrowserItems(macrosListContainer, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDMacroName);
	(OSStatus)UpdateDataBrowserItems(macrosListContainer, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDContents);
}// refreshDisplay


/*!
Sets the widths of the data browser columns
proportionately based on the total width.

(3.1)
*/
void
setDataBrowserColumnWidths	(My_MacrosPanelUIPtr	inInterfacePtr)
{
	Rect			containerRect;
	HIViewWrap		macrosListContainer(idMyDataBrowserMacroSetList, GetControlOwner(inInterfacePtr->mainView));
	assert(macrosListContainer.exists());
	
	
	(OSStatus)GetControlBounds(macrosListContainer, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		kAvailableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */;
		Float32		calculatedWidth = 0;
		UInt16		totalWidthSoFar = 0;
		
		
		// key equivalent is relatively small
		calculatedWidth = kMy_MaxMacroKeyColumnWidthInPixels/* arbitrary */;
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(macrosListContainer, kMyDataBrowserPropertyIDInvokeWith,
						STATIC_CAST(calculatedWidth, UInt16));
		totalWidthSoFar += STATIC_CAST(calculatedWidth, UInt16);
		
		// action menu is relatively small
		calculatedWidth = kMy_MaxMacroActionColumnWidthInPixels/* arbitrary */;
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(macrosListContainer, kMyDataBrowserPropertyIDAction,
						STATIC_CAST(calculatedWidth, UInt16));
		totalWidthSoFar += STATIC_CAST(calculatedWidth, UInt16);
		
		// title space is moderate
		calculatedWidth = kAvailableWidth * 0.25/* arbitrary */;
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(macrosListContainer, kMyDataBrowserPropertyIDMacroName,
						STATIC_CAST(calculatedWidth, UInt16));
		totalWidthSoFar += STATIC_CAST(calculatedWidth, UInt16);
		
		// give all remaining space to the content string
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(macrosListContainer, kMyDataBrowserPropertyIDContents,
						kAvailableWidth - totalWidthSoFar);
	}
}// setDataBrowserColumnWidths

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
