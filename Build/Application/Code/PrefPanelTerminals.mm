/*!	\file PrefPanelTerminals.mm
	\brief Implements the Terminals panel of Preferences.
	
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

#import "PrefPanelTerminals.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Unix includes
#import <strings.h>

// Mac includes
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <BoundName.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaExtensions.objc++.h>
#import <ColorUtilities.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <DialogAdjust.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "Emulation.h"
#import "GenericPanelTabs.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "PreferenceValue.objc++.h"
#import "Terminal.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



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
HIViewID const	idMyHelpTextTweaks				= { 'TwkH', 0/* ID */ };
HIViewID const	idMyCheckBoxLineWrap			= { 'XWrL', 0/* ID */ };
HIViewID const	idMyCheckBoxEightBit			= { 'X8Bt', 0/* ID */ };
HIViewID const	idMyCheckBoxSaveOnClear			= { 'XClS', 0/* ID */ };
HIViewID const	idMyCheckBoxNormalKeypadTopRow	= { 'XNKT', 0/* ID */ };
HIViewID const	idMyCheckBoxLocalPageKeys		= { 'XLPg', 0/* ID */ };

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
	setAnswerBackFromEmulator	(Emulation_FullType);
	
	void
	setEmulator		(Emulation_FullType, Boolean = true);

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
	
	static OSStatus
	receiveViewHit		(EventHandlerCallRef, EventRef, void*);
	
	void
	setEightBit		(Boolean);
	
	void
	setLineWrap		(Boolean);
	
	void
	setNormalKeypadTopRow	(Boolean);
	
	void
	setPageKeysControlLocalTerminal		(Boolean);
	
	void
	setSaveOnClear	(Boolean);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	Boolean
	updateCheckBoxPreference	(HIViewRef);

private:
	CarbonEventHandlerWrap				_viewClickHandler;
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


