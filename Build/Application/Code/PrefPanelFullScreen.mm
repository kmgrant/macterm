/*!	\file PrefPanelFullScreen.mm
	\brief Implements the Full Screen panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
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

#import "PrefPanelFullScreen.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaExtensions.objc++.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <DialogAdjust.h>
#import <HelpSystem.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>

// resource includes
#import "SpacingConstants.r"

// application includes
#import "AppResources.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "Panel.h"
#import "Preferences.h"
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
static HIViewID const	idMyCheckBoxShowMenuBar					= { 'XKSM', 0/* ID */ };
static HIViewID const	idMyCheckBoxShowScrollBar				= { 'XKSS', 0/* ID */ };
static HIViewID const	idMyCheckBoxShowWindowFrame				= { 'XKWF', 0/* ID */ };
static HIViewID const	idMyCheckBoxAllowForceQuit				= { 'XKFQ', 0/* ID */ };
static HIViewID const	idMyCheckBoxSuperfluousEffects			= { 'XKFX', 0/* ID */ };
static HIViewID const	idMyHelpTextSuperfluousEffects			= { 'HTSG', 0/* ID */ };
static HIViewID const	idMyCheckBoxShowOffSwitchWindow			= { 'XKFW', 0/* ID */ };
static HIViewID const	idMyHelpTextDisableKiosk				= { 'HTDK', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

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

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void		deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
void		disposePanel					(Panel_Ref, void*);
SInt32		panelChanged					(Panel_Ref, Panel_Message, void*);
OSStatus	receiveViewHit					(EventHandlerCallRef, EventRef, void*);
Boolean		updateCheckBoxPreference		(MyKioskPanelUIPtr, HIViewRef);

} // anonymous namespace

@interface PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal)

- (BOOL)
readFlagForPreferenceTag:(Preferences_Tag)_
defaultValue:(BOOL)_;

- (BOOL)
writeFlag:(BOOL)_
forPreferenceTag:(Preferences_Tag)_;

@end // PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal)

#pragma mark Variables
namespace {

Float32		gIdealPanelWidth = 0.0;
Float32		gIdealPanelHeight = 0.0;

} // anonymous namespace



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
PrefPanelFullScreen_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (result != nullptr)
	{
		MyKioskPanelDataPtr		dataPtr = new MyKioskPanelData;
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorKiosk);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelKiosk);
		if (UIStrings_Copy(kUIStrings_PrefPanelFullScreenCategoryName, nameCFString).ok())
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
namespace {

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
		Boolean		showWindowFrame = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagKioskShowsWindowFrame, sizeof(showWindowFrame),
									&showWindowFrame, &actualSize))
		{
			showWindowFrame = true; // assume a default, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxShowWindowFrame, inOwningWindow), BooleanToCheckBoxValue(showWindowFrame));
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
void
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
void
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
SInt32
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
	
	case kPanel_MessageGetUsefulResizeAxes: // request for panel to return the directions in which resizing makes sense
		result = kPanel_ResponseResizeHorizontal;
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
OSStatus
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
Boolean
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
		else if (HIViewIDWrap(idMyCheckBoxShowWindowFrame) == viewID)
		{
			Preferences_SetData(kPreferences_TagKioskShowsWindowFrame,
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

} // anonymous namespace


@implementation PrefPanelFullScreen_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelFullScreenCocoa" delegate:self];
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
- (BOOL)
isForceQuitEnabled
{
	return [self readFlagForPreferenceTag:kPreferences_TagKioskAllowsForceQuit defaultValue:NO];
}
- (void)
setForceQuitEnabled:(BOOL)	aFlag
{
	BOOL	writeOK = [self writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskAllowsForceQuit];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save Force-Quit-in-full-screen preference");
	}
}// setForceQuitEnabled:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isMenuBarShownOnDemand
{
	return [self readFlagForPreferenceTag:kPreferences_TagKioskShowsMenuBar defaultValue:NO];
}
- (void)
setMenuBarShownOnDemand:(BOOL)	aFlag
{
	BOOL	writeOK = [self writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsMenuBar];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save menu-bar-on-demand preference");
	}
}// setMenuBarShownOnDemand:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isScrollBarVisible
{
	return [self readFlagForPreferenceTag:kPreferences_TagKioskShowsScrollBar defaultValue:NO];
}
- (void)
setScrollBarVisible:(BOOL)	aFlag
{
	BOOL	writeOK = [self writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsScrollBar];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save scroll-bar-in-full-screen preference");
	}
}// setScrollBarVisible:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isWindowFrameVisible
{
	return [self readFlagForPreferenceTag:kPreferences_TagKioskShowsWindowFrame defaultValue:NO];
}
- (void)
setWindowFrameVisible:(BOOL)	aFlag
{
	BOOL	writeOK = [self writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsWindowFrame];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save window-frame-in-full-screen preference");
	}
}// setWindowFrameVisible:


