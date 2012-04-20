/*!	\file PrefPanelSessions.cp
	\brief Implements the Sessions panel of Preferences.
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

#include "PrefPanelSessions.h"
#include <UniversalDefines.h>

// standard-C includes
#include <cstring>
#include <map>

// Unix includes
extern "C"
{
#	include <netdb.h>
}
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
#include "Keypads.h"
#include "NetEvents.h"
#include "Panel.h"
#include "Preferences.h"
#include "ServerBrowser.h"
#include "Session.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelSessions.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyFieldCommandLine			= { 'CmdL', 0/* ID */ };
HIViewID const	idMyButtonShellCommand			= { 'CmSh', 0/* ID */ };
HIViewID const	idMyButtonLogInShellCommand		= { 'CmLS', 0/* ID */ };
HIViewID const	idMyPopUpMenuCopySessionCommand	= { 'CpSs', 0/* ID */ };
HIViewID const	idMyButtonConnectToServer		= { 'CtoS', 0/* ID */ };
HIViewID const	idMySeparatorCommandRegion		= { 'ConH', 0/* ID */ };
HIViewID const	idMyPopUpMenuTerminal			= { 'Term', 0/* ID */ };
HIViewID const	idMyPopUpMenuFormat				= { 'Frmt', 0/* ID */ };
HIViewID const	idMyPopUpMenuTranslation		= { 'Xlat', 0/* ID */ };
HIViewID const	idMyHelpTextPresets				= { 'THlp', 0/* ID */ };
HIViewID const	idMyCheckBoxLocalEcho			= { 'Echo', 0/* ID */ };
HIViewID const	idMyFieldLineInsertionDelay		= { 'LIDl', 0/* ID */ };
HIViewID const	idMySliderScrollSpeed			= { 'SSpd', 0/* ID */ };
HIViewID const	idMyStaticTextCaptureFilePath	= { 'CapP', 0/* ID */ };
HIViewID const	idMyButtonNoCaptureFile			= { 'NoCF', 0/* ID */ };
HIViewID const	idMyButtonChooseCaptureFile		= { 'CapF', 0/* ID */ };
HIViewID const	idMyButtonChangeInterruptKey	= { 'Intr', 0/* ID */ };
HIViewID const	idMyButtonChangeSuspendKey		= { 'Susp', 0/* ID */ };
HIViewID const	idMyButtonChangeResumeKey		= { 'Resu', 0/* ID */ };
HIViewID const	idMyCheckBoxMapArrowsForEmacs	= { 'XEAr', 0/* ID */ };
HIViewID const	idMySegmentsEmacsMetaKey		= { 'Meta', 0/* ID */ };
HIViewID const	idMySegmentsDeleteKeyMapping	= { 'DMap', 0/* ID */ };
HIViewID const	idMyPopUpMenuNewLineMapping		= { 'NewL', 0/* ID */ };
HIViewID const	idMyHelpTextControlKeys			= { 'CtlH', 0/* ID */ };
HIViewID const	idMySeparatorKeyboardOptions	= { 'KSep', 0/* ID */ };
HIViewID const	idMyRadioButtonTEKDisabled		= { 'RTNo', 0/* ID */ };
HIViewID const	idMyRadioButtonTEK4014			= { '4014', 0/* ID */ };
HIViewID const	idMyRadioButtonTEK4105			= { '4105', 0/* ID */ };
HIViewID const	idMyCheckBoxTEKPageClearsScreen	= { 'XPCS', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

typedef std::map< UInt8, CFStringRef >	My_CharacterToCFStringMap;	//!< e.g. '\0' maps to CFSTR("^@")

/*!
Implements the “Data Flow” tab.
*/
struct My_SessionsPanelDataFlowUI
{
	My_SessionsPanelDataFlowUI	(Panel_Ref, HIWindowRef);
	~My_SessionsPanelDataFlowUI	();
	
	Panel_Ref			panel;				//!< the panel using this UI
	Float32				idealWidth;			//!< best size in pixels
	Float32				idealHeight;		//!< best size in pixels
	ControlActionUPP	sliderActionUPP;	//!< for live tracking
	HIViewWrap			mainView;
	
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
	setLineInsertionDelay	(EventTime);
	
	void
	setLocalEcho	(Boolean);
	
	void
	setScrollDelay	(EventTime);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	static void
	sliderProc	(HIViewRef, HIViewPartCode);

private:
	CommonEventHandlers_HIViewResizer	_containerResizer;
	CarbonEventHandlerWrap				_fieldLineDelayInputHandler;	//!< invoked when text is entered in the “line insertion delay” field
	CarbonEventHandlerWrap				_localEchoCommandHandler;		//!< invoked when the Local Echo checkbox is changed
};

/*!
Implements the “Vector Graphics” tab.
*/
struct My_SessionsPanelGraphicsUI
{
	My_SessionsPanelGraphicsUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	void
	setMode		(VectorInterpreter_Mode);
	
	void
	setPageClears	(Boolean);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	CommonEventHandlers_HIViewResizer	_containerResizer;
	CarbonEventHandlerWrap				_buttonCommandsHandler;		//!< invoked when a radio button is clicked
};

/*!
Implements the “Keyboard” tab.
*/
struct My_SessionsPanelKeyboardUI
{
	My_SessionsPanelKeyboardUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	OSStatus
	setButtonFromKey	(HIViewRef, char);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	CommonEventHandlers_HIViewResizer	_containerResizer;
	CarbonEventHandlerWrap				_button1CommandHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_button2CommandHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_button3CommandHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_emacsArrowsCommandHandler;	//!< invoked when this checkbox is clicked
	CarbonEventHandlerWrap				_emacsMetaCommandHandler;	//!< invoked when this segmented view is clicked
	CarbonEventHandlerWrap				_deleteMapCommandHandler;	//!< invoked when this segmented view is clicked
	CarbonEventHandlerWrap				_newLineMapCommandHandler;	//!< invoked when this menu is clicked
};

/*!
Implements the “Resource” tab.
*/
struct My_SessionsPanelResourceUI
{
	My_SessionsPanelResourceUI	(Panel_Ref, HIWindowRef);
	~My_SessionsPanelResourceUI	();
	
	Panel_Ref			panel;				//!< the panel using this UI
	Float32				idealWidth;			//!< best size in pixels
	Float32				idealHeight;		//!< best size in pixels
	HIViewWrap			mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readCommandLinePreferenceFromSession	(Preferences_ContextRef);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	UInt16
	readPreferencesForRemoteServers		(Preferences_ContextRef, Boolean, Session_Protocol&,
										 CFStringRef&, UInt16&, CFStringRef&);
	
	void
	rebuildFavoritesMenu	(HIViewID const&, UInt32, Quills::Prefs::Class,
							 UInt32, MenuItemIndex&);
	
	void
	rebuildFormatMenu ();
	
	void
	rebuildSessionMenu ();
	
	void
	rebuildTerminalMenu ();
	
	void
	rebuildTranslationMenu ();
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	void
	saveFieldPreferences	(Preferences_ContextRef);
	
	UInt16
	savePreferencesForRemoteServers		(Preferences_ContextRef, Session_Protocol,
										 CFStringRef, UInt16, CFStringRef);
	
	void
	serverBrowserDisplay	(Session_Protocol, CFStringRef, UInt16, CFStringRef);
	
	void
	serverBrowserRemove ();
	
	void
	setAssociatedFormat		(CFStringRef);
	
	void
	setAssociatedTerminal	(CFStringRef);
	
	void
	setAssociatedTranslation	(CFStringRef);
	
	void
	setCommandLine	(CFStringRef);
	
	Boolean
	setCommandLineFromArray		(CFArrayRef);
	
	Boolean
	updateCommandLine	(Session_Protocol, CFStringRef, UInt16, CFStringRef);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	MenuItemIndex						_numberOfFormatItemsAdded;		//!< used to manage Format pop-up menu
	MenuItemIndex						_numberOfSessionItemsAdded;		//!< used to manage Copy From Session Preferences pop-up menu
	MenuItemIndex						_numberOfTerminalItemsAdded;	//!< used to manage Terminal pop-up menu
	MenuItemIndex						_numberOfTranslationItemsAdded;	//!< used to manage Translation pop-up menu
	ServerBrowser_Ref					_serverBrowser;					//!< if requested, interface for remote servers
	HIViewWrap							_fieldCommandLine;				//!< text of Unix command line to run
	CommonEventHandlers_HIViewResizer	_containerResizer;
	CarbonEventHandlerWrap				_buttonCommandsHandler;			//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_whenCommandLineChangedHandler;	//!< invoked when the command line field changes
	CarbonEventHandlerWrap				_whenServerPanelChangedHandler;	//!< invoked when the server browser panel changes
	ListenerModel_ListenerWrap			_whenFavoritesChangedHandler;	//!< used to manage Terminal and Format pop-up menus
};

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelDataFlowData
{
	My_SessionsPanelDataFlowData	();
	~My_SessionsPanelDataFlowData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelDataFlowUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelDataFlowData*		My_SessionsPanelDataFlowDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelGraphicsData
{
	My_SessionsPanelGraphicsData	();
	~My_SessionsPanelGraphicsData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelGraphicsUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelGraphicsData*		My_SessionsPanelGraphicsDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelKeyboardData
{
	My_SessionsPanelKeyboardData	();
	~My_SessionsPanelKeyboardData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelKeyboardUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelKeyboardData*		My_SessionsPanelKeyboardDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelResourceData
{
	My_SessionsPanelResourceData	();
	~My_SessionsPanelResourceData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelResourceUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelResourceData*		My_SessionsPanelResourceDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

My_CharacterToCFStringMap&	initCharacterToCFStringMap		();
void						makeAllBevelButtonsUseTheSystemFont		(HIWindowRef);
void						preferenceChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
OSStatus					receiveFieldChangedInCommandLine		(EventHandlerCallRef, EventRef, void*);
OSStatus					receiveServerBrowserEvent				(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_CharacterToCFStringMap&		gCharacterToCFStringMap ()	{ return initCharacterToCFStringMap(); }
}



#pragma mark Public Methods

/*!
Creates a new preference panel for the Sessions category.
Destroy it using Panel_Dispose().

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelSessions_New ()
{
	Panel_Ref				result = nullptr;
	CFStringRef				nameCFString = nullptr;
	GenericPanelTabs_List	tabList;
	
	
	tabList.push_back(PrefPanelSessions_NewResourcePane());
	tabList.push_back(PrefPanelSessions_NewDataFlowPane());
	tabList.push_back(PrefPanelSessions_NewKeyboardPane());
	tabList.push_back(PrefPanelSessions_NewGraphicsPane());
	
	if (UIStrings_Copy(kUIStrings_PrefPanelSessionsCategoryName, nameCFString).ok())
	{
		result = GenericPanelTabs_New(nameCFString, kConstantsRegistry_PrefPanelDescriptorSessions, tabList);
		if (nullptr != result)
		{
			Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessions);
			Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
											AppResources_ReturnCreatorCode(),
											kConstantsRegistry_IconServicesIconPrefPanelSessions);
		}
		CFRelease(nameCFString), nameCFString = nullptr;
	}
	
	return result;
}// New


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewDataFlowPane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelDataFlowUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelDataFlowDataPtr		dataPtr = new My_SessionsPanelDataFlowData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionDataFlow);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsDataFlow);
		if (UIStrings_Copy(kUIStrings_PrefPanelSessionsDataFlowTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewDataFlowPane


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewGraphicsPane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelGraphicsUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelGraphicsDataPtr		dataPtr = new My_SessionsPanelGraphicsData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionGraphics);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsGraphics);
		if (UIStrings_Copy(kUIStrings_PrefPanelSessionsGraphicsTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewGraphicsPane


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewKeyboardPane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelKeyboardUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelKeyboardDataPtr		dataPtr = new My_SessionsPanelKeyboardData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionKeyboard);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsKeyboard);
		if (UIStrings_Copy(kUIStrings_PrefPanelSessionsKeyboardTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewKeyboardPane


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewResourcePane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelResourceUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelResourceDataPtr		dataPtr = new My_SessionsPanelResourceData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionResource);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsResource);
		if (UIStrings_Copy(kUIStrings_PrefPanelSessionsResourceTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewResourcePane


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_SessionsPanelDataFlowData structure.

(3.1)
*/
My_SessionsPanelDataFlowData::
My_SessionsPanelDataFlowData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelDataFlowData default constructor


/*!
Tears down a My_SessionsPanelDataFlowData structure.

(3.1)
*/
My_SessionsPanelDataFlowData::
~My_SessionsPanelDataFlowData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelDataFlowData destructor


/*!
Initializes a My_SessionsPanelDataFlowUI structure.

(3.1)
*/
My_SessionsPanelDataFlowUI::
My_SessionsPanelDataFlowUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
sliderActionUPP			(NewControlActionUPP(sliderProc)),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_SessionsPanelDataFlowUI::deltaSize, this/* context */),
_fieldLineDelayInputHandler(GetControlEventTarget(HIViewWrap(idMyFieldLineInsertionDelay, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_localEchoCommandHandler(GetControlEventTarget(HIViewWrap(idMyCheckBoxLocalEcho, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
	assert(_fieldLineDelayInputHandler.isInstalled());
	assert(_localEchoCommandHandler.isInstalled());
}// My_SessionsPanelDataFlowUI 2-argument constructor


/*!
Tears down a My_SessionsPanelDataFlowUI structure.

(4.0)
*/
My_SessionsPanelDataFlowUI::
~My_SessionsPanelDataFlowUI ()
{
	DisposeControlActionUPP(sliderActionUPP), sliderActionUPP = nullptr;
}// My_SessionsPanelDataFlowUI destructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelDataFlowUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("DataFlow"), inOwningWindow,
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
	
	// permit only numbers in the line insertion delay field
	{
		HIViewWrap		delayField(idMyFieldLineInsertionDelay, inOwningWindow);
		
		
		delayField << HIViewWrap_InstallKeyFilter(NumericalLimiterKeyFilterUPP());
	}
	
	// install live tracking on the slider
	{
		HIViewWrap		slider(idMySliderScrollSpeed, inOwningWindow);
		
		
		error = SetControlProperty(slider, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeOwningPanel,
									sizeof(inPanel), &inPanel);
		if (noErr == error)
		{
			assert(nullptr != this->sliderActionUPP);
			SetControlAction(HIViewWrap(idMySliderScrollSpeed, inOwningWindow), this->sliderActionUPP);
		}
	}
	
	return result;
}// My_SessionsPanelDataFlowUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsPanelDataFlowUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_SessionsTabDataFlow*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabDataFlow*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyStaticTextCaptureFilePath, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyButtonNoCaptureFile, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyButtonChooseCaptureFile, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
}// My_SessionsPanelDataFlowUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelDataFlowUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionDataFlow, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelDataFlowUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(inDataPtr, My_SessionsPanelDataFlowDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
			delete panelDataPtr;
		}
		break;
	
	case kPanel_MessageFocusFirst: // notification that the first logical view should become focused
		{
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
			HIWindowRef							owningWindow = HIViewGetWindow(panelDataPtr->interfacePtr->mainView);
			HIViewWrap							delayField(idMyFieldLineInsertionDelay, owningWindow);
			
			
			DialogUtilities_SetKeyboardFocus(delayField);
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
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
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
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext)
			{
				panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
				Preferences_ContextSave(oldContext);
			}
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
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
}// My_SessionsPanelDataFlowUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelDataFlowUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// INCOMPLETE
		
		// set duplication
		{
			Boolean		localEcho = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagLocalEchoEnabled,
														sizeof(localEcho), &localEcho, true/* search defaults too */,
														&actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setLocalEcho(localEcho);
			}
		}
		
		// set line insertion delay
		{
			EventTime	asEventTime = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagPasteNewLineDelay,
														sizeof(asEventTime), &asEventTime, true/* search defaults too */,
														&actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setLineInsertionDelay(asEventTime);
			}
		}
		
		// set scroll speed delay
		{	
			EventTime	asEventTime = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagScrollDelay,
														sizeof(asEventTime), &asEventTime, true/* search defaults too */,
														&actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setScrollDelay(asEventTime);
			}
		}
	}
}// My_SessionsPanelDataFlowUI::readPreferences


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the fields in this panel by saving
their preferences when new text arrives.