/*!
The private class interface.
*/
@interface PrefPanelTerminals_EmulationViewManager (PrefPanelTerminals_EmulationViewManagerInternal) //{

	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelTerminals_OptionsViewManager (PrefPanelTerminals_OptionsViewManagerInternal) //{

	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelTerminals_ScreenViewManager (PrefPanelTerminals_ScreenViewManagerInternal) //{

	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}



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
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewEmulationPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads emulation-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalEmulatorType);
	tagList.push_back(kPreferences_TagTerminalAnswerBackMessage);
	tagList.push_back(kPreferences_TagVT100FixLineWrappingBug);
	tagList.push_back(kPreferences_TagXTerm256ColorsEnabled);
	tagList.push_back(kPreferences_TagXTermBackgroundColorEraseEnabled);
	tagList.push_back(kPreferences_TagXTermColorEnabled);
	tagList.push_back(kPreferences_TagXTermGraphicsEnabled);
	tagList.push_back(kPreferences_TagXTermWindowAlterationEnabled);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewEmulationPaneTagSet


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
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewOptionsPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads option-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalLineWrap);
	tagList.push_back(kPreferences_TagDataReceiveDoNotStripHighBit);
	tagList.push_back(kPreferences_TagTerminalClearSavesLines);
	tagList.push_back(kPreferences_TagMapKeypadTopRowForVT220);
	tagList.push_back(kPreferences_TagPageKeysControlLocalTerminal);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewOptionsPaneTagSet


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


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewTagSet ()
{
	Preferences_TagSetRef	result = Preferences_NewTagSet(40); // arbitrary initial capacity
	Preferences_TagSetRef	emulationTags = PrefPanelTerminals_NewEmulationPaneTagSet();
	Preferences_TagSetRef	screenTags = PrefPanelTerminals_NewScreenPaneTagSet();
	Preferences_TagSetRef	optionTags = PrefPanelTerminals_NewOptionsPaneTagSet();
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_TagSetMerge(result, emulationTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, screenTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, optionTags);
	assert(kPreferences_ResultOK == prefsResult);
	
	Preferences_ReleaseTagSet(&emulationTags);
	Preferences_ReleaseTagSet(&screenTags);
	Preferences_ReleaseTagSet(&optionTags);
	
	return result;
}// NewTagSet


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
_fieldAnswerBackInputHandler(HIViewGetEventTarget(HIViewWrap(idMyFieldAnswerBackMessage, inOwningWindow)), receiveFieldChanged,
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
	bzero(&dummy, sizeof(dummy));
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
		UNUSED_RETURN(OSStatus)DataBrowserChangeAttributes(emulationTweaksList,
															kDataBrowserAttributeListViewAlternatingRowColors/* attributes to set */,
															0/* attributes to clear */);
		UNUSED_RETURN(OSStatus)SetDataBrowserListViewUsePlainBackground(emulationTweaksList, false);
		UNUSED_RETURN(OSStatus)SetDataBrowserHasScrollBars(emulationTweaksList, false/* horizontal */, true/* vertical */);
		
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
			Emulation_FullType		emulatorType = kEmulation_FullTypeVT100;
			
			
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
			
			
			UNUSED_RETURN(OSStatus)UpdateDataBrowserItems(dataBrowser, kDataBrowserNoItem/* parent */,
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
					Emulation_FullType					newEmulator = kEmulation_FullTypeDumb;
					My_TerminalsPanelEmulationDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(emulationInterfacePtr->panel),
																					My_TerminalsPanelEmulationDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					switch (received.commandID)
					{
					case kCommandSetEmulatorNone:
						newEmulator = kEmulation_FullTypeDumb;
						break;
					
					case kCommandSetEmulatorVT100:
						newEmulator = kEmulation_FullTypeVT100;
						break;
					
					case kCommandSetEmulatorVT102:
						newEmulator = kEmulation_FullTypeVT102;
						break;
					
					case kCommandSetEmulatorVT220:
						newEmulator = kEmulation_FullTypeVT220;
						break;
					
					case kCommandSetEmulatorXTermOriginal:
						newEmulator = kEmulation_FullTypeXTerm256Color;
						{
							// assume that by default the user will want all XTerm-related tweaks
							// enabled when using XTerm as the base type (they can still be disabled
							// individually if the user so desires)
							Boolean		flagValue = true;
							
							
							UNUSED_RETURN(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTerm256ColorsEnabled,
																						sizeof(flagValue), &flagValue);
							UNUSED_RETURN(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermBackgroundColorEraseEnabled,
																						sizeof(flagValue), &flagValue);
							UNUSED_RETURN(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermColorEnabled,
																						sizeof(flagValue), &flagValue);
							UNUSED_RETURN(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermGraphicsEnabled,
																						sizeof(flagValue), &flagValue);
							UNUSED_RETURN(Preferences_Result)Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagXTermWindowAlterationEnabled,
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
setAnswerBackFromEmulator		(Emulation_FullType		inEmulator)
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
	
	
	UNUSED_RETURN(Rect*)GetControlBounds(dataBrowser, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		availableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */;
		UInt16		integerWidth = 0;
		
		
		// subtract the fixed-size checkbox column from the total width
		UNUSED_RETURN(OSStatus)GetDataBrowserTableViewNamedColumnWidth
								(dataBrowser, kMy_DataBrowserPropertyIDTweakIsEnabled, &integerWidth);
		availableWidth -= integerWidth;
		
		// set other column to use remaining space
		{
			integerWidth = availableWidth;
			UNUSED_RETURN(OSStatus)SetDataBrowserTableViewNamedColumnWidth
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
setEmulator		(Emulation_FullType		inEmulator,
				 Boolean				inSynchronizeAnswerBackMessage)
{
	HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	switch (inEmulator)
	{
	case kEmulation_FullTypeVT102:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
																		kCommandSetEmulatorVT102);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kEmulation_FullTypeVT220:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
																		kCommandSetEmulatorVT220);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kEmulation_FullTypeXTermOriginal:
	case kEmulation_FullTypeXTermColor:
	case kEmulation_FullTypeXTerm256Color:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
																		kCommandSetEmulatorXTermOriginal);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kEmulation_FullTypeDumb:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
																		kCommandSetEmulatorNone);
		if (inSynchronizeAnswerBackMessage) this->setAnswerBackFromEmulator(inEmulator);
		break;
	
	case kEmulation_FullTypeVT100:
	default:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuEmulationType, kOwningWindow),
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
_viewClickHandler		(HIViewGetEventTarget(this->mainView), receiveViewHit,
							CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
							this/* user data */),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_TerminalsPanelOptionsUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
	assert(_viewClickHandler.isInstalled());
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
	size_t						actualSize = 0;
	Boolean						flag = false;
	
	
	// create the tab pane
	bzero(&dummy, sizeof(dummy));
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
	{
		HIViewWrap		checkBox(idMyCheckBoxLineWrap, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagTerminalLineWrap, sizeof(flag), &flag, &actualSize))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxEightBit, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagDataReceiveDoNotStripHighBit, sizeof(flag), &flag, &actualSize))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxSaveOnClear, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagTerminalClearSavesLines, sizeof(flag), &flag, &actualSize))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxNormalKeypadTopRow, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagMapKeypadTopRowForVT220, sizeof(flag), &flag, &actualSize))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(false == flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxLocalPageKeys, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagPageKeysControlLocalTerminal, sizeof(flag), &flag, &actualSize))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	
	return result;
}// My_TerminalsPanelOptionsUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_TerminalsPanelOptionsUI::
deltaSize	(HIViewRef		UNUSED_ARGUMENT(inContainer),
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
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
		// IMPORTANT: the tags read here should be in sync with those
		// returned by PrefPanelTerminals_NewOptionsPaneTagSet()
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// set line wrap
		{
			Boolean		flag = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalLineWrap, sizeof(flag),
														&flag, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setLineWrap(flag);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read line wrap setting");
			}
		}
		
		// set eight bit mode
		{
			Boolean		flag = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagDataReceiveDoNotStripHighBit, sizeof(flag),
														&flag, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setEightBit(flag);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read 8-bit setting");
			}
		}
		
		// set keypad top row behavior
		{
			Boolean		flag = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagMapKeypadTopRowForVT220, sizeof(flag),
														&flag, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setNormalKeypadTopRow(false == flag);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read keypad top-row behavior setting");
			}
		}
		
		// set page key behavior
		{
			Boolean		flag = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagPageKeysControlLocalTerminal, sizeof(flag),
														&flag, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setPageKeysControlLocalTerminal(flag);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read page key behavior setting");
			}
		}
		
		// set clear-screen behavior
		{
			Boolean		flag = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalClearSavesLines, sizeof(flag),
														&flag, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setSaveOnClear(flag);
			}
			else
			{
				Console_Warning(Console_WriteLine, "failed to read save-on-clear setting");
			}
		}
	}
}// My_TerminalsPanelOptionsUI::readPreferences