/*!
Accessor.

(4.1)
*/
- (BOOL)
offSwitchWindowEnabled
{
	return [self readFlagForPreferenceTag:kPreferences_TagKioskShowsOffSwitch defaultValue:NO];
}
- (void)
setOffSwitchWindowEnabled:(BOOL)	aFlag
{
	BOOL	writeOK = [self writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsOffSwitch];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save off-switch-window preference");
	}
}// setOffSwitchWindowEnabled:


/*!
Accessor.

(4.1)
*/
- (BOOL)
superfluousEffectsEnabled
{
	return [self readFlagForPreferenceTag:kPreferences_TagKioskUsesSuperfluousEffects defaultValue:NO];
}
- (void)
setSuperfluousEffectsEnabled:(BOOL)	aFlag
{
	BOOL	writeOK = [self writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskUsesSuperfluousEffects];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save full-screen-effects-enabled preference");
	}
}// setSuperfluousEffectsEnabled:


#pragma mark Panel_Delegate


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeNormal;
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
	(HelpSystem_Result)HelpSystem_DisplayHelpWithoutContext();
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)		aViewManager
containerView:(NSView*)						aContainerView
willChangeVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aContainerView, aVisibility)
}// panelViewManager:containerView:willChangeVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)		aViewManager
containerView:(NSView*)						aContainerView
didChangeVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aContainerView, aVisibility)
}// panelViewManager:containerView:didChangeVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

Not applicable to this panel because it only sets global
(Default) preferences.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
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
Returns a selector that can be sent to the first responder
in order to cause this panel to appear.  Aggregates (e.g.
a window displaying panels in tabs) should interpret this
message by bringing the appropriate panel to the front.

(4.1)
*/
- (SEL)
panelDisplayAction
{
	return @selector(performDisplayPrefPanelFullScreen:);
}// panelDisplayAction


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [NSImage imageNamed:@"IconForPrefPanelKiosk"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.FullScreen";
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
	return NSLocalizedStringFromTable(@"Full Screen", @"PrefPanelFullScreen", @"the name of this panel");
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


@end // PrefPanelFullScreen_ViewManager


@implementation PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal)


/*!
Reads a true/false value from the Default preferences context.
If the value is not found, the specified default is returned
instead.

IMPORTANT:	Only tags with values of type Boolean should be given!

(4.1)
*/
- (BOOL)
readFlagForPreferenceTag:(Preferences_Tag)	aTag
defaultValue:(BOOL)							aDefault
{
	BOOL		result = aDefault;
	Boolean		flagValue = false;
	size_t		actualSize = 0;
	
	
	if (kPreferences_ResultOK ==
		Preferences_GetData(aTag, sizeof(flagValue), &flagValue, &actualSize))
	{
		result = (flagValue) ? YES : NO;
	}
	else
	{
		result = aDefault; // assume a default, if preference can’t be found
	}
	return result;
}// readFlagForPreferenceTag:defaultValue:


/*!
Writes a true/false value to the Default preferences context and
returns true only if this succeeds.

IMPORTANT:	Only tags with values of type Boolean should be given!

(4.1)
*/
- (BOOL)
writeFlag:(BOOL)					aFlag
forPreferenceTag:(Preferences_Tag)	aTag
{
	BOOL				result = NO;
	Boolean				flagValue = (YES == aFlag) ? true : false;
	Preferences_Result	prefsResult = Preferences_SetData(aTag, sizeof(flagValue), &flagValue);
	
	
	if (kPreferences_ResultOK == prefsResult)
	{
		result = YES;
	}
	return result;
}// writeFlag:forPreferenceTag:


@end // PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