(4.0)
*/
OSStatus
My_SessionsPanelDataFlowUI::
receiveFieldChanged		(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelDataFlowUI*		interfacePtr = REINTERPRET_CAST(inMyTerminalsPanelUIPtr, My_SessionsPanelDataFlowUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with preferences
	{
		My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																			My_SessionsPanelDataFlowDataPtr);
		
		
		interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
	}
	
	return result;
}// My_SessionsPanelDataFlowUI::receiveFieldChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the Data Flow tab.

(4.0)
*/
OSStatus
My_SessionsPanelDataFlowUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMySessionsUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelDataFlowUI*		dataFlowInterfacePtr = REINTERPRET_CAST
															(inMySessionsUIPtr, My_SessionsPanelDataFlowUI*);
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
			switch (received.commandID)
			{
			case kCommandEcho:
				{
					HIViewWrap							checkBox(idMyCheckBoxLocalEcho, HIViewGetWindow(dataFlowInterfacePtr->mainView));
					My_SessionsPanelDataFlowDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(dataFlowInterfacePtr->panel),
																					My_SessionsPanelDataFlowDataPtr);
					Boolean								flag = (GetControl32BitValue(checkBox) == kControlCheckBoxCheckedValue);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagLocalEchoEnabled,
																sizeof(flag), &flag);
					assert(kPreferences_ResultOK == prefsResult);
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
}// My_SessionsPanelDataFlowUI::receiveHICommand


/*!
Saves every setting to the data model that may change outside
of easily-detectable means (e.g. not buttons, but things like
text fields and sliders).

(4.0)
*/
void
My_SessionsPanelDataFlowUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings)
{
	if (nullptr != inoutSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set line insertion delay
		{
			HIViewWrap	delayField(idMyFieldLineInsertionDelay, kOwningWindow);
			SInt32		lineInsertionDelay = 0;
			size_t		stringLength = 0;
			Boolean		saveFailed = true;
			
			
			GetControlNumericalText(delayField, &lineInsertionDelay, &stringLength);
			if ((stringLength > 0) && (lineInsertionDelay >= 0))
			{
				// convert into seconds
				EventTime	asEventTime = STATIC_CAST(lineInsertionDelay, EventTime) * kEventDurationMillisecond;
				
				
				prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagPasteNewLineDelay,
															sizeof(asEventTime), &asEventTime);
				if (kPreferences_ResultOK == prefsResult)
				{
					saveFailed = false;
				}
			}
			
			if (saveFailed)
			{
				Console_Warning(Console_WriteLine, "failed to set line insertion delay");
				SetControlTextWithCFString(delayField, CFSTR(""));
			}
		}
		
		// set scroll delay
		{
			SInt32		delayDiscreteValue = GetControl32BitValue(HIViewWrap(idMySliderScrollSpeed, kOwningWindow));
			EventTime	asEventTime = 0.0; // in seconds
			
			
			// warning, this MUST be consistent with the NIB and setScrollDelay()
			if (1 == delayDiscreteValue) asEventTime = 0.040;
			else if (2 == delayDiscreteValue) asEventTime = 0.030;
			else if (3 == delayDiscreteValue) asEventTime = 0.020;
			else if (4 == delayDiscreteValue) asEventTime = 0.010;
			else asEventTime = 0.0;
			
			prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagScrollDelay,
														sizeof(asEventTime), &asEventTime);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "failed to set scroll delay value");
			}
		}
	}
}// My_SessionsPanelDataFlowUI::saveFieldPreferences


/*!
Updates the line insertion delay display with the given value.
(This is a visual adornment only, it does not change any stored
preference.)

(4.0)
*/
void
My_SessionsPanelDataFlowUI::
setLineInsertionDelay	(EventTime		inTime)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	inTime /= kEventDurationMillisecond; // convert to milliseconds
	SetControlNumericalText(HIViewWrap(idMyFieldLineInsertionDelay, kOwningWindow), STATIC_CAST(inTime, SInt32));
}// My_SessionsPanelDataFlowUI::setLineInsertionDelay


/*!
Changes the Local Echo menu selection.  (This is a visual
adornment only, it does not change any stored preference.)

(4.0)
*/
void
My_SessionsPanelDataFlowUI::
setLocalEcho	(Boolean	inLocalEcho)
{
	HIViewWrap	checkBox(idMyCheckBoxLocalEcho, HIViewGetWindow(this->mainView));
	
	
	SetControlValue(checkBox, BooleanToCheckBoxValue(inLocalEcho));
}// My_SessionsPanelDataFlowUI::setLocalEcho


