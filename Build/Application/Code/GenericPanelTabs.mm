/*!	\file GenericPanelTabs.mm
	\brief Implements a panel that can contain others.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#import "GenericPanelTabs.h"
#import <UniversalDefines.h>

// standard-C++ includes
#import <map>
#import <stdexcept>
#import <utility>
#import <vector>

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
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>

// application includes
#import "Panel.h"
#import "PrefsWindow.h"



#pragma mark Types
namespace {

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

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void			deltaSizePanelContainerHIView		(HIViewRef, Float32, Float32, void*);
SInt32			panelChanged						(Panel_Ref, Panel_Message, void*);
OSStatus		receiveViewHit						(EventHandlerCallRef, EventRef, void*);
void			showTabPane							(My_GenericPanelTabsUIPtr, UInt16);

} // anonymous namespace



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
namespace {

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
	for (auto panelRef : this->tabPanels)
	{
		Panel_Dispose(&panelRef);
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
viewClickHandler		(HIViewGetEventTarget(this->tabView), receiveViewHit,
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
		
		
		bzero(&containerBounds, sizeof(containerBounds));
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
	
	
	for (auto tabPanel : inTabPanels)
	{
		result.push_back(new My_TabPanelView(inOwningWindow, tabPanel));
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
	for (auto viewPtr : this->tabPanePtrList)
	{
		assert(viewPtr->exists());
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
	bzero(&containerBounds, sizeof(containerBounds));
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
			
			
			for (auto viewPtr : this->tabPanePtrList)
			{
				error = HIViewSetFrame(*viewPtr, &containerFrame);
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
		for (auto viewPtr : this->tabPanePtrList)
		{
			error = HIViewAddSubview(result, *viewPtr);
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
void
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
		for (auto viewPtr : interfacePtr->tabPanePtrList)
		{
			resizer(viewPtr->panelRef);
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
SInt32
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
			
			
			for (auto panelRef : dataPtr->tabPanels)
			{
				UNUSED_RETURN(SInt32)Panel_PropagateMessage(panelRef, inMessage, inDataPtr);
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
OSStatus
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
void
showTabPane		(My_GenericPanelTabsUIPtr	inUIPtr,
				 UInt16						inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	Panel_Ref	selectedPanel = nullptr;
	
	
	if ((inTabIndex >= 1) && (inTabIndex <= inUIPtr->tabPanePtrList.size()))
	{
		selectedTabPane = *(inUIPtr->tabPanePtrList[inTabIndex - 1]);
		selectedPanel = (inUIPtr->tabPanePtrList[inTabIndex - 1])->panelRef;
		{
			UInt16		i = 1;
			
			
			for (auto viewPtr : inUIPtr->tabPanePtrList)
			{
				if (inTabIndex != i)
				{
					Boolean const	kWasVisible = HIViewIsVisible(*viewPtr);
					
					
					assert_noerr(HIViewSetVisible(*viewPtr, false/* visible */));
					if (kWasVisible)
					{
						Panel_SendMessageNewVisibility(viewPtr->panelRef, false/* visible */);
					}
				}
				++i;
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
		Panel_SendMessageNewVisibility(selectedPanel, true/* visible */);
	}
}// showTabPane

} // anonymous namespace


#pragma mark -
@implementation GenericPanelTabs_ViewManager


/*!
Designated initializer.

The array must consist of view manager objects that are
consistent with the usage of the tabs panel itself; for
instance if you intend to put the tabs into the Preferences
window then EVERY view manager in the array must support
PrefsWindow_PanelInterface; otherwise they need only
support Panel_Delegate.

(4.1)
*/
- (id)
initWithIdentifier:(NSString*)	anIdentifier
localizedName:(NSString*)		aName
localizedIcon:(NSImage*)		anImage
viewManagerArray:(NSArray*)		anArray
{
	NSDictionary*	contextDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
															anIdentifier, @"identifier",
															aName, @"localizedName",
															anImage, @"localizedIcon",
															anArray, @"viewManagerArray",
															nil];
	
	
	self = [super initWithNibNamed:@"GenericPanelTabsCocoa" delegate:self context:contextDictionary];
	if (nil != self)
	{
		for (Panel_ViewManager* viewManager in self->viewManagerArray)
		{
			viewManager.panelParent = self;
		}
	}
	return self;
}// initWithIdentifier:localizedName:localizedIcon:viewManagerArray:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	for (Panel_ViewManager* viewManager in self->viewManagerArray)
	{
		viewManager.panelParent = nil;
	}
	[identifier release];
	[localizedName release];
	[localizedIcon release];
	[viewManagerArray release];
	delete viewManagerByView;
	[super dealloc];
}// dealloc


