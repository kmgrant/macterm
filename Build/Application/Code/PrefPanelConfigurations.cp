/*###############################################################

	PrefPanelConfigurations.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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

// Unix includes
#include <strings.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <Console.h>
#include <Embedding.h>
#include <ListUtilities.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "StringResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "Configure.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "DuplicateDialog.h"
#include "EventLoop.h"
#include "MenuBar.h"
#include "Panel.h"
#include "PrefPanelConfigurations.h"
#include "SessionEditorDialog.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants

#define NUMBER_OF_CONFIGURATION_TYPES		3
enum
{
	// must be consecutive and zero-based, in order of tabs
	kConfigurationTypeIndexSessions = 0,
	kConfigurationTabIndexSessions = (kConfigurationTypeIndexSessions + 1),
	kConfigurationTypeIndexTerminals = 1,
	kConfigurationTabIndexTerminals = (kConfigurationTypeIndexTerminals + 1),
	kConfigurationTypeIndexFormats = 2,
	kConfigurationTabIndexFormats = (kConfigurationTypeIndexFormats + 1)
};

// The following cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMyDataBrowserPropertyIDFavoriteName	= FOUR_CHAR_CODE('Name')
};

#pragma mark Types

struct ConfigurationsPanelData
{
	struct
	{
		ControlRef		tabs,
						descriptionStaticText,
						dataBrowser,
						buttonContainer,
						newButton,
						newGroupButton,
						removeButton,
						duplicateButton,
						editButton;
	} controls;
	
	Str255				originalConfigurationName;	//!< used in duplication dialog callback
	
	EventHandlerUPP		controlClickUPP;			//!< wrapper for click callback
	EventHandlerRef		listBoxClickHandler;		//!< invoked when the list box is clicked
	EventHandlerRef		tabClickHandler;			//!< invoked when the tabs are clicked
	EventHandlerUPP		buttonCommandsUPP;			//!< wrapper for button commands callback
	EventHandlerRef		newButtonClickHandler;		//!< invoked when the New button is clicked
	EventHandlerRef		newGroupButtonClickHandler;	//!< invoked when the New Group button is clicked
	EventHandlerRef		removeButtonClickHandler;	//!< invoked when the Remove button is clicked
	EventHandlerRef		duplicateButtonClickHandler;//!< invoked when the Duplicate button is clicked
	EventHandlerRef		editButtonClickHandler;		//!< invoked when the Edit button is clicked
	
	DataBrowserItemID	itemToRemove;	//!< only defined while a confirmation sheet is open; used in the callback
};
typedef struct ConfigurationsPanelData	ConfigurationsPanelData;
typedef ConfigurationsPanelData const*	ConfigurationsPanelDataConstPtr;
typedef ConfigurationsPanelData*		ConfigurationsPanelDataPtr;

//
// internal methods
//

static void				createPanelControls				(Panel_Ref							inPanel,
														 WindowRef							inOwningWindow);

static void				deltaPanelSize					(Panel_Ref							inPanel,
														 Point								inChangeInWidthAndHeight);

static void				disposePanel					(Panel_Ref							inPanel,
														 void*								inDataPtr);

static void				duplicateNameEntered			(DuplicateDialogRef					inDialogThatClosed,
														 Boolean							inOKButtonPressed,
														 void*								inUserData);

static CFStringRef		findDataBrowserItemName			(ConfigurationsPanelDataConstPtr	inDataPtr,
														 DataBrowserItemID					inItemID);

static Preferences_Class	getCurrentPreferencesClass	(ConfigurationsPanelDataConstPtr	inDataPtr);

static PrefPanelConfigurations_EditProcPtr	getCurrentResourceEditor	(ConfigurationsPanelDataConstPtr	inDataPtr);

Boolean					isDefaultItem					(ConfigurationsPanelDataConstPtr	inDataPtr,
														 DataBrowserItemID					inItemID);

static SInt32			panelChanged					(Panel_Ref							inPanel,
														 Panel_Message						inMessage,
														 void*								inDataPtr);

static pascal OSStatus	receiveHICommand				(EventHandlerCallRef				inHandlerCallRef,
														 EventRef							inEvent,
														 void*								inConfigurationsPanelDataPtr);

static pascal OSStatus	receiveListBoxHit				(EventHandlerCallRef				inHandlerCallRef,
														 EventRef							inEvent,
														 void*								inConfigurationsPanelDataPtr);

static void				removeListItemWarningCloseNotifyProc	(InterfaceLibAlertRef		inAlertThatClosed,
														 SInt16								inItemHit,
														 void*								inConfigurationsPanelDataPtr);

static void				setButtonStates					(ConfigurationsPanelDataPtr			inDataPtr);

static void				setUpConfigurationsControls		(Panel_Ref							inPanel);

static void				setUpConfigurationsListControl	(Preferences_Class					inPreferencesClass,
														 ConfigurationsPanelDataPtr			inDataPtr);



//
// public methods
//

/*!
This routine creates a new preference panel for
the Configurations category, initializes it, and
returns a reference to it.  You must destroy the
reference using Panel_Dispose() when you are
done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelConfigurations_New ()
{
	Panel_Ref		result = Panel_New(panelChanged);
	
	
	if (nullptr != result)
	{
		ConfigurationsPanelDataPtr	dataPtr = (ConfigurationsPanelDataPtr)Memory_NewPtr(sizeof(ConfigurationsPanelData));
		
		
		Panel_SetKind(result, CFSTR("FOO"));
		Panel_SetName(result, CFSTR("Favorites"));
		//Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFavoritesIconFilenameNoExtension(),
		//								kConstantsRegistry_ApplicationCreatorSignature, 'PFav');
		Panel_SetImplementation(result, dataPtr);
		
		// initialize the data
		dataPtr->listBoxClickHandler = nullptr;
		dataPtr->tabClickHandler = nullptr;
		dataPtr->newButtonClickHandler = nullptr;
		dataPtr->newGroupButtonClickHandler = nullptr;
		dataPtr->removeButtonClickHandler = nullptr;
		dataPtr->duplicateButtonClickHandler = nullptr;
		dataPtr->editButtonClickHandler = nullptr;
		dataPtr->controlClickUPP = nullptr;
	}
	return result;
}// New


//
// internal methods
//
#pragma mark -

/*!
This routine creates the controls that belong in the
Configurations panel, and attaches them to the
specified owning window in the proper embedding
hierarchy.  The specified panel’s descriptor must be
"kMyPrefPanelDescriptorFavorites".

The controls are not arranged or sized.

(3.0)
*/
static void
createPanelControls		(Panel_Ref		inPanel,
						 WindowRef		inOwningWindow)
{
	ControlRef					container = nullptr;
	ControlRef					control = nullptr;
	ConfigurationsPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), ConfigurationsPanelDataPtr);
	Rect						containerBounds;
	OSStatus					error = noErr;
	
	
	SetRect(&containerBounds, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &container);
	Panel_SetContainerView(inPanel, container);
	SetControlVisibility(container, false/* visible */, false/* draw */);
	
	// tab control
	{
		ControlTabEntry		tabInfo[NUMBER_OF_CONFIGURATION_TYPES];
		UInt16				i = 0;
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		
		
		// nullify or zero-fill everything, then set only what matters
		bzero(&tabInfo, sizeof(tabInfo));
		tabInfo[0].enabled =
			tabInfo[1].enabled =
			tabInfo[2].enabled = true;
		//stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoriteSessionsTabName,
		//								tabInfo[kConfigurationTypeIndexSessions].name);
		//stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoriteTerminalsTabName,
		//								tabInfo[kConfigurationTypeIndexTerminals].name);
		//stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoriteFormatsTabName,
		//								tabInfo[kConfigurationTypeIndexFormats].name);
		SetRect(&containerBounds, 0, 0, 0, 0);
		error = CreateTabsControl(inOwningWindow, &containerBounds, kControlTabSizeLarge, kControlTabDirectionNorth,
									sizeof(tabInfo) / sizeof(ControlTabEntry)/* number of tabs */, tabInfo,
									&dataPtr->controls.tabs);
		for (i = 0; i < sizeof(tabInfo) / sizeof(ControlTabEntry); ++i)
		{
			if (nullptr != tabInfo[i].name) CFRelease(tabInfo[i].name), tabInfo[i].name = nullptr;
		}
	}
	control = dataPtr->controls.tabs;
	(OSStatus)EmbedControl(control, container);
	
	// group box contents
	{
		ControlFontStyleRec		styleInfo;
		
		
		bzero(&styleInfo, sizeof(styleInfo));
		SetRect(&containerBounds, 0, 0, 0, 0);
		error = CreateStaticTextControl(inOwningWindow, &containerBounds, CFSTR(""), &styleInfo,
										&dataPtr->controls.descriptionStaticText);
	}
	control = dataPtr->controls.descriptionStaticText;
	Localization_SetControlThemeFontInfo(control, kThemeSmallSystemFont);
	(OSStatus)EmbedControl(control, dataPtr->controls.tabs);
	{
		DataBrowserCallbacks	callbacks;
		
		
		// define a callback for specifying what data belongs in the list
		callbacks.version = kDataBrowserLatestCallbacks;
		error = InitDataBrowserCallbacks(&callbacks);
		assert_noerr(error);
	#if 0
		callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
		assert(nullptr != callbacks.u.v1.itemDataCallback);
		callbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(compareDataBrowserItems);
		assert(nullptr != callbacks.u.v1.itemCompareCallback);
		callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
		assert(nullptr != callbacks.u.v1.itemNotificationCallback);
	#endif
		
		SetRect(&containerBounds, 0, 0, 0, 0);
		(OSStatus)CreateDataBrowserControl(inOwningWindow, &containerBounds, kDataBrowserListView,
											&dataPtr->controls.dataBrowser);
		control = dataPtr->controls.dataBrowser;
		(OSStatus)SetDataBrowserHasScrollBars(control, false/* horizontal */, true/* vertical */);
		(OSStatus)SetDataBrowserSelectionFlags(control, kDataBrowserSelectOnlyOne | kDataBrowserNoDisjointSelection |
														kDataBrowserResetSelection | kDataBrowserCmdTogglesSelection);
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		if (FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)DataBrowserChangeAttributes
						(control, FUTURE_SYMBOL(1 << 1, kDataBrowserAttributeListViewAlternatingRowColors)/* attributes to set */,
							0/* attributes to clear */);
		}
	#endif
		(OSStatus)SetDataBrowserListViewUsePlainBackground(control, false);
		(OSStatus)EmbedControl(control, dataPtr->controls.tabs);
	}
	SetRect(&containerBounds, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &dataPtr->controls.buttonContainer);
	control = dataPtr->controls.buttonContainer;
	(OSStatus)EmbedControl(control, dataPtr->controls.tabs);
	{
		CFStringRef			titleCFString = nullptr;
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		
		
		SetRect(&containerBounds, 0, 0, 0, 0);
		
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesNewButtonName, titleCFString);
		error = CreatePushButtonControl(inOwningWindow, &containerBounds, titleCFString, &control);
		if (nullptr != titleCFString) CFRelease(titleCFString), titleCFString = nullptr;
		dataPtr->controls.newButton = control;
		(OSStatus)EmbedControl(control, dataPtr->controls.buttonContainer);
		(OSStatus)SetControlCommandID(control, kCommandPreferencesNewFavorite);
		
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesNewGroupButtonName, titleCFString);
		error = CreatePushButtonControl(inOwningWindow, &containerBounds, titleCFString, &control);
		if (nullptr != titleCFString) CFRelease(titleCFString), titleCFString = nullptr;
		dataPtr->controls.newGroupButton = control;
		(OSStatus)EmbedControl(control, dataPtr->controls.buttonContainer);
		//(OSStatus)SetControlCommandID(control, kCommandPreferencesNewFavoriteGroup);
		
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesRemoveButtonName, titleCFString);
		error = CreatePushButtonControl(inOwningWindow, &containerBounds, titleCFString, &control);
		if (nullptr != titleCFString) CFRelease(titleCFString), titleCFString = nullptr;
		dataPtr->controls.removeButton = control;
		(OSStatus)EmbedControl(control, dataPtr->controls.buttonContainer);
		(OSStatus)SetControlCommandID(control, kCommandPreferencesDeleteFavorite);
		
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesDuplicateButtonName, titleCFString);
		error = CreatePushButtonControl(inOwningWindow, &containerBounds, titleCFString, &control);
		if (nullptr != titleCFString) CFRelease(titleCFString), titleCFString = nullptr;
		dataPtr->controls.duplicateButton = control;
		(OSStatus)EmbedControl(control, dataPtr->controls.buttonContainer);
		(OSStatus)SetControlCommandID(control, kCommandPreferencesDuplicateFavorite);
		
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesEditButtonName, titleCFString);
		error = CreatePushButtonControl(inOwningWindow, &containerBounds, titleCFString, &control);
		if (nullptr != titleCFString) CFRelease(titleCFString), titleCFString = nullptr;
		dataPtr->controls.editButton = control;
		(OSStatus)EmbedControl(control, dataPtr->controls.buttonContainer);
		//(OSStatus)SetControlCommandID(control, kCommandPreferencesEditFavorite);
	}
	
	// install a callback that responds to buttons in the window
	{
		EventTypeSpec const		whenButtonClicked[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		
		
		dataPtr->buttonCommandsUPP = NewEventHandlerUPP(receiveHICommand);
		error = HIViewInstallEventHandler(dataPtr->controls.newButton, dataPtr->buttonCommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, dataPtr/* user data */,
											&dataPtr->newButtonClickHandler/* event handler reference */);
		error = HIViewInstallEventHandler(dataPtr->controls.newGroupButton, dataPtr->buttonCommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, dataPtr/* user data */,
											&dataPtr->newGroupButtonClickHandler/* event handler reference */);
		error = HIViewInstallEventHandler(dataPtr->controls.removeButton, dataPtr->buttonCommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, dataPtr/* user data */,
											&dataPtr->removeButtonClickHandler/* event handler reference */);
		error = HIViewInstallEventHandler(dataPtr->controls.duplicateButton, dataPtr->buttonCommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, dataPtr/* user data */,
											&dataPtr->duplicateButtonClickHandler/* event handler reference */);
		error = HIViewInstallEventHandler(dataPtr->controls.editButton, dataPtr->buttonCommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, dataPtr/* user data */,
											&dataPtr->editButtonClickHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that responds to clicks in the list box
	{
		EventTypeSpec const		whenListIsHit[] =
								{
									{ kEventClassControl, kEventControlHit }
								};
		
		
		dataPtr->controlClickUPP = NewEventHandlerUPP(receiveListBoxHit);
		error = HIViewInstallEventHandler(dataPtr->controls.dataBrowser, dataPtr->controlClickUPP, GetEventTypeCount(whenListIsHit),
											whenListIsHit, dataPtr/* user data */,
											&dataPtr->listBoxClickHandler/* event handler reference */);
		error = HIViewInstallEventHandler(dataPtr->controls.tabs, dataPtr->controlClickUPP, GetEventTypeCount(whenListIsHit),
											whenListIsHit, dataPtr/* user data */,
											&dataPtr->tabClickHandler/* event handler reference */);
		assert(error == noErr);
	}
}// createPanelControls


/*!
This routine adjusts the controls in preference panels
to match the specified change in dimensions of a panel’s
container.

(3.0)
*/
static void
deltaPanelSize		(Panel_Ref		inPanel,
					 Point			inChangeInWidthAndHeight)
{
	ControlRef					control = nullptr;
	ConfigurationsPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), ConfigurationsPanelDataPtr);
	Rect						controlRect;
	
	
	// move and/or resize controls
	{
		Str255		text;
		
		
		// resize the containing group box
		control = dataPtr->controls.tabs;
		GetControlBounds(control, &controlRect);
		SizeControl(control, controlRect.right - controlRect.left + inChangeInWidthAndHeight.h,
								controlRect.bottom - controlRect.top + inChangeInWidthAndHeight.v);
		
		// recalculate the smallest area that the text can occupy
		control = dataPtr->controls.descriptionStaticText;
		GetControlBounds(control, &controlRect);
		MoveControl(control, controlRect.left, controlRect.top + inChangeInWidthAndHeight.v);
		SizeControl(control, controlRect.right - controlRect.left + inChangeInWidthAndHeight.h, controlRect.bottom - controlRect.top);
		GetControlText(control, text);
		(UInt16)Localization_SetUpMultiLineTextControl(control, text); // auto-sizes text area
		
		// depending on the locale, decide how to resize and/or rearrange the list and buttons
		{
			Boolean		isLeftToRight = Localization_IsLeftToRight(); // text reading direction
			
			
			control = dataPtr->controls.dataBrowser;
			GetControlBounds(control, &controlRect);
			unless (isLeftToRight) MoveControl(control, controlRect.left + inChangeInWidthAndHeight.h, controlRect.top);
			SizeControl(control, controlRect.right - controlRect.left + inChangeInWidthAndHeight.h,
									controlRect.bottom - controlRect.top + inChangeInWidthAndHeight.v);
			
			control = dataPtr->controls.buttonContainer;
			GetControlBounds(control, &controlRect);
			if (isLeftToRight) MoveControl(control, controlRect.left + inChangeInWidthAndHeight.h, controlRect.top);
			SizeControl(control, controlRect.right - controlRect.left,
									controlRect.bottom - controlRect.top + inChangeInWidthAndHeight.v);
		}
	}
}// deltaPanelSize