/*!
Updates the scroll speed display with the given value.  (This
is a visual adornment only, it does not change any stored
preference.)

If the value does not exactly match a slider tick, the closest
matching value will be chosen.

(4.0)
*/
void
My_SessionsPanelDataFlowUI::
setScrollDelay		(EventTime		inTime)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	EventTime const		kTolerance = 0.005;
	
	
	// floating point numbers are not exact values, they technically are
	// discrete at a very fine granularity; so do not use pure equality,
	// use a range with a small tolerance to choose a matching number
	if (inTime > (0.030/* warning: must be consistent with NIB label and tick marks */ + kTolerance))
	{
		SetControl32BitValue(HIViewWrap(idMySliderScrollSpeed, kOwningWindow), 1/* first slider value, slowest (0.040) */);
	}
	else if ((inTime <= (0.030/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inTime > (0.020/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderScrollSpeed, kOwningWindow), 2/* next slider value */);
	}
	else if ((inTime <= (0.020/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inTime > (0.010/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderScrollSpeed, kOwningWindow), 3/* middle slider value */);
	}
	else if ((inTime <= (0.010/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inTime > (0.0/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderScrollSpeed, kOwningWindow), 4/* next slider value */);
	}
	else if (inTime <= (0.0/* warning: must be consistent with NIB label */ + kTolerance))
	{
		SetControl32BitValue(HIViewWrap(idMySliderScrollSpeed, kOwningWindow), 5/* last slider value, fastest (zero) */);
	}
	else
	{
		assert(false && "scroll delay value should have triggered one of the preceding if/else cases");
	}
}// My_SessionsPanelDataFlowUI::setScrollDelay


/*!
A standard control action routine for the slider; responds by
immediately saving the new value in the preferences context.

(4.0)
*/
void
My_SessionsPanelDataFlowUI::
sliderProc	(HIViewRef			inSlider,
			 HIViewPartCode		UNUSED_ARGUMENT(inSliderPart))
{
	Panel_Ref		panel = nullptr;
	UInt32			actualSize = 0;
	OSStatus		error = GetControlProperty(inSlider, AppResources_ReturnCreatorCode(),
												kConstantsRegistry_ControlPropertyTypeOwningPanel,
												sizeof(panel), &actualSize, &panel);
	
	
	if (noErr == error)
	{
		// now synchronize the change with preferences
		My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(panel),
																			My_SessionsPanelDataFlowDataPtr);
		
		
		panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
	}
}// My_SessionsPanelDataFlowUI::sliderProc


/*!
Initializes a My_SessionsPanelGraphicsData structure.

(3.1)
*/
My_SessionsPanelGraphicsData::
My_SessionsPanelGraphicsData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelGraphicsData default constructor


/*!
Tears down a My_SessionsPanelGraphicsData structure.

(3.1)
*/
My_SessionsPanelGraphicsData::
~My_SessionsPanelGraphicsData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelGraphicsData destructor


/*!
Initializes a My_SessionsPanelGraphicsUI structure.

(3.1)
*/
My_SessionsPanelGraphicsUI::
My_SessionsPanelGraphicsUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_SessionsPanelGraphicsUI::deltaSize, this/* context */),
_buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_SessionsPanelGraphicsUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelGraphicsUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("TEK"), inOwningWindow,
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
}// My_SessionsPanelGraphicsUI::createContainerView


/*!
Resizes the views in this panel.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_SessionsPanelGraphicsUI*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsPanelGraphicsUI*);
	
	
	// UNIMPLEMENTED
}// My_SessionsPanelGraphicsUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelGraphicsUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionGraphics, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelGraphicsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelGraphicsDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelGraphicsUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_SessionsPanelGraphicsDataPtr));
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
			My_SessionsPanelGraphicsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelGraphicsDataPtr);
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
			My_SessionsPanelGraphicsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelGraphicsDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
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
}// My_SessionsPanelGraphicsUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// set TEK mode
		{
			VectorInterpreter_Mode		graphicsMode = kVectorInterpreter_ModeDisabled;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTektronixMode, sizeof(graphicsMode),
														&graphicsMode, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setMode(graphicsMode);
			}
		}
		
		// set TEK PAGE checkbox
		{
			Boolean		pageClearsScreen = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTektronixPAGEClearsScreen, sizeof(pageClearsScreen),
														&pageClearsScreen, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setPageClears(pageClearsScreen);
			}
		}
	}
}// My_SessionsPanelGraphicsUI::readPreferences


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the radio buttons in the Graphics tab.

(3.1)
*/
OSStatus
My_SessionsPanelGraphicsUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMySessionsUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelGraphicsUI*		graphicsInterfacePtr = REINTERPRET_CAST
															(inMySessionsUIPtr, My_SessionsPanelGraphicsUI*);
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
			switch (received.commandID)
			{
			case kCommandSetTEKModeDisabled:
			case kCommandSetTEKModeTEK4014:
			case kCommandSetTEKModeTEK4105:
				{
					VectorInterpreter_Mode				newMode = kVectorInterpreter_ModeDisabled;
					My_SessionsPanelGraphicsDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(graphicsInterfacePtr->panel),
																					My_SessionsPanelGraphicsDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					if (kCommandSetTEKModeTEK4014 == received.commandID) newMode = kVectorInterpreter_ModeTEK4014;
					if (kCommandSetTEKModeTEK4105 == received.commandID) newMode = kVectorInterpreter_ModeTEK4105;
					graphicsInterfacePtr->setMode(newMode);
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTektronixMode,
																sizeof(newMode), &newMode);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to save TEK mode setting");
					}
					result = noErr;
				}
				break;
			
			case kCommandSetTEKPageClearsScreen:
				assert(received.attributes & kHICommandFromControl);
				{
					HIViewRef const						kCheckBox = received.source.control;
					Boolean const						kIsSet = (kControlCheckBoxCheckedValue == GetControl32BitValue(kCheckBox));
					My_SessionsPanelGraphicsDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(graphicsInterfacePtr->panel),
																					My_SessionsPanelGraphicsDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					graphicsInterfacePtr->setPageClears(kIsSet);
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTektronixPAGEClearsScreen,
																sizeof(kIsSet), &kIsSet);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to save TEK PAGE setting");
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
}// My_SessionsPanelGraphicsUI::receiveHICommand


/*!
Updates the TEK clear checkbox based on the given setting.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
setPageClears	(Boolean	inTEKPageClearsScreen)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxTEKPageClearsScreen, kOwningWindow),
							BooleanToCheckBoxValue(inTEKPageClearsScreen));
}// My_SessionsPanelGraphicsUI::setPageClears


/*!
Updates the TEK graphics mode display based on the
given setting.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
setMode		(VectorInterpreter_Mode		inMode)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyRadioButtonTEKDisabled, kOwningWindow),
							BooleanToRadioButtonValue(kVectorInterpreter_ModeDisabled == inMode));
	SetControl32BitValue(HIViewWrap(idMyRadioButtonTEK4014, kOwningWindow),
							BooleanToRadioButtonValue(kVectorInterpreter_ModeTEK4014 == inMode));
	SetControl32BitValue(HIViewWrap(idMyRadioButtonTEK4105, kOwningWindow),
							BooleanToRadioButtonValue(kVectorInterpreter_ModeTEK4105 == inMode));
}// My_SessionsPanelGraphicsUI::setMode


/*!
Initializes a My_SessionsPanelKeyboardData structure.

(3.1)
*/
My_SessionsPanelKeyboardData::
My_SessionsPanelKeyboardData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelKeyboardData default constructor


/*!
Tears down a My_SessionsPanelKeyboardData structure.

(3.1)
*/
My_SessionsPanelKeyboardData::
~My_SessionsPanelKeyboardData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelKeyboardData destructor


