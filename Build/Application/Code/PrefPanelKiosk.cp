/*###############################################################

	PrefPanelKiosk.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>

// resource includes
#include "GeneralResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelKiosk.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanels.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyCheckBoxShowMenuBar					= { 'XKSM', 0/* ID */ };
static HIViewID const	idMyCheckBoxShowScrollBar				= { 'XKSS', 0/* ID */ };
static HIViewID const	idMyCheckBoxAllowForceQuit				= { 'XKFQ', 0/* ID */ };
static HIViewID const	idMyCheckBoxSuperfluousEffects			= { 'XKFX', 0/* ID */ };
static HIViewID const	idMyHelpTextSuperfluousEffects			= { 'HTSG', 0/* ID */ };
static HIViewID const	idMyCheckBoxShowOffSwitchWindow			= { 'XKFW', 0/* ID */ };
static HIViewID const	idMyHelpTextDisableKiosk				= { 'HTDK', 0/* ID */ };

#pragma mark Types

/*!
Implements the user interface of the panel - only
created when the panel directs for this to happen.
*/
struct MyKioskPanelUI
{
public:
	MyKioskPanelUI		(Panel_Ref, HIWindowRef);
	
	HIViewWrap							mainView;
	CommonEventHandlers_HIViewResizer	containerResizer;	//!< invoked when the panel is resized
	CarbonEventHandlerWrap				viewClickHandler;	//!< invoked when a button is clicked

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
};
typedef MyKioskPanelUI*		MyKioskPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct MyKioskPanelData
{
	MyKioskPanelData ();
	
	Panel_Ref			panel;			//!< the panel this data is for
	MyKioskPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef MyKioskPanelData*		MyKioskPanelDataPtr;

#pragma mark Internal Method Prototypes

static void				deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
static void				disposePanel					(Panel_Ref, void*);
static SInt32			panelChanged					(Panel_Ref, Panel_Message, void*);
static pascal OSStatus	receiveViewHit					(EventHandlerCallRef, EventRef, void*);
static Boolean			updateCheckBoxPreference		(MyKioskPanelUIPtr, HIViewRef);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealPanelWidth = 0.0;
	Float32		gIdealPanelHeight = 0.0;
}



#pragma mark Public Methods

/*!
This routine creates a new preference panel for
the Kiosk category, initializes it, and returns
a reference to it.  You must destroy the reference
using Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelKiosk_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (result != nullptr)
	{
		MyKioskPanelDataPtr		dataPtr = new MyKioskPanelData;
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorKiosk);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelKiosk);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowKioskCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelKioskIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFullScreen);
		Panel_SetImplementation(result, dataPtr);
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a MyKioskPanelData structure.

(3.1)
*/
MyKioskPanelData::
MyKioskPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr)
{
}// MyKioskPanelData default constructor


/*!
Initializes a MyKioskPanelUI structure.

(3.1)
*/
MyKioskPanelUI::
MyKioskPanelUI	(Panel_Ref		inPanel,
				 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
mainView			(createContainerView(inPanel, inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						deltaSizePanelContainerHIView, this/* context */),
viewClickHandler	(GetControlEventTarget(this->mainView), receiveViewHit,
						CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
						this/* user data */)
{
	assert(containerResizer.isInstalled());
	assert(viewClickHandler.isInstalled());
}// MyKioskPanelUI 2-argument constructor


/*!
Constructs the main panel container view and its
sub-view hierarchy.  The container is returned.

(3.1)
*/
HIViewWrap
MyKioskPanelUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
const
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						idealContainerBounds;
	size_t						actualSize = 0;
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
	
	// create HIViews for the panel based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("Kiosk"), inOwningWindow,
				result, viewList, idealContainerBounds);
	assert_noerr(error);
	
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
	
	// initialize checkboxes
	{
		Boolean		showMenuBar = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagKioskShowsMenuBar, sizeof(showMenuBar),
									&showMenuBar, &actualSize))
		{
			showMenuBar = false; // assume a default, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxShowMenuBar, inOwningWindow), BooleanToCheckBoxValue(showMenuBar));
	}
	{
		Boolean		showScrollBar = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagKioskShowsScrollBar, sizeof(showScrollBar),
									&showScrollBar, &actualSize))
		{
			showScrollBar = false; // assume a default, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxShowScrollBar, inOwningWindow), BooleanToCheckBoxValue(showScrollBar));
	}
	{
		Boolean		allowForceQuit = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
									&allowForceQuit, &actualSize))
		{
			allowForceQuit = false; // assume a default, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxAllowForceQuit, inOwningWindow), BooleanToCheckBoxValue(allowForceQuit));
	}
	{
		Boolean		superfluousEffects = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagKioskUsesSuperfluousEffects, sizeof(superfluousEffects),
									&superfluousEffects, &actualSize))
		{
			superfluousEffects = false; // assume a default, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxSuperfluousEffects, inOwningWindow), BooleanToCheckBoxValue(superfluousEffects));
	}
	{
		Boolean		showOffSwitch = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagKioskShowsOffSwitch, sizeof(showOffSwitch),
									&showOffSwitch, &actualSize))
		{
			showOffSwitch = false; // assume a default, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxShowOffSwitchWindow, inOwningWindow), BooleanToCheckBoxValue(showOffSwitch));
	}
	
	return result;
}// MyKioskPanelUI::createContainerView