/*!
Handles "kEventControlHit" of "kEventClassControl" for this
preferences panel.  Responds by saving preferences associated
with checkboxes.

(4.1)
*/
OSStatus
My_TerminalsPanelOptionsUI::
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_TerminalsPanelOptionsUI*		interfacePtr = REINTERPRET_CAST(inMyTerminalsPanelUIPtr, My_TerminalsPanelOptionsUI*);
	assert(nullptr != interfacePtr);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the control was found, proceed
		if (noErr == result)
		{
			ControlKind		controlKind;
			OSStatus		error = GetControlKind(view, &controlKind);
			
			
			result = eventNotHandledErr; // unless set otherwise
			
			if ((noErr == error) && (kControlKindSignatureApple == controlKind.signature) &&
				((kControlKindCheckBox == controlKind.kind) || (kControlKindRadioButton == controlKind.kind)))
			{
				UNUSED_RETURN(Boolean)interfacePtr->updateCheckBoxPreference(view);
			}
		}
	}
	
	return result;
}// My_TerminalsPanelOptionsUI::receiveViewHit


/*!
Updates the 8-bit-mode checkbox.

(4.1)
*/
void
My_TerminalsPanelOptionsUI::
setEightBit		(Boolean	inIsWrapping)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxEightBit, kOwningWindow), BooleanToCheckBoxValue(inIsWrapping));
}// My_TerminalsPanelOptionsUI::setEightBit