/*!
Initializes a My_SessionsPanelKeyboardUI structure.

(3.1)
*/
My_SessionsPanelKeyboardUI::
My_SessionsPanelKeyboardUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_SessionsPanelKeyboardUI::deltaSize, this/* context */),
_button1CommandHandler	(GetControlEventTarget(HIViewWrap(idMyButtonChangeInterruptKey, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_button2CommandHandler	(GetControlEventTarget(HIViewWrap(idMyButtonChangeSuspendKey, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_button3CommandHandler	(GetControlEventTarget(HIViewWrap(idMyButtonChangeResumeKey, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_emacsArrowsCommandHandler(GetControlEventTarget(HIViewWrap(idMyCheckBoxMapArrowsForEmacs, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_emacsMetaCommandHandler(GetControlEventTarget(HIViewWrap(idMySegmentsEmacsMetaKey, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_deleteMapCommandHandler(GetControlEventTarget(HIViewWrap(idMySegmentsDeleteKeyMapping, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_newLineMapCommandHandler(GetControlEventTarget(HIViewWrap(idMyPopUpMenuNewLineMapping, inOwningWindow)), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_SessionsPanelKeyboardUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelKeyboardUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("Keyboard"), inOwningWindow,
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
	
	// make all bevel buttons use a larger font than is provided by default in a NIB
	makeAllBevelButtonsUseTheSystemFont(inOwningWindow);
	
	// make the main bevel buttons use a bold font
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyButtonChangeInterruptKey, inOwningWindow), kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyButtonChangeSuspendKey, inOwningWindow), kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyButtonChangeResumeKey, inOwningWindow), kThemeAlertHeaderFont);
	
	// initialize values
	// UNIMPLEMENTED
	
	return result;
}// My_SessionsPanelKeyboardUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsPanelKeyboardUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_SessionsTabControlKeys*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabControlKeys*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextControlKeys, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMySeparatorKeyboardOptions, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_SessionsPanelKeyboardUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelKeyboardUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionKeyboard, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelKeyboardDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelKeyboardDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelKeyboardUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_SessionsPanelKeyboardDataPtr));
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
			My_SessionsPanelKeyboardDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelKeyboardDataPtr);
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
			My_SessionsPanelKeyboardDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelKeyboardDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
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
}// My_SessionsPanelKeyboardUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelKeyboardUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef				window = HIViewGetWindow(this->mainView);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		OSStatus				error = noErr;
		size_t					actualSize = 0;
		
		
		// set interrupt key
		{
			char	keyCode = '\0';
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagKeyInterruptProcess,
														sizeof(keyCode), &keyCode,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		button(idMyButtonChangeInterruptKey, window);
				
				
				error = this->setButtonFromKey(button, keyCode);
				assert_noerr(error);
			}
		}
		
		// set suspend key
		{
			char	keyCode = '\0';
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagKeySuspendOutput,
														sizeof(keyCode), &keyCode,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		button(idMyButtonChangeSuspendKey, window);
				
				
				error = this->setButtonFromKey(button, keyCode);
				assert_noerr(error);
			}
		}
		
		// set resume key
		{
			char	keyCode = '\0';
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagKeyResumeOutput,
														sizeof(keyCode), &keyCode,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		button(idMyButtonChangeResumeKey, window);
				
				
				error = this->setButtonFromKey(button, keyCode);
				assert_noerr(error);
			}
		}
		
		// set Emacs arrows checkbox
		{
			Boolean		mapArrows = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagMapArrowsForEmacs,
														sizeof(mapArrows), &mapArrows,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		checkBox(idMyCheckBoxMapArrowsForEmacs, window);
				
				
				SetControl32BitValue(checkBox, BooleanToCheckBoxValue(mapArrows));
			}
		}
		
		// set meta key
		{
			Session_EmacsMetaKey	metaKey = kSession_EmacsMetaKeyOff;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagEmacsMetaKey,
														sizeof(metaKey), &metaKey,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		segmentedView(idMySegmentsEmacsMetaKey, window);
				UInt32			commandID = kCommandSetMetaNone;
				
				
				switch (metaKey)
				{
				case kSession_EmacsMetaKeyShiftOption:
					commandID = kCommandSetMetaShiftAndOptionKeys;
					break;
				
				case kSession_EmacsMetaKeyOption:
					commandID = kCommandSetMetaOptionKey;
					break;
				
				case kSession_EmacsMetaKeyOff:
				default:
					commandID = kCommandSetMetaNone;
					break;
				}
				(OSStatus)DialogUtilities_SetSegmentByCommand(segmentedView, commandID);
			}
		}
		
		// set delete key mapping
		{
			Boolean		deleteUsesBackspace = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagMapDeleteToBackspace,
														sizeof(deleteUsesBackspace), &deleteUsesBackspace,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		segmentedView(idMySegmentsDeleteKeyMapping, window);
				
				
				(OSStatus)DialogUtilities_SetSegmentByCommand(segmentedView,
																(deleteUsesBackspace)
																? kCommandDeletePressSendsBackspace
																: kCommandDeletePressSendsDelete);
			}
		}
		
		// set new-line mapping
		{
			Session_NewlineMode		newLineMapping = kSession_NewlineModeMapLF;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagNewLineMapping,
														sizeof(newLineMapping), &newLineMapping,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				HIViewWrap		popUpMenu(idMyPopUpMenuNewLineMapping, window);
				UInt32			commandID = kCommandSetNewlineCarriageReturnLineFeed;
				
				
				switch (newLineMapping)
				{
				case kSession_NewlineModeMapCR:
					commandID = kCommandSetNewlineCarriageReturnOnly;
					break;
				
				case kSession_NewlineModeMapCRLF:
					commandID = kCommandSetNewlineCarriageReturnLineFeed;
					break;
				
				case kSession_NewlineModeMapCRNull:
					commandID = kCommandSetNewlineCarriageReturnNull;
					break;
				
				case kSession_NewlineModeMapLF:
				default:
					commandID = kCommandSetNewlineLineFeedOnly;
					break;
				}
				
				(OSStatus)DialogUtilities_SetPopUpItemByCommand(popUpMenu, commandID);
			}
		}
	}
}// My_SessionsPanelKeyboardUI::readPreferences


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the Keyboard tab.

(3.1)
*/
OSStatus
My_SessionsPanelKeyboardUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMySessionsUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelKeyboardUI*		keyboardInterfacePtr = REINTERPRET_CAST
															(inMySessionsUIPtr, My_SessionsPanelKeyboardUI*);
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
			enum
			{
				kInitialChosenCharValue		= 'X'
			};
			HIWindowRef							window = HIViewGetWindow(keyboardInterfacePtr->mainView);
			My_SessionsPanelKeyboardDataPtr		dataPtr = REINTERPRET_CAST
															(Panel_ReturnImplementation(keyboardInterfacePtr->panel),
																My_SessionsPanelKeyboardDataPtr);
			char								chosenChar = kInitialChosenCharValue;
			
			
			switch (received.commandID)
			{
			case kCommandEditInterruptKey:
			case kCommandEditSuspendKey:
			case kCommandEditResumeKey:
				{
					HIViewWrap		buttonSetInterruptKey(idMyButtonChangeInterruptKey, window);
					HIViewWrap		buttonSetSuspendKey(idMyButtonChangeSuspendKey, window);
					HIViewWrap		buttonSetResumeKey(idMyButtonChangeResumeKey, window);
					
					
					// show the control keys palette and target the button
					switch (received.commandID)
					{
					case kCommandEditInterruptKey:
						Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetControlEventTarget(buttonSetInterruptKey));
						break;
					
					case kCommandEditSuspendKey:
						Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetControlEventTarget(buttonSetSuspendKey));
						break;
					
					case kCommandEditResumeKey:
						Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetControlEventTarget(buttonSetResumeKey));
						break;
					
					default:
						// ???
						break;
					}
					
					// change the active button
					SetControl32BitValue(buttonSetInterruptKey,
											(kCommandEditInterruptKey == received.commandID)
											? kControlCheckBoxCheckedValue
											: kControlCheckBoxUncheckedValue);
					SetControl32BitValue(buttonSetSuspendKey,
											(kCommandEditSuspendKey == received.commandID)
											? kControlCheckBoxCheckedValue
											: kControlCheckBoxUncheckedValue);
					SetControl32BitValue(buttonSetResumeKey,
											(kCommandEditResumeKey == received.commandID)
											? kControlCheckBoxCheckedValue
											: kControlCheckBoxUncheckedValue);
				}
				break;
			
			case kCommandKeypadControlAtSign:
				chosenChar = '@' - '@';
				break;
			
			case kCommandKeypadControlA:
				chosenChar = 'A' - '@';
				break;
			
			case kCommandKeypadControlB:
				chosenChar = 'B' - '@';
				break;
			
			case kCommandKeypadControlC:
				chosenChar = 'C' - '@';
				break;
			
			case kCommandKeypadControlD:
				chosenChar = 'D' - '@';
				break;
			
			case kCommandKeypadControlE:
				chosenChar = 'E' - '@';
				break;
			
			case kCommandKeypadControlF:
				chosenChar = 'F' - '@';
				break;
			
			case kCommandKeypadControlG:
				chosenChar = 'G' - '@';
				break;
			
			case kCommandKeypadControlH:
				chosenChar = 'H' - '@';
				break;
			
			case kCommandKeypadControlI:
				chosenChar = 'I' - '@';
				break;
			
			case kCommandKeypadControlJ:
				chosenChar = 'J' - '@';
				break;
			
			case kCommandKeypadControlK:
				chosenChar = 'K' - '@';
				break;
			
			case kCommandKeypadControlL:
				chosenChar = 'L' - '@';
				break;
			
			case kCommandKeypadControlM:
				chosenChar = 'M' - '@';
				break;
			
			case kCommandKeypadControlN:
				chosenChar = 'N' - '@';
				break;
			
			case kCommandKeypadControlO:
				chosenChar = 'O' - '@';
				break;
			
			case kCommandKeypadControlP:
				chosenChar = 'P' - '@';
				break;
			
			case kCommandKeypadControlQ:
				chosenChar = 'Q' - '@';
				break;
			
			case kCommandKeypadControlR:
				chosenChar = 'R' - '@';
				break;
			
			case kCommandKeypadControlS:
				chosenChar = 'S' - '@';
				break;
			
			case kCommandKeypadControlT:
				chosenChar = 'T' - '@';
				break;
			
			case kCommandKeypadControlU:
				chosenChar = 'U' - '@';
				break;
			
			case kCommandKeypadControlV:
				chosenChar = 'V' - '@';
				break;
			
			case kCommandKeypadControlW:
				chosenChar = 'W' - '@';
				break;
			
			case kCommandKeypadControlX:
				chosenChar = 'X' - '@';
				break;
			
			case kCommandKeypadControlY:
				chosenChar = 'Y' - '@';
				break;
			
			case kCommandKeypadControlZ:
				chosenChar = 'Z' - '@';
				break;
			
			case kCommandKeypadControlLeftSquareBracket:
				chosenChar = '[' - '@';
				break;
			
			case kCommandKeypadControlBackslash:
				chosenChar = '\\' - '@';
				break;
			
			case kCommandKeypadControlRightSquareBracket:
				chosenChar = ']' - '@';
				break;
			
			case kCommandKeypadControlTilde:
				chosenChar = '~' - '@';
				break;
			
			case kCommandKeypadControlQuestionMark:
				chosenChar = '\?' - '@';
				break;
			
			case kCommandEmacsArrowMapping:
				{
					HIViewWrap				checkBox(idMyCheckBoxMapArrowsForEmacs, window);
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					Boolean					flag = (GetControl32BitValue(checkBox) == kControlCheckBoxCheckedValue);
					
					
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagMapArrowsForEmacs,
																sizeof(flag), &flag);
					assert(kPreferences_ResultOK == prefsResult);
				}
				break;
			
			case kCommandSetMetaNone:
			case kCommandSetMetaOptionKey:
			case kCommandSetMetaShiftAndOptionKeys:
				{
					Session_EmacsMetaKey	metaKey = kSession_EmacsMetaKeyOff;
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					
					
					switch (received.commandID)
					{
					case kCommandSetMetaShiftAndOptionKeys:
						metaKey = kSession_EmacsMetaKeyShiftOption;
						break;
					
					case kCommandSetMetaOptionKey:
						metaKey = kSession_EmacsMetaKeyOption;
						break;
					
					case kCommandSetMetaNone:
					default:
						metaKey = kSession_EmacsMetaKeyOff;
						break;
					}
					
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagEmacsMetaKey,
																sizeof(metaKey), &metaKey);
					assert(kPreferences_ResultOK == prefsResult);
				}
				break;
			
			case kCommandDeletePressSendsBackspace:
			case kCommandDeletePressSendsDelete:
				{
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					Boolean					flag = (kCommandDeletePressSendsBackspace == received.commandID);
					
					
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel,
																kPreferences_TagMapDeleteToBackspace,
																sizeof(flag), &flag);
					assert(kPreferences_ResultOK == prefsResult);
				}
				break;
			
			case kCommandSetNewlineCarriageReturnLineFeed:
			case kCommandSetNewlineCarriageReturnNull:
			case kCommandSetNewlineCarriageReturnOnly:
			case kCommandSetNewlineLineFeedOnly:
				{
					Preferences_Result		prefsResult = kPreferences_ResultOK;
					Session_NewlineMode		newLineMapping = kSession_NewlineModeMapLF;
					
					
					switch (received.commandID)
					{
					case kCommandSetNewlineCarriageReturnOnly:
						newLineMapping = kSession_NewlineModeMapCR;
						break;
					
					case kCommandSetNewlineCarriageReturnLineFeed:
						newLineMapping = kSession_NewlineModeMapCRLF;
						break;
					
					case kCommandSetNewlineCarriageReturnNull:
						newLineMapping = kSession_NewlineModeMapCRNull;
						break;
					
					case kCommandSetNewlineLineFeedOnly:
					default:
						newLineMapping = kSession_NewlineModeMapLF;
						break;
					}
					
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel,
																kPreferences_TagNewLineMapping,
																sizeof(newLineMapping), &newLineMapping);
					assert(kPreferences_ResultOK == prefsResult);
					
					// update the pop-up button
					result = eventNotHandledErr;
				}
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
			
			// if the chosen character is not the default value, a control key in
			// the palette was clicked; use it to change the Interrupt, Suspend or
			// Resume key (whichever is active)
			if (chosenChar != kInitialChosenCharValue)
			{
				HIViewWrap			buttonSetInterruptKey(idMyButtonChangeInterruptKey, window);
				HIViewWrap			buttonSetSuspendKey(idMyButtonChangeSuspendKey, window);
				HIViewWrap			buttonSetResumeKey(idMyButtonChangeResumeKey, window);
				HIViewRef			view = nullptr;
				Preferences_Tag		chosenPreferencesTag = '----';
				Preferences_Result	prefsResult = kPreferences_ResultOK;
				OSStatus			error = noErr;
				
				
				// one of the 3 buttons should always be active
				// INCOMPLETE - arrange to remember the selected key somewhere
				if (GetControl32BitValue(buttonSetInterruptKey) == kControlCheckBoxCheckedValue)
				{
					chosenPreferencesTag = kPreferences_TagKeyInterruptProcess;
					view = buttonSetInterruptKey;
				}
				else if (GetControl32BitValue(buttonSetSuspendKey) == kControlCheckBoxCheckedValue)
				{
					chosenPreferencesTag = kPreferences_TagKeySuspendOutput;
					view = buttonSetSuspendKey;
				}
				else if (GetControl32BitValue(buttonSetResumeKey) == kControlCheckBoxCheckedValue)
				{
					chosenPreferencesTag = kPreferences_TagKeyResumeOutput;
					view = buttonSetResumeKey;
				}
				
				prefsResult = Preferences_ContextSetData(dataPtr->dataModel, chosenPreferencesTag,
															sizeof(chosenChar), &chosenChar);
				if (kPreferences_ResultOK == prefsResult)
				{
					error = keyboardInterfacePtr->setButtonFromKey(view, chosenChar);
					assert_noerr(error);
				}
			}
		}
	}
	
	return result;
}// My_SessionsPanelKeyboardUI::receiveHICommand


