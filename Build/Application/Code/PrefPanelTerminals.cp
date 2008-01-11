/*###############################################################

	PrefPanelTerminals.cp
	
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

// Unix includes
#include <strings.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <DialogAdjust.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>

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
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelTerminals.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Types

struct My_TerminalsPanelData
{
	Panel_Ref		panel;	//!< the panel this data is for
};
typedef struct My_TerminalsPanelData	My_TerminalsPanelData;
typedef My_TerminalsPanelData*			My_TerminalsPanelDataPtr;

#pragma mark Internal Method Prototypes

static void			createPanelControls		(Panel_Ref, WindowRef);
static void			disposePanel			(Panel_Ref, void*);
static SInt32		panelChanged			(Panel_Ref, Panel_Message, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealWidth = 0.0;
	Float32		gIdealHeight = 0.0;
}



#pragma mark Public Methods

/*!
Creates a new preference panel for the Terminals
category, initializes it, and returns a reference
to it.  You must destroy the reference using
Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelTerminals_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (result != nullptr)
	{
		My_TerminalsPanelDataPtr	dataPtr = new My_TerminalsPanelData();
		CFStringRef					nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorTerminals);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelTerminals);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowTerminalsCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelTerminalsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelTerminals);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Creates the controls that belong in the “Terminals” panel, and
attaches them to the specified owning window in the proper
embedding hierarchy.  The specified panel’s descriptor must
be "kConstantsRegistry_PrefPanelDescriptorTerminals".

The controls are not arranged or sized.

(3.1)
*/
static void
createPanelControls		(Panel_Ref		inPanel,
						 WindowRef		inOwningWindow)
{
	HIViewRef					container = nullptr;
	HIViewRef					control = nullptr;
	My_TerminalsPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), My_TerminalsPanelDataPtr);
	Rect						containerBounds;
	OSStatus					error = noErr;
	
	
	SetRect(&containerBounds, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &container);
	assert_noerr(error);
	Panel_SetContainerView(inPanel, container);
	SetControlVisibility(container, false/* visible */, false/* draw */);
	
	// INCOMPLETE
}// createPanelControls


/*!
Cleans up a panel that is about to be destroyed.

(3.1)
*/
static void
disposePanel	(Panel_Ref	UNUSED_ARGUMENT(inPanel),
				 void*		inDataPtr)
{
	My_TerminalsPanelDataPtr	dataPtr = REINTERPRET_CAST(inDataPtr, My_TerminalsPanelDataPtr);
	
	
	delete dataPtr;
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
												kConstantsRegistry_PrefPanelDescriptorTerminals, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			WindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			createPanelControls(inPanel, *windowPtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			void*	panelAuxiliaryDataPtr = inDataPtr;
			
			
			disposePanel(inPanel, panelAuxiliaryDataPtr);
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
			HIViewRef const*	controlPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// INCOMPLETE
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after control creation)
		{
			HISize&		newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((gIdealWidth != 0) && (gIdealHeight != 0))
			{
				newLimits.width = gIdealWidth;
				newLimits.height = gIdealHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate control sizes
		{
			// this notification is currently ignored, but shouldn’t be...
		}
		break;
	
	case kPanel_MessageNewDataSet:
		{
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			
			
			// UNIMPLEMENTED
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
}// panelChanged

// BELOW IS REQUIRED NEWLINE TO END FILE