#pragma mark NSColorPanel


/*!
Forwards this message to the active tab’s panel if the active
tab’s panel implements a "changeColor:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
	if ([self->activePanel respondsToSelector:@selector(changeColor:)])
	{
		[self->activePanel changeColor:sender];
	}
}// changeColor:


#pragma mark NSFontPanel


/*!
Forwards this message to the active tab’s panel if the active
tab’s panel implements a "changeFont:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	if ([self->activePanel respondsToSelector:@selector(changeFont:)])
	{
		[self->activePanel changeFont:sender];
	}
}// changeFont:


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


#pragma mark NSTabViewDelegate


/*!
Keeps track of the current tab so that certain messages can be
forwarded to the appropriate panel.

(4.1)
*/
- (void)
tabView:(NSTabView*)					aTabView
didSelectTabViewItem:(NSTabViewItem*)	anItem
{
#pragma unused(aTabView)
	auto	toPair = self->viewManagerByView->find([anItem view]);
	
	
	if (self->viewManagerByView->end() != toPair)
	{
		self->activePanel = toPair->second;
	}
	else
	{
		Console_Warning(Console_WriteLine, "tab view has selected item with no associated view manager");
		self->activePanel = nil;
	}
}// tabView:didSelectTabViewItem:


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
#pragma unused(aViewManager)
	// IMPORTANT: see the initializer for the construction of this dictionary and the names of keys that are used
	NSDictionary*	asDictionary = (NSDictionary*)aContext;
	NSString*		givenIdentifier = [asDictionary objectForKey:@"identifier"];
	NSString*		givenName = [asDictionary objectForKey:@"localizedName"];
	NSImage*		givenIcon = [asDictionary objectForKey:@"localizedIcon"];
	NSArray*		givenViewManagers = [asDictionary objectForKey:@"viewManagerArray"];
	
	
	self->activePanel = nil;
	self->identifier = [givenIdentifier retain];
	self->localizedName = [givenName retain];
	self->localizedIcon = [givenIcon retain];
	self->viewManagerArray = [givenViewManagers copy];
	
	// an NSView* object is not a valid key in an NSMutableDictionary
	// but it will certainly work as a key in an STL map
	self->viewManagerByView = new GenericPanelTabs_ViewManagerByView();
	for (Panel_ViewManager* viewMgr in givenViewManagers)
	{
		self->viewManagerByView->insert(std::make_pair([viewMgr managedView], viewMgr));
	}
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
	// consult all tabs
	*outEditType = kPanel_EditTypeNormal;
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		Panel_EditType		newEditType = kPanel_EditTypeNormal;
		
		
		[viewMgr.delegate panelViewManager:viewMgr requestingEditType:&newEditType];
		switch (newEditType)
		{
		case kPanel_EditTypeNormal:
			// ignore in case any panel requires inspector-mode
			break;
		case kPanel_EditTypeInspector:
		default:
			*outEditType = newEditType;
			break;
		}
	}
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
	assert(nil != tabView);
	
	// arrange to be notified of certain changes
	[tabView setDelegate:self];
	
	// create tabs for every view that was provided
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		NSTabViewItem*	tabItem = [[NSTabViewItem alloc] initWithIdentifier:[viewMgr panelIdentifier]];
		
		
		[tabItem setLabel:[viewMgr panelName]];
		[tabItem setView:[viewMgr managedView]];
		[tabItem setInitialFirstResponder:[viewMgr logicalFirstResponder]];
		
		[self->tabView addTabViewItem:tabItem];
		[tabItem release];
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
	NSRect			containerFrame = [[self managedView] frame];
	NSRect			tabContentRect = [self->tabView contentRect];
	
	
	*outIdealSize = containerFrame.size;
	
	// find ideal size after considering all tabs
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		NSSize		panelIdealSize = *outIdealSize;
		
		
		[viewMgr.delegate panelViewManager:viewMgr requestingIdealSize:&panelIdealSize];
		outIdealSize->width = MAX(panelIdealSize.width, outIdealSize->width);
		outIdealSize->height = MAX(panelIdealSize.height, outIdealSize->height);
	}
	
	// add in the additional space required by the tabs themselves
	outIdealSize->width += (NSWidth(containerFrame) - NSWidth(tabContentRect));
	outIdealSize->height += (NSHeight(containerFrame) - NSHeight(tabContentRect));
}// panelViewManager:requestingIdealSize:


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager)
	// forward to active tab
	[self->activePanel.delegate panelViewManager:self->activePanel didPerformContextSensitiveHelp:sender];
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager)
	// forward to active tab
	[self->activePanel.delegate panelViewManager:self->activePanel willChangePanelVisibility:aVisibility];
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
	// forward to active tab
	[self->activePanel.delegate panelViewManager:self->activePanel didChangePanelVisibility:aVisibility];
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