/*!
This routine responds to a change in the existence
of a panel.  The panel is about to be destroyed, so
this routine disposes of private data structures
associated with the specified panel.

(3.0)
*/
static void
disposePanel	(Panel_Ref		UNUSED_ARGUMENT(inPanel),
				 void*			inDataPtr)
{
	ConfigurationsPanelDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, ConfigurationsPanelDataPtr);
	
	
	// remove event handlers
	RemoveEventHandler(dataPtr->listBoxClickHandler), dataPtr->listBoxClickHandler = nullptr;
	RemoveEventHandler(dataPtr->tabClickHandler), dataPtr->tabClickHandler = nullptr;
	RemoveEventHandler(dataPtr->newButtonClickHandler), dataPtr->newButtonClickHandler = nullptr;
	RemoveEventHandler(dataPtr->newGroupButtonClickHandler), dataPtr->newGroupButtonClickHandler = nullptr;
	RemoveEventHandler(dataPtr->removeButtonClickHandler), dataPtr->removeButtonClickHandler = nullptr;
	RemoveEventHandler(dataPtr->duplicateButtonClickHandler), dataPtr->duplicateButtonClickHandler = nullptr;
	RemoveEventHandler(dataPtr->editButtonClickHandler), dataPtr->editButtonClickHandler = nullptr;
	DisposeEventHandlerUPP(dataPtr->controlClickUPP), dataPtr->controlClickUPP = nullptr;
	DisposeEventHandlerUPP(dataPtr->buttonCommandsUPP), dataPtr->buttonCommandsUPP = nullptr;
	
	Memory_DisposePtr(REINTERPRET_CAST(&dataPtr, Ptr*));
}// disposePanel