/*!
Sets the title of the specified button to a string that
describes the given key character.

\retval noErr
if successful

\retval paramErr
if the given character is not supported

\retval (other)
any OSStatus value based on Mac OS APIs used

(4.0)
*/
OSStatus
My_SessionsPanelKeyboardUI::
setButtonFromKey	(HIViewRef		inView,
					 char			inCharacter)
{
	CFStringRef		newTitle = gCharacterToCFStringMap()[inCharacter];
	OSStatus		result = noErr;
	
	
	if (newTitle == nullptr) result = paramErr;
	else
	{
		result = SetControlTitleWithCFString(inView, newTitle);
	}
	return result;
}// My_SessionsPanelKeyboardUI::setButtonFromKey


/*!
Initializes a My_SessionsPanelResourceData structure.

(3.1)
*/
My_SessionsPanelResourceData::
My_SessionsPanelResourceData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelResourceData default constructor


/*!
Tears down a My_SessionsPanelResourceData structure.

(3.1)
*/
My_SessionsPanelResourceData::
~My_SessionsPanelResourceData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelResourceData destructor


/*!
Initializes a My_SessionsPanelResourceUI structure.

(3.1)
*/
My_SessionsPanelResourceUI::
My_SessionsPanelResourceUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel							(inPanel),
idealWidth						(0.0),
idealHeight						(0.0),
mainView						(createContainerView(inPanel, inOwningWindow)
									<< HIViewWrap_AssertExists),
_numberOfFormatItemsAdded		(0),
_numberOfSessionItemsAdded		(0),
_numberOfTerminalItemsAdded		(0),
_numberOfTranslationItemsAdded	(0),
_serverBrowser					(nullptr),
_fieldCommandLine				(HIViewWrap(idMyFieldCommandLine, inOwningWindow)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(UnixCommandLineLimiter)),
_containerResizer				(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
									My_SessionsPanelResourceUI::deltaSize, this/* context */),
_buttonCommandsHandler			(GetWindowEventTarget(inOwningWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this/* user data */),
_whenCommandLineChangedHandler	(GetControlEventTarget(this->_fieldCommandLine), receiveFieldChangedInCommandLine,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this/* user data */),
_whenServerPanelChangedHandler	(GetWindowEventTarget(inOwningWindow)/* see serverBrowserDisplay() for target dependency */,
									receiveServerBrowserEvent,
									CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_ServerBrowser),
															kEventNetEvents_ServerBrowserNewData, kEventNetEvents_ServerBrowserClosed),
									this/* user data */),
_whenFavoritesChangedHandler	(ListenerModel_NewStandardListener(preferenceChanged, this/* context */), true/* is retained */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
	
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeNumberOfContexts,
						true/* notify of initial value */);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeContextName,
						true/* notify of initial value */);
	assert(kPreferences_ResultOK == prefsResult);
}// My_SessionsPanelResourceUI 2-argument constructor


/*!
Tears down a My_SessionsPanelResourceUI structure.

(3.1)
*/
My_SessionsPanelResourceUI::
~My_SessionsPanelResourceUI ()
{
	if (nullptr != _serverBrowser)
	{
		ServerBrowser_Dispose(&_serverBrowser);
	}
	Preferences_StopMonitoring(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeNumberOfContexts);
	Preferences_StopMonitoring(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeContextName);
}// My_SessionsPanelResourceUI destructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelResourceUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("Resource"), inOwningWindow,
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
	// INCOMPLETE
	
	return result;
}// My_SessionsPanelResourceUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsPanelResourceUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			inContext)
{
	HIWindowRef const				kPanelWindow = HIViewGetWindow(inContainer);
	My_SessionsPanelResourceUI*		dataPtr = REINTERPRET_CAST(inContext, My_SessionsPanelResourceUI*);
	HIViewWrap						viewWrap;
	
	
	viewWrap = HIViewWrap(idMySeparatorCommandRegion, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyHelpTextPresets, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	dataPtr->_fieldCommandLine << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	// INCOMPLETE
}// My_SessionsPanelResourceUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelResourceUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionResource, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelResourceUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(inDataPtr, My_SessionsPanelResourceDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
			panelDataPtr->interfacePtr->serverBrowserRemove();
			delete panelDataPtr;
		}
		break;
	
	case kPanel_MessageFocusFirst: // notification that the first logical view should become focused
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			HIWindowRef							owningWindow = HIViewGetWindow(panelDataPtr->interfacePtr->mainView);
			HIViewWrap							commandLineField(idMyFieldCommandLine, owningWindow);
			
			
			DialogUtilities_SetKeyboardFocus(commandLineField);
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
			//HIViewRef const*						viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
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
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			panelDataPtr->interfacePtr->serverBrowserRemove();
			
			if (nullptr != oldContext)
			{
				panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
				Preferences_ContextSave(oldContext);
			}
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext) panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
			panelDataPtr->dataModel = newContext;
			panelDataPtr->interfacePtr->readPreferences(newContext);
		}
		break;
	
	case kPanel_MessageNewVisibility: // visible state of the panel’s container has changed to visible (true) or invisible (false)
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			Boolean								isNowVisible = *((Boolean*)inDataPtr);
			
			
			if (false == isNowVisible)
			{
				panelDataPtr->interfacePtr->serverBrowserRemove();
			}
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// My_SessionsPanelResourceUI::panelChanged


/*!
Fills in the command-line field using the given Session context.
If the Session does not have a command line argument array, the
command line is generated based on remote server information;
and if even that fails, the Default settings will be used.

(4.0)
*/
void
My_SessionsPanelResourceUI::
readCommandLinePreferenceFromSession	(Preferences_ContextRef		inSession)
{
	CFArrayRef				argumentListCFArray = nullptr;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_ContextGetData(inSession, kPreferences_TagCommandLine,
												sizeof(argumentListCFArray), &argumentListCFArray);
	if ((kPreferences_ResultOK != prefsResult) ||
		(false == this->setCommandLineFromArray(argumentListCFArray)))
	{
		// ONLY if no actual command line was stored, generate a
		// command line based on other settings (like host name)
		Session_Protocol	givenProtocol = kSession_ProtocolSSH1;
		CFStringRef			hostCFString = nullptr;
		UInt16				portNumber = 0;
		CFStringRef			userCFString = nullptr;
		UInt16				preferenceCountOK = 0;
		Boolean				updateOK = false;
		
		
		preferenceCountOK = this->readPreferencesForRemoteServers(inSession, true/* search defaults too */,
																	givenProtocol, hostCFString, portNumber, userCFString);
		if (4 != preferenceCountOK)
		{
			Console_Warning(Console_WriteLine, "unable to read one or more remote server preferences!");
		}
		updateOK = this->updateCommandLine(givenProtocol, hostCFString, portNumber, userCFString);
		if (false == updateOK)
		{
			Console_Warning(Console_WriteLine, "unable to update some part of command line based on given preferences!");
		}
		
		CFRelease(hostCFString), hostCFString = nullptr;
		CFRelease(userCFString), userCFString = nullptr;
	}
}// readCommandLinePreferenceFromSession


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelResourceUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// INCOMPLETE
		
		// set associated Terminal settings
		{
			CFStringRef		associatedTerminalName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagAssociatedTerminalFavorite,
														sizeof(associatedTerminalName), &associatedTerminalName,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedTerminal(associatedTerminalName);
				CFRelease(associatedTerminalName), associatedTerminalName = nullptr;
			}
			else
			{
				this->setAssociatedTerminal(nullptr);
			}
		}
		
		// set associated Format settings
		{
			CFStringRef		associatedFormatName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagAssociatedFormatFavorite,
														sizeof(associatedFormatName), &associatedFormatName,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedFormat(associatedFormatName);
				CFRelease(associatedFormatName), associatedFormatName = nullptr;
			}
			else
			{
				this->setAssociatedFormat(nullptr);
			}
		}
		
		// set associated Translation settings
		{
			CFStringRef		associatedTranslationName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagAssociatedTranslationFavorite,
														sizeof(associatedTranslationName), &associatedTranslationName,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedTranslation(associatedTranslationName);
				CFRelease(associatedTranslationName), associatedTranslationName = nullptr;
			}
			else
			{
				this->setAssociatedTranslation(nullptr);
			}
		}
		
		// set command line
		readCommandLinePreferenceFromSession(inSettings);
	}
}// My_SessionsPanelResourceUI::readPreferences


