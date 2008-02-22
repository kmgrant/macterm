/*###############################################################

	GenericPanelTabs.cp
	
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

// standard-C++ includes
#include <stdexcept>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>

// MacTelnet includes
#include "GenericPanelTabs.h"



#pragma mark Types

/*!
Implements a tab based on a panel.
*/
struct My_TabPanelView:
public HIViewWrap
{
public:
	My_TabPanelView		(HIWindowRef, Panel_Ref);
	
	Panel_Ref		panelRef;

protected:
	HIViewWrap
	createPaneView	(HIWindowRef, Panel_Ref) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
};

typedef std::vector< My_TabPanelView* >		My_TabPanelViewPtrList;

/*!
Implements the entire panel user interface.
*/
struct My_GenericPanelTabsUI
{
public:
	My_GenericPanelTabsUI	(Panel_Ref, HIWindowRef, GenericPanelTabs_List const&);
	
	Float32								idealPanelWidth;		//!< metrics used to make the tabs agree
	Float32								idealPanelHeight;		//!< metrics used to make the tabs agree
	Float32								maximumTabPaneWidth;	//!< metrics used to make the tabs agree
	Float32								maximumTabPaneHeight;	//!< metrics used to make the tabs agree
	My_TabPanelViewPtrList				tabPanePtrList;
	HIViewWrap							tabView;
	HIViewWrap							mainView;
	CommonEventHandlers_HIViewResizer	containerResizer;	//!< invoked when the panel is resized
	CarbonEventHandlerWrap				viewClickHandler;	//!< invoked when a tab is clicked

protected:
	My_TabPanelViewPtrList
	createTabPanes	(HIWindowRef, GenericPanelTabs_List const&) const;
	
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	HIViewWrap
	createTabsView	(HIWindowRef);
};
typedef My_GenericPanelTabsUI*			My_GenericPanelTabsUIPtr;
typedef My_GenericPanelTabsUI const*	My_GenericPanelTabsUIConstPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_GenericPanelTabsData
{
	My_GenericPanelTabsData		(Panel_Ref, GenericPanelTabs_List const&);
	~My_GenericPanelTabsData	();
	
	Panel_Ref				containerPanel;			//!< the panel this data is for
	GenericPanelTabs_List	tabPanels;				//!< the panels used for each tab pane
	My_GenericPanelTabsUI*	interfacePtr;			//!< if not nullptr, the panel user interface is active
};
typedef My_GenericPanelTabsData*	My_GenericPanelTabsDataPtr;

#pragma mark Internal Method Prototypes

static void				deltaSizePanelContainerHIView		(HIViewRef, Float32, Float32, void*);
static SInt32			panelChanged						(Panel_Ref, Panel_Message, void*);
static pascal OSStatus	receiveViewHit						(EventHandlerCallRef, EventRef, void*);
static void				showTabPane							(My_GenericPanelTabsUIPtr, UInt16);



#pragma mark Public Methods

/*!
Creates a new preference panel that can contain any number
of unique panels, represented in a set of tabs.  Destroy the
panel using Panel_Dispose().

You assign responsibility for the panels to this module, so
DO NOT call Panel_Dispose() on the tabs yourself; that will
be done when this container is destroyed.

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
GenericPanelTabs_New	(CFStringRef					inName,
						 Panel_Kind						inKind,
						 GenericPanelTabs_List const&	inTabs)
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (nullptr != result)
	{
		My_GenericPanelTabsDataPtr	dataPtr = new My_GenericPanelTabsData(result, inTabs);
		
		
		Panel_SetKind(result, inKind);
		Panel_SetName(result, inName);
		Panel_SetImplementation(result, dataPtr);
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a My_GenericPanelTabsData structure.  The UI is
not constructed until it is needed by the user of the panel.

(3.1)
*/
My_GenericPanelTabsData::
My_GenericPanelTabsData		(Panel_Ref						inPanel,
							 GenericPanelTabs_List const&	inTabPanels)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
containerPanel(inPanel),
tabPanels(inTabPanels),
interfacePtr(nullptr)
{
	if (inTabPanels.size() < 2) throw std::invalid_argument("panel requires at least 2 tabs");
}// My_GenericPanelTabsData default constructor