/*!
Responds to a close of the Duplicate dialog sheet.  This
will cause a new resource to be created that is a copy
of the currently selected one, using the name most
recently entered in the dialog.

(3.1)
*/
static void
duplicateNameEntered	(DuplicateDialogRef		inDialogThatClosed,
						 Boolean				inOKButtonPressed,
						 void*					inUserData)
{
	ConfigurationsPanelDataPtr   dataPtr = REINTERPRET_CAST(inUserData, ConfigurationsPanelDataPtr);
	
	
	assert(nullptr != dataPtr);
	if (inOKButtonPressed)
	{
		CFStringRef			duplicateName = nullptr;
		Preferences_Class	preferencesClass = kPreferences_ClassGeneral;
		
		
		// get the name entered by the user
		DuplicateDialog_GetNameString(inDialogThatClosed, duplicateName);
		if (nullptr != duplicateName)
		{
			CFRetainRelease		duplicateNameObject(duplicateName);
			Handle				itemsHandle = Memory_NewHandle(sizeof(DataBrowserItemID));
			OSStatus			error = noErr;
			
			
			// determine the type of resource being examined
			preferencesClass = getCurrentPreferencesClass(dataPtr);
			
			// determine the name of the currently selected context
			error = GetDataBrowserItems(dataPtr->controls.dataBrowser, kDataBrowserNoItem/* container */,
										false/* recurse */, kDataBrowserItemIsSelected, itemsHandle);
			if (noErr == error)
			{
				// expect just one item selected
				assert(1 == GetHandleSize(itemsHandle) / sizeof(DataBrowserItemID));
				DataBrowserItemID const		kSelectedItem = **REINTERPRET_CAST(itemsHandle, DataBrowserItemID**);
				Preferences_ContextRef		originalContext = nullptr;
				Preferences_ContextRef		duplicateContext = nullptr;
				CFStringRef					originalNameCFString = nullptr;
				
				
				// find the original
				originalNameCFString = findDataBrowserItemName(dataPtr, kSelectedItem);
				originalContext = Preferences_NewContext(preferencesClass, originalNameCFString);
				if (nullptr == originalContext)
				{
					Console_WriteLine("unable to find selected item's preferences context");
					Sound_StandardAlert();
				}
				else
				{
					// now duplicate the preferences data as requested
					//duplicateContext = Preferences_NewContextDuplicate(originalContext, duplicateName);
					
					// 3.0 - theme sounds!
					(OSStatus)PlayThemeSound(kThemeSoundNewItem);
					
					// now read the resource with the original name, and construct a copy of it
					if (nullptr == duplicateContext)
					{
						Console_WriteLine("unable to duplicate preferences context");
						Sound_StandardAlert();
					}
					else
					{
						// success!
						(Preferences_Result)Preferences_ContextSave(duplicateContext);
						setUpConfigurationsListControl(preferencesClass, dataPtr);
						Preferences_ReleaseContext(&duplicateContext);
					}
					Preferences_ReleaseContext(&originalContext);
				}
			}
			Memory_DisposeHandle(&itemsHandle);
		}
	}
	
	// finally, call the standard handler to destroy the dialog
	DuplicateDialog_StandardCloseNotifyProc(inDialogThatClosed, inOKButtonPressed, inUserData);
}// duplicateNameEntered