/*!
Updates the line-wrap checkbox.

(4.1)
*/
void
My_TerminalsPanelOptionsUI::
setLineWrap		(Boolean	inIsWrapping)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxLineWrap, kOwningWindow), BooleanToCheckBoxValue(inIsWrapping));
}// My_TerminalsPanelOptionsUI::setLineWrap


/*!
Updates the normal-keypad-top-row checkbox.

(4.1)
*/
void
My_TerminalsPanelOptionsUI::
setNormalKeypadTopRow	(Boolean	inIsWrapping)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxNormalKeypadTopRow, kOwningWindow), BooleanToCheckBoxValue(inIsWrapping));
}// My_TerminalsPanelOptionsUI::setNormalKeypadTopRow


/*!
Updates the page key checkbox.

(4.1)
*/
void
My_TerminalsPanelOptionsUI::
setPageKeysControlLocalTerminal		(Boolean	inIsWrapping)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxLocalPageKeys, kOwningWindow), BooleanToCheckBoxValue(inIsWrapping));
}// My_TerminalsPanelOptionsUI::setPageKeysControlLocalTerminal


/*!
Updates the save-lines-on-clear checkbox.

(4.1)
*/
void
My_TerminalsPanelOptionsUI::
setSaveOnClear	(Boolean	inIsWrapping)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxSaveOnClear, kOwningWindow), BooleanToCheckBoxValue(inIsWrapping));
}// My_TerminalsPanelOptionsUI::setSaveOnClear