/*!
Tears down a My_GenericPanelTabsData structure.

(3.1)
*/
My_GenericPanelTabsData::
~My_GenericPanelTabsData ()
{
	for (GenericPanelTabs_List::iterator toPanel = this->tabPanels.begin();
			toPanel != this->tabPanels.end(); ++toPanel)
	{
		Panel_Dispose(&*toPanel);
	}
}// My_GenericPanelTabsData destructor


/*!
Initializes a My_GenericPanelTabsUI structure.

(3.1)
*/
My_GenericPanelTabsUI::
My_GenericPanelTabsUI	(Panel_Ref						inPanel,
						 HIWindowRef					inOwningWindow,
						 GenericPanelTabs_List const&	inTabList)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.,
idealPanelWidth			(0.0),
idealPanelHeight		(0.0),
maximumTabPaneWidth		(0.0),
maximumTabPaneHeight	(0.0),
tabPanePtrList			(createTabPanes(inOwningWindow, inTabList)),
tabView					(createTabsView(inOwningWindow)
							<< HIViewWrap_AssertExists),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
									kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
							deltaSizePanelContainerHIView, this/* context */),
viewClickHandler		(GetControlEventTarget(this->tabView), receiveViewHit,
							CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
							this/* user data */)
{
	assert(containerResizer.isInstalled());
	assert(viewClickHandler.isInstalled());
}// My_GenericPanelTabsUI 2-argument constructor


/*!
Constructs the container for the panel.  Assumes that
the tabs view already exists.

(3.1)
*/
HIViewWrap
My_GenericPanelTabsUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
{
	assert(this->tabView.exists());
	assert(0 != this->maximumTabPaneWidth);
	assert(0 != this->maximumTabPaneHeight);
	
	HIViewRef	result = nullptr;
	OSStatus	error = noErr;
	
	
	// create the container
	{
		Rect	containerBounds;
		
		
		SetRect(&containerBounds, 0, 0, 0, 0);
		error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &result);
		assert_noerr(error);
		Panel_SetContainerView(inPanel, result);
		SetControlVisibility(result, false/* visible */, false/* draw */);
	}
	
	// calculate the ideal size
	{
		Point		tabFrameTopLeft;
		Point		tabFrameWidthHeight;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// calculate initial frame and pane offsets (ignore width/height)
		Panel_CalculateTabFrame(result, &tabFrameTopLeft, &tabFrameWidthHeight);
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		this->idealPanelWidth = tabFrameTopLeft.h + tabPaneTopLeft.h + this->maximumTabPaneWidth +
									tabPaneBottomRight.h + tabFrameTopLeft.h/* right is same as left */;
		this->idealPanelHeight = tabFrameTopLeft.v + tabPaneTopLeft.v + this->maximumTabPaneHeight +
									tabPaneBottomRight.v + tabFrameTopLeft.v/* bottom is same as top */;
		
		// make the container big enough for the tabs
		{
			HIRect		containerFrame = CGRectMake(0, 0, this->idealPanelWidth, this->idealPanelHeight);
			
			
			error = HIViewSetFrame(result, &containerFrame);
			assert_noerr(error);
		}
		
		// recalculate the frame, this time the width and height will be correct
		Panel_CalculateTabFrame(result, &tabFrameTopLeft, &tabFrameWidthHeight);
		
		// make the tabs match the ideal frame, because the size
		// and position of NIB views is used to size subviews
		// and subview resizing deltas are derived directly from
		// changes to the container view size
		{
			HIRect		containerFrame = CGRectMake(tabFrameTopLeft.h, tabFrameTopLeft.v,
													tabFrameWidthHeight.h, tabFrameWidthHeight.v);
			
			
			error = HIViewSetFrame(this->tabView, &containerFrame);
			assert_noerr(error);
		}
	}
	
	// embed the tabs in the content pane; done at this stage
	// (after positioning the tabs) just in case the origin
	// of the container is not (0, 0); this prevents the tabs
	// from having to know where they will end up in the window
	error = HIViewAddSubview(result, this->tabView);
	assert_noerr(error);
	
	return result;
}// My_GenericPanelTabsUI::createContainerView


/*!
Creates a list of wrappers for each tab pane.

(3.1)
*/
My_TabPanelViewPtrList
My_GenericPanelTabsUI::
createTabPanes	(HIWindowRef					inOwningWindow,
				 GenericPanelTabs_List const&	inTabPanels)