/*!
Returns the name of the given item, or nullptr.

Currently, the string is not automatically retained.

(3.1)
*/
CFStringRef
findDataBrowserItemName		(ConfigurationsPanelDataConstPtr	inDataPtr,
							 DataBrowserItemID					inItemID)
{
	CFStringRef		result = nullptr;
	
	
	// UNIMPLEMENTED
	return result;
}// findDataBrowserItemName


/*!
Returns the preferences class associated with the
currently displayed configuration set list.

(3.0)
*/
static Preferences_Class
getCurrentPreferencesClass	(ConfigurationsPanelDataConstPtr	inDataPtr)
{
	Preferences_Class	result = kPreferences_ClassGeneral;
	
	
	switch (GetControlValue(inDataPtr->controls.tabs))
	{
	case kConfigurationTabIndexSessions: // Session Favorites
		result = kPreferences_ClassSession;
		break;
	
	case kConfigurationTabIndexTerminals: // Terminals
		result = kPreferences_ClassTerminal;
		break;
	
	case kConfigurationTabIndexFormats: // Formats
		result = kPreferences_ClassFormat;
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// getCurrentPreferencesClass


/*!
Returns the resource editor associated with the currently
displayed configuration set list.

(3.0)
*/
static PrefPanelConfigurations_EditProcPtr
getCurrentResourceEditor	(ConfigurationsPanelDataConstPtr	inDataPtr)
{
	PrefPanelConfigurations_EditProcPtr		result = nullptr;
	
	
	switch (GetControlValue(inDataPtr->controls.tabs))
	{
	case kConfigurationTabIndexSessions: // Session Favorites
		result = SessionEditorDialog_Display;
		break;
	
	case kConfigurationTabIndexTerminals: // Terminals
		result = EditTerminal;
		break;
	
	case kConfigurationTabIndexFormats: // Formats
		result = nullptr; // TEMP - BROKEN!!!
		break;
	
	default:
		// ???
		result = nullptr; // signal failure
		break;
	}
	return result;
}// getCurrentResourceEditor


/*!
Returns true if the specified item ID is the default
item of the given panel’s data browser.

(3.1)
*/
Boolean
isDefaultItem	(ConfigurationsPanelDataConstPtr	inDataPtr,
				 DataBrowserItemID					inItemID)
{
	Boolean		result = false;
	
	
	// UNIMPLEMENTED
	return result;
}// isDefaultItem


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.0)
*/
static SInt32
panelChanged	(Panel_Ref			inPanel,
				 Panel_Message		inMessage,
				 void*				inDataPtr)
{
	SInt32		result = 0L;
	
	
	//if (Panel_ReturnDescriptor(inPanel) == kMyPrefPanelDescriptorFavorites)
	{
		switch (inMessage)
		{
		case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
			{
				WindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
				
				
				createPanelControls(inPanel, *windowPtr);
				setUpConfigurationsControls(inPanel);
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
				//ControlRef const*		controlPtr = REINTERPRET_CAST(inDataPtr, ControlRef*);
				
				
				// do nothing
			}
			break;
		
		case kPanel_MessageFocusLost: // notification that a control is no longer focused
			{
				//ControlRef const*		controlPtr = REINTERPRET_CAST(inDataPtr, ControlRef*);
				
				
				// do nothing
			}
			break;
		
		case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate control sizes
			{
				// this notification is currently ignored, but shouldn’t be...
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
	}
	
	return result;
}// panelChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inConfigurationsPanelDataPtr)
{
	OSStatus					result = eventNotHandledErr;
	ConfigurationsPanelDataPtr	dataPtr = REINTERPRET_CAST(inConfigurationsPanelDataPtr, ConfigurationsPanelDataPtr);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			PrefPanelConfigurations_EditProcPtr		procPtr = getCurrentResourceEditor(dataPtr);
			Preferences_Class						preferencesClass = getCurrentPreferencesClass(dataPtr);
			
			
			switch (received.commandID)
			{
			case kCommandPreferencesNewFavorite:
				// display the appropriate dialog box for editing a new item
				if (nullptr == procPtr) Sound_StandardAlert();
				else
				{
					if (PrefPanelConfigurations_InvokeEditProc(procPtr, preferencesClass, nullptr/* name */,
																nullptr/* zoom rectangle starting boundaries */))
					{
						// 3.0 - theme sounds!
						(OSStatus)PlayThemeSound(kThemeSoundNewItem);
						setUpConfigurationsListControl(preferencesClass, dataPtr);
					}
				}
				break;
			
			case 'Foob'://kCommandPreferencesNewFavoriteGroup:
				// create a new group
				{
					Str255		dialogTitle;
					Str255		groupName;
					
					
					PLstrcpy(dialogTitle, "\pCreate New Group"); // LOCALIZE THIS
					PLstrcpy(groupName, EMPTY_PSTRING/* default name */);
					if (Configure_NameNewConfigurationDialogDisplay(dialogTitle, groupName/* updated on output, if "true" is returned */,
																	nullptr/* source rectangle */, nullptr/* destination rectangle */))
					{
						Sound_StandardAlert(); // unimplemented
					}
				}
				break;
			
			case kCommandPreferencesDeleteFavorite:
				// destroy the selected item’s corresponding preferences
				{
					Handle		itemsHandle = Memory_NewHandle(sizeof(DataBrowserItemID));
					OSStatus	error = noErr;
					
					
					error = GetDataBrowserItems(dataPtr->controls.dataBrowser, kDataBrowserNoItem/* container */,
												false/* recurse */, kDataBrowserItemIsSelected, itemsHandle);
					if (noErr == error)
					{
						DataBrowserItemID const		kSelectedItem = **REINTERPRET_CAST(itemsHandle, DataBrowserItemID**);
						InterfaceLibAlertRef		box = nullptr;
						
						
						if (isDefaultItem(dataPtr, kSelectedItem))
						{
							CFStringRef			dialogTextCFString = nullptr;
							UIStrings_Result	stringResult = kUIStrings_ResultOK;
							
							
							stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesCannotRemoveDefault,
															dialogTextCFString);
							if (stringResult.ok())
							{
								box = Alert_New();
								Alert_SetHelpButton(box, false);
								Alert_SetParamsFor(box, kAlert_StyleOK);
								Alert_SetTextCFStrings(box, dialogTextCFString, nullptr/* help text */);
								Alert_SetType(box, kAlertStopAlert);
								Alert_MakeWindowModal(box, GetControlOwner(dataPtr->controls.dataBrowser), false/* is window close alert */,
														Alert_StandardCloseNotifyProc, nullptr/* user data */);
								Alert_Display(box); // notifier disposes the alert when the sheet eventually closes
								CFRelease(dialogTextCFString), dialogTextCFString = nullptr;
							}
							else
							{
								Sound_StandardAlert();
							}
						}
						else
						{
							SInt16		alertItemHit = kAlertStdAlertOKButton;
							
							
							if (EventLoop_IsOptionKeyDown())
							{
								alertItemHit = kAlertStdAlertOKButton; // option key means “just delete it, spare the commercial”
							}
							else
							{
								CFStringRef			itemNameCFString = findDataBrowserItemName(dataPtr, kSelectedItem);
								CFStringRef			templateCFString = nullptr;
								UIStrings_Result	stringResult = kUIStrings_ResultOK;
								
								
								if (nullptr == itemNameCFString) itemNameCFString = CFSTR("");
								stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesRemoveWarning,
																templateCFString);
								if (stringResult.ok())
								{
									CFStringRef		finalCFString = CFStringCreateWithFormat
																	(kCFAllocatorDefault, nullptr/* format options */,
																		templateCFString, itemNameCFString);
									
									
									if (nullptr != finalCFString)
									{
										CFStringRef		helpTextCFString = nullptr;
										
										
										stringResult = UIStrings_Copy
														(kUIStrings_PreferencesWindowFavoritesRemoveWarningHelpText,
															helpTextCFString);
										box = Alert_New();
										Alert_SetHelpButton(box, false);
										Alert_SetParamsFor(box, kAlert_StyleOKCancel);
										Alert_SetTextCFStrings(box, finalCFString, helpTextCFString);
										Alert_SetType(box, kAlertCautionAlert);
										dataPtr->itemToRemove = kSelectedItem; // store this for callback’s convenient access
										Alert_MakeWindowModal(box, GetControlOwner(dataPtr->controls.dataBrowser), false/* is window close alert */,
																removeListItemWarningCloseNotifyProc, dataPtr/* user data */);
										Alert_Display(box); // notifier disposes the alert when the sheet eventually closes
										if (nullptr != helpTextCFString) CFRelease(helpTextCFString), helpTextCFString = nullptr;
										CFRelease(finalCFString), finalCFString = nullptr;
									}
									CFRelease(templateCFString), templateCFString = nullptr;
								}
							}
						}
					}
					Memory_DisposeHandle(&itemsHandle);
				}
				break;
			
			case kCommandPreferencesDuplicateFavorite:
				// create a copy of the selected item’s preferences, and update the list
				{
					Handle		itemsHandle = Memory_NewHandle(sizeof(DataBrowserItemID));
					OSStatus	error = noErr;
					
					
					error = GetDataBrowserItems(dataPtr->controls.dataBrowser, kDataBrowserNoItem/* container */,
												false/* recurse */, kDataBrowserItemIsSelected, itemsHandle);
					if (noErr == error)
					{
						DataBrowserItemID const		kSelectedItem = **REINTERPRET_CAST(itemsHandle, DataBrowserItemID**);
						CFStringRef					itemNameCFString = findDataBrowserItemName(dataPtr, kSelectedItem);
						CFStringRef					templateCFString = nullptr;
						UIStrings_Result			stringResult = kUIStrings_ResultOK;
						
						
						if (nullptr == itemNameCFString) itemNameCFString = CFSTR("");
						stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFavoritesDuplicateNameTemplate,
														templateCFString);
						if (stringResult.ok())
						{
							CFStringRef		duplicateNameCFString = CFStringCreateWithFormat
																	(kCFAllocatorDefault, nullptr/* format options */,
																		templateCFString, itemNameCFString);
							
							
							if (nullptr != duplicateNameCFString)
							{
								DuplicateDialogRef		duplicateDialog = nullptr;
								
								
								// display a dialog asking the user what the name of the duplicate should be
								duplicateDialog = DuplicateDialog_New(duplicateNameEntered, duplicateNameCFString,
																		dataPtr);
								if (nullptr != duplicateDialog)
								{
									DuplicateDialog_Display(duplicateDialog,
															GetControlOwner(dataPtr->controls.dataBrowser));
								}
								else
								{
									Sound_StandardAlert();
								}
								CFRelease(duplicateNameCFString), duplicateNameCFString = nullptr;
							}
							CFRelease(templateCFString), templateCFString = nullptr;
						}
					}
					Memory_DisposeHandle(&itemsHandle);
				}
				break;
			
			case 'Edit'://kCommandPreferencesEditFavorite:
				// display the appropriate editor dialog box for editing the new item
				if (nullptr == procPtr) Sound_StandardAlert();
				else
				{
					Handle		itemsHandle = Memory_NewHandle(sizeof(DataBrowserItemID));
					OSStatus	error = noErr;
					
					
					error = GetDataBrowserItems(dataPtr->controls.dataBrowser, kDataBrowserNoItem/* container */,
												false/* recurse */, kDataBrowserItemIsSelected, itemsHandle);
					if (noErr == error)
					{
						DataBrowserItemID const		kSelectedItem = **REINTERPRET_CAST(itemsHandle, DataBrowserItemID**);
						CFStringRef					itemNameCFString = findDataBrowserItemName(dataPtr, kSelectedItem);
						
						
						if (nullptr != itemNameCFString)
						{
							Rect	sourceCellRect;
							
							
							(OSStatus)GetDataBrowserItemPartBounds(dataPtr->controls.dataBrowser, kSelectedItem,
																	kMyDataBrowserPropertyIDFavoriteName,
																	kDataBrowserPropertyEnclosingPart, &sourceCellRect);
							(Boolean)PrefPanelConfigurations_InvokeEditProc(procPtr, preferencesClass, itemNameCFString,
																			&sourceCellRect/* zoom rectangle starting boundaries */);
						}
					}
				}
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventControlClick" of "kEventClassControl"
for the list box of this preferences panel.  Responds
by changing the selection, which affects button
states, etc.  Also detects double-clicking.

Also, temporarily, this handles clicks in other
controls.  Eventually, the buttons will simply have
commands associated with them, and a command handler
will be installed instead.

(3.1)
*/
static pascal OSStatus
receiveListBoxHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inConfigurationsPanelDataPtr)
{
	OSStatus					result = eventNotHandledErr;
	ConfigurationsPanelDataPtr	dataPtr = REINTERPRET_CAST(inConfigurationsPanelDataPtr, ConfigurationsPanelDataPtr);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		ControlRef		control = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the control was found, proceed
		if (result == noErr)
		{
			Preferences_Class	preferencesClass = getCurrentPreferencesClass(dataPtr);
			Boolean				handled = false;
			
			
			result = eventNotHandledErr; // unless set otherwise
			
			if (control == dataPtr->controls.tabs)
			{
				Str255		descriptionString;
				
				
				// change the description string
				//GetIndString(descriptionString, rStringsConfigurationData,
				//				dataPtr->descriptionStringIndices[GetControlValue(dataPtr->controls.tabs) - 1]);
				SetControlText(dataPtr->controls.descriptionStaticText, descriptionString);
				RegionUtilities_RedrawControlOnNextUpdate(dataPtr->controls.descriptionStaticText);
				
				// update the list to reflect a new set of preferences
				setUpConfigurationsListControl(preferencesClass, dataPtr);
				
				// for Carbon’s benefit - handle updates immediately
			#if TARGET_API_MAC_CARBON
				//EventLoop_HandlePendingUpdates();
			#endif
				
				handled = true;
			}
			else if (control == dataPtr->controls.dataBrowser)
			{
				// handle double-clicks?
			#if 0
				// set button states based on the list selection
				setButtonStates(dataPtr);
				
				//if (double click) flash action button
				//UNIMPLEMENTED
			#endif
				// send kCommandPreferencesEditFavorite as Carbon Event?
			}
			
			if (handled) result = noErr;
		}
	}
	
	return result;
}// receiveListBoxHit