/*!
Call this when a click in a checkbox or radio button has been
detected AND the view has been toggled to its new value.

The appropriate preference will have been updated in memory
using Preferences_SetData().

If the specified view is “known”, true is returned to indicate
that the click was handled.  Otherwise, false is returned.

(4.1)
*/
Boolean
My_TerminalsPanelOptionsUI::
updateCheckBoxPreference	(HIViewRef		inCheckBoxClicked)
{
	WindowRef const		kWindow = GetControlOwner(inCheckBoxClicked);
	assert(IsValidWindowRef(kWindow));
	HIViewIDWrap		viewID(HIViewWrap(inCheckBoxClicked).identifier());
	Boolean				checkBoxFlagValue = (GetControl32BitValue(inCheckBoxClicked) == kControlCheckBoxCheckedValue);
	Boolean		result = false;
	
	
	if (HIViewIDWrap(idMyCheckBoxEightBit) == viewID)
	{
		Preferences_SetData(kPreferences_TagDataReceiveDoNotStripHighBit,
							sizeof(checkBoxFlagValue), &checkBoxFlagValue);
		result = true;
	}
	else if (HIViewIDWrap(idMyCheckBoxLineWrap) == viewID)
	{
		Preferences_SetData(kPreferences_TagTerminalLineWrap,
							sizeof(checkBoxFlagValue), &checkBoxFlagValue);
		result = true;
	}
	else if (HIViewIDWrap(idMyCheckBoxNormalKeypadTopRow) == viewID)
	{
		Boolean		invertedFlag = (false == checkBoxFlagValue);
		
		
		Preferences_SetData(kPreferences_TagMapKeypadTopRowForVT220,
							sizeof(invertedFlag), &invertedFlag);
		result = true;
	}
	else if (HIViewIDWrap(idMyCheckBoxLocalPageKeys) == viewID)
	{
		Preferences_SetData(kPreferences_TagPageKeysControlLocalTerminal,
							sizeof(checkBoxFlagValue), &checkBoxFlagValue);
		result = true;
	}
	else if (HIViewIDWrap(idMyCheckBoxSaveOnClear) == viewID)
	{
		Preferences_SetData(kPreferences_TagTerminalClearSavesLines,
							sizeof(checkBoxFlagValue), &checkBoxFlagValue);
		result = true;
	}
	
	return result;
}// updateCheckBoxPreference


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
_fieldColumnsInputHandler	(HIViewGetEventTarget(HIViewWrap(idMyFieldColumns, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldRowsInputHandler		(HIViewGetEventTarget(HIViewWrap(idMyFieldRows, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldScrollbackInputHandler(HIViewGetEventTarget(HIViewWrap(idMyFieldScrollback, inOwningWindow)), receiveFieldChanged,
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
	bzero(&dummy, sizeof(dummy));
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
deltaSize	(HIViewRef		UNUSED_ARGUMENT(inContainer),
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
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
				UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackUnits,
																				Panel_ReturnOwningWindow(screenInterfacePtr->panel)),
																				received.commandID);
				result = noErr; // event is handled
				break;
			
			case kCommandSetScrollbackUnitsKilobytes:
				// UNIMPLEMENTED
				Sound_StandardAlert();
			#if 0
				UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackUnits,
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
	UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackUnits, kOwningWindow),
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
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
																		kCommandSetScrollbackTypeDisabled);
		this->setScrollbackCustomizationEnabled(false);
		break;
	
	case kTerminal_ScrollbackTypeUnlimited:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
																		kCommandSetScrollbackTypeUnlimited);
		this->setScrollbackCustomizationEnabled(false);
		break;
	
	case kTerminal_ScrollbackTypeDistributed:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
																		kCommandSetScrollbackTypeDistributed);
		this->setScrollbackCustomizationEnabled(false);
		break;
	
	case kTerminal_ScrollbackTypeFixed:
	default:
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
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
				stringResult = UIStrings_Copy(STATIC_CAST(inItemID, UIStrings_PrefPanelTerminalsCFString), preferenceDescription);
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


@implementation PrefPanelTerminals_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	NSArray*	subViewManagers = [NSArray arrayWithObjects:
												[[[PrefPanelTerminals_OptionsViewManager alloc] init] autorelease],
												[[[PrefPanelTerminals_EmulationViewManager alloc] init] autorelease],
												[[[PrefPanelTerminals_ScreenViewManager alloc] init] autorelease],
												nil];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Terminals"
										localizedName:NSLocalizedStringFromTable(@"Terminals", @"PrefPanelTerminals",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelTerminals"]
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


@end // PrefPanelTerminals_ViewManager


@implementation PrefPanelTerminals_BaseEmulatorValue


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeDumb
																description:NSLocalizedStringFromTable
																			(@"None (“Dumb”)", @"PrefPanelTerminals"/* table */,
																				@"emulator disabled")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeVT100
																description:NSLocalizedStringFromTable
																			(@"VT100", @"PrefPanelTerminals"/* table */,
																				@"emulator of VT100 terminal device")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeVT102
																description:NSLocalizedStringFromTable
																			(@"VT102", @"PrefPanelTerminals"/* table */,
																				@"emulator of VT102 terminal device")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeVT220
																description:NSLocalizedStringFromTable
																			(@"VT220", @"PrefPanelTerminals"/* table */,
																				@"emulator of VT220 terminal device")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeXTerm256Color
																description:NSLocalizedStringFromTable
																			(@"XTerm", @"PrefPanelTerminals"/* table */,
																				@"emulator of XTerm terminal program")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:kPreferences_TagTerminalEmulatorType
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


@end // PrefPanelTerminals_BaseEmulatorValue


@implementation PrefPanelTerminals_EmulationTweaksValue


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		NSMutableArray*		asMutableArray = [[[NSMutableArray alloc] init] autorelease];
		id					valueObject = nil;
		
		
		self->featureArray = [asMutableArray retain];
		
		// VT100 line-wrapping bug
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagVT100FixLineWrappingBug
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"VT100 Fix Line Wrapping Bug",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm 256-color support
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTerm256ColorsEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm 256 Colors",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm BCE
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermBackgroundColorEraseEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Background Color Erase",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm Color
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermColorEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Color",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm Graphics
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermGraphicsEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Graphics Characters",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm Window Alteration
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermWindowAlterationEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Window Alteration",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
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
	[featureArray release];
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


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSArray*)
featureArray
{
	return [[featureArray retain] autorelease];
}// featureArray


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = YES; // initially...
	
	
	for (UInt16 i = 0; i < [[self featureArray] count]; ++i)
	{
		PreferenceValue_Flag*	asValue = (PreferenceValue_Flag*)[[self featureArray] objectAtIndex:i];
		
		
		if (NO == [asValue isInherited])
		{
			result = NO;
			break;
		}
	}
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
	for (UInt16 i = 0; i < [[self featureArray] count]; ++i)
	{
		PreferenceValue_Flag*	asValue = (PreferenceValue_Flag*)[[self featureArray] objectAtIndex:i];
		
		
		[asValue setNilPreferenceValue];
	}
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PrefPanelTerminals_EmulationTweaksValue


@implementation PrefPanelTerminals_EmulationViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelTerminalEmulationCocoa" delegate:self context:nullptr];
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


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelTerminals_BaseEmulatorValue*)
baseEmulator
{
	return [self->byKey objectForKey:@"baseEmulator"];
}// baseEmulator


/*!
Accessor.

(4.1)
*/
- (PrefPanelTerminals_EmulationTweaksValue*)
emulationTweaks
{
	return [self->byKey objectForKey:@"emulationTweaks"];
}// emulationTweaks


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
identity
{
	return [self->byKey objectForKey:@"identity"];
}// identity


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
	assert(nil != tweaksTableView);
	assert(nil != byKey);
	assert(nil != prefsMgr);
	
	// do not show highlighting in this table
	[tweaksTableView setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleNone];
	
	// remember frame from XIB (it might be changed later)
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PrefPanelTerminals_BaseEmulatorValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"baseEmulator"];
	[self->byKey setObject:[[[PrefPanelTerminals_EmulationTweaksValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"emulationTweaks"];
	[self->byKey setObject:[[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagTerminalAnswerBackMessage
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"identity"];
	
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
	*outIdealSize = self->idealFrame.size;
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
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Terminals.Emulation";
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
	return NSLocalizedStringFromTable(@"Emulation", @"PrefPanelTerminals", @"the name of this panel");
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end // PrefPanelTerminals_EmulationViewManager


@implementation PrefPanelTerminals_EmulationViewManager (PrefPanelTerminals_EmulationViewManagerInternal)


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return [NSArray arrayWithObjects:
						@"baseEmulator", @"emulationTweaks",
						@"identity",
						nil];
}// primaryDisplayBindingKeys


@end // PrefPanelTerminals_EmulationViewManager (PrefPanelTerminals_EmulationViewManagerInternal)


@implementation PrefPanelTerminals_OptionsViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelTerminalOptionsCocoa" delegate:self context:nullptr];
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


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
wrapLines
{
	return [self->byKey objectForKey:@"wrapLines"];
}// wrapLines


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
eightBit
{
	return [self->byKey objectForKey:@"eightBit"];
}// eightBit


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
saveLinesOnClear
{
	return [self->byKey objectForKey:@"saveLinesOnClear"];
}// saveLinesOnClear


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
normalKeypadTopRow
{
	return [self->byKey objectForKey:@"normalKeypadTopRow"];
}// normalKeypadTopRow


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
localPageKeys
{
	return [self->byKey objectForKey:@"localPageKeys"];
}// localPageKeys


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
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagTerminalLineWrap
														contextManager:self->prefsMgr] autorelease]
					forKey:@"wrapLines"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagDataReceiveDoNotStripHighBit
														contextManager:self->prefsMgr] autorelease]
					forKey:@"eightBit"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagTerminalClearSavesLines
														contextManager:self->prefsMgr] autorelease]
					forKey:@"saveLinesOnClear"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagMapKeypadTopRowForVT220
														contextManager:self->prefsMgr
														inverted:YES] autorelease]
					forKey:@"normalKeypadTopRow"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagPageKeysControlLocalTerminal
														contextManager:self->prefsMgr] autorelease]
					forKey:@"localPageKeys"];
	
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
	*outIdealSize = self->idealFrame.size;
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
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Terminals.Options";
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
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelTerminals", @"the name of this panel");
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end // PrefPanelTerminals_OptionsViewManager


