/*###############################################################

	PrefPanelScripts.cp
	
	MacTerm
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

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CommonEventHandlers.h>
#include <Console.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>

// application includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelScripts.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanels.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyHelpTextScriptsMenu		= { 'HTSM', 0/* ID */ };

#pragma mark Types

/*!
Implements the user interface of the panel - only
created when the panel directs for this to happen.
*/
struct MyScriptsPanelUI
{
public:
	MyScriptsPanelUI	(Panel_Ref, HIWindowRef);
	
	HIViewWrap							mainView;
	CommonEventHandlers_HIViewResizer	containerResizer;			//!< invoked when the panel is resized

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
};
typedef MyScriptsPanelUI*	MyScriptsPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct MyScriptsPanelData
{
	MyScriptsPanelData ();
	
	Panel_Ref			panel;			//!< the panel this data is for
	MyScriptsPanelUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef MyScriptsPanelData*		MyScriptsPanelDataPtr;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealPanelWidth = 0.0;
	Float32		gIdealPanelHeight = 0.0;
}

#pragma mark Internal Method Prototypes

static void				deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
static void				disposePanel					(Panel_Ref, void*);
static SInt32			panelChanged					(Panel_Ref, Panel_Message, void*);



#pragma mark Public Methods

/*!
This routine creates a new preference panel for
the Scripts category, initializes it, and returns
a reference to it.  You must destroy the reference
using Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelScripts_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (result != nullptr)
	{
		MyScriptsPanelDataPtr	dataPtr = new MyScriptsPanelData;
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorScripts);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelScripts);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowScriptsCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelScriptsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelScripts);
		Panel_SetImplementation(result, dataPtr);
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a MyScriptsPanelData structure.

(3.1)
*/
MyScriptsPanelData::
MyScriptsPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr)
{
}// MyScriptsPanelData default constructor


/*!
Initializes a MyScriptsPanelUI structure.

(3.1)
*/
MyScriptsPanelUI::
MyScriptsPanelUI	(Panel_Ref		inPanel,
					 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
mainView			(createContainerView(inPanel, inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						deltaSizePanelContainerHIView, this/* context */)
{
	assert(containerResizer.isInstalled());
}// MyScriptsPanelUI 2-argument constructor


/*!
Constructs the sub-view hierarchy for the panel,
and makes the specified container the parent.
Returns this parent.

(3.1)
*/
HIViewWrap
MyScriptsPanelUI::
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
	
	// create HIViews for the panel based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("Scripts"), inOwningWindow,
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
	
	return result;
}// MyScriptsPanelUI::createContainerSubviews


/*!
Adjusts the data browser columns in the “Scripts”
preference panel.

(3.1)
*/
static void
deltaSizePanelContainerHIView	(HIViewRef		inView,
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			UNUSED_ARGUMENT(inContext))
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		HIWindowRef const		kPanelWindow = GetControlOwner(inView);
		//MyScriptsPanelUIPtr		interfacePtr = REINTERPRET_CAST(inContext, MyScriptsPanelUIPtr);
		HIViewWrap				viewWrap;
		
		
		viewWrap = HIViewWrap(idMyHelpTextScriptsMenu, kPanelWindow);
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
static void
disposePanel	(Panel_Ref	UNUSED_ARGUMENT(inPanel),
				 void*		inDataPtr)
{
	MyScriptsPanelDataPtr	dataPtr = REINTERPRET_CAST(inDataPtr, MyScriptsPanelDataPtr);
	
	
	// destroy UI, if present
	if (nullptr != dataPtr->interfacePtr) delete dataPtr->interfacePtr;
	
	delete dataPtr, dataPtr = nullptr;
}// disposePanel


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.0)
*/
static SInt32
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorScripts, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			MyScriptsPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	MyScriptsPanelDataPtr);
			HIWindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new MyScriptsPanelUI(inPanel, *windowPtr);
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
		// do nothing
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

// BELOW IS REQUIRED NEWLINE TO END FILE