/*!
The responder to a closed “really remove list item?” alert.
This routine removes the selected item in the list if the
item hit is the OK button, otherwise it does not remove any
items from the list.  The given alert is destroyed.

(3.0)
*/
static void
removeListItemWarningCloseNotifyProc	(InterfaceLibAlertRef	inAlertThatClosed,
										 SInt16					inItemHit,
										 void*					inConfigurationsPanelDataPtr)
{
	ConfigurationsPanelDataPtr		dataPtr = REINTERPRET_CAST(inConfigurationsPanelDataPtr, ConfigurationsPanelDataPtr);
	
	
	if (inItemHit == kAlertStdAlertOKButton)
	{
		Preferences_ContextRef	preferencesContext = nullptr;
		Preferences_Class		preferencesClass = kPreferences_ClassGeneral;
		CFStringRef				itemNameCFString = nullptr;
		
		
		// determine the type of data being removed
		preferencesClass = getCurrentPreferencesClass(dataPtr);
		
		// get the selected cell’s name
		itemNameCFString = findDataBrowserItemName(dataPtr, dataPtr->itemToRemove);
		
		// find and delete the appropriate preferences data
		preferencesContext = Preferences_NewContext(preferencesClass, itemNameCFString);
		(Preferences_Result)Preferences_ContextDeleteSaved(preferencesContext);
		Preferences_ReleaseContext(&preferencesContext);
		
		// update UI
		setButtonStates(dataPtr);
		//MenuBar_RebuildNewSessionFromFavoritesMenu();
		//MenuBar_RebuildFormatFavoritesMenu();
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
	
	// reset state that is only supposed to be valid in the callback
	dataPtr->itemToRemove = kDataBrowserNoItem;
}// removeListItemWarningCloseNotifyProc


/*!
Checks the list control to see if any items are selected,
and activates or deactivates buttons appropriately.

(3.0)
*/
static void
setButtonStates		(ConfigurationsPanelDataPtr		inDataPtr)
{
	UInt32		numberOfSelectedItems = 0;
	OSStatus	error = noErr;
	
	
	// set button states based on the list selection
	error = GetDataBrowserItemCount(inDataPtr->controls.dataBrowser, kDataBrowserNoItem/* container */,
									false/* recurse */, kDataBrowserItemIsSelected, &numberOfSelectedItems);
	if ((noErr == error) && (0 != numberOfSelectedItems))
	{
		ActivateControl(inDataPtr->controls.newButton);
		ActivateControl(inDataPtr->controls.newGroupButton);
		ActivateControl(inDataPtr->controls.removeButton);
		ActivateControl(inDataPtr->controls.duplicateButton);
		ActivateControl(inDataPtr->controls.editButton);
	}
	else
	{
		ActivateControl(inDataPtr->controls.newButton);
		ActivateControl(inDataPtr->controls.newGroupButton);
		DeactivateControl(inDataPtr->controls.removeButton);
		DeactivateControl(inDataPtr->controls.duplicateButton);
		DeactivateControl(inDataPtr->controls.editButton);
	}
}// setButtonStates


/*!
This routine sizes and arranges controls in preference
panels to fit the prescribed dimensions of a panel’s
container.

(3.0)
*/
static void
setUpConfigurationsControls		(Panel_Ref		inPanel)
{
	enum
	{
		kOutsetButtonContainerFromButtonsH = 10,	// in pixels; arbitrary whitespace between outer edges of buttons and user pane edges
		kOutsetButtonContainerFromButtonsV = 10		//   (important on Mac OS X to leave enough room for button shadows)
	};
	ConfigurationsPanelDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), ConfigurationsPanelDataPtr);
	ControlRef						container = nullptr;
	ControlRef						control = nullptr;
	Rect							containerBounds;
	Rect							controlRect;
	UInt16							leftEdge = 0; // cumulative, resettable measurement allowing controls to sandwich horizontally
	UInt16							topEdge = 0; // cumulative, resettable measurement allowing controls to stack vertically
	UInt16							buttonsMaximumWidth = 0;
	UInt16							interiorWidth = 0;
	UInt16							interiorHeight = 0;
	UInt16							listInitialHeight = 0;
	Point							tabFrameTopLeft;
	Point							tabFrameWidthHeight;
	Point							tabPaneTopLeftInset;
	Point							tabPaneBottomRightInset;
	
	
	Panel_GetContainerView(inPanel, container);
	GetControlBounds(container, &containerBounds);
	
	Panel_CalculateTabFrame(container, &tabFrameTopLeft, &tabFrameWidthHeight);
	Panel_GetTabPaneInsets(&tabPaneTopLeftInset, &tabPaneBottomRightInset);
	interiorWidth = tabFrameWidthHeight.h - tabPaneTopLeftInset.h - tabPaneBottomRightInset.h;
	interiorHeight = tabFrameWidthHeight.v - tabPaneTopLeftInset.v - tabPaneBottomRightInset.v;
	
	// size controls
	control = dataPtr->controls.tabs;
	SizeControl(control, tabFrameWidthHeight.h, tabFrameWidthHeight.v);
	control = dataPtr->controls.descriptionStaticText;
	{
		Str255		string;
		UInt16		textHeight = 0;
		
		
		// initialize to the first set (may change if the user selects a different tab)
		//GetIndString(string, rStringsConfigurationData, dataPtr->descriptionStringIndices[0]);
		SizeControl(control, interiorWidth, 0);
		textHeight = Localization_SetUpMultiLineTextControl(control, string); // auto-sizes and sets font
		// TMP - hack in extra space since I can’t figure out why this gets calculated incorrectly
		textHeight += 10;
		listInitialHeight = interiorHeight - textHeight - VSP_CONTROLS;
	}
	{
		// find the widest button, and make the container that wide
		UInt16		buttonWidth = 0;
		
		
		buttonWidth = Localization_AutoSizeButtonControl(dataPtr->controls.newButton, 0);
		buttonWidth = Localization_AutoSizeButtonControl(dataPtr->controls.newGroupButton, buttonWidth);
		buttonWidth = Localization_AutoSizeButtonControl(dataPtr->controls.removeButton, buttonWidth);
		buttonWidth = Localization_AutoSizeButtonControl(dataPtr->controls.duplicateButton, buttonWidth);
		buttonWidth = Localization_AutoSizeButtonControl(dataPtr->controls.editButton, buttonWidth);
		buttonsMaximumWidth = buttonWidth;
		control = dataPtr->controls.newButton;
		GetControlBounds(control, &controlRect);
		SizeControl(control, buttonsMaximumWidth, controlRect.bottom - controlRect.top);
		control = dataPtr->controls.newGroupButton;
		SizeControl(control, buttonsMaximumWidth, controlRect.bottom - controlRect.top);
		control = dataPtr->controls.removeButton;
		SizeControl(control, buttonsMaximumWidth, controlRect.bottom - controlRect.top);
		control = dataPtr->controls.duplicateButton;
		SizeControl(control, buttonsMaximumWidth, controlRect.bottom - controlRect.top);
		control = dataPtr->controls.editButton;
		SizeControl(control, buttonsMaximumWidth, controlRect.bottom - controlRect.top);
	}
	control = dataPtr->controls.buttonContainer;
	SizeControl(control, buttonsMaximumWidth + INTEGER_DOUBLED(kOutsetButtonContainerFromButtonsH),
				listInitialHeight + INTEGER_DOUBLED(kOutsetButtonContainerFromButtonsV));
	control = dataPtr->controls.dataBrowser;
	SizeControl(control, interiorWidth - HSP_BUTTON_AND_CONTROL - buttonsMaximumWidth, listInitialHeight);
	
	// arrange controls
	topEdge = tabPaneTopLeftInset.v;
	leftEdge = tabPaneTopLeftInset.h;
	{
		UInt16		buttonTopEdge = kOutsetButtonContainerFromButtonsV;
		
		
		control = dataPtr->controls.newButton;
		GetControlBounds(control, &controlRect);
		MoveControl(control, kOutsetButtonContainerFromButtonsH, buttonTopEdge);
		buttonTopEdge += (controlRect.bottom - controlRect.top + VSP_BUTTONS);
		control = dataPtr->controls.newGroupButton;
		MoveControl(control, kOutsetButtonContainerFromButtonsH, buttonTopEdge);
		buttonTopEdge += (controlRect.bottom - controlRect.top + VSP_BUTTONS);
		control = dataPtr->controls.removeButton;
		MoveControl(control, kOutsetButtonContainerFromButtonsH, buttonTopEdge);
		buttonTopEdge += (controlRect.bottom - controlRect.top + VSP_BUTTONS);
		control = dataPtr->controls.duplicateButton;
		MoveControl(control, kOutsetButtonContainerFromButtonsH, buttonTopEdge);
		buttonTopEdge += (controlRect.bottom - controlRect.top + VSP_BUTTONS);
		control = dataPtr->controls.editButton;
		MoveControl(control, kOutsetButtonContainerFromButtonsH, buttonTopEdge);
		buttonTopEdge += (controlRect.bottom - controlRect.top + VSP_BUTTONS);
	}
	control = dataPtr->controls.buttonContainer;
	MoveControl(control, leftEdge + interiorWidth - buttonsMaximumWidth - kOutsetButtonContainerFromButtonsH,
				topEdge - kOutsetButtonContainerFromButtonsV);
	control = dataPtr->controls.dataBrowser;
	MoveControl(control, leftEdge, topEdge);
	topEdge += (listInitialHeight + VSP_CONTROLS);
	control = dataPtr->controls.descriptionStaticText;
	MoveControl(control, leftEdge, topEdge);
	topEdge = tabFrameTopLeft.v;
	leftEdge = tabFrameTopLeft.h;
	control = dataPtr->controls.tabs;
	MoveControl(control, leftEdge, topEdge);
	
	// now localize by potentially switching the positions of the buttons and the list
	Localization_HorizontallyPlaceControls(dataPtr->controls.dataBrowser, dataPtr->controls.buttonContainer);
	
	// initialize list contents
	SetControl32BitValue(dataPtr->controls.tabs, kConfigurationTabIndexSessions);
	setUpConfigurationsListControl(kPreferences_ClassSession, dataPtr);
}// setUpConfigurationsControls