const
{
	My_TabPanelViewPtrList		result;
	
	
	for (GenericPanelTabs_List::const_iterator toPanel = inTabPanels.begin();
			toPanel != inTabPanels.end(); ++toPanel)
	{
		result.push_back(new My_TabPanelView(inOwningWindow, *toPanel));
	}
	return result;
}// My_GenericPanelTabsUI::createTabPanes


/*!
Constructs the container for all panel tab panes.
Assumes that all tabs already exist.

(3.1)
*/
HIViewWrap
My_GenericPanelTabsUI::
createTabsView	(HIWindowRef	inOwningWindow)
{
	for (My_TabPanelViewPtrList::const_iterator toViewPtr = this->tabPanePtrList.begin();
			toViewPtr != this->tabPanePtrList.end(); ++toViewPtr)
	{
		assert((*toViewPtr)->exists());
	}
	
	HIViewRef			result = nullptr;
	Rect				containerBounds;
	ControlTabEntry		tabInfo[this->tabPanePtrList.size()];
	OSStatus			error = noErr;
	
	
	// nullify or zero-fill everything, then set only what matters
	bzero(&tabInfo, sizeof(tabInfo));
	for (UInt16 i = 0; i < this->tabPanePtrList.size(); ++i)
	{
		tabInfo[i].enabled = true;
		Panel_GetName(this->tabPanePtrList[i]->panelRef, tabInfo[i].name);
	}
	SetRect(&containerBounds, 0, 0, 0, 0);
	error = CreateTabsControl(inOwningWindow, &containerBounds, kControlTabSizeLarge, kControlTabDirectionNorth,
								sizeof(tabInfo) / sizeof(ControlTabEntry)/* number of tabs */, tabInfo,
								&result);
	assert_noerr(error);
	
	// calculate the largest area required for all tabs, and keep this as the ideal size
	{
		Point	tabPaneTopLeft;
		Point	tabPaneBottomRight;
		
		
		// also include pane margin in panel size
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		// find the widest tab and the highest tab
		if (1 == this->tabPanePtrList.size())
		{
			// in the odd case of one tab, the maximum size is obvious...
			Rect	tabSize;
			
			
			GetControlBounds(*(this->tabPanePtrList[0]), &tabSize);
			this->maximumTabPaneWidth = tabSize.right - tabSize.left;
			this->maximumTabPaneHeight = tabSize.bottom - tabSize.top;
		}
		else
		{
			Rect	tabSizeA;
			Rect	tabSizeB;
			
			
			// find the maximum size occupied by any of the tabs; check as few as possible
			GetControlBounds(*(this->tabPanePtrList[0]), &tabSizeA);
			GetControlBounds(*(this->tabPanePtrList[1]), &tabSizeB);
			this->maximumTabPaneWidth = std::max(tabSizeA.right - tabSizeA.left, tabSizeB.right - tabSizeB.left);
			this->maximumTabPaneHeight = std::max(tabSizeA.bottom - tabSizeA.top, tabSizeB.bottom - tabSizeB.top);
			if (this->tabPanePtrList.size() > 2)
			{
				// factor in the remaining tabs
				for (My_TabPanelViewPtrList::size_type i = 2; i < this->tabPanePtrList.size(); ++i)
				{
					GetControlBounds(*(this->tabPanePtrList[i]), &tabSizeA);
					this->maximumTabPaneWidth = std::max(tabSizeA.right - tabSizeA.left, STATIC_CAST(this->maximumTabPaneWidth, int));
					this->maximumTabPaneHeight = std::max(tabSizeA.bottom - tabSizeA.top, STATIC_CAST(this->maximumTabPaneHeight, int));
				}
			}
		}
		
		// make every tab pane match the ideal pane size
		{
			HIRect		containerFrame = CGRectMake(tabPaneTopLeft.h, tabPaneTopLeft.v,
													this->maximumTabPaneWidth, this->maximumTabPaneHeight);
			
			
			for (My_TabPanelViewPtrList::const_iterator toViewPtr = this->tabPanePtrList.begin();
					toViewPtr != this->tabPanePtrList.end(); ++toViewPtr)
			{
				error = HIViewSetFrame(*(*toViewPtr), &containerFrame);
				assert_noerr(error);
			}
		}
		
		// make the tabs big enough for any of the panes
		{
			HIRect		containerFrame = CGRectMake(0, 0,
													tabPaneTopLeft.h + this->maximumTabPaneWidth + tabPaneBottomRight.h,
													tabPaneTopLeft.v + this->maximumTabPaneHeight + tabPaneBottomRight.v);
			
			
			error = HIViewSetFrame(result, &containerFrame);
			assert_noerr(error);
		}
		
		// embed every tab pane in the tabs view; done at this stage
		// so that the subsequent move of the tab frame (later) will
		// also offset the embedded panes; if embedding is done too
		// soon, then the panes have to know too much about where
		// they will physically reside within the window content area
		for (My_TabPanelViewPtrList::const_iterator toViewPtr = this->tabPanePtrList.begin();
				toViewPtr != this->tabPanePtrList.end(); ++toViewPtr)
		{
			error = HIViewAddSubview(result, *(*toViewPtr));
			assert_noerr(error);
		}
	}
	
	return result;
}// My_GenericPanelTabsUI::createTabsView