/*!
Adjusts the views in preference panels to match the
specified change in dimensions of a panel’s container.

(3.1)
*/
static void
deltaSizePanelContainerHIView	(HIViewRef		inView,
								 Float32		inDeltaX,
								 Float32		UNUSED_ARGUMENT(inDeltaY),
								 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef			kPanelWindow = GetControlOwner(inView);
	//KioskPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), KioskPanelDataPtr);
	HIViewWrap			viewWrap;
	
	
	viewWrap.setCFTypeRef(HIViewWrap(idMyHelpTextSuperfluousEffects, kPanelWindow));
	viewWrap
		<< HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */)
		;
	viewWrap.setCFTypeRef(HIViewWrap(idMyHelpTextDisableKiosk, kPanelWindow));
	viewWrap
		<< HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */)
		;
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
	MyKioskPanelDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, MyKioskPanelDataPtr);
	
	
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
												kConstantsRegistry_PrefPanelDescriptorKiosk, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			MyKioskPanelDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	MyKioskPanelDataPtr);
			HIWindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			panelDataPtr->interfacePtr = new MyKioskPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
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
for this preferences panel.  Responds by updating
preferences bound to checkboxes.

(3.1)
*/
static pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyKioskPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	MyKioskPanelUIPtr		interfacePtr = REINTERPRET_CAST(inMyKioskPanelUIPtr, MyKioskPanelUIPtr);
	assert(nullptr != interfacePtr);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
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
			
			{
				ControlKind		controlKind;
				OSStatus		error = GetControlKind(view, &controlKind);
				
				
				if ((noErr == error) && (kControlKindSignatureApple == controlKind.signature) &&
					(kControlKindCheckBox == controlKind.kind))
				{
					(Boolean)updateCheckBoxPreference(interfacePtr, view);
				}
			}
		}
	}
	
	return result;
}// receiveViewHit


/*!
Call this method when a click in a checkbox has
been detected AND the control value has been
toggled to its new value.

The appropriate preference will have been updated
in memory using Preferences_SetData().

If the specified view is “known”, "true" is
returned to indicate that the click was handled.
Otherwise, "false" is returned.

(3.1)
*/
static Boolean
updateCheckBoxPreference	(MyKioskPanelUIPtr		inInterfacePtr,
							 HIViewRef				inCheckBoxClicked)
{
	Boolean		result = false;
	
	
	if (nullptr != inInterfacePtr)
	{
		WindowRef const		kWindow = GetControlOwner(inCheckBoxClicked);
		assert(IsValidWindowRef(kWindow));
		HIViewIDWrap		viewID(HIViewWrap(inCheckBoxClicked).identifier());
		Boolean				checkBoxFlagValue = (GetControl32BitValue(inCheckBoxClicked) == kControlCheckBoxCheckedValue);
		
		
		if (HIViewIDWrap(idMyCheckBoxShowMenuBar) == viewID)
		{
			Preferences_SetData(kPreferences_TagKioskShowsMenuBar,
								sizeof(checkBoxFlagValue), &checkBoxFlagValue);
			result = true;
		}
		else if (HIViewIDWrap(idMyCheckBoxShowScrollBar) == viewID)
		{
			Preferences_SetData(kPreferences_TagKioskShowsScrollBar,
								sizeof(checkBoxFlagValue), &checkBoxFlagValue);
			result = true;
		}
		else if (HIViewIDWrap(idMyCheckBoxAllowForceQuit) == viewID)
		{
			Preferences_SetData(kPreferences_TagKioskAllowsForceQuit,
								sizeof(checkBoxFlagValue), &checkBoxFlagValue);
			result = true;
		}
		else if (HIViewIDWrap(idMyCheckBoxSuperfluousEffects) == viewID)
		{
			Preferences_SetData(kPreferences_TagKioskUsesSuperfluousEffects,
								sizeof(checkBoxFlagValue), &checkBoxFlagValue);
			result = true;
		}
		else if (HIViewIDWrap(idMyCheckBoxShowOffSwitchWindow) == viewID)
		{
			Preferences_SetData(kPreferences_TagKioskShowsOffSwitch,
								sizeof(checkBoxFlagValue), &checkBoxFlagValue);
			result = true;
		}
	}
	
	return result;
}// updateCheckBoxPreference

// BELOW IS REQUIRED NEWLINE TO END FILE