/*!
Reads all preferences from the specified context that define a
remote server, optionally using global defaults as a fallback
for undefined settings.

Every result is defined, even if it is an empty string or a
zero value.  CFRelease() should be called on all strings.

Returns the number of preferences read successfully, which is
normally 4.  Any unsuccessful reads will have arbitrary values.

(4.0)
*/
UInt16
My_SessionsPanelResourceUI::
readPreferencesForRemoteServers		(Preferences_ContextRef		inSettings,
									 Boolean					inSearchDefaults,
									 Session_Protocol&			outProtocol,
									 CFStringRef&				outHostNameCopy,
									 UInt16&					outPortNumber,
									 CFStringRef&				outUserIDCopy)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	size_t					actualSize = 0;
	UInt16					result = 0;
	
	
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerProtocol, sizeof(outProtocol),
												&outProtocol, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outProtocol = kSession_ProtocolSSH1; // arbitrary
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerHost, sizeof(outHostNameCopy),
												&outHostNameCopy, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outHostNameCopy = CFSTR(""); // arbitrary
		CFRetain(outHostNameCopy);
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerPort, sizeof(outPortNumber),
												&outPortNumber, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outPortNumber = 22; // arbitrary
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerUserID, sizeof(outUserIDCopy),
												&outUserIDCopy, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outUserIDCopy = CFSTR(""); // arbitrary
		CFRetain(outUserIDCopy);
	}
	
	return result;
}// My_SessionsPanelResourceUI::readPreferencesForRemoteServers


/*!
Deletes all the items in the specified pop-up menu and
rebuilds the menu based on current preferences of the
given type.  Also tracks the number of items added so
that they can be removed later.

(4.0)
*/
void
My_SessionsPanelResourceUI::
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
			(OSStatus)DeleteMenuItems(favoritesMenu, defaultIndex + 1/* first item */, inoutItemCountTracker);
		}
		otherItemCount = CountMenuItems(favoritesMenu);
		
		// add the names of all terminal configurations to the menu;
		// update global count of items added at that location
		inoutItemCountTracker = 0;
		(Preferences_Result)Preferences_InsertContextNamesInMenu(inCollectionsToUse, favoritesMenu,
																	defaultIndex, 0/* indentation level */,
																	inEachNewItemCommandID, inoutItemCountTracker);
		SetControl32BitMaximum(popUpMenuView, otherItemCount + inoutItemCountTracker);
		
		// TEMPORARY: verify that this final step is necessary...
		error = SetControlData(popUpMenuView, kControlMenuPart, kControlPopupButtonMenuRefTag,
								sizeof(favoritesMenu), &favoritesMenu);
		assert_noerr(error);
	}
}// My_SessionsPanelResourceUI::rebuildFavoritesMenu


/*!
Deletes all the items in the Format pop-up menu and
rebuilds the menu based on current preferences.

(4.0)
*/
void
My_SessionsPanelResourceUI::
rebuildFormatMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuFormat, kCommandFormatDefault/* anchor */, Quills::Prefs::FORMAT,
							kCommandFormatByFavoriteName/* command ID of new items */, this->_numberOfFormatItemsAdded);
}// My_SessionsPanelResourceUI::rebuildFormatMenu


/*!
Deletes all the items in the Copy From Session Preferences
pop-up menu and rebuilds the menu based on current preferences.

(4.0)
*/
void
My_SessionsPanelResourceUI::
rebuildSessionMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuCopySessionCommand, kCommandCopySessionDefaultCommandLine/* anchor */, Quills::Prefs::SESSION,
							kCommandCopySessionFavoriteCommandLine/* command ID of new items */, this->_numberOfSessionItemsAdded);
}// My_SessionsPanelResourceUI::rebuildSessionMenu


/*!
Deletes all the items in the Terminal pop-up menu and
rebuilds the menu based on current preferences.

(3.1)
*/
void
My_SessionsPanelResourceUI::
rebuildTerminalMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuTerminal, kCommandTerminalDefault/* anchor */, Quills::Prefs::TERMINAL,
							kCommandTerminalByFavoriteName/* command ID of new items */, this->_numberOfTerminalItemsAdded);
}// My_SessionsPanelResourceUI::rebuildTerminalMenu


/*!
Deletes all the items in the Translation pop-up menu and
rebuilds the menu based on current preferences.

(4.0)
*/
void
My_SessionsPanelResourceUI::
rebuildTranslationMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuTranslation, kCommandTranslationTableDefault/* anchor */, Quills::Prefs::TRANSLATION,
							kCommandTranslationTableByFavoriteName/* command ID of new items */, this->_numberOfTranslationItemsAdded);
}// My_SessionsPanelResourceUI::rebuildTranslationMenu


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the Resource tab.

(3.1)
*/
OSStatus
My_SessionsPanelResourceUI::
receiveHICommand	(EventHandlerCallRef	inHandlerCallRef,
					 EventRef				inEvent,
					 void*					inMySessionsUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		resourceInterfacePtr = REINTERPRET_CAST
															(inMySessionsUIPtr, My_SessionsPanelResourceUI*);
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
			switch (received.commandID)
			{
			case kCommandFormatDefault:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// delete the “associated Format” preference, which will cause
					// a fallback to the default when it is later queried
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel, kPreferences_TagAssociatedFormatFavorite);
					
					// pass this handler through to the window, which will update the font and colors!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandFormatByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation
																							(resourceInterfacePtr->panel),
																							My_SessionsPanelResourceDataPtr);
							Preferences_Result					prefsResult = kPreferences_ResultOK;
							
							
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagAssociatedFormatFavorite,
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult) isError = false;
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window, which will update the font and colors!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTerminalDefault:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// delete the “associated Terminal” preference, which will cause
					// a fallback to the default when it is later queried
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel, kPreferences_TagAssociatedTerminalFavorite);
					
					// pass this handler through to the window
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTerminalByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation
																							(resourceInterfacePtr->panel),
																							My_SessionsPanelResourceDataPtr);
							Preferences_Result					prefsResult = kPreferences_ResultOK;
							
							
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagAssociatedTerminalFavorite,
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult) isError = false;
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window, which will update the terminal settings!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTranslationTableDefault:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// delete the “associated Translation” preference, which will cause
					// a fallback to the default when it is later queried
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel, kPreferences_TagAssociatedTranslationFavorite);
					
					// pass this handler through to the window
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTranslationTableByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation
																							(resourceInterfacePtr->panel),
																							My_SessionsPanelResourceDataPtr);
							Preferences_Result					prefsResult = kPreferences_ResultOK;
							
							
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagAssociatedTranslationFavorite,
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult) isError = false;
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window, which will update the translation settings!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandShowCommandLine:
				// this normally means “show command line floater”, but in the context
				// of an active New Session sheet, it means “select command line field”
				{
					HIWindowRef		window = HIViewGetWindow(resourceInterfacePtr->mainView);
					
					
					(OSStatus)DialogUtilities_SetKeyboardFocus(HIViewWrap(idMyFieldCommandLine, window));
					result = noErr;
				}
				break;
			
			case kCommandCopyLogInShellCommandLine:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					CFArrayRef							argumentCFArray = nullptr;
					Local_Result						localResult = kLocal_ResultOK;
					Boolean								isError = true;
					
					
					localResult = Local_GetLoginShellCommandLine(argumentCFArray);
					if ((kLocal_ResultOK == localResult) && (nullptr != argumentCFArray))
					{
						if (resourceInterfacePtr->setCommandLineFromArray(argumentCFArray))
						{
							resourceInterfacePtr->saveFieldPreferences(dataPtr->dataModel);
							isError = false;
						}
						CFRelease(argumentCFArray), argumentCFArray = nullptr;
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					result = noErr;
				}
				break;
			
			case kCommandCopyShellCommandLine:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					CFArrayRef							argumentCFArray = nullptr;
					Local_Result						localResult = kLocal_ResultOK;
					Boolean								isError = true;
					
					
					localResult = Local_GetDefaultShellCommandLine(argumentCFArray);
					if ((kLocal_ResultOK == localResult) && (nullptr != argumentCFArray))
					{
						if (resourceInterfacePtr->setCommandLineFromArray(argumentCFArray))
						{
							resourceInterfacePtr->saveFieldPreferences(dataPtr->dataModel);
							isError = false;
						}
						CFRelease(argumentCFArray), argumentCFArray = nullptr;
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					result = noErr;
				}
				break;
			
			case kCommandCopySessionDefaultCommandLine:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Boolean								isError = true;
					Preferences_ContextRef				defaultContext = nullptr;
					Preferences_Result					prefsResult = Preferences_GetDefaultContext
																		(&defaultContext, Quills::Prefs::SESSION);
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					if (kPreferences_ResultOK == prefsResult)
					{
						resourceInterfacePtr->readCommandLinePreferenceFromSession(defaultContext);
						resourceInterfacePtr->saveFieldPreferences(dataPtr->dataModel);
						isError = false;
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window
					result = eventNotHandledErr;
					
					result = noErr;
				}
				break;
			
			case kCommandCopySessionFavoriteCommandLine:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Boolean								isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							Preferences_ContextWrap		sessionContext(Preferences_NewContextFromFavorites
																		(Quills::Prefs::SESSION, collectionName),
																		true/* is retained */);
							
							
							if (sessionContext.exists())
							{
								resourceInterfacePtr->readCommandLinePreferenceFromSession(sessionContext.returnRef());
								resourceInterfacePtr->saveFieldPreferences(dataPtr->dataModel);
								isError = false;
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
			
			case kCommandEditCommandLine:
				// show the server browser panel and target the command line field, or hide the browser
				{
					HIWindowRef		window = HIViewGetWindow(resourceInterfacePtr->mainView);
					HIViewWrap		panelDisplayButton(idMyButtonConnectToServer, window);
					
					
					if (kControlCheckBoxCheckedValue == GetControl32BitValue(panelDisplayButton))
					{
						resourceInterfacePtr->serverBrowserRemove();
					}
					else
					{
						My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																						My_SessionsPanelResourceDataPtr);
						Session_Protocol					dataModelProtocol = kSession_ProtocolSSH1;
						CFStringRef							dataModelHostName = nullptr;
						UInt16								dataModelPort = 0;
						CFStringRef							dataModelUserID = nullptr;
						UInt16								preferenceCountOK = 0;
						
						
						preferenceCountOK = resourceInterfacePtr->readPreferencesForRemoteServers
											(dataPtr->dataModel, false/* search defaults too */,
												dataModelProtocol, dataModelHostName, dataModelPort, dataModelUserID);
						if (4 != preferenceCountOK)
						{
							Console_Warning(Console_WriteLine, "unable to read one or more remote server preferences!");
						}
						SetControl32BitValue(panelDisplayButton, kControlCheckBoxCheckedValue);
						resourceInterfacePtr->serverBrowserDisplay(dataModelProtocol, dataModelHostName,
																	dataModelPort, dataModelUserID);
						
						CFRelease(dataModelHostName), dataModelHostName = nullptr;
						CFRelease(dataModelUserID), dataModelUserID = nullptr;
						
						result = noErr;
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
}// My_SessionsPanelResourceUI::receiveHICommand


/*!
Saves every text field in the panel to the data model.
It is necessary to treat fields specially because they
do not have obvious state changes (as, say, buttons do);
they might need saving when focus is lost or the window
is closed, etc.

(3.1)
*/
void
My_SessionsPanelResourceUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings)
{
	if (nullptr != inoutSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set command line
		{
			CFStringRef		commandLineCFString = nullptr;
			Boolean			saveOK = false;
			
			
			GetControlTextAsCFString(_fieldCommandLine, commandLineCFString);
			if (nullptr != commandLineCFString)
			{
				CFArrayRef		argumentListCFArray = nullptr;
				
				
				argumentListCFArray = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, commandLineCFString,
																				CFSTR(" ")/* separator */);
				if (nullptr != argumentListCFArray)
				{
					prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagCommandLine,
																sizeof(argumentListCFArray), &argumentListCFArray);
					if (kPreferences_ResultOK == prefsResult) saveOK = true;
					CFRelease(argumentListCFArray), argumentListCFArray = nullptr;
				}
				CFRelease(commandLineCFString), commandLineCFString = nullptr;
			}
			if (false == saveOK)
			{
				Console_Warning(Console_WriteLine, "failed to set command line");
			}
		}
	}
}// My_SessionsPanelResourceUI::saveFieldPreferences