The message is forwarded to each tab’s panel in turn.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
	// forward to all tabs
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		[viewMgr.delegate panelViewManager:viewMgr didChangeFromDataSet:oldDataSet toDataSet:newDataSet];
	}
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).

The message is forwarded to each tab’s panel in turn.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView, isAccepted)
	// forward to all tabs
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		[viewMgr.delegate panelViewManager:viewMgr didFinishUsingContainerView:aContainerView userAccepted:isAccepted];
	}
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_Parent


/*!
Selects the tab with the given panel identifier, or does
nothing if the identifier does not match any tab.

Currently the animation flag has no effect.

(4.1)
*/
- (void)
panelParentDisplayChildWithIdentifier:(NSString*)	anIdentifier
withAnimation:(BOOL)								isAnimated
{
#pragma unused(isAnimated)
	// when the tab view items are created their tab view item
	// identifiers are made the same as their panel identifiers
	// so it is possible to simply ask the tab view to select
	// a tab with the given identifier (this will fail however
	// if the identifier does not match any tab)
	@try
	{
		[self->tabView selectTabViewItemWithIdentifier:anIdentifier];
	}
	@catch (NSException*	anException)
	{
		Console_Warning(Console_WriteValueCFString, "tab view was unable to display child panel with identifier", BRIDGE_CAST(anIdentifier, CFStringRef));
	}
}// panelParentDisplayChildWithIdentifier:withAnimation:


/*!
Returns an enumerator over the Panel_ViewManager* objects
for the tabs in this view.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	return [self->viewManagerArray objectEnumerator];
}// panelParentEnumerateChildViewManagers


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
	return [[self->localizedIcon retain] autorelease];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return [[self->identifier retain] autorelease];
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
	return [[self->localizedName retain] autorelease];
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
	// consult all tabs and return the tightest constraint possible
	Panel_ResizeConstraint	result = kPanel_ResizeConstraintBothAxes;
	
	
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		Panel_ResizeConstraint	panelConstraint = [viewMgr panelResizeAxes];
		
		
		switch (panelConstraint)
		{
		case kPanel_ResizeConstraintNone:
			// completely restrictive; keep
			result = panelConstraint;
			break;
		case kPanel_ResizeConstraintHorizontal:
			// partially restrictive; merge
			switch (result)
			{
			case kPanel_ResizeConstraintHorizontal:
			case kPanel_ResizeConstraintNone:
				// do not change
				break;
			case kPanel_ResizeConstraintVertical:
				// more restrictive
				result = kPanel_ResizeConstraintNone;
				break;
			case kPanel_ResizeConstraintBothAxes:
			default:
				result = panelConstraint;
				break;
			}
			break;
		case kPanel_ResizeConstraintVertical:
			// partially restrictive; merge
			switch (result)
			{
			case kPanel_ResizeConstraintVertical:
			case kPanel_ResizeConstraintNone:
				// do not change
				break;
			case kPanel_ResizeConstraintHorizontal:
				// more restrictive
				result = kPanel_ResizeConstraintNone;
				break;
			case kPanel_ResizeConstraintBothAxes:
			default:
				result = panelConstraint;
				break;
			}
			break;
		case kPanel_ResizeConstraintBothAxes:
		default:
			// not restrictive; ignore
			break;
		}
	}
	return result;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

IMPORTANT:	This interface is implemented for convenience but may
			only be used if the delegates of each view manager
			(each tab) support "PrefsWindow_PanelInterface".

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	// consult all tabs to ensure that they all agree on this
	Quills::Prefs::Class	result = Quills::Prefs::GENERAL;
	
	
	for (Panel_ViewManager* viewMgr in self->viewManagerArray)
	{
		id									panelDelegate = viewMgr.delegate;
		assert([[panelDelegate class] conformsToProtocol:@protocol(PrefsWindow_PanelInterface)]);
		id< PrefsWindow_PanelInterface >	asPrefPanel = (id< PrefsWindow_PanelInterface >)panelDelegate;
		
		
		result = [asPrefPanel preferencesClass];
	}
	return result;
}// preferencesClass


@end // GenericPanelTabs_ViewManager

// BELOW IS REQUIRED NEWLINE TO END FILE