@implementation PrefPanelTerminals_OptionsViewManager (PrefPanelTerminals_OptionsViewManagerInternal)


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return [NSArray arrayWithObjects:
						@"wrapLines", @"eightBit",
						@"saveLinesOnClear", @"normalKeypadTopRow",
						@"localPageKeys",
						nil];
}// primaryDisplayBindingKeys


@end // PrefPanelTerminals_OptionsViewManager (PrefPanelTerminals_OptionsViewManagerInternal)


@implementation PrefPanelTerminals_ScrollbackValue


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		self->behaviorArray = [[[NSArray alloc] initWithObjects:
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeDisabled
															description:NSLocalizedStringFromTable
																		(@"Off", @"PrefPanelTerminals"/* table */,
																			@"scrollback disabled")]
									autorelease],
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeFixed
															description:NSLocalizedStringFromTable
																		(@"Fixed Size", @"PrefPanelTerminals"/* table */,
																			@"fixed-size scrollback")]
									autorelease],
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeUnlimited
															description:NSLocalizedStringFromTable
																		(@"Unlimited", @"PrefPanelTerminals"/* table */,
																			@"unlimited scrollback")]
									autorelease],
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeDistributed
															description:NSLocalizedStringFromTable
																		(@"Distributed", @"PrefPanelTerminals"/* table */,
																			@"distributed scrollback")]
									autorelease],
								nil] autorelease];
		self->behaviorObject = [[PreferenceValue_Number alloc]
									initWithPreferencesTag:kPreferences_TagTerminalScreenScrollbackType
															contextManager:aContextMgr
															preferenceCType:kPreferenceValue_CTypeUInt16];
		self->rowsObject = [[PreferenceValue_Number alloc]
									initWithPreferencesTag:kPreferences_TagTerminalScreenScrollbackRows
															contextManager:aContextMgr
															preferenceCType:kPreferenceValue_CTypeUInt32];
		
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
	[behaviorArray release];
	[behaviorObject release];
	[rowsObject release];
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
	[self didChangeValueForKey:@"rowsEnabled"];
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didChangeValueForKey:@"currentBehavior"];
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
	[self willChangeValueForKey:@"currentBehavior"];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	[self willChangeValueForKey:@"rowsEnabled"];
}// prefsContextWillChange:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSArray*)
behaviorArray
{
	return [[behaviorArray retain] autorelease];
}// behaviorArray