/*!
Saves the given preferences to the specified context.

Returns the number of preferences saved successfully, which is
normally 4.

(4.0)
*/
UInt16
My_SessionsPanelResourceUI::
savePreferencesForRemoteServers		(Preferences_ContextRef		inoutSettings,
									 Session_Protocol			inProtocol,
									 CFStringRef				inHostName,
									 UInt16						inPortNumber,
									 CFStringRef				inUserID)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	SInt16					portAsSInt16 = STATIC_CAST(inPortNumber, SInt16);
	UInt16					result = 0;
	
	
	// host name
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerHost,
												sizeof(inHostName), &inHostName);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save host name setting");
	}
	// protocol
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerProtocol,
												sizeof(inProtocol), &inProtocol);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save protocol setting");
	}
	// port number
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerPort,
												sizeof(portAsSInt16), &portAsSInt16);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save port number setting");
	}
	// user ID
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerUserID,
												sizeof(inUserID), &inUserID);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save user name setting");
	}
	
	return result;
}// My_SessionsPanelResourceUI::savePreferencesForRemoteServers


/*!
Shows a popover for editing server settings such as the
host name of a remote session’s command line.

(4.0)
*/
void
My_SessionsPanelResourceUI::
serverBrowserDisplay	(Session_Protocol	inProtocol,
						 CFStringRef		inHostName,
						 UInt16				inPortNumber,
						 CFStringRef		inUserID)
{
	HIWindowRef const	kWindow = HIViewGetWindow(_fieldCommandLine);
	
	
	if (nullptr == this->_serverBrowser)
	{
		HIViewRef	panelView = nullptr;
		HIRect		panelBounds;
		
		
		Panel_GetContainerView(this->panel, panelView);
		(OSStatus)HIViewGetBounds(panelView, &panelBounds);
		
		(OSStatus)HIViewConvertRect(&panelBounds, panelView, nullptr/* make window-relative */);
		
		// IMPORTANT: the event target given here should be the same one
		// used to register event handlers in the constructor!
		this->_serverBrowser = ServerBrowser_New
								(kWindow, CGPointMake(panelBounds.origin.x + panelBounds.size.width - 128/* arbitrary */,
														panelBounds.origin.y + 200)/* refers to field; see NIB setup */,
									GetWindowEventTarget(kWindow));
	}
	
	// update the browser with the new settings (might be sharing
	// a previously-constructed interface with old data)
	ServerBrowser_Configure(this->_serverBrowser, inProtocol, inHostName, inPortNumber, inUserID);
	
	// display server browser (closed by the user)
	ServerBrowser_Display(this->_serverBrowser);
}// My_SessionsPanelResourceUI::serverBrowserDisplay


/*!
Hides the popover for editing server settings.  If
"inReset" is true, the popover is also detached from
any target it might have forwarded events to (so the
next time it is displayed, it will not affect that
target).

(4.0)
*/
void
My_SessionsPanelResourceUI::
serverBrowserRemove ()
{
	if (nullptr != this->_serverBrowser)
	{
		ServerBrowser_Remove(this->_serverBrowser);
	}
}// My_SessionsPanelResourceUI::serverBrowserRemove


/*!
Changes the Format menu selection to the specified
match, or chooses the Default if none is found or
the name is not defined.

(4.0)
*/
void
My_SessionsPanelResourceUI::
setAssociatedFormat		(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 1; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuFormat, HIViewGetWindow(this->mainView));
	
	
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
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Format context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
}// My_SessionsPanelResourceUI::setAssociatedFormat


/*!
Changes the Terminal menu selection to the specified
match, or chooses the Default if none is found or
the name is not defined.

(4.0)
*/
void
My_SessionsPanelResourceUI::
setAssociatedTerminal	(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 1; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuTerminal, HIViewGetWindow(this->mainView));
	
	
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
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Terminal context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
}// My_SessionsPanelResourceUI::setAssociatedTerminal


/*!
Changes the Translation menu selection to the specified
match, or chooses the Default if none is found or the
name is not defined.

(4.0)
*/
void
My_SessionsPanelResourceUI::
setAssociatedTranslation		(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 1; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuTranslation, HIViewGetWindow(this->mainView));
	
	
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
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Translation context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
}// My_SessionsPanelResourceUI::setAssociatedTranslation


/*!
Changes the command line field.

IMPORTANT:	Normally, this is for initialization; it overwrites
			the entire field with a set value.  Most of the time
			it is more appropriate to use updateCommandLine() to
			respond to other changes (such as, the host).

(3.1)
*/
void
My_SessionsPanelResourceUI::
setCommandLine		(CFStringRef	inCommandLine)
{
	SetControlTextWithCFString(_fieldCommandLine, inCommandLine);
}// My_SessionsPanelResourceUI::setCommandLine


/*!
Calls setCommandLine() with a string constructed from the
specified list of individual command-line arguments (the first
of which is a program name).  Returns true only if a string
could be constructed.

(4.0)
*/
Boolean
My_SessionsPanelResourceUI::
setCommandLineFromArray		(CFArrayRef		inArgumentListCFArray)
{
	Boolean		result = false;
	
	
	if (CFArrayGetCount(inArgumentListCFArray) > 0)
	{
		CFRetainRelease		concatenatedString(CFStringCreateByCombiningStrings
												(kCFAllocatorDefault, inArgumentListCFArray,
													CFSTR(" ")/* separator */),
												true/* is retained */);
		
		
		if (concatenatedString.exists())
		{
			this->setCommandLine(concatenatedString.returnCFStringRef());
			result = true;
		}
	}
	return result;
}// setCommandLineFromArray