/*!
Initializes a My_GenericPanelTab structure.

(3.1)
*/
My_TabPanelView::
My_TabPanelView		(HIWindowRef	inOwningWindow,
					 Panel_Ref		inPanel)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap			(createPaneView(inOwningWindow, inPanel)
						<< HIViewWrap_AssertExists),
panelRef			(inPanel),
containerResizer	(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
							kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
						My_TabPanelView::deltaSize, this/* context */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_TabPanelView 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_TabPanelView::
createPaneView		(HIWindowRef	inOwningWindow,
					 Panel_Ref		inPanel)
const
{
	HIViewRef	result = nullptr;
	OSStatus	error = noErr;
	
	
	// create the tab pane
	Panel_SendMessageCreateViews(inPanel, inOwningWindow);
	Panel_GetContainerView(inPanel, result);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HISize		idealSize;
		
		
		if (kPanel_ResponseSizeProvided == Panel_SendMessageGetIdealSize(inPanel, idealSize))
		{
			HIRect		containerFrame = CGRectMake(0, 0, idealSize.width, idealSize.height);
			
			
			error = HIViewSetFrame(result, &containerFrame);
			assert_noerr(error);
		}
	}
	return result;
}// My_TabPanelView::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_TabPanelView::
deltaSize	(HIViewRef		UNUSED_ARGUMENT(inContainer),
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	// nothing needed
}// My_TabPanelView::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_TabPanelView::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_TabPanelView::operator =


/*!
Adjusts the controls in the “General” preference panel
to match the specified change in dimensions of its
container. 

(3.1)
*/
static void
deltaSizePanelContainerHIView	(HIViewRef		UNUSED_ARGUMENT(inView),
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		//HIWindowRef					kPanelWindow = HIViewGetWindow(inView);
		My_GenericPanelTabsUIPtr	interfacePtr = REINTERPRET_CAST(inContext, My_GenericPanelTabsUIPtr);
		assert(nullptr != interfacePtr);
		
		
		interfacePtr->tabView << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		
		Panel_Resizer	resizer(inDeltaX, inDeltaY, true/* is delta */);
		for (My_TabPanelViewPtrList::iterator toViewPtr = interfacePtr->tabPanePtrList.begin();
				toViewPtr != interfacePtr->tabPanePtrList.end(); ++toViewPtr)
		{
			resizer((*toViewPtr)->panelRef);
		}
	}
}// deltaSizePanelContainerHIView