/*!
Accessor.

(4.1)
*/
- (id)
currentBehavior
{
	UInt32		currentType = [[self->behaviorObject numberStringValue] intValue];
	id			result = nil;
	
	
	for (UInt16 i = 0; i < [[self behaviorArray] count]; ++i)
	{
		PreferenceValue_IntegerDescriptor*	asInfo = (PreferenceValue_IntegerDescriptor*)
														[[self behaviorArray] objectAtIndex:i];
		
		
		if (currentType == [asInfo describedIntegerValue])
		{
			result = asInfo;
			break;
		}
	}
	return result;
}
- (void)
setCurrentBehavior:(id)		selectedObject
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentBehavior"];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	[self willChangeValueForKey:@"rowsEnabled"];
	
	if (nil == selectedObject)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		PreferenceValue_IntegerDescriptor*	asInfo = (PreferenceValue_IntegerDescriptor*)selectedObject;
		
		
		[self->behaviorObject setNumberStringValue:
								[[NSNumber numberWithInt:[asInfo describedIntegerValue]] stringValue]];
	}
	
	[self didChangeValueForKey:@"rowsEnabled"];
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didChangeValueForKey:@"currentBehavior"];
	[self didSetPreferenceValue];
}// setCurrentBehavior:


/*!
Accessor.

(4.1)
*/
- (BOOL)
rowsEnabled
{
	Terminal_ScrollbackType		currentBehavior = STATIC_CAST([[self->behaviorObject numberStringValue] intValue],
																Terminal_ScrollbackType);
	
	
	return (kTerminal_ScrollbackTypeFixed == currentBehavior);
}// rowsEnabled