/*!
Since everything is “really” a local Unix command, this
routine uses the given remote options and updates the
command line field with appropriate values.

Note that the port number should be redundantly specified
as both a number and a string.

Every value should be defined, but if a string is empty
or the port number is 0, it is skipped in the resulting
command line.

Returns "true" only if the update was successful.

NOTE:	Currently, this routine basically obliterates
		whatever the user may have entered.  A more
		desirable solution is to implement this with
		a find/replace strategy that changes only
		those command line arguments that need to be
		updated, preserving any additional parts of
		the command line.

See also setCommandLine().

(3.1)
*/
Boolean
My_SessionsPanelResourceUI::
updateCommandLine	(Session_Protocol	inProtocol,
					 CFStringRef		inHostName,
					 UInt16				inPortNumber,
					 CFStringRef		inUserID)
{
	CFMutableStringRef		newCommandLineCFString = CFStringCreateMutable(kCFAllocatorDefault,
																			0/* length, or 0 for unlimited */);
	Str31					portString;
	Boolean					result = false;
	
	
	// the host field is required when updating the command line
	if ((nullptr != inHostName) && (0 == CFStringGetLength(inHostName)))
	{
		inHostName = nullptr;
	}
	
	PLstrcpy(portString, "\p");
	if (0 != inPortNumber) NumToString(STATIC_CAST(inPortNumber, SInt32), portString);
	
	if ((nullptr != newCommandLineCFString) && (nullptr != inHostName))
	{
		CFRetainRelease		portNumberCFString(CFStringCreateWithPascalStringNoCopy
												(kCFAllocatorDefault, portString, kCFStringEncodingASCII, kCFAllocatorNull),
												true/* is retained */);
		Boolean				standardLoginOptionAppend = false;
		Boolean				standardHostNameAppend = false;
		Boolean				standardPortAppend = false;
		Boolean				standardIPv6Append = false;
		
		
		// see what is available
		// NOTE: In the following, whitespace should probably be stripped
		//       from all field values first, since otherwise the user
		//       could enter a non-empty but blank value that would become
		//       a broken command line.
		if ((false == portNumberCFString.exists()) || (0 == CFStringGetLength(portNumberCFString.returnCFStringRef())))
		{
			portNumberCFString.clear();
		}
		if ((nullptr != inUserID) && (0 == CFStringGetLength(inUserID)))
		{
			inUserID = nullptr;
		}
		
		// TEMPORARY - convert all of this logic to Python!
		
		// set command based on the protocol, and add arguments
		// based on the specific command and the available data
		result = true; // initially...
		switch (inProtocol)
		{
		case kSession_ProtocolFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ftp"));
			if (nullptr != inHostName)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != inUserID)
				{
					CFStringAppend(newCommandLineCFString, inUserID);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardPortAppend = true; // ftp supports a standalone server port number argument
			break;
		
		case kSession_ProtocolSFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/sftp"));
			if (nullptr != inHostName)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != inUserID)
				{
					CFStringAppend(newCommandLineCFString, inUserID);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if (portNumberCFString.exists())
			{
				// sftp uses "-oPort=port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -oPort="));
				CFStringAppend(newCommandLineCFString, portNumberCFString.returnCFStringRef());
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			break;
		
		case kSession_ProtocolSSH1:
		case kSession_ProtocolSSH2:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ssh"));
			if (kSession_ProtocolSSH2 == inProtocol)
			{
				// -2: protocol version 2
				CFStringAppend(newCommandLineCFString, CFSTR(" -2 "));
			}
			else
			{
				// -1: protocol version 1
				CFStringAppend(newCommandLineCFString, CFSTR(" -1 "));
			}
			if (portNumberCFString.exists())
			{
				// ssh uses "-p port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -p "));
				CFStringAppend(newCommandLineCFString, portNumberCFString.returnCFStringRef());
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardIPv6Append = true; // ssh supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // ssh supports a "-l userid" option
			standardHostNameAppend = true; // ssh supports a standalone server host argument
			break;
		
		case kSession_ProtocolTelnet:
			// -K: disable automatic login
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/telnet -K "));
			standardIPv6Append = true; // telnet supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // telnet supports a "-l userid" option
			standardHostNameAppend = true; // telnet supports a standalone server host argument
			standardPortAppend = true; // telnet supports a standalone server port number argument
			break;
		
		default:
			// ???
			result = false;
			break;
		}
		
		if (result)
		{
			// since commands tend to follow similar conventions, this section
			// appends options in “standard” form if supported by the protocol
			// (if MOST commands do not have an option, append it in the above
			// switch, instead)
			if ((standardIPv6Append) && (nullptr != inHostName) &&
				(CFStringFind(inHostName, CFSTR(":"), 0/* options */).length > 0))
			{
				// -6: address is in IPv6 format
				CFStringAppend(newCommandLineCFString, CFSTR(" -6 "));
			}
			if ((standardLoginOptionAppend) && (nullptr != inUserID))
			{
				// standard form is a Unix command accepting a "-l" option
				// followed by the user ID for login
				CFStringAppend(newCommandLineCFString, CFSTR(" -l "));
				CFStringAppend(newCommandLineCFString, inUserID);
			}
			if ((standardHostNameAppend) && (nullptr != inHostName))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the host name of the server to connect to
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if ((standardPortAppend) && (portNumberCFString.exists()))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the port number on the server to connect to, AFTER the
				// standalone host name option
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, portNumberCFString.returnCFStringRef());
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			
			// update command line field
			if (0 == CFStringGetLength(newCommandLineCFString))
			{
				result = false;
			}
			else
			{
				SetControlTextWithCFString(this->_fieldCommandLine, newCommandLineCFString);
				(OSStatus)HIViewSetNeedsDisplay(this->_fieldCommandLine, true);
				result = true;
			}
		}
		
		CFRelease(newCommandLineCFString), newCommandLineCFString = nullptr;
	}
	
	// if unsuccessful, clear the command line
	if (false == result)
	{
		SetControlTextWithCFString(this->_fieldCommandLine, CFSTR(""));
		(OSStatus)HIViewSetNeedsDisplay(this->_fieldCommandLine, true);
	}
	
	return result;
}// My_SessionsPanelResourceUI::updateCommandLine


/*!
Returns a reference to a map from character codes (which
may be invisible ASCII) to descriptive strings; the map
is automatically initialized the first time this is called.

(3.1)
*/
My_CharacterToCFStringMap&
initCharacterToCFStringMap ()
{
	// instantiate only one of these
	static My_CharacterToCFStringMap	result;
	
	
	if (result.empty())
	{
		// set up map of ASCII codes to CFStrings, for convenience
		result['@' - '@'] = CFSTR("^@");
		result['A' - '@'] = CFSTR("^A");
		result['B' - '@'] = CFSTR("^B");
		result['C' - '@'] = CFSTR("^C");
		result['D' - '@'] = CFSTR("^D");
		result['E' - '@'] = CFSTR("^E");
		result['F' - '@'] = CFSTR("^F");
		result['G' - '@'] = CFSTR("^G");
		result['H' - '@'] = CFSTR("^H");
		result['I' - '@'] = CFSTR("^I");
		result['J' - '@'] = CFSTR("^J");
		result['K' - '@'] = CFSTR("^K");
		result['L' - '@'] = CFSTR("^L");
		result['M' - '@'] = CFSTR("^M");
		result['N' - '@'] = CFSTR("^N");
		result['O' - '@'] = CFSTR("^O");
		result['P' - '@'] = CFSTR("^P");
		result['Q' - '@'] = CFSTR("^Q");
		result['R' - '@'] = CFSTR("^R");
		result['S' - '@'] = CFSTR("^S");
		result['T' - '@'] = CFSTR("^T");
		result['U' - '@'] = CFSTR("^U");
		result['V' - '@'] = CFSTR("^V");
		result['W' - '@'] = CFSTR("^W");
		result['X' - '@'] = CFSTR("^X");
		result['Y' - '@'] = CFSTR("^Y");
		result['Z' - '@'] = CFSTR("^Z");
		result['[' - '@'] = CFSTR("^[");
		result['\\' - '@'] = CFSTR("^\\");
		result[']' - '@'] = CFSTR("^]");
		result['~' - '@'] = CFSTR("^~");
		result['\?' - '@'] = CFSTR("^?");
		assert(result.size() == 32/* number of entries above */);
	}
	
	return result;
}// initializeCharacterStringMap


/*!
Changes the theme font of all bevel button controls
in the specified window to use the system font
(which automatically sets the font size).  By default
a NIB makes bevel buttons use a small font, so this
routine corrects that.

(3.0)
*/
void
makeAllBevelButtonsUseTheSystemFont		(HIWindowRef	inWindow)
{
	OSStatus	error = noErr;
	HIViewRef	contentView = nullptr;
	
	
	error = HIViewFindByID(HIViewGetRoot(inWindow), kHIViewWindowContentID, &contentView);
	if (error == noErr)
	{
		ControlKind		kindInfo;
		HIViewRef		button = HIViewGetFirstSubview(contentView);
		
		
		while (nullptr != button)
		{
			error = GetControlKind(button, &kindInfo);
			if (noErr == error)
			{
				if ((kControlKindSignatureApple == kindInfo.signature) &&
					(kControlKindBevelButton == kindInfo.kind))
				{
					// only change bevel buttons
					Localization_SetControlThemeFontInfo(button, kThemeSystemFont);
				}
			}
			button = HIViewGetNextView(button);
		}
	}
}// makeAllButtonsUseTheSystemFont


/*!
Invoked whenever a monitored preference value is changed.
Responds by updating the user interface.

(3.1)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					inMySessionsTabResourcePtr)
{
	//Preferences_ChangeContext*		contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	My_SessionsPanelResourceUI*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeNumberOfContexts:
	case kPreferences_ChangeContextName:
		ptr->rebuildFormatMenu();
		ptr->rebuildSessionMenu();
		ptr->rebuildTerminalMenu();
		ptr->rebuildTranslationMenu();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the command line field
in the Resource tab.  This saves the preferences
for text field data.

(3.1)
*/
OSStatus
receiveFieldChangedInCommandLine	(EventHandlerCallRef	inHandlerCallRef,
									 EventRef				inEvent,
									 void*					inMySessionsTabResourcePtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		resourceInterfacePtr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// synchronize the post-input change with preferences
	{
		My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																			My_SessionsPanelResourceDataPtr);
		
		
		resourceInterfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
	}
	
	return result;
}// receiveFieldChangedInCommandLine


/*!
Implements "kEventNetEvents_ServerBrowserNewData" and
"kEventNetEvents_ServerBrowserClosed" of
"kEventClassNetEvents_ServerBrowser" for the Resource tab.

When data is changed, the command line is rewritten to use the
new protocol, remote server, port number, and user ID from the
panel.

When the event target is changed, the highlighting is removed
from the button that originally spawned the server browser panel.

(4.0)
*/
OSStatus
receiveServerBrowserEvent	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inMySessionsPanelResourceUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		resourceInterfacePtr = REINTERPRET_CAST(inMySessionsPanelResourceUIPtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassNetEvents_ServerBrowser);
	assert((kEventKind == kEventNetEvents_ServerBrowserNewData) ||
			(kEventKind == kEventNetEvents_ServerBrowserClosed));
	
	switch (kEventKind)
	{
	case kEventNetEvents_ServerBrowserNewData:
		// update command line field
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																				My_SessionsPanelResourceDataPtr);
			Session_Protocol					newProtocol = kSession_ProtocolSSH1; // arbitrary initial value
			Session_Protocol					existingProtocol = kSession_ProtocolSSH1; // arbitrary initial value
			CFStringRef							newHostName = nullptr;
			CFStringRef							existingHostName = nullptr;
			UInt32								newPortNumber = 0;
			UInt16								existingPortNumber = 0;
			CFStringRef							newUserID = nullptr;
			CFStringRef							existingUserID = nullptr;
			UInt16								preferenceCountOK = 0;
			OSStatus							error = noErr;
			Boolean								updateOK = false;
			
			
			// first set defaults
			preferenceCountOK = resourceInterfacePtr->readPreferencesForRemoteServers(panelDataPtr->dataModel, true/* search defaults too */,
																						existingProtocol, existingHostName,
																						existingPortNumber, existingUserID);
			if (4 != preferenceCountOK) // TEMPORARY - should this be an assertion?
			{
				Console_Warning(Console_WriteLine, "unable to set defaults for one or more remote server preferences!");
			}
			
			// now read any updated values; note that CFStrings in events are automatically released
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_Protocol, typeNetEvents_SessionProtocol,
															newProtocol);
			if (noErr != error)
			{
				//Console_WriteLine("no protocol found in event"); // debug - this is not an error
				newProtocol = existingProtocol;
			}
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_HostName, typeCFStringRef,
															newHostName);
			if (noErr != error)
			{
				//Console_WriteLine("no host name found in event"); // debug - this is not an error
				newHostName = existingHostName;
			}
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_PortNumber, typeUInt32,
															newPortNumber);
			if (noErr != error)
			{
				//Console_WriteLine("no port number found in event"); // debug - this is not an error
				newPortNumber = existingPortNumber;
			}
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_UserID, typeCFStringRef,
															newUserID);
			if (noErr != error)
			{
				//Console_WriteLine("no user ID found in event"); // debug - this is not an error
				newUserID = existingUserID;
			}
			
			// finally, update the user interface to incorporate the changes
			updateOK = resourceInterfacePtr->updateCommandLine(newProtocol, newHostName, newPortNumber, newUserID);
			if (false == updateOK) // TEMPORARY - should this be an assertion?
			{
				Console_Warning(Console_WriteLine, "unable to update some part of command line based on panel settings!");
			}
			preferenceCountOK = resourceInterfacePtr->savePreferencesForRemoteServers
														(panelDataPtr->dataModel, newProtocol, newHostName,
															newPortNumber, newUserID);
			if (4 != preferenceCountOK)
			{
				Console_Warning(Console_WriteValue, "some remote server settings could not be saved; 4 exist, saved",
								preferenceCountOK);
			}
			resourceInterfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
			
			CFRelease(existingHostName), existingHostName = nullptr;
			CFRelease(existingUserID), existingUserID = nullptr;
			
			result = noErr;
		}
		break;
	
	case kEventNetEvents_ServerBrowserClosed:
		// remove highlighting from the panel-spawning button
		{
			HIWindowRef		window = HIViewGetWindow(resourceInterfacePtr->mainView);
			HIViewWrap		panelDisplayButton(idMyButtonConnectToServer, window);
			
			
			SetControl32BitValue(panelDisplayButton, kControlCheckBoxUncheckedValue);
			result = noErr;
		}
		break;
	
	default:
		// ???
		result = eventNotHandledErr;
		break;
	}
	return result;
}// receiveServerBrowserEvent

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