/*!
This routine, of standard PanelChangedProcPtr form, is invoked
by the dialog using this panel, and by the Panel module itself.
The change could be a request for information, or a general
notification.

The message is usually propagated to all tab panels.  The
exceptions are things like kPanel_MessageGetIdealSize(), which
can be answered accurately by this generic parent panel.

(3.1)
*/
static SInt32
panelChanged	(Panel_Ref			inPanel,
				 Panel_Message		inMessage,
				 void*				inDataPtr)
{
	SInt32		result = 0L;
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_GenericPanelTabsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_GenericPanelTabsDataPtr);
			HIWindowRef const*			windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_GenericPanelTabsUI(inPanel, *windowPtr, panelDataPtr->tabPanels);
			assert(nullptr != panelDataPtr->interfacePtr);
			showTabPane(panelDataPtr->interfacePtr, 1/* tab index */);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_GenericPanelTabsDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, My_GenericPanelTabsDataPtr);
			
			
			// destroy UI, if present
			if (nullptr != dataPtr->interfacePtr) delete dataPtr->interfacePtr;
			
			delete dataPtr;
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		// assert that all panels have the same editing style (i.e. all-inspectors or
		// all-modeless); it does not make sense for them to coexist in the same dialog
		// if they behave differently
		{
			My_GenericPanelTabsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_GenericPanelTabsDataPtr);
			SInt32						editType = Panel_SendMessageGetEditType(panelDataPtr->tabPanels[0]);
			
			
			for (GenericPanelTabs_List::size_type i = 1; i < panelDataPtr->tabPanels.size(); ++i)
			{
				if (Panel_SendMessageGetEditType(panelDataPtr->tabPanels[i]) != editType)
				{
					assert(false && "tab panels do not all report the same edit type");
				}
			}
			result = editType;
		}
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after control creation)
		{
			My_GenericPanelTabsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_GenericPanelTabsDataPtr);
			HISize&						newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != panelDataPtr->interfacePtr->idealPanelWidth) && (0 != panelDataPtr->interfacePtr->idealPanelHeight))
			{
				newLimits.width = panelDataPtr->interfacePtr->idealPanelWidth;
				newLimits.height = panelDataPtr->interfacePtr->idealPanelHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageGetGrowBoxLook: // request for panel to return its preferred appearance for the window grow box
		result = kPanel_ResponseGrowBoxTransparent;
		break;
	
	case kPanel_MessageFocusGained: // notification that a control is now focused
	case kPanel_MessageFocusLost: // notification that a control is no longer focused
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate control sizes
	case kPanel_MessageNewDataSet: // (inspectors only) a new data set has been selected externally, the panel should update its views
	case kPanel_MessageNewVisibility: // visible state of the panel’s container has changed to visible (true) or invisible (false)
	default:
		// propagate
		{
			My_GenericPanelTabsDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_GenericPanelTabsDataPtr);
			
			
			for (GenericPanelTabs_List::iterator toPanel = dataPtr->tabPanels.begin();
					toPanel != dataPtr->tabPanels.end(); ++toPanel)
			{
				(SInt32)Panel_PropagateMessage(*toPanel, inMessage, inDataPtr);
			}
		}
		break;
	}
	
	return result;
}// panelChanged


/*!
Handles "kEventControlClick" of "kEventClassControl"
for this panel.  Responds by changing the currently
selected tab.

(3.1)
*/
static pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyGenericTabsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_GenericPanelTabsUIPtr	interfacePtr = REINTERPRET_CAST(inMyGenericTabsPanelUIPtr, My_GenericPanelTabsUIPtr);
	assert(nullptr != interfacePtr);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
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
			
			if (view == interfacePtr->tabView)
			{
				showTabPane(interfacePtr, GetControl32BitValue(view));
				result = noErr; // completely handled
			}
		}
	}
	
	return result;
}// receiveViewHit


/*!
Displays the specified tab pane and hides the others.

(3.1)
*/
static void
showTabPane		(My_GenericPanelTabsUIPtr	inUIPtr,
				 UInt16						inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	
	
	if ((inTabIndex >= 1) && (inTabIndex <= inUIPtr->tabPanePtrList.size()))
	{
		selectedTabPane = *(inUIPtr->tabPanePtrList[inTabIndex - 1]);
		{
			UInt16		i = 1;
			
			
			for (My_TabPanelViewPtrList::iterator toViewPtr = inUIPtr->tabPanePtrList.begin();
					toViewPtr != inUIPtr->tabPanePtrList.end(); ++toViewPtr, ++i)
			{
				if (inTabIndex != i) assert_noerr(HIViewSetVisible(*(*toViewPtr), false/* visible */));
			}
		}
	}
	else
	{
		assert(false && "not a recognized tab index");
	}
	
	if (nullptr != selectedTabPane)
	{
		assert_noerr(HIViewSetVisible(selectedTabPane, true/* visible */));
	}
}// showTabPane

// BELOW IS REQUIRED NEWLINE TO END FILE