/*!
Accessor.

(4.1)
*/
- (NSString*)
rowsNumberStringValue
{
	return [self->rowsObject numberStringValue];
}
- (void)
setRowsNumberStringValue:(NSString*)	aNumberString
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	
	if (nil == aNumberString)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		[self->rowsObject setNumberStringValue:aNumberString];
	}
	
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didSetPreferenceValue];
}// setRowsNumberStringValue:


#pragma mark Validators


/*!
Validates a number entered by the user, returning an appropriate
error (and a NO result) if the number is incorrect.

(4.1)
*/
- (BOOL)
validateRowsNumberStringValue:(id*/* NSString* */)		ioValue
error:(NSError**)									outError
{
	return [self->rowsObject validateNumberStringValue:ioValue error:outError];
}// validateRowsNumberStringValue:error:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = ([self->behaviorObject isInherited] && [self->rowsObject isInherited]);
	
	
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
	[self willChangeValueForKey:@"currentBehavior"];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	[self willChangeValueForKey:@"rowsEnabled"];
	[self->rowsObject setNilPreferenceValue];
	[self->behaviorObject setNilPreferenceValue];
	[self didChangeValueForKey:@"rowsEnabled"];
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didChangeValueForKey:@"currentBehavior"];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PrefPanelTerminals_ScrollbackValue


@implementation PrefPanelTerminals_ScreenViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelTerminalScreenCocoa" delegate:self context:nullptr];
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


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
screenWidth
{
	return [self->byKey objectForKey:@"screenWidth"];
}// screenWidth


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
screenHeight
{
	return [self->byKey objectForKey:@"screenHeight"];
}// screenHeight


/*!
Accessor.

(4.1)
*/
- (PrefPanelTerminals_ScrollbackValue*)
scrollback
{
	return [self->byKey objectForKey:@"scrollback"];
}// scrollback


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
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagTerminalScreenColumns
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16]
							autorelease]
					forKey:@"screenWidth"];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagTerminalScreenRows
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16]
							autorelease]
					forKey:@"screenHeight"];
	[self->byKey setObject:[[[PrefPanelTerminals_ScrollbackValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"scrollback"];
	
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
	*outIdealSize = self->idealFrame.size;
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
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Terminals.Screen";
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
	return NSLocalizedStringFromTable(@"Screen", @"PrefPanelTerminals", @"the name of this panel");
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end // PrefPanelTerminals_ScreenViewManager


@implementation PrefPanelTerminals_ScreenViewManager (PrefPanelTerminals_ScreenViewManagerInternal)


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return [NSArray arrayWithObjects:
						@"screenWidth", @"screenHeight",
						@"scrollback",
						nil];
}// primaryDisplayBindingKeys


@end // PrefPanelTerminals_ScreenViewManager (PrefPanelTerminals_ScreenViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