/*!
Erases and rebuilds the specified list to
include all Favorites of the given class in
the preferences file.

(3.1)
*/
static void
setUpConfigurationsListControl		(Preferences_Class				inPreferencesClass,
									 ConfigurationsPanelDataPtr		inDataPtr)
{
return; // !!! TEMPORARY !!! (update to use DB APIs)
	ListHandle				list = nullptr;
	Preferences_Result		preferencesResult = kPreferences_ResultOK;
	Preferences_Class		preferencesClass = kPreferences_ClassSession;
	CFArrayRef				nameCFStringCFArray = nullptr;
	
	
	// first, erase the list
	LDelRow(0, 0, list);
	
	// determine the type of data being displayed
	preferencesClass = getCurrentPreferencesClass(inDataPtr);
	
	preferencesResult = Preferences_CreateContextNameArray(preferencesClass, nameCFStringCFArray);
	if (preferencesResult == kPreferences_ResultOK)
	{
		// now re-populate the list using resource information;
		// TEMPORARY: the data is converted to Pascal string for display,
		//            but eventually a CFString-savvy display will be used
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		SInt16 const		kListItemCount = CFArrayGetCount(nameCFStringCFArray);
		Cell				cell;
		CFStringRef			nameCFString;
		Str255				nameString;
		
		
		// put the Default item at the top
		LAddRow(1, -1, list);
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowDefaultFavoriteName, nameCFString);
		if (stringResult.ok())
		{
			CFStringGetPascalString(nameCFString, nameString, sizeof(nameString), kCFStringEncodingMacRoman);
			ListUtilities_SetListItemText(list, 0/* row */, nameString);
		}
		
		// now add any other items
		for (cell.v = 1, cell.h = 0; cell.v <= kListItemCount; ++(cell.v))
		{
			nameCFString = CFUtilities_StringCast(CFArrayGetValueAtIndex(nameCFStringCFArray, cell.v - 1));
			if (nullptr != nameCFString)
			{
				CFStringGetPascalString(nameCFString, nameString, sizeof(nameString), kCFStringEncodingMacRoman);
				LAddRow(1, -1, list);
				ListUtilities_SetListItemText(list, cell.v, nameString);
			}
		}
		cell.v = 0;
		cell.h = 0;
		if (kListItemCount) LSetSelect(true/* highlight */, cell, list);
		setButtonStates(inDataPtr);
		
		CFRelease(nameCFStringCFArray), nameCFStringCFArray = nullptr;
	}
	
	// update the list
	//Embedding_OffscreenDrawOneControl(inDataPtr->controls.listBox);
	RegionUtilities_RedrawControlOnNextUpdate(inDataPtr->controls.dataBrowser);
	
	// fix “New Session From Favorites” submenu
	// TEMPORARY: eventually, these will be handled by Preferences notifiers
	//MenuBar_RebuildNewSessionFromFavoritesMenu();
	//MenuBar_RebuildFormatFavoritesMenu();
}// setUpConfigurationsListControl

// BELOW IS REQUIRED NEWLINE TO END FILE
