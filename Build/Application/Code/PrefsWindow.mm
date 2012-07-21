/*!	\file PrefsWindow.mm
	\brief Implements the shell of the Preferences window.
	
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

#import "PrefsWindow.h"
#import <UniversalDefines.h>

// standard-C++ includes
#import <algorithm>
#import <functional>
#import <vector>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <Cursors.h>
#import <DialogAdjust.h>
#import <Embedding.h>
#import <FileSelectionDialogs.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <IconManager.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <SoundSystem.h>
#import <WindowInfo.h>

// resource includes
#import "SpacingConstants.r"

// application includes
#import "AppResources.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "FileUtilities.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "PrefPanelFormats.h"
#import "PrefPanelFullScreen.h"
#import "PrefPanelGeneral.h"
#import "PrefPanelMacros.h"
#import "PrefPanelSessions.h"
#import "PrefPanelTerminals.h"
#import "PrefPanelTranslations.h"
#import "PrefPanelWorkspaces.h"
#import "QuillsPrefs.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Window" NIB from the package "PrefsWindow.nib".
*/
HIViewID const		idMyUserPaneAnyPrefPanel		= { 'Panl', 0/* ID */ };
HIViewID const		idMyUserPaneFooter				= { 'Foot', 0/* ID */ };
HIViewID const		idMySeparatorBottom				= { 'BSep', 0/* ID */ };
HIViewID const		idMyButtonHelp					= { 'Help', 0/* ID */ };

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Drawer" NIB from the package "PrefsWindow.nib".
*/
HIViewID const		idMyStaticTextListTitle			= { 'Titl', 0/* ID */ };
HIViewID const		idMyDataBrowserCollections		= { 'Favs', 0/* ID */ };
HIViewID const		idMyButtonAddCollection			= { 'NewC', 0/* ID */ };
HIViewID const		idMyButtonRemoveCollection		= { 'DelC', 0/* ID */ };
HIViewID const		idMyButtonMoveCollectionUp		= { 'MvUC', 0/* ID */ };
HIViewID const		idMyButtonMoveCollectionDown	= { 'MvDC', 0/* ID */ };
HIViewID const		idMyButtonManipulateCollection	= { 'MnpC', 0/* ID */ };
FourCharCode const	kMyDataBrowserPropertyIDSets	= 'Sets';

/*!
The data for "kMy_PrefsWindowSourceListDataType" should be an
NSKeyedArchive of an NSIndexSet object (dragged row index).
*/
const NSString*		kMy_PrefsWindowSourceListDataType = @"net.macterm.MacTerm.prefswindow.sourcelistdata";

} // anonymous namespace

#pragma mark Types
namespace {

struct MyPanelData
{
	inline MyPanelData		(Panel_Ref, SInt16);
	
	Panel_Ref			panel;					//!< the panel that will be displayed when selected by the user
	SInt16				listIndex;				//!< row index of panel in the panel choice list
	DataBrowserItemID	recentCollection;		//!< not all panels need this; recent selection in collections drawer
};
typedef MyPanelData*		MyPanelDataPtr;

typedef std::vector< MyPanelDataPtr >		MyPanelDataList;
typedef std::vector< HIToolbarItemRef >		CategoryToolbarItems;
typedef std::map< UInt32, SInt16 >			IndexByCommandID;

} // anonymous namespace

/*!
This class represents a particular preferences context
in the source list.
*/
@interface PrefsWindow_Collection : NSObject
{
@private
	Preferences_ContextRef		preferencesContext;
	BOOL						isDefault;
}

// initializers

- (id)
initWithPreferencesContext:(Preferences_ContextRef)_
asDefault:(BOOL)_;

// accessors

- (NSString*)
boundName;
- (void)
setBoundName:(NSString*)_; // binding

- (NSString*)
description;
- (void)
setDescription:(NSString*)_; // binding

- (BOOL)
isEditable; // binding

- (Preferences_ContextRef)
preferencesContext;

// NSObject

- (unsigned int)
hash;

- (BOOL)
isEqual:(id)_;

@end // PrefsWindow_Collection

#pragma mark Internal Method Prototypes
namespace {

OSStatus				accessDataBrowserItemData		(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
														 DataBrowserItemDataRef, Boolean);
void					chooseContext					(Preferences_ContextRef);
void					choosePanel						(UInt16);
Boolean					createCollection				(CFStringRef = nullptr);
void					displayCollectionRenameUI		(DataBrowserItemID);
Boolean					duplicateCollection				(Preferences_ContextRef);
void					findBestPanelSize				(HISize const&, HISize&);
void					handleNewDrawerWindowSize		(WindowRef, Float32, Float32, void*);
void					handleNewFooterSize				(HIViewRef, Float32, Float32, void*);
void					handleNewMainWindowSize			(WindowRef, Float32, Float32, void*);
void					init							();
void					installPanel					(Panel_Ref);
void					monitorDataBrowserItems			(ControlRef, DataBrowserItemID, DataBrowserItemNotification);
void					navigationExportPrefsDialogEvent(NavEventCallbackMessage, NavCBRecPtr, void*);
void					newPanelSelector				(Panel_Ref);
void					preferenceChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					rebuildList						();
OSStatus				receiveFooterActiveStateChange	(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveFooterDraw				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveHICommand				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowClosing			(EventHandlerCallRef, EventRef, void*);
void					refreshDisplay					();
void					removeCollectionRenameUI		();
Quills::Prefs::Class	returnCurrentPreferencesClass	();
DataBrowserItemID		returnCurrentSelection			();
OSStatus				selectCollection				(DataBrowserItemID = 0);
void					showWindow						();
void					sizePanels						(HISize const&);

} // anonymous namespace

@interface PrefsWindow_Controller (PrefsWindow_ControllerInternal)

// new methods

- (Quills::Prefs::Class)
currentPreferencesClass;

- (Preferences_ContextRef)
currentPreferencesContext;

- (void)
didEndExportPanel:(NSSavePanel*)_
returnCode:(int)_
contextInfo:(void*)_;

- (void)
didEndImportPanel:(NSOpenPanel*)_
returnCode:(int)_
contextInfo:(void*)_;

- (void)
displayPanel:(Panel_ViewManager*)_
withAnimation:(BOOL)_;

- (void)
rebuildSourceList;

- (void)
updateUserInterfaceForSourceListTransition:(id)_;

// actions

- (void)
performDisplayPrefPanelFormats:(id)_;

- (void)
performDisplayPrefPanelFullScreen:(id)_;

- (void)
performDisplayPrefPanelGeneral:(id)_;

- (void)
performDisplayPrefPanelTranslations:(id)_;

@end // PrefsWindow_Controller (PrefsWindow_ControllerInternal)

#pragma mark Variables
namespace {

Panel_Ref								gCurrentPanel = nullptr;
WindowInfo_Ref							gPreferencesWindowInfo = nullptr;
WindowRef								gPreferencesWindow = nullptr; // the Mac OS window pointer
WindowRef								gDrawerWindow = nullptr; // the Mac OS window pointer
CommonEventHandlers_HIViewResizer		gPreferencesWindowFooterResizeHandler;
CommonEventHandlers_WindowResizer		gPreferencesWindowResizeHandler;
CommonEventHandlers_WindowResizer		gDrawerWindowResizeHandler;
CarbonEventHandlerWrap					gPrefsCommandHandler(GetApplicationEventTarget(),
																receiveHICommand,
																CarbonEventSetInClass
																	(CarbonEventClass(kEventClassCommand),
																		kEventCommandProcess),
																nullptr/* user data */);
Console_Assertion						_1(gPrefsCommandHandler.isInstalled(), __FILE__, __LINE__);
CarbonEventHandlerWrap					gPrefsFooterActiveStateHandler;
CarbonEventHandlerWrap					gPrefsFooterDrawHandler;
EventHandlerUPP							gPreferencesWindowClosingUPP = nullptr;
EventHandlerRef							gPreferencesWindowClosingHandler = nullptr;
ListenerModel_ListenerRef				gPreferenceChangeEventListener = nullptr;
HISize									gMaximumWindowSize = CGSizeMake(600, 400); // arbitrary; overridden later
HIViewWrap								gDataBrowserTitle;
HIViewWrap								gDataBrowserForCollections;
HIViewWrap								gCollectionAddMenuButton;
HIViewWrap								gCollectionRemoveButton;
HIViewWrap								gCollectionMoveUpButton;
HIViewWrap								gCollectionMoveDownButton;
HIViewWrap								gCollectionManipulateMenuButton;
SInt16									gPanelChoiceListLastRowIndex = -1;
MyPanelDataList&						gPanelList ()	{ static MyPanelDataList x; return x; }
CategoryToolbarItems&					gCategoryToolbarItems ()	{ static CategoryToolbarItems x; return x; }
IndexByCommandID&						gIndicesByCommandID ()		{ static IndexByCommandID x; return x; }
Preferences_ContextRef					gCurrentDataSet = nullptr;
Float32									gBottomMargin = 0;

} // anonymous namespace


#pragma mark Functors

/*!
This is a functor that determines if a given panel
data structure is intended for a particular panel.

Model of STL Predicate.

(3.0)
*/
class isPanelDataForSpecificPanel:
public std::unary_function< MyPanelDataPtr/* argument */, bool/* return */ >
{
public:
	isPanelDataForSpecificPanel	(Panel_Ref	inPanel)
	: _panel(inPanel)
	{
	}
	
	bool
	operator ()	(MyPanelDataPtr		inPanelDataPtr)
	{
		return (inPanelDataPtr->panel == _panel);
	}

protected:

private:
	Panel_Ref	_panel;
};

/*!
Finds a panel’s ideal size.  If invoked on several
panels successively, the cumulative maximum is stored.

Model of STL Unary Function.

(3.1)
*/
class panelFindIdealSize:
public std::unary_function< MyPanelDataPtr/* argument */, void/* return */ >
{
public:
	panelFindIdealSize ()
	: idealSize(CGSizeMake(0, 0))
	{
	}
	
	void
	operator ()	(MyPanelDataPtr		inPanelDataPtr)
	{
		HISize		panelIdealSize = CGSizeMake(0, 0);
		
		
		if (kPanel_ResponseSizeProvided == Panel_SendMessageGetIdealSize(inPanelDataPtr->panel, panelIdealSize))
		{
			this->idealSize.width = std::max(panelIdealSize.width, idealSize.width);
			this->idealSize.height = std::max(panelIdealSize.height, idealSize.height);
		}
	}
	
	HISize		idealSize;

protected:

private:
};

/*!
Resizes a panel, given its internal data structure.

Model of STL Unary Function.

(3.1)
*/
class panelResizer:
public std::unary_function< MyPanelDataPtr/* argument */, void/* return */ >
{
public:
	panelResizer	(Float32	inHorizontal,
					 Float32	inVertical,
					 Boolean	inIsDelta)
	: _horizontal(inHorizontal), _vertical(inVertical), _delta(inIsDelta)
	{
	}
	
	void
	operator ()	(MyPanelDataPtr		inPanelDataPtr)
	{
		Panel_Resizer	resizer(_horizontal, _vertical, _delta);
		
		
		resizer(inPanelDataPtr->panel);
	}

protected:

private:
	Float32		_horizontal;
	Float32		_vertical;
	Boolean		_delta;
};


#pragma mark Public Methods

/*!
Call this method in the exit routine of the program
to ensure that this window’s memory allocations
get destroyed.  You can also call this method after
you are done using this window, but then a call to
PrefsWindow_Display() will re-initialize the window
before redisplaying it.

(3.0)
*/
void
PrefsWindow_Done ()
{
	PrefsWindow_Remove(); // saves preferences
	
	if (nullptr != gPreferencesWindow)
	{
		MyPanelDataList::iterator		panelDataIterator;
		CategoryToolbarItems::iterator	toolbarItemIterator;
		
		
		// clean up the Help System
		HelpSystem_SetWindowKeyPhrase(gPreferencesWindow, kHelpSystem_KeyPhraseDefault);
		
		// disable preferences callbacks
		Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts);
		Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeContextName);
		ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
		
		// disable event callbacks and destroy the window
		RemoveEventHandler(gPreferencesWindowClosingHandler), gPreferencesWindowClosingHandler = nullptr;
		DisposeEventHandlerUPP(gPreferencesWindowClosingUPP), gPreferencesWindowClosingUPP = nullptr;
		for (panelDataIterator = gPanelList().begin(); panelDataIterator != gPanelList().end(); ++panelDataIterator)
		{
			// dispose each panel
			MyPanelDataPtr		dataPtr = *panelDataIterator;
			if (nullptr != dataPtr)
			{
				Panel_Dispose(&dataPtr->panel);
				delete dataPtr, dataPtr = nullptr;
			}
		}
		for (toolbarItemIterator = gCategoryToolbarItems().begin();
				toolbarItemIterator != gCategoryToolbarItems().end(); ++toolbarItemIterator)
		{
			// release each item
			if (nullptr != *toolbarItemIterator) CFRelease(*toolbarItemIterator);
		}
		(OSStatus)SetDrawerParent(gDrawerWindow, nullptr/* parent */);
		DisposeWindow(gDrawerWindow), gDrawerWindow = nullptr;
		DisposeWindow(gPreferencesWindow), gPreferencesWindow = nullptr; // automatically destroys all controls
		WindowInfo_Dispose(gPreferencesWindowInfo);
	}
}// Done


/*!
Use this method to display the preferences window.
If the window is not yet in memory, it is created.

(3.1)
*/
void
PrefsWindow_Display ()
{
	showWindow();
	// if no panel has ever been highlighted, choose the first one
	assert(false == gPanelList().empty());
	if (nullptr == gCurrentPanel)
	{
		choosePanel(0);
	}
}// Display


/*!
Use this method to hide the preferences window, which
will cause all changes to be saved.

The window remains in memory and can be re-displayed
using PrefsWindow_Display().  To destroy the window,
use the method PrefsWindow_Done().

(3.0)
*/
void
PrefsWindow_Remove ()
{
	if (nullptr != gPreferencesWindow)
	{
		HIViewRef	focusControl = nullptr;
		
		
		if (GetKeyboardFocus(gPreferencesWindow, &focusControl) == noErr)
		{
			// if the user is editing a text field, this makes sure the changes “stick”
			if (nullptr != focusControl) Panel_SendMessageFocusLost(gCurrentPanel, focusControl);
		}
	}
	
	// write all the preference data in memory to disk
	(Preferences_Result)Preferences_Save();
	
	if (nullptr != gPreferencesWindow)
	{
		HideWindow(gPreferencesWindow);
	}
}// Remove


#pragma mark Internal Methods
namespace {

/*!
Initializes a MyPanelData structure.

(3.1)
*/
MyPanelData::
MyPanelData		(Panel_Ref	inPanel,
				 SInt16		inListIndex)
:
panel(inPanel),
listIndex(inListIndex),
recentCollection(0)
{
}// MyPanelData 2-argument constructor


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
OSStatus
accessDataBrowserItemData	(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus				result = noErr;
	Preferences_ContextRef	context = REINTERPRET_CAST(inItemID, Preferences_ContextRef);
	
	
	assert(gDataBrowserForCollections.operator HIViewRef() == inDataBrowser);
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kDataBrowserItemIsEditableProperty:
			{
				DataBrowserTableViewRowIndex	rowIndex = 0;
				OSStatus						error = noErr;
				
				
				result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
				
				// the Default item is in the first row and cannot be renamed
				error = GetDataBrowserTableViewItemRow(inDataBrowser, inItemID, &rowIndex);
				if (noErr == error)
				{
					if (0 == rowIndex)
					{
						result = SetDataBrowserItemDataBooleanValue(inItemData, false/* is editable */);
					}
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDSets:
			// return the name of the collection
			{
				Preferences_Result		prefsResult = kPreferences_ResultOK;
				CFStringRef				contextName = nullptr;
				
				
				prefsResult = Preferences_ContextGetName(context, contextName);
				if (kPreferences_ResultOK != prefsResult)
				{
					result = paramErr;
				}
				else
				{
					result = SetDataBrowserItemDataText(inItemData, contextName);
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
		case kMyDataBrowserPropertyIDSets:
			// user has changed the collection name; update preferences
			{
				CFStringRef		newName = nullptr;
				
				
				result = GetDataBrowserItemDataText(inItemData, &newName);
				if (noErr == result)
				{
					if (nullptr == newName)
					{
						result = paramErr;
					}
					else
					{
						Preferences_Result		prefsResult = kPreferences_ResultOK;
						
						
						prefsResult = Preferences_ContextRename(context, newName);
						if (kPreferences_ResultOK != prefsResult)
						{
							result = paramErr;
						}
						else
						{
							result = noErr;
						}
					}
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
Sends a panel event indicating that the current
global context (if any) is being swapped out for
the specified context.  Then, sets the global
context to the given context.

Pass nullptr to cause a full reset, a “select
nothing” event.

(3.1)
*/
void
chooseContext	(Preferences_ContextRef		inContext)
{
	Panel_DataSetTransition		setChange;
	
	
	bzero(&setChange, sizeof(setChange));
	setChange.oldDataSet = gCurrentDataSet;
	setChange.newDataSet = inContext;
	Panel_SendMessageNewDataSet(gCurrentPanel, setChange);
	gCurrentDataSet = inContext;
}// chooseContext


/*!
Displays the panel with the specified index in the
main list.  0 indicates the first panel, etc.

WARNING:	No boundary checking is done.  Pass in
			a valid index, which should be between
			0 and the size of the global panel list.

(3.1)
*/
void
choosePanel		(UInt16		inZeroBasedPanelNumber)
{
	MyPanelDataPtr		newPanelDataPtr = nullptr;
	
	
	// unhighlight all, then highlight the new one; only possible on Tiger or later
	if (FlagManager_Test(kFlagOS10_4API))
	{
		CategoryToolbarItems::const_iterator	toItem = gCategoryToolbarItems().begin();
		CategoryToolbarItems::const_iterator	itemEnd = gCategoryToolbarItems().end();
		
		
		assert(false == gCategoryToolbarItems().empty());
		for (; toItem != itemEnd; ++toItem)
		{
			(OSStatus)HIToolbarItemChangeAttributes(*toItem, 0/* attributes to set */,
													FUTURE_SYMBOL(1 << 7, kHIToolbarItemSelected)/* attributes to clear */);
		}
		(OSStatus)HIToolbarItemChangeAttributes(gCategoryToolbarItems()[inZeroBasedPanelNumber],
												FUTURE_SYMBOL(1 << 7, kHIToolbarItemSelected)/* attributes to set */,
												0/* attributes to clear */);
	}
	
	// get the selected item’s data (which ought to always be defined)
	newPanelDataPtr = gPanelList()[inZeroBasedPanelNumber];
	if ((nullptr != newPanelDataPtr) && (newPanelDataPtr->panel != gCurrentPanel))
	{
		HIViewRef	nowVisibleContainer = nullptr;
		HIViewRef	nowInvisibleContainer = nullptr;
		
		
		// note which panel was displayed before (if any)
		{
			// this panel’s button is currently selected, so this is the panel that will disappear
			if (nullptr != gCurrentPanel)
			{
				Panel_GetContainerView(gCurrentPanel, nowInvisibleContainer);
				
				// remember the size of this panel, so it can be restored later
				{
					HIRect		preferredBounds;
					
					
					if (noErr == HIViewGetBounds(nowInvisibleContainer, &preferredBounds))
					{
						Panel_SetPreferredSize(gCurrentPanel, preferredBounds.size);
					}
				}
				
				// notify the panel
				Panel_SendMessageNewVisibility(newPanelDataPtr->panel, false/* visible */);
			}
			
			// set the title of the collections drawer’s list to match the panel name
			{
				CFStringRef		newListTitleCFString = nullptr;
				
				
				Panel_GetName(newPanelDataPtr->panel, newListTitleCFString);
				SetControlTextWithCFString(gDataBrowserTitle, newListTitleCFString);
			}
			
			// change the current panel
			gCurrentPanel = newPanelDataPtr->panel;
			
			// tell the new panel that it is becoming visible; also adapt the window size,
			// and perform any related actions (like showing or hiding a drawer)
			if (nullptr != gCurrentPanel)
			{
				HISize	idealSize;
				
				
				// determine the minimum allowed size for the panel
				if (kPanel_ResponseSizeProvided == Panel_SendMessageGetIdealSize(newPanelDataPtr->panel, idealSize))
				{
					// the new absolute minimum window size is the size required to fit all panel views
					assert(0 != gBottomMargin);
					gPreferencesWindowResizeHandler.setWindowMinimumSize(idealSize.width, idealSize.height + gBottomMargin);
				}
				else
				{
					idealSize.width = 0;
					idealSize.height = 0;
				}
				
				// perhaps the user prefers the panel to be larger...
				{
					HISize	preferredSize;
					
					
					Panel_GetPreferredSize(gCurrentPanel, preferredSize);
					
					// determine which is bigger...the width the user has chosen, or the ideal width;
					// ditto for height; this becomes the most ideal size (that is, stay as close to
					// the user’s preferred size as possible, while still leaving room for all subviews)
					idealSize.width = std::max(idealSize.width, preferredSize.width);
					idealSize.height = std::max(idealSize.height, preferredSize.height);
				}
				
				Panel_GetContainerView(gCurrentPanel, nowVisibleContainer);
				Panel_SendMessageNewVisibility(newPanelDataPtr->panel, true/* visible */);
				
				// animate a resize of the window if the new panel size is different
				{
					Rect	windowContentBounds;
					
					
					assert_noerr(GetWindowBounds(gPreferencesWindow, kWindowContentRgn, &windowContentBounds));
					
					// if the ideal size is empty, force it to match the window size
					if ((idealSize.width < 100/* arbitrary */) || (idealSize.height < 100/* arbitrary */))
					{
						idealSize.width = windowContentBounds.right - windowContentBounds.left;
						idealSize.height = windowContentBounds.bottom - windowContentBounds.top;
					}
					idealSize.height += gBottomMargin;
					
					// size container according to limits; due to event handlers,
					// a resize of the window changes the panel size too
					{
						Rect		windowStructureBounds;
						Float32		structureAdditionX = 0;
						Float32		structureAdditionY = 0;
						HIRect		newBounds;
						OSStatus	error = noErr;
						
						
						// determine window structure location
						assert_noerr(GetWindowBounds(gPreferencesWindow, kWindowStructureRgn, &windowStructureBounds));
						
						// determine the extra width and height contributed by the frame, versus the content area
						structureAdditionX = (windowStructureBounds.right - windowStructureBounds.left) -
												(windowContentBounds.right - windowContentBounds.left);
						structureAdditionY = (windowStructureBounds.bottom - windowStructureBounds.top) -
												(windowContentBounds.bottom - windowContentBounds.top);
						
						// define the new structure boundaries in terms of the desired content width
						// and the extra width/height contributed by the non-content regions
						newBounds = CGRectMake(windowStructureBounds.left, windowStructureBounds.top,
												idealSize.width + structureAdditionX,
												idealSize.height + structureAdditionY);
						
						// finally, reshape the window!
						error = TransitionWindowWithOptions(gPreferencesWindow, kWindowSlideTransitionEffect,
															kWindowResizeTransitionAction, &newBounds,
															false/* asynchronously */, nullptr/* options */);
						if (noErr != error)
						{
							// if the transition fails, just resize the window normally
							SizeWindow(gPreferencesWindow, STATIC_CAST(idealSize.width, SInt16),
										STATIC_CAST(idealSize.height, SInt16), true/* update */);
						}
					}
				}
				
				// modify the data displayed in the list drawer, and select Default
				rebuildList();
				(OSStatus)selectCollection();
				
				// if the panel is an inspector type, show the collections drawer
				if (kPanel_ResponseEditTypeInspector == Panel_SendMessageGetEditType(newPanelDataPtr->panel))
				{
					(OSStatus)OpenDrawer(gDrawerWindow, kWindowEdgeDefault, true/* asynchronously */);
				}
				else
				{
					(OSStatus)CloseDrawer(gDrawerWindow, true/* asynchronously */);
				}
				
				// grow box appearance
				{
					HIViewWrap		growBox(kHIViewWindowGrowBoxID, gPreferencesWindow);
					
					
				#if 1
					// since a footer is now present, panels are no longer abutted to the bottom
					// of the window, so their grow box preferences are irrelevant
					(OSStatus)HIGrowBoxViewSetTransparent(growBox, true);
				#else
					// make the grow box transparent by default, which looks better most of the time;
					// however, give the panel the option to change this appearance
					if (kPanel_ResponseGrowBoxOpaque == Panel_SendMessageGetGrowBoxLook(newPanelDataPtr->panel))
					{
						(OSStatus)HIGrowBoxViewSetTransparent(growBox, false);
					}
					else
					{
						(OSStatus)HIGrowBoxViewSetTransparent(growBox, true);
					}
				#endif
				}
				
				// help button behavior
				{
					SInt32					panelResponse = Panel_SendMessageGetHelpKeyPhrase(newPanelDataPtr->panel);
					HelpSystem_KeyPhrase	keyPhrase = STATIC_CAST(panelResponse, HelpSystem_KeyPhrase);
					
					
					if (0 == panelResponse) keyPhrase = kHelpSystem_KeyPhrasePreferences;
					HelpSystem_SetWindowKeyPhrase(gPreferencesWindow, keyPhrase);
				}
			}
		}
		
		// swap panels
		Embedding_OffscreenSwapOverlappingControls(gPreferencesWindow, nowInvisibleContainer, nowVisibleContainer);
		(OSStatus)HIViewSetVisible(nowVisibleContainer, true/* visible */);
	}
}// choosePanel


/*!
Creates a new item in the collections list for the
current preferences class, optionally initializing
its name.  Returns true only if successful.

(3.1)
*/
Boolean
createCollection	(CFStringRef	inNameOrNull)
{
	// create a data browser item; this is accomplished by
	// creating and immediately saving a new named context
	// (a preferences callback elsewhere in this file is
	// then notified by the Preferences module of the change)
	Preferences_ContextWrap		newContext(Preferences_NewContextFromFavorites
											(returnCurrentPreferencesClass(), inNameOrNull), true/* is retained */);
	Boolean						result = false;
	
	
	if (newContext.exists())
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSave(newContext.returnRef());
		if (kPreferences_ResultOK == prefsResult)
		{
			DataBrowserItemID const		kNewItemID = REINTERPRET_CAST(newContext.returnRef(), DataBrowserItemID);
			
			
			// success!
			result = true;
			
			// automatically switch to editing the new item; ignore
			// errors for now because this is not critical to creating
			// the actual item
			selectCollection(kNewItemID);
			displayCollectionRenameUI(kNewItemID);
		}
	}
	
	return result;
}// createCollection


/*!
Focuses the collections drawer and displays an editable
text area for the specified item.

(3.1)
*/
void
displayCollectionRenameUI	(DataBrowserItemID		inItemToRename)
{
	// focus the drawer
	(OSStatus)SetUserFocusWindow(gDrawerWindow);
	
	// focus the data browser itself
	(OSStatus)DialogUtilities_SetKeyboardFocus(gDataBrowserForCollections);
	
	// open the new item for editing
	(OSStatus)SetDataBrowserEditItem(gDataBrowserForCollections, inItemToRename,
										kMyDataBrowserPropertyIDSets);
}// displayCollectionRenameUI


/*!
Creates a new item in the collections list for the current
preferences class, optionally initializing its name.  Returns
true only if successful.

IMPORTANT:	This should behave similarly to createCollection(),
			having the same side effects on the UI and any
			saved preferences on disk.

(3.1)
*/
Boolean
duplicateCollection		(Preferences_ContextRef		inReferenceContext)
{
	// duplicate a data browser item; this is accomplished by
	// creating and immediately saving a new named context
	// (a preferences callback elsewhere in this file is
	// then notified by the Preferences module of the change)
	Preferences_ContextWrap		newContext(Preferences_NewCloneContext(inReferenceContext, false/* detach */),
											true/* is retained */);
	Boolean						result = false;
	
	
	if (newContext.exists())
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSave(newContext.returnRef());
		if (kPreferences_ResultOK == prefsResult)
		{
			DataBrowserItemID const		kNewItemID = REINTERPRET_CAST(newContext.returnRef(), DataBrowserItemID);
			
			
			// success!
			result = true;
			
			// automatically switch to editing the new item; ignore
			// errors for now because this is not critical to creating
			// the actual item
			selectCollection(kNewItemID);
			displayCollectionRenameUI(kNewItemID);
		}
	}
	
	return result;
}// duplicateCollection


/*!
Determines the ideal sizes of all installed panels, and
finds the best compromise between them all.

This routine presumes a panel cannot become any smaller
than its ideal size or the given initial size.

IMPORTANT:	Invoke this only after all desired panels
			have been installed.

(3.1)
*/
void
findBestPanelSize	(HISize const&		inInitialSize,
					 HISize&			outIdealSize)
{
	panelFindIdealSize	idealSizeFinder;
	
	
	// find the best size, accounting for the ideal sizes of all panels
	idealSizeFinder = std::for_each(gPanelList().begin(), gPanelList().end(), idealSizeFinder);
	outIdealSize.width = std::max(inInitialSize.width, idealSizeFinder.idealSize.width);
	outIdealSize.height = std::max(inInitialSize.height, idealSizeFinder.idealSize.height);
}// findBestPanelSize


/*!
This routine is called whenever the drawer is
resized, to make sure the drawer does not become
too large.

(3.1)
*/
void
handleNewDrawerWindowSize	(WindowRef		inWindowRef,
							 Float32		UNUSED_ARGUMENT(inDeltaX),
							 Float32		UNUSED_ARGUMENT(inDeltaY),
							 void*			UNUSED_ARGUMENT(inContext))
{
	// nothing really needs to be done here; the routine exists only
	// to ensure the initial constraints (at routine install time)
	// are always enforced, otherwise the drawer size cannot shrink
	assert(inWindowRef == gDrawerWindow);
}// handleNewDrawerWindowSize


/*!
Rearranges the contents of the footer as it is resized.

(3.1)
*/
void
handleNewFooterSize		(HIViewRef		inView,
						 Float32		inDeltaX,
						 Float32		inDeltaY,
						 void*			UNUSED_ARGUMENT(inContext))
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		HIViewWrap		viewWrap;
		
		
		viewWrap = HIViewWrap(idMySeparatorBottom, HIViewGetWindow(inView));
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	}
}// handleNewFooterSize


/*!
This routine is called whenever the window is
resized, to make sure panels match its new size.

(3.0)
*/
void
handleNewMainWindowSize	(WindowRef		inWindow,
						 Float32		UNUSED_ARGUMENT(inDeltaX),
						 Float32		inDeltaY,
						 void*			UNUSED_ARGUMENT(inContext))
{
	Rect		windowContentBounds;
	HIViewWrap	viewWrap;
	
	
	assert(inWindow == gPreferencesWindow);
	assert_noerr(GetWindowBounds(inWindow, kWindowContentRgn, &windowContentBounds));
	
	// resize footer
	viewWrap = HIViewWrap(idMyUserPaneFooter, inWindow);
	{
		HIRect		footerFrame;
		OSStatus	error = noErr;
		
		
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		error = HIViewGetFrame(viewWrap, &footerFrame);
		assert_noerr(error);
		footerFrame.size.width = windowContentBounds.right - windowContentBounds.left;
		error = HIViewSetFrame(viewWrap, &footerFrame);
		assert_noerr(error);
	}
	
	// resize panels
	panelResizer	resizer(windowContentBounds.right - windowContentBounds.left,
							windowContentBounds.bottom - windowContentBounds.top - gBottomMargin,
							false/* is delta */);
	std::for_each(gPanelList().begin(), gPanelList().end(), resizer);
	
	Cursors_UseArrow();
}// handleNewMainWindowSize


/*!
Initializes the preferences window.  The window is
created invisibly, and then global preference data is
used to set up the corresponding controls in the
window panes.

You should generally call PrefsWindow_Display(), which
will automatically invoke init() if necessary.  This
allows you to create the window only the first time it
is necessary to display it, and from then on to retain
it in memory.

Be sure to call PrefsWindow_Done() at the end of the
program to dispose of memory that this module
allocates.

(3.0)
*/
void
init ()
{
	NIBWindow	mainWindow(AppResources_ReturnBundleForNIBs(), CFSTR("PrefsWindow"), CFSTR("Window"));
	NIBWindow	drawerWindow(AppResources_ReturnBundleForNIBs(), CFSTR("PrefsWindow"), CFSTR("Drawer"));
	
	
	mainWindow << NIBLoader_AssertWindowExists;
	drawerWindow << NIBLoader_AssertWindowExists;
	gPreferencesWindow = mainWindow;
	gDrawerWindow = drawerWindow;
	(OSStatus)SetDrawerParent(drawerWindow, mainWindow);
	
	if (nullptr != gPreferencesWindow)
	{
		Rect		panelBounds;
		
		
		// some attributes that are unset in the NIB are not recognized on 10.3
		if (false == FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)ChangeWindowAttributes
						(gPreferencesWindow,
							0/* attributes to set */,
							kWindowInWindowMenuAttribute/* attributes to clear */);
		}
		
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		if (FlagManager_Test(kFlagOS10_5API))
		{
			// although the API is available on 10.4, the Spaces-related flags
			// will only work on Leopard
			(OSStatus)HIWindowChangeAvailability
						(gPreferencesWindow,
							FUTURE_SYMBOL(1 << 9, kHIWindowMoveToActiveSpace)/* attributes to set */,
							0/* attributes to clear */);
		}
	#endif
		
		{
			HIViewRef	panelUserPane = (mainWindow.returnHIViewWithID(idMyUserPaneAnyPrefPanel) << HIViewWrap_AssertExists);
			
			
			// initialize the panel rectangle based on the NIB definition
			GetControlBounds(panelUserPane, &panelBounds);
			
			// determine how far from the bottom edge the panel user pane is
			{
				Rect	windowContentBounds;
				
				
				GetControlBounds(panelUserPane, &panelBounds);
				GetWindowBounds(gPreferencesWindow, kWindowContentRgn, &windowContentBounds);
				assert((windowContentBounds.right - windowContentBounds.left) == (panelBounds.right - panelBounds.left));
				gBottomMargin = (windowContentBounds.bottom - windowContentBounds.top) - (panelBounds.bottom - panelBounds.top);
			}
			
			// since the user pane is only used to define these boundaries, it can be destroyed now
			CFRelease(panelUserPane), panelUserPane = nullptr;
		}
		
		// set up the Help System
		{
			HIViewWrap		helpButton(idMyButtonHelp, gPreferencesWindow);
			
			
			DialogUtilities_SetUpHelpButton(helpButton);
			HelpSystem_SetWindowKeyPhrase(gPreferencesWindow, kHelpSystem_KeyPhrasePreferences);
		}
		
		// enable window drag tracking, so the data browser column moves and toolbar item drags work
		(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(gPreferencesWindow, true);
		
		// set up the WindowInfo stuff
		gPreferencesWindowInfo = WindowInfo_New();
		WindowInfo_SetWindowDescriptor(gPreferencesWindowInfo, kConstantsRegistry_WindowDescriptorPreferences);
		WindowInfo_SetForWindow(gPreferencesWindow, gPreferencesWindowInfo);
		
		// set the item text for the Dock’s window menu
		{
			CFStringRef		titleCFString = nullptr;
			
			
			(UIStrings_Result)UIStrings_Copy(kUIStrings_PreferencesWindowIconName, titleCFString);
			SetWindowAlternateTitle(gPreferencesWindow, titleCFString);
			CFRelease(titleCFString), titleCFString = nullptr;
		}
		
		// find references to all NIB-based controls that are needed for any operation
		// (button clicks, dealing with text or responding to window resizing)
		gDataBrowserTitle = (drawerWindow.returnHIViewWithID(idMyStaticTextListTitle) << HIViewWrap_AssertExists);
		gDataBrowserForCollections = (drawerWindow.returnHIViewWithID(idMyDataBrowserCollections) << HIViewWrap_AssertExists);
		gCollectionAddMenuButton = (drawerWindow.returnHIViewWithID(idMyButtonAddCollection) << HIViewWrap_AssertExists);
		gCollectionRemoveButton = (drawerWindow.returnHIViewWithID(idMyButtonRemoveCollection) << HIViewWrap_AssertExists);
		gCollectionMoveUpButton = (drawerWindow.returnHIViewWithID(idMyButtonMoveCollectionUp) << HIViewWrap_AssertExists);
		gCollectionMoveDownButton = (drawerWindow.returnHIViewWithID(idMyButtonMoveCollectionDown) << HIViewWrap_AssertExists);
		gCollectionManipulateMenuButton = (drawerWindow.returnHIViewWithID(idMyButtonManipulateCollection) << HIViewWrap_AssertExists);
		
		// set up the data browser
		{
			DataBrowserCallbacks	callbacks;
			OSStatus				error = noErr;
			
			
			// define a callback for specifying what data belongs in the list
			callbacks.version = kDataBrowserLatestCallbacks;
			error = InitDataBrowserCallbacks(&callbacks);
			assert_noerr(error);
			callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
			assert(nullptr != callbacks.u.v1.itemDataCallback);
			callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
			assert(nullptr != callbacks.u.v1.itemNotificationCallback);
			
			// attach data not specified in NIB
			error = SetDataBrowserCallbacks(gDataBrowserForCollections, &callbacks);
			assert_noerr(error);
			error = SetDataBrowserSelectionFlags(gDataBrowserForCollections,
													kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
			assert_noerr(error);
		}
		
		// add "+" and "-" icons to the add and remove buttons, and a gear icon
		// to the contextual menu button
		{
			IconManagerIconRef		buttonIcon = nullptr;
			
			
			buttonIcon = IconManager_NewIcon();
			if (nullptr != buttonIcon)
			{
				if (noErr == IconManager_MakeIconRefFromBundleFile
								(buttonIcon, AppResources_ReturnItemAddIconFilenameNoExtension(),
									AppResources_ReturnCreatorCode(),
									kConstantsRegistry_IconServicesIconItemAdd))
				{
					if (noErr == IconManager_SetButtonIcon(gCollectionAddMenuButton, buttonIcon))
					{
						// once the icon is set successfully, the equivalent text title can be removed
						(OSStatus)SetControlTitleWithCFString(gCollectionAddMenuButton, CFSTR(""));
					}
				}
				IconManager_DisposeIcon(&buttonIcon);
			}
			
			buttonIcon = IconManager_NewIcon();
			if (nullptr != buttonIcon)
			{
				if (noErr == IconManager_MakeIconRefFromBundleFile
								(buttonIcon, AppResources_ReturnItemRemoveIconFilenameNoExtension(),
									AppResources_ReturnCreatorCode(),
									kConstantsRegistry_IconServicesIconItemRemove))
				{
					if (noErr == IconManager_SetButtonIcon(gCollectionRemoveButton, buttonIcon))
					{
						// once the icon is set successfully, the equivalent text title can be removed
						(OSStatus)SetControlTitleWithCFString(gCollectionRemoveButton, CFSTR(""));
					}
				}
				IconManager_DisposeIcon(&buttonIcon);
			}
			
			buttonIcon = IconManager_NewIcon();
			if (nullptr != buttonIcon)
			{
				if (noErr == IconManager_MakeIconRefFromBundleFile
								(buttonIcon, AppResources_ReturnContextMenuFilenameNoExtension(),
									AppResources_ReturnCreatorCode(),
									kConstantsRegistry_IconServicesIconContextMenu))
				{
					if (noErr == IconManager_SetButtonIcon(gCollectionManipulateMenuButton, buttonIcon))
					{
						// once the icon is set successfully, the equivalent text title can be removed
						(OSStatus)SetControlTitleWithCFString(gCollectionManipulateMenuButton, CFSTR(""));
					}
				}
				IconManager_DisposeIcon(&buttonIcon);
			}
		}
		
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		// set accessibility relationships, if possible
		if (FlagManager_Test(kFlagOS10_4API))
		{
			CFStringRef		accessibilityDescCFString = nullptr;
			
			
			if (UIStrings_Copy(kUIStrings_ButtonAddAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionAddMenuButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
			if (UIStrings_Copy(kUIStrings_ButtonRemoveAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionRemoveButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
			if (UIStrings_Copy(kUIStrings_ButtonMoveUpAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionMoveUpButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
			if (UIStrings_Copy(kUIStrings_ButtonMoveDownAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionMoveDownButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
		}
	#endif
		
		//
		// create base controls
		//
		
		// create preferences icons
		{
			HIToolbarRef	categoryIcons = nullptr;
			
			
			if (noErr == HIToolbarCreate(kConstantsRegistry_HIToolbarIDPreferences, kHIToolbarNoAttributes,
											&categoryIcons))
			{
				// add standard items to display the collections drawer
			#if 0
				HIToolbarItemRef	drawerDisplayItem = nullptr;
				HIToolbarItemRef	drawerSeparatorItem = nullptr;
				
				
				if (noErr == HIToolbarItemCreate
								(kMyHIToolbarItemIDCollections,
									kHIToolbarItemCantBeRemoved | kHIToolbarItemAnchoredLeft/* item options */,
									&drawerDisplayItem))
				{
					CFStringRef		labelCFString = nullptr;
					CFStringRef		descriptionCFString = nullptr;
					FSRef			iconFile;
					
					
					// set the icon, label and tooltip
					(OSStatus)HIToolbarItemSetCommandID(drawerDisplayItem, kCommandShowHidePrefCollectionsDrawer);
					if (AppResources_GetArbitraryResourceFileFSRef
						(AppResources_ReturnPreferenceCollectionsIconFilenameNoExtension(),
							CFSTR("icns")/* type */, iconFile))
					{
						IconRef		iconRef = nullptr;
						
						
						if (noErr == RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
																kMyIconServicesIconPreferenceCollections,
																&iconFile, &iconRef))
						{
							(OSStatus)HIToolbarItemSetIconRef(drawerDisplayItem, iconRef);
						}
					}
					if (UIStrings_Copy(kUIStrings_PreferencesWindowCollectionsDrawerShowHideName, labelCFString).ok())
					{
						(OSStatus)HIToolbarItemSetLabel(drawerDisplayItem, labelCFString);
						CFRelease(labelCFString), labelCFString = nullptr;
					}
					if (UIStrings_Copy(kUIStrings_PreferencesWindowCollectionsDrawerDescription, descriptionCFString).ok())
					{
						(OSStatus)HIToolbarItemSetHelpText(drawerDisplayItem, descriptionCFString, nullptr/* long version */);
						CFRelease(descriptionCFString), descriptionCFString = nullptr;
					}
					
					// add the item to the toolbar
					(OSStatus)HIToolbarAppendItem(categoryIcons, drawerDisplayItem);
				}
				if (noErr == HIToolbarCreateItemWithIdentifier(categoryIcons, kHIToolbarSeparatorIdentifier,
																nullptr/* configuration data */, &drawerSeparatorItem))
				{
					// add the item to the toolbar
					(OSStatus)HIToolbarAppendItem(categoryIcons, drawerSeparatorItem);
				}
			#endif
				
				(OSStatus)SetWindowToolbar(gPreferencesWindow, categoryIcons);
				(OSStatus)ShowHideWindowToolbar(gPreferencesWindow, true/* show */, false/* animate */);
				CFRelease(categoryIcons);
			}
		}
		
		// create category panels - call these routines in the order you want their category buttons to appear
		installPanel(PrefPanelGeneral_New());
		installPanel(PrefPanelMacros_New());
		installPanel(PrefPanelWorkspaces_New());
		installPanel(PrefPanelSessions_New());
		installPanel(PrefPanelTerminals_New());
		installPanel(PrefPanelFormats_New());
		installPanel(PrefPanelTranslations_New());
		installPanel(PrefPanelFullScreen_New());
		
		// footer callbacks
		{
			HIViewWrap		footerView(idMyUserPaneFooter, gPreferencesWindow);
			
			
			// install a callback that responds as the main window’s footer is resized
			gPreferencesWindowFooterResizeHandler.install(footerView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
																		kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
															handleNewFooterSize, nullptr/* context */);
			
			// install a callback to refresh the footer when its state changes
			gPrefsFooterActiveStateHandler.install(GetControlEventTarget(footerView), receiveFooterActiveStateChange,
													CarbonEventSetInClass(CarbonEventClass(kEventClassControl),
																			kEventControlActivate, kEventControlDeactivate),
													nullptr/* user data */);
			
			// install a callback to render the footer background
			gPrefsFooterDrawHandler.install(GetControlEventTarget(footerView), receiveFooterDraw,
											CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlDraw),
											nullptr/* user data */);
		}
		
		// install a callback that responds as the main window is resized
		{
			HISize		initialSize = CGSizeMake(panelBounds.right - panelBounds.left,
													panelBounds.bottom - panelBounds.top);
			HISize		idealWindowContentSize = CGSizeMake(0, 0);
			
			
			findBestPanelSize(initialSize, idealWindowContentSize);
			sizePanels(idealWindowContentSize);
			
			gPreferencesWindowResizeHandler.install(gPreferencesWindow, handleNewMainWindowSize, nullptr/* user data */,
													idealWindowContentSize.width/* minimum width */,
													idealWindowContentSize.height/* minimum height */,
													idealWindowContentSize.width + 500/* arbitrary maximum width */,
													idealWindowContentSize.height + 350/* arbitrary maximum height */);
			assert(gPreferencesWindowResizeHandler.isInstalled());
			
			// remember this maximum width, it is used to set the window size
			// whenever a new panel is chosen
			gPreferencesWindowResizeHandler.getWindowMaximumSize(gMaximumWindowSize.width, gMaximumWindowSize.height);
		}
		
		// install a callback that responds as the drawer window is resized; this is used
		// primarily to enforce a maximum drawer height, not to allow a resizable drawer
		{
			Rect		currentBounds;
			OSStatus	error = noErr;
			
			
			error = GetWindowBounds(gDrawerWindow, kWindowContentRgn, &currentBounds);
			assert_noerr(error);
			gDrawerWindowResizeHandler.install(gDrawerWindow, handleNewDrawerWindowSize, nullptr/* user data */,
												currentBounds.right - currentBounds.left/* minimum width */,
												currentBounds.bottom - currentBounds.top/* minimum height */,
												currentBounds.right - currentBounds.left/* maximum width */,
												currentBounds.bottom - currentBounds.top/* maximum height */);
			assert(gDrawerWindowResizeHandler.isInstalled());
		}
		
		// set the initial offset of the drawer
		{
			float		leadingOffset = 52/* arbitrary; the current height of a standard toolbar */;
			float		trailingOffset = kWindowOffsetUnchanged;
			OSStatus	error = noErr;
			
			
			if (FlagManager_Test(kFlagOS10_5API))
			{
				// on Leopard, the aesthetically-pleasing spot for the drawer
				// is slightly higher
				--leadingOffset;
			}
			error = SetDrawerOffsets(gDrawerWindow, leadingOffset, trailingOffset);
		}
		
		// install a callback that disposes of the window properly when it should be closed
		{
			EventTypeSpec const		whenWindowClosing[] =
									{
										{ kEventClassWindow, kEventWindowClose }
									};
			OSStatus				error = noErr;
			
			
			gPreferencesWindowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
			error = InstallWindowEventHandler(gPreferencesWindow, gPreferencesWindowClosingUPP, GetEventTypeCount(whenWindowClosing),
												whenWindowClosing, nullptr/* user data */,
												&gPreferencesWindowClosingHandler/* event handler reference */);
			assert(error == noErr);
		}
		
		// install a callback that finds out about changes to available preferences collections
		gPreferenceChangeEventListener = ListenerModel_NewStandardListener(preferenceChanged);
		{
			Preferences_Result		error = kPreferences_ResultOK;
			
			
			error = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeContextName,
												false/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
			error = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts,
												true/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
		}
	}
}// init


/*!
To add a panel to the preferences window from
an abstract Panel object, use this method.  A
preference panel should define a name and an
icon (as recommended by the Panel module anyway),
which is used to render a bevel button for the
panel.  The preferences window automatically
adapts itself for the panel, creating a new icon
to represent it and firing notifications to the
panel whenever necessary.  If you respond to the
standard Panel interface, the panel will function
perfectly in the preferences window.

There is no way to arrange preference panels
except by invoking PrefsWindow_InstallPanel() for
each panel, in the order you want the panels to
appear in the toolbar.

(3.0)
*/
void
installPanel	(Panel_Ref		inPanel)
{
	newPanelSelector(inPanel);
	
	// allow the panel to create its controls by providing a pointer to the containing window
	Panel_SendMessageCreateViews(inPanel, gPreferencesWindow);
}// installPanel


/*!
Responds to changes in the data browser.  Currently,
most of the possible messages are ignored, but this
is used to determine when to update the panel views.

(3.1)
*/
void
monitorDataBrowserItems		(ControlRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	Preferences_ContextRef	prefsContext = REINTERPRET_CAST(inItemID, Preferences_ContextRef);
	
	
	assert(gDataBrowserForCollections.operator HIViewRef() == inDataBrowser);
	switch (inMessage)
	{
	case kDataBrowserItemSelected:
		{
			DataBrowserTableViewRowIndex	rowIndex = 0;
			OSStatus						error = noErr;
			
			
			// update the panel views to match the newly-selected Favorite
			chooseContext(prefsContext);
			
			// the Default item is in the first row and cannot be deleted
			error = GetDataBrowserTableViewItemRow(inDataBrowser, inItemID, &rowIndex);
			if (noErr == error)
			{
				if (0 == rowIndex)
				{
					// cannot operate on the Default item
					DeactivateControl(gCollectionRemoveButton);
					DeactivateControl(gCollectionMoveUpButton);
					DeactivateControl(gCollectionMoveDownButton);
				}
				else
				{
					UInt32		itemCount = 0;
					
					
					error = GetDataBrowserItemCount(inDataBrowser, kDataBrowserNoItem/* container */, false/* recurse */,
													kDataBrowserItemAnyState, &itemCount);
					assert_noerr(error);
					
					// any other item can be removed
					ActivateControl(gCollectionRemoveButton);
					
					// only the first item (below Default) cannot be moved
					if (1 == rowIndex)
					{
						DeactivateControl(gCollectionMoveUpButton);
					}
					else
					{
						ActivateControl(gCollectionMoveUpButton);
					}
					
					// the last item cannot be moved down
					if (rowIndex >= (itemCount - 1))
					{
						DeactivateControl(gCollectionMoveDownButton);
					}
					else
					{
						ActivateControl(gCollectionMoveDownButton);
					}
				}
			}
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
Invoked by the Mac OS whenever something interesting happens
in a Navigation Services save dialog attached to the
Preferences window.

(4.0)
*/
void
navigationExportPrefsDialogEvent	(NavEventCallbackMessage	inMessage,
								 	 NavCBRecPtr				inParameters,
								 	 void*						inPrefsContextRef)
{
	Preferences_ContextRef	sourceContext = REINTERPRET_CAST(inPrefsContextRef, Preferences_ContextRef);
	
	
	switch (inMessage)
	{
	case kNavCBUserAction:
		if (kNavUserActionSaveAs == inParameters->userAction)
		{
			NavReplyRecord		reply;
			OSStatus			error = noErr;
			
			
			// save file
			error = NavDialogGetReply(inParameters->context/* dialog */, &reply);
			if ((noErr == error) && (reply.validRecord))
			{
				FSRef	saveFile;
				FSRef	temporaryFile;
				
				
				error = FileSelectionDialogs_CreateOrFindUserSaveFile
						(reply, AppResources_ReturnCreatorCode(), 'TEXT', saveFile, temporaryFile);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to create export file, error", error);
				}
				else
				{
					// export the contents of the preferences context
					Preferences_Result	writeResult = Preferences_ContextSaveAsXMLFileRef(sourceContext, temporaryFile);
					
					
					if (kPreferences_ResultOK != writeResult)
					{
						Console_Warning(Console_WriteValue, "failed to save preferences as XML, error", writeResult);
					}
					else
					{
						// after successfully saving the data in the temporary space,
						// move the content to its proper location
						error = FSExchangeObjects(&temporaryFile, &saveFile);
						if (noErr != error)
						{
							Console_Warning(Console_WriteValue, "failed to create export file, error", error);
						}
						else
						{
							// success; now delete unused temporary file
							error = FSDeleteObject(&temporaryFile);
							if (noErr != error)
							{
								Console_Warning(Console_WriteValue, "failed to remove temporary file after export, error", error);
							}
						}
					}
				}
			}
			else
			{
				Console_Warning(Console_WriteLine, "export skipped, no valid reply record given");
			}
			Alert_ReportOSStatus(error);
			error = FileSelectionDialogs_CompleteSave(&reply);
		}
		break;
	
	case kNavCBTerminate:
		// clean up
		NavDialogDispose(inParameters->context);
		(OSStatus)ChangeWindowAttributes
		(gPreferencesWindow,
			kWindowCollapseBoxAttribute/* attributes to set */,
			0/* attributes to clear */);
		break;
	
	default:
		// not handled
		break;
	}
}// navigationExportPrefsDialogEvent


/*!
To create a new list item for the preferences window that
represents the specified panel, use this method.  The
selector’s title and icon are acquired from the specified
panel.

(3.0)
*/
void
newPanelSelector	(Panel_Ref		inPanel)
{
	MyPanelDataPtr		dataPtr = new MyPanelData(inPanel, ++gPanelChoiceListLastRowIndex);
	
	
	if (nullptr != dataPtr)
	{
		// store the panel
		dataPtr->panel = inPanel;
		gPanelList().push_back(dataPtr);
		
		// remember panel position and command ID, for later event handling
		gIndicesByCommandID()[Panel_ReturnShowCommandID(inPanel)] = dataPtr->listIndex;
		
		// create a toolbar item for this panel
		{
			HIToolbarRef	categoryIcons = nullptr;
			
			
			if (noErr == GetWindowToolbar(gPreferencesWindow, &categoryIcons))
			{
				HIToolbarItemRef	newItem = nullptr;
				OptionBits			itemOptions = kHIToolbarItemCantBeRemoved | kHIToolbarItemAnchoredLeft;
				
				
				if ((0 == dataPtr->listIndex) &&
					(FlagManager_Test(kFlagOS10_4API)))
				{
					itemOptions |= FUTURE_SYMBOL(1 << 7, kHIToolbarItemSelected);
				}
				if (noErr == HIToolbarItemCreate(Panel_ReturnKind(inPanel), itemOptions, &newItem))
				{
					CFStringRef		descriptionCFString = nullptr;
					
					
					// PrefsWindow_Done() should clean up this retain
					CFRetain(newItem), gCategoryToolbarItems().push_back(newItem);
					
					// set the icon, label and tooltip
					(OSStatus)Panel_SetToolbarItemIconAndLabel(newItem, inPanel);
					Panel_GetDescription(inPanel, descriptionCFString);
					if (nullptr != descriptionCFString)
					{
						(OSStatus)HIToolbarItemSetHelpText(newItem, descriptionCFString, nullptr/* long version */);
					}
					
					// add the item to the toolbar
					(OSStatus)HIToolbarAppendItem(categoryIcons, newItem);
				}
			}
		}
	}
}// newPanelSelector


/*!
Invoked whenever a monitored preference value is changed
(see init() to see which preferences are monitored).

This routine responds by ensuring that the list of
contexts displayed to the user is up-to-date.

(3.0)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeContextName:
		// a context has been renamed; refresh the list
		refreshDisplay();
		break;
	
	case kPreferences_ChangeNumberOfContexts:
		// contexts were added or removed; destroy and rebuild the list
		rebuildList();
		refreshDisplay();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Destroys all the items in the data browser and reconstructs
them based on the list of contexts for the current preferences
class.  It is appropriate to call this whenever a change to
the set of items is made (choosing a new panel, adding or
removing an item, etc.).

FUTURE: Callbacks could send specific context references, so
that changes do not necessarily have to target the entire list
in this way.

(3.1)
*/
void
rebuildList ()
{
	Quills::Prefs::Class const	kCurrentPreferencesClass = returnCurrentPreferencesClass();
	typedef std::vector< Preferences_ContextRef >	ContextsList;
	ContextsList				contextList;
	
	
	if (Preferences_GetContextsInClass(kCurrentPreferencesClass, contextList))
	{
		// start by destroying all items in the list
		(OSStatus)RemoveDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
											0/* size of array */, nullptr/* items array; nullptr = destroy all */,
											kDataBrowserItemNoProperty/* pre-sort property */);
		
		// now acquire contexts for all available names in this class,
		// and add data browser items for each of them
		for (ContextsList::const_iterator toContextRef = contextList.begin();
				toContextRef != contextList.end(); ++toContextRef)
		{
			DataBrowserItemID	ids[] = { REINTERPRET_CAST(*toContextRef, DataBrowserItemID) };
			
			
			(OSStatus)AddDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
											sizeof(ids) / sizeof(DataBrowserItemID), ids,
											kDataBrowserItemNoProperty/* pre-sort property */);
		}
		
		// ensure something is always selected
		//(OSStatus)selectCollection();
	}
}// rebuildList


/*!
Handles "kEventControlActivate" and "kEventControlDeactivate"
of "kEventClassControl".

Invoked by Mac OS X whenever the footer is activated or dimmed.

(3.1)
*/
OSStatus
receiveFooterActiveStateChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert((kEventKind == kEventControlActivate) || (kEventKind == kEventControlDeactivate));
	{
		HIViewRef	viewBeingActivatedOrDeactivated = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef,
														viewBeingActivatedOrDeactivated);
		if (noErr == result)
		{
			// since inactive and active footers have different appearances, force redraws
			// when they are activated or deactivated
			result = HIViewSetNeedsDisplay(viewBeingActivatedOrDeactivated, true);
		}
	}
	return result;
}// receiveFooterActiveStateChange


/*!
Handles "kEventControlDraw" of "kEventClassControl".

Paints the background of the footer.

(3.1)
*/
OSStatus
receiveFooterDraw	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef					inEvent,
					 void*						UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	{
		HIViewRef	footerView = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, footerView);
		
		// if the view was found, continue
		if (noErr == result)
		{
			//HIViewPartCode		partCode = 0;
			CGContextRef		drawingContext = nullptr;
			
			
			// could determine which part (if any) to draw; if none, draw everything
			// (ignored, not needed)
			//result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			//result = noErr; // ignore part code parameter if absent
			
			// determine the context to use for drawing
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, drawingContext);
			assert_noerr(result);
			
			// if all information can be found, proceed with drawing
			if (noErr == result)
			{
				HIThemeBackgroundDrawInfo	backgroundInfo;
				HIRect						floatBounds;
				OSStatus					error = noErr;
				
				
				error = HIViewGetBounds(footerView, &floatBounds);
				assert_noerr(error);
				
				// arbitrarily avoid two pixels off the top to not overrun separator
				floatBounds.origin.y += 2;
				floatBounds.size.height -= 2;
				
				// reserve this darker colored background for Leopard
				if (FlagManager_Test(kFlagOS10_5API))
				{
					bzero(&backgroundInfo, sizeof(backgroundInfo));
					backgroundInfo.version = 0;
					backgroundInfo.state = IsControlActive(footerView) ? kThemeStateActive : kThemeStateInactive;
					backgroundInfo.kind = kThemeBackgroundMetal;
					error = HIThemeDrawBackground(&floatBounds, &backgroundInfo, drawingContext, kHIThemeOrientationNormal);
				}
			}
		}
	}
	return result;
}// receiveFooterDraw


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the preferences toolbar.  Responds by changing
the currently-displayed panel.

(3.1)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// execute a command selected from the toolbar
				switch (received.commandID)
				{
				case kCommandPreferencesNewFavorite:
					{
						// create a data browser item; this is accomplished by
						// creating and immediately saving a new named context
						// (a preferences callback elsewhere in this file is
						// then notified by the Preferences module of the change)
						Boolean		createOK = false;
						
						
						createOK = createCollection();
						if (createOK)
						{
							result = noErr;
						}
						else
						{
							Sound_StandardAlert();
							result = eventNotHandledErr;
						}
					}
					break;
				
				case kCommandPreferencesDeleteFavorite:
					{
						// delete a data browser item; this is accomplished by
						// destroying the underlying context (a preferences callback
						// elsewhere in this file is then notified by the Preferences
						// module of the change)
						DataBrowserItemID	selectedID = returnCurrentSelection();
						Boolean				isError = false;
						
						
						if (0 == selectedID) isError = true;
						else
						{
							Preferences_ContextRef		deletedContext = REINTERPRET_CAST(selectedID,
																							Preferences_ContextRef);
							
							
							if (nullptr == deletedContext) isError = true;
							else
							{
								Preferences_Result		prefsResult = kPreferences_ResultOK;
								
								
								// first end any editing session...it appears the data browser
								// can crash if the user happens to delete the item being renamed
								removeCollectionRenameUI();
								
								// if the deleted context is current, update the global; the
								// global will be corrected later when a new selection is made
								if (deletedContext == gCurrentDataSet) gCurrentDataSet = nullptr;
								
								prefsResult = Preferences_ContextDeleteSaved(deletedContext);
								if (kPreferences_ResultOK != prefsResult) isError = true;
								else
								{
									// success!
									result = noErr;
									
									// select something else...
									(OSStatus)selectCollection();
								}
							}
						}
						
						if (isError)
						{
							Sound_StandardAlert();
							result = eventNotHandledErr;
						}
					}
					break;
				
				case kCommandPreferencesDuplicateFavorite:
					{
						// make a copy of the selected data browser item; this is
						// accomplished by creating and immediately saving a new
						// named context (a preferences callback elsewhere in this
						// file is then notified by the Preferences module of the
						// change)
						DataBrowserItemID	selectedID = returnCurrentSelection();
						Boolean				isError = false;
						
						
						if (0 == selectedID) isError = true;
						else
						{
							Preferences_ContextRef		referenceContext = REINTERPRET_CAST(selectedID,
																							Preferences_ContextRef);
							
							
							if (nullptr == referenceContext) isError = true;
							else
							{
								if (duplicateCollection(referenceContext)) isError = false;
								else isError = true;
							}
						}
						
						if (isError)
						{
							Sound_StandardAlert();
							result = eventNotHandledErr;
						}
					}
					break;
				
				case kCommandPreferencesImportFavorite:
					{
						// IMPORTANT: These should be consistent with declared types in the application "Info.plist".
						void const*			kTypeList[] = { CFSTR("com.apple.property-list"),
															CFSTR("plist")/* redundant, needed for older systems */,
															CFSTR("xml") };
						CFRetainRelease		fileTypes(CFArrayCreate(kCFAllocatorDefault, kTypeList,
														sizeof(kTypeList) / sizeof(CFStringRef), &kCFTypeArrayCallBacks),
														true/* is retained */);
						CFStringRef			promptCFString = nullptr;
						CFStringRef			titleCFString = nullptr;
						
						
						(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptOpenPrefs, promptCFString);
						(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogTitleOpenPrefs, titleCFString);
						(Boolean)CocoaBasic_FileOpenPanelDisplay(promptCFString, titleCFString, fileTypes.returnCFArrayRef());
						
						result = noErr;
					}
					break;
				
				case kCommandPreferencesExportFavorite:
					{
						DataBrowserItemID	selectedID = returnCurrentSelection();
						Boolean				isError = false;
						
						
						// fix display glitch where menu button highlighting is not removed by the OS
						HiliteControl(gCollectionManipulateMenuButton, kControlNoPart);
						
						// temporarily disable minimization of the parent window because the OS does
						// not correctly handle this (the window can be minimized but after it is
						// restored the Navigation Services sheet is gone; also, all future export
						// commands start failing because Navigation Services sheets do not open);
						// minimization is restored in navigationExportPrefsDialogEvent()
						(OSStatus)ChangeWindowAttributes
						(gPreferencesWindow,
							0/* attributes to set */,
							kWindowCollapseBoxAttribute/* attributes to clear */);
						
						if (0 == selectedID) isError = true;
						else
						{
							Preferences_ContextRef		referenceContext = REINTERPRET_CAST(selectedID,
																							Preferences_ContextRef);
							NavDialogCreationOptions	dialogOptions;
							NavDialogRef				navigationServicesDialog = nullptr;
							OSStatus					error = noErr;
							
							
							error = NavGetDefaultDialogCreationOptions(&dialogOptions);
							if (noErr == error)
							{
								dialogOptions.optionFlags |= kNavDontAddTranslateItems;
								Localization_GetCurrentApplicationNameAsCFString(&dialogOptions.clientName);
								dialogOptions.preferenceKey = kPreferences_NavPrefKeyGenericSaveFile;
								dialogOptions.parentWindow = gPreferencesWindow;
								(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultExportPreferences, dialogOptions.saveFileName);
								(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptSavePrefs, dialogOptions.message);
								dialogOptions.modality = kWindowModalityWindowModal;
							}
							error = NavCreatePutFileDialog(&dialogOptions, 'TEXT'/* type */, AppResources_ReturnCreatorCode(),
															NewNavEventUPP(navigationExportPrefsDialogEvent),
															referenceContext/* client data */, &navigationServicesDialog);
							if (noErr == error)
							{
								// display the dialog; it is a sheet, so this will return immediately
								// and the dialog will close whenever the user is actually done with it
								error = NavDialogRun(navigationServicesDialog);
							}
						}
						
						if (isError)
						{
							Sound_StandardAlert();
							result = eventNotHandledErr;
						}
						else
						{
							result = noErr;
						}
					}
					break;
				
				case kCommandPreferencesRenameFavorite:
					// switch to editing the specified item
					displayCollectionRenameUI(returnCurrentSelection());
					break;
				
				case kCommandPreferencesMoveFavoriteUp:
				case kCommandPreferencesMoveFavoriteDown:
					{
						// reposition a data browser item; this requires changing its
						// underlying order in the contexts list of the Preferences module
						DataBrowserItemID	selectedID = returnCurrentSelection();
						Boolean				isError = false;
						
						
						if (0 == selectedID) isError = true;
						else
						{
							Preferences_ContextRef		movedContext = REINTERPRET_CAST(selectedID,
																						Preferences_ContextRef);
							
							
							if (nullptr == movedContext) isError = true;
							else
							{
								Preferences_Result		prefsResult = kPreferences_ResultOK;
								
								
								// first end any editing session...it appears the data browser
								// can crash if the user happens to move the item being renamed
								removeCollectionRenameUI();
								
								// reposition the context in the list of contexts
								prefsResult = Preferences_ContextRepositionRelativeToSelf
												(movedContext, (kCommandPreferencesMoveFavoriteUp == received.commandID) ? -1 : +1);
								if (kPreferences_ResultOK != prefsResult) isError = true;
								else
								{
									// success!
									result = noErr;
								}
							}
						}
						
						if (isError)
						{
							Sound_StandardAlert();
						}
						
						// refresh the data browser to show the new order
						rebuildList();
						selectCollection(selectedID);
					}
					break;
				
				case kCommandShowHidePrefCollectionsDrawer:
					result = ToggleDrawer(gDrawerWindow);
					break;
				
				case kCommandDisplayPrefPanelFormats:
				case kCommandDisplayPrefPanelGeneral:
				case kCommandDisplayPrefPanelKiosk:
				case kCommandDisplayPrefPanelMacros:
				case kCommandDisplayPrefPanelSessions:
				case kCommandDisplayPrefPanelTerminals:
				case kCommandDisplayPrefPanelTranslations:
				case kCommandDisplayPrefPanelWorkspaces:
					showWindow();
					assert(false == gIndicesByCommandID().empty());
					choosePanel(gIndicesByCommandID()[received.commandID]);
					result = noErr;
					break;
				
				default:
					// ???
					break;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for the preferences window.

(3.1)
*/
OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			PrefsWindow_Remove();
			result = noErr; // event is completely handled
		}
	}
	
	return result;
}// receiveWindowClosing


/*!
Redraws the list display.  Use this when you have caused
it to change in some way (by changing a preference elsewhere
in the application, for example).

(3.1)
*/
void
refreshDisplay ()
{
	(OSStatus)UpdateDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDSets);
}// refreshDisplay


/*!
Removes any editable text area in the collections list.

(3.1)
*/
void
removeCollectionRenameUI ()
{
	(OSStatus)SetDataBrowserEditItem(gDataBrowserForCollections, kDataBrowserNoItem, kDataBrowserItemNoProperty);
}// removeCollectionRenameUI


/*!
Returns the preference class to use for settings,
based on the panel that is showing.  Most of the
time the class will be for general preferences,
however if a collection drawer is showing then a
collection class will be returned.

(3.1)
*/
Quills::Prefs::Class
returnCurrentPreferencesClass ()
{
	Quills::Prefs::Class	result = Quills::Prefs::GENERAL;
	Panel_Kind				currentPanelKind = Panel_ReturnKind(gCurrentPanel);
	
	
	assert(nullptr != currentPanelKind);
	if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorFormats, currentPanelKind,
												kCFCompareBackwards))
	{
		result = Quills::Prefs::FORMAT;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorMacros, currentPanelKind,
													kCFCompareBackwards))
	{
		result = Quills::Prefs::MACRO_SET;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorSessions, currentPanelKind,
													kCFCompareBackwards))
	{
		result = Quills::Prefs::SESSION;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorTerminals, currentPanelKind,
													kCFCompareBackwards))
	{
		result = Quills::Prefs::TERMINAL;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorTranslations, currentPanelKind,
													kCFCompareBackwards))
	{
		result = Quills::Prefs::TRANSLATION;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorWorkspaces, currentPanelKind,
													kCFCompareBackwards))
	{
		result = Quills::Prefs::WORKSPACE;
	}
	return result;
}// returnCurrentPreferencesClass


/*!
Returns the currently-selected item, which is actually
a Preferences_ContextRef.  The result is zero if the
selection is not found (but assert this can never
happen, because a selection should always exist).

(3.1)
*/
DataBrowserItemID
returnCurrentSelection ()
{
	DataBrowserItemID	result = 0;
	OSStatus			error = noErr;
	
	
	error = GetDataBrowserSelectionAnchor(gDataBrowserForCollections, &result, &result);
	if (noErr != error) result = 0;
	
	return result;
}// returnCurrentSelection


/*!
Sets the (only one) item currently selected in the collections
drawer, triggering callbacks to ensure the corresponding panel
user interface is updated for the new item (see
monitorDataBrowserItems()).

If the item is zero, then the default is selected.

(3.1)
*/
OSStatus
selectCollection	(DataBrowserItemID		inNewItem)
{
	DataBrowserItemID	itemList[] = { inNewItem };
	OSStatus			result = noErr;
	
	
	if (0 == inNewItem)
	{
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		Preferences_ContextRef		defaultContext = nullptr;
		
		
		prefsResult = Preferences_GetDefaultContext(&defaultContext, returnCurrentPreferencesClass());
		assert(kPreferences_ResultOK == prefsResult);
		itemList[0] = REINTERPRET_CAST(defaultContext, DataBrowserItemID);
	}
	
	result = SetDataBrowserSelectedItems(gDataBrowserForCollections, 1/* number of items */, itemList,
											kDataBrowserItemsAssign);
	
	return result;
}// selectCollection


/*!
Initializes the Preferences module if necessary, and
displays the Preferences window.

This is a low-level routine used by PrefsWindow_Display();
you MUST call choosePanel() if this is the first time
the window is displayed; otherwise, choosing a panel is
optional.

(3.1)
*/
void
showWindow ()
{
	if (nullptr == gPreferencesWindow) init();
	if (nullptr == gPreferencesWindow) Alert_ReportOSStatus(memPCErr, true/* assertion */);
	else
	{
		// display the window and handle events
		unless (IsWindowVisible(gPreferencesWindow)) ShowWindow(gPreferencesWindow);
		EventLoop_SelectBehindDialogWindows(gPreferencesWindow);
	}
}// showWindow


/*!
Resizes every installed panel to match the given size.
This is only intended to be used once, initially.

(3.1)
*/
void
sizePanels	(HISize const&		inInitialSize)
{
	std::for_each(gPanelList().begin(), gPanelList().end(),
					panelResizer(inInitialSize.width, inInitialSize.height, false/* is delta */));
}// sizePanels

} // anonymous namespace


@implementation PrefsWindow_Collection


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithPreferencesContext:(Preferences_ContextRef)		aContext
asDefault:(BOOL)										aFlag
{
	self = [super init];
	if (nil != self)
	{
		self->preferencesContext = aContext;
		if (nullptr != self->preferencesContext)
		{
			Preferences_RetainContext(self->preferencesContext);
		}
		self->isDefault = aFlag;
	}
	return self;
}// initWithPreferencesContext:asDefault:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	Preferences_ReleaseContext(&preferencesContext);
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

IMPORTANT:	The "boundName" key is ONLY required because older
			versions of Mac OS X do not seem to work properly
			when bound to the "description" accessor.  (Namely,
			the OS seems to stubbornly use its own "description"
			instead of invoking the right one.)  In the future
			this might be removed and rebound to "description".

(4.1)
*/
- (NSString*)
boundName
{
	NSString*	result = @"?";
	
	
	if (self->isDefault)
	{
		result = NSLocalizedStringFromTable(@"Default", @"PreferencesWindow", @"the name of the Default collection");
	}
	else if (false == Preferences_ContextIsValid(self->preferencesContext))
	{
		Console_Warning(Console_WriteValueAddress, "binding to source list has become invalid; preferences context", self->preferencesContext);
	}
	else
	{
		CFStringRef				nameCFString = nullptr;
		Preferences_Result		prefsResult = Preferences_ContextGetName(self->preferencesContext, nameCFString);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(nameCFString, NSString*);
		}
	}
	return result;
}
- (void)
setBoundName:(NSString*)	aString
{
	if (self->isDefault)
	{
		// changing the name is illegal
	}
	else if (false == Preferences_ContextIsValid(self->preferencesContext))
	{
		Console_Warning(Console_WriteValueAddress, "binding to source list has become invalid; preferences context", self->preferencesContext);
	}
	else
	{
		Preferences_Result		prefsResult = Preferences_ContextRename(self->preferencesContext, (CFStringRef)aString);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteValueCFString, "failed to rename context, given name", (CFStringRef)aString);
		}
	}
}// setBoundName:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEditable
{
	return (NO == self->isDefault);
}// isEditable


/*!
Accessor.

(4.1)
*/
- (NSString*)
description
{
	return [self boundName];
}
- (void)
setDescription:(NSString*)	aString
{
	[self setBoundName:aString];
}// setDescription:


/*!
Accessor.

(4.1)
*/
- (Preferences_ContextRef)
preferencesContext
{
	return preferencesContext;
}// preferencesContext


#pragma mark NSObject


/*!
Returns a hash code for this object (a value that should
ideally change whenever a property significant to "isEqualTo:"
is changed).

(4.1)
*/
- (unsigned int)
hash
{
	return ((unsigned int)self->preferencesContext);
}// hash


/*!
Returns true if the given object is considered equivalent
to this one (which is the case if its "preferencesContext"
is the same).

(4.1)
*/
- (BOOL)
isEqual:(id)	anObject
{
	BOOL	result = NO;
	
	
	if ([anObject isKindOfClass:[self class]])
	{
		PrefsWindow_Collection*		asCollection = (PrefsWindow_Collection*)anObject;
		
		
		result = ([asCollection preferencesContext] == [self preferencesContext]);
	}
	return result;
}// isEqual:


@end // PrefsWindow_Collection


@implementation PrefsWindow_Controller


static PrefsWindow_Controller*	gPrefsWindow_Controller = nil;


/*!
Returns the singleton.

(4.1)
*/
+ (id)
sharedPrefsWindowController
{
	if (nil == gPrefsWindow_Controller)
	{
		gPrefsWindow_Controller = [[[self class] allocWithZone:NULL] init];
	}
	return gPrefsWindow_Controller;
}// sharedPrefsWindowController


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"PrefsWindowCocoa"];
	if (nil != self)
	{
		self->currentPreferenceCollectionIndexes = [[NSIndexSet alloc] init];
		self->currentPreferenceCollections = [[NSMutableArray alloc] init];
		self->panelIDArray = [[NSMutableArray arrayWithCapacity:7/* arbitrary */] retain];
		self->panelsByID = [[NSMutableDictionary dictionaryWithCapacity:7/* arbitrary */] retain];
		self->windowSizesByID = [[NSMutableDictionary dictionaryWithCapacity:7/* arbitrary */] retain];
		self->windowMinSizesByID = [[NSMutableDictionary dictionaryWithCapacity:7/* arbitrary */] retain];
		self->extraWindowContentSize = NSZeroSize; // set later
		self->isSourceListHidden = NO; // set later
		self->activePanel = nil;
		self->translationsPanel = nil;
		self->fullScreenPanel = nil;
		
		// install a callback that finds out about changes to available preferences collections
		{
			Preferences_Result		error = kPreferences_ResultOK;
			
			
			self->preferenceChangeListener = [[ListenerModel_StandardListener alloc]
												initWithTarget:self
																eventFiredSelector:@selector(model:preferenceChange:context:)];
			
			error = Preferences_StartMonitoring([self->preferenceChangeListener listenerRef], kPreferences_ChangeContextName,
												false/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
			error = Preferences_StartMonitoring([self->preferenceChangeListener listenerRef], kPreferences_ChangeNumberOfContexts,
												true/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
		}
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
	(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
													kPreferences_ChangeContextName);
	(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
													kPreferences_ChangeNumberOfContexts);
	[preferenceChangeListener release];
	[currentPreferenceCollectionIndexes release];
	[currentPreferenceCollections release];
	[panelIDArray release];
	[panelsByID release];
	[windowSizesByID release];
	[windowMinSizesByID release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSIndexSet*)
currentPreferenceCollectionIndexes
{
	// TEMPORARY; should retrieve translation table that is saved in preferences
	return [[currentPreferenceCollectionIndexes retain] autorelease];
}
- (void)
setCurrentPreferenceCollectionIndexes:(NSIndexSet*)		indexes
{
	if (indexes != currentPreferenceCollectionIndexes)
	{
		UInt16						oldIndex = ([self->currentPreferenceCollectionIndexes count] > 0)
												? [self->currentPreferenceCollectionIndexes firstIndex]
												: 0;
		UInt16						newIndex = ([indexes count] > 0)
												? [indexes firstIndex]
												: 0;
		PrefsWindow_Collection*		oldCollection = (oldIndex < [currentPreferenceCollections count])
													? [currentPreferenceCollections objectAtIndex:oldIndex]
													: nil;
		PrefsWindow_Collection*		newCollection = (newIndex < [currentPreferenceCollections count])
													? [currentPreferenceCollections objectAtIndex:newIndex]
													: nil;
		Preferences_ContextRef		oldDataSet = (nil != oldCollection)
													? [oldCollection preferencesContext]
													: nullptr;
		Preferences_ContextRef		newDataSet = (nil != newCollection)
													? [newCollection preferencesContext]
													: nullptr;
		
		
		[currentPreferenceCollectionIndexes release];
		currentPreferenceCollectionIndexes = [indexes retain];
		
		// notify the panel that a new data set has been selected
		[[self->activePanel delegate] panelViewManager:self->activePanel
														didChangeFromDataSet:oldDataSet toDataSet:newDataSet];
	}
}// setCurrentPreferenceCollectionIndexes:


/*!
Accessor.

(4.1)
*/
- (NSArray*)
currentPreferenceCollections
{
	return [[currentPreferenceCollections retain] autorelease];
}
- (BOOL)
autoNotifyOnChangeToCurrentPreferenceCollections
{
	return NO;
}


/*!
Accessor.

(4.1)
*/
- (BOOL)
isSourceListHidden
{
	return isSourceListHidden;
}// isSourceListHidden
- (BOOL)
isSourceListShowing
{
	return (NO == isSourceListHidden);
}// isSourceListShowing
- (void)
setSourceListHidden:(BOOL)	aFlag
{
	// “cheat” by notifying of a change to the opposite property as well
	// (Cocoa bindings will automatically send the notification for the
	// non-inverted property derived from this method’s name)
	[self willChangeValueForKey:@"sourceListShowing"];
	
	isSourceListHidden = aFlag;
	
	[self didChangeValueForKey:@"sourceListShowing"];
}// setSourceListHidden:


#pragma mark Actions


/*!
Adds a new preferences collection.  Also immediately enters
editing mode on the new collection so that the user may
change its name.

(4.1)
*/
- (IBAction)
performAddNewPreferenceCollection:(id)	sender
{
#pragma unused(sender)
	// NOTE: notifications are sent when a context is created as a Favorite
	// so the source list should automatically resynchronize
	Preferences_ContextRef	newContext = Preferences_NewContextFromFavorites
											([self currentPreferencesClass], nullptr/* auto-set name */);
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_ContextSave(newContext);
	if (kPreferences_ResultOK != prefsResult)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteValue, "failed to save newly-created context; error", prefsResult);
	}
	
	// reset the selection to focus on the new item, and automatically
	// assume the user wants to rename it
	[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:
															([self->currentPreferenceCollections count] - 1)]];
	[self performRenamePreferenceCollection:nil];
}// performAddNewPreferenceCollection:


/*!
Invoked when the help button is clicked.

(4.1)
*/
- (IBAction)
performContextSensitiveHelp:(id)	sender
{
#pragma unused(sender)
	[self->activePanel performContextSensitiveHelp:sender];
}// performContextSensitiveHelp:


/*!
Adds a new preferences collection by making a copy of the
one selected in the source list.  Also immediately enters
editing mode on the new collection so that the user may
change its name.

(4.1)
*/
- (IBAction)
performDuplicatePreferenceCollection:(id)	sender
{
#pragma unused(sender)
	UInt16						selectedIndex = [self->sourceListTableView selectedRow];
	PrefsWindow_Collection*		baseCollection = [self->currentPreferenceCollections objectAtIndex:selectedIndex];
	Preferences_ContextRef		baseContext = [baseCollection preferencesContext];
	
	
	if (false == Preferences_ContextIsValid(baseContext))
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "failed to clone context; current selection is invalid");
	}
	else
	{
		// NOTE: notifications are sent when a context is created as a Favorite
		// so the source list should automatically resynchronize
		Preferences_ContextRef	newContext = Preferences_NewCloneContext(baseContext, false/* detach */);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSave(newContext);
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteValue, "failed to save duplicated context; error", prefsResult);
		}
		
		// reset the selection to focus on the new item, and automatically
		// assume the user wants to rename it
		[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:
																([self->currentPreferenceCollections count] - 1)]];
		[self performRenamePreferenceCollection:nil];
	}
}// performDuplicatePreferenceCollection:


/*!
Saves the contents of a preferences collection to a file.

(4.1)
*/
- (IBAction)
performExportPreferenceCollectionToFile:(id)	sender
{
#pragma unused(sender)
	NSSavePanel*	savePanel = [NSSavePanel savePanel];
	CFStringRef		saveFileCFString = nullptr;
	CFStringRef		promptCFString = nullptr;
	
	
	(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultExportPreferences, saveFileCFString);
	(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptSavePrefs, promptCFString);
	[savePanel setMessage:(NSString*)promptCFString];
	
	[savePanel beginSheetForDirectory:nil file:(NSString*)saveFileCFString
										modalForWindow:[self window] modalDelegate:self
										didEndSelector:@selector(didEndExportPanel:returnCode:contextInfo:)
										contextInfo:nullptr];
}// performExportPreferenceCollectionToFile:


/*!
Adds a new preferences collection by asking the user for a
file from which to import settings.

(4.1)
*/
- (IBAction)
performImportPreferenceCollectionFromFile:(id)	sender
{
#pragma unused(sender)
	NSOpenPanel*	openPanel = [NSOpenPanel openPanel];
	NSArray*		allowedTypes = [NSArray arrayWithObjects:
												@"com.apple.property-list",
												@"plist"/* redundant, needed for older systems */,
												@"xml",
												nil];
	CFStringRef		promptCFString = nullptr;
	
	
	(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptOpenPrefs, promptCFString);
	[openPanel setMessage:(NSString*)promptCFString];
	
	[openPanel beginSheetForDirectory:nil file:nil types:allowedTypes
										modalForWindow:[self window] modalDelegate:self
										didEndSelector:@selector(didEndImportPanel:returnCode:contextInfo:)
										contextInfo:nullptr];
}// performImportPreferenceCollectionFromFile:


/*!
Deletes the currently-selected preference collection from
the source list (and saved user preferences).

(4.1)
*/
- (IBAction)
performRemovePreferenceCollection:(id)	sender
{
#pragma unused(sender)
	UInt16		selectedIndex = [self->sourceListTableView selectedRow];
	
	
	if (0 == selectedIndex)
	{
		// the Default collection; this should never be possible to remove
		// (and should be prevented by other guards before this point, but
		// just in case...)
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "attempt to delete the Default context");
	}
	else
	{
		PrefsWindow_Collection*		deadCollection = [self->currentPreferenceCollections objectAtIndex:selectedIndex];
		Preferences_ContextRef		deadContext = [deadCollection preferencesContext];
		
		
		if (false == Preferences_ContextIsValid(deadContext))
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to delete context; current selection is invalid");
		}
		else
		{
			// NOTE: notifications are sent when a context is deleted from Favorites
			// so the source list should automatically resynchronize
			Preferences_Result		prefsResult = Preferences_ContextDeleteSaved(deadContext);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				Sound_StandardAlert();
				Console_Warning(Console_WriteValue, "failed to delete currently-selected context; error", prefsResult);
			}
			
			// reset the selection, as it will be invalidated when the target item is removed
			[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:0]];
		}
	}
}// performRemovePreferenceCollection:


/*!
Enters editing mode on the currently-selected preference
collection in the source list.

(4.1)
*/
- (IBAction)
performRenamePreferenceCollection:(id)	sender
{
#pragma unused(sender)
	UInt16		selectedIndex = [self->sourceListTableView selectedRow];
	
	
	// do not allow the Default item to be edited (this is
	// also ensured by the "isEditable" method on the objects
	// in the array)
	if (0 == selectedIndex)
	{
		Sound_StandardAlert();
	}
	else
	{
		[self->sourceListTableView editColumn:0 row:selectedIndex withEvent:nil select:NO];
		[[self->sourceListTableView currentEditor] selectAll:nil];
	}
}// performRenamePreferenceCollection:


/*!
Responds to a “remove collection” segment click.

(No other commands need to be handled because they are
already bound to items in menus that the other segments
display.)

(4.1)
*/
- (IBAction)
performSegmentedControlAction:(id)	sender
{
	if ([sender isKindOfClass:[NSSegmentedControl class]])
	{
		NSSegmentedControl*		segments = (NSSegmentedControl*)sender;
		
		
		// IMPORTANT: this should agree with the button arrangement
		// in the Preferences window’s NIB file...
		if (0 == [segments selectedSegment])
		{
			// this is redundantly handled as the “quick click” action
			// for the button; it is also in the segment’s pop-up menu
			[self performAddNewPreferenceCollection:sender];
		}
		else if (1 == [segments selectedSegment])
		{
			[self performRemovePreferenceCollection:sender];
		}
	}
	else
	{
		Console_Warning(Console_WriteLine, "received 'performSegmentedControlAction:' message from unexpected sender");
	}
}// performSegmentedControlAction:


#pragma mark NSColorPanel


/*!
Since this window controller is in the responder chain, it
may receive events from NSColorPanel that have not been
handled anywhere else.  This responds by forwarding the
message to the active panel if the active panel implements
a "changeColor:" method.

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
Since this window controller is in the responder chain, it
may receive events from NSFontPanel that have not been
handled anywhere else.  This responds by forwarding the
message to the active panel if the active panel implements
a "changeFont:" method.

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


#pragma mark NSTableDataSource


/*!
For drag-and-drop support; adds the data for the given rows
to the specified pasteboard.

NOTE:	This is defined for Mac OS X 10.3 support; it may be
		removed in the future.  See also the preferred method
		"tableView:writeRowIndexes:toPasteboard:".

(4.1)
*/
- (BOOL)
tableView:(NSTableView*)		aTableView
writeRows:(NSArray*)			aRowArray
toPasteboard:(NSPasteboard*)	aPasteboard
{
#pragma unused(aTableView)
	NSEnumerator*		eachRowIndex = [aRowArray objectEnumerator];
	BOOL				result = YES; // initially...
	NSMutableIndexSet*	indexSet = [[NSMutableIndexSet alloc] init];
	
	
	// drags are confined to the table, so it’s only necessary to copy row numbers;
	// note that the Mac OS X 10.3 version gives arrays of NSNumber and the actual
	// representation (in "tableView:writeRowIndexes:toPasteboard:") is NSIndexSet*
	while (NSNumber* numberObject = [eachRowIndex nextObject])
	{
		if (0 == [numberObject intValue])
		{
			// the item at index zero is Default, which may not be moved
			result = NO;
			break;
		}
		[indexSet addIndex:[numberObject intValue]];
	}
	
	if (result)
	{
		NSData*		copiedData = [NSKeyedArchiver archivedDataWithRootObject:indexSet];
		
		
		[aPasteboard declareTypes:[NSArray arrayWithObject:kMy_PrefsWindowSourceListDataType] owner:self];
		[aPasteboard setData:copiedData forType:kMy_PrefsWindowSourceListDataType];
	}
	
	[indexSet release];
	
	return result;
}// tableView:writeRows:toPasteboard:


/*!
For drag-and-drop support; adds the data for the given rows
to the specified pasteboard.

This is the version used by Mac OS X 10.4 and beyond; see
also "tableView:writeRows:toPasteboard:".

(4.1)
*/
- (BOOL)
tableView:(NSTableView*)		aTableView
writeRowIndexes:(NSIndexSet*)	aRowSet
toPasteboard:(NSPasteboard*)	aPasteboard
{
#pragma unused(aTableView)
	BOOL	result = NO;
	
	
	// the item at index zero is Default, which may not be moved
	if (NO == [aRowSet containsIndex:0])
	{
		// drags are confined to the table, so it’s only necessary to copy row numbers
		NSData*		copiedData = [NSKeyedArchiver archivedDataWithRootObject:aRowSet];
		
		
		[aPasteboard declareTypes:[NSArray arrayWithObject:kMy_PrefsWindowSourceListDataType] owner:self];
		[aPasteboard setData:copiedData forType:kMy_PrefsWindowSourceListDataType];
		result = YES;
	}
	return result;
}// tableView:writeRowIndexes:toPasteboard:


/*!
For drag-and-drop support; validates a drag operation.

(4.1)
*/
- (NSDragOperation)
tableView:(NSTableView*)							aTableView
validateDrop:(id< NSDraggingInfo >)					aDrop
proposedRow:(int)									aTargetRow
proposedDropOperation:(NSTableViewDropOperation)	anOperation
{
#pragma unused(aTableView, aDrop)
	NSDragOperation		result = NSDragOperationMove;
	
	
	switch (anOperation)
	{
	case NSTableViewDropOn:
		// rows may only be inserted above, not dropped on top
		result = NSDragOperationNone;
		break;
	
	case NSTableViewDropAbove:
		if (0 == aTargetRow)
		{
			// row zero always contains the Default item so nothing else may move there
			result = NSDragOperationNone;
		}
		break;
	
	default:
		// ???
		result = NSDragOperationNone;
		break;
	}
	
	return result;
}// tableView:validateDrop:proposedRow:proposedDropOperation:


/*!
For drag-and-drop support; moves the specified table row.

(4.1)
*/
- (BOOL)
tableView:(NSTableView*)					aTableView
acceptDrop:(id< NSDraggingInfo >)			aDrop
row:(int)									aTargetRow
dropOperation:(NSTableViewDropOperation)	anOperation
{
#pragma unused(aTableView, aDrop)
	BOOL	result = NO;
	
	
	switch (anOperation)
	{
	case NSTableViewDropOn:
		// rows may only be inserted above, not dropped on top
		result = NO;
		break;
	
	case NSTableViewDropAbove:
		{
			NSPasteboard*	pasteboard = [aDrop draggingPasteboard];
			NSData*			rowIndexSetData = [pasteboard dataForType:kMy_PrefsWindowSourceListDataType];
			NSIndexSet*		rowIndexes = [NSKeyedUnarchiver unarchiveObjectWithData:rowIndexSetData];
			
			
			if (nil != rowIndexes)
			{
				int		draggedRow = [rowIndexes firstIndex];
				
				
				if (draggedRow != aTargetRow)
				{
					PrefsWindow_Collection*		movingCollection = [self->currentPreferenceCollections objectAtIndex:draggedRow];
					Preferences_Result			prefsResult = kPreferences_ResultOK;
					
					
					if (aTargetRow >= STATIC_CAST([self->currentPreferenceCollections count], int))
					{
						// move to the end of the list (not a real target row)
						PrefsWindow_Collection*		targetCollection = [self->currentPreferenceCollections lastObject];
						
						
						if ([targetCollection preferencesContext] != [movingCollection preferencesContext])
						{
							// NOTE: notifications are sent when a context in Favorites is rearranged
							// so the source list should automatically resynchronize
							prefsResult = Preferences_ContextRepositionRelativeToContext
											([movingCollection preferencesContext], [targetCollection preferencesContext],
												false/* insert before */);
						}
					}
					else
					{
						PrefsWindow_Collection*		targetCollection = [self->currentPreferenceCollections objectAtIndex:aTargetRow];
						
						
						if ([targetCollection preferencesContext] != [movingCollection preferencesContext])
						{
							// NOTE: notifications are sent when a context in Favorites is rearranged
							// so the source list should automatically resynchronize
							prefsResult = Preferences_ContextRepositionRelativeToContext
											([movingCollection preferencesContext], [targetCollection preferencesContext],
												true/* insert before */);
						}
					}
					
					if (kPreferences_ResultOK != prefsResult)
					{
						Sound_StandardAlert();
						Console_Warning(Console_WriteValue, "failed to reorder preferences contexts; error", prefsResult);
					}
					
					result = YES;
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// tableView:acceptDrop:row:dropOperation:


#pragma mark NSToolbarDelegate


/*!
Responds when the specified kind of toolbar item should be
constructed for the given toolbar.

(4.1)
*/
- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	flag
{
#pragma unused(toolbar, flag)
	NSToolbarItem*		result = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
	Panel_ViewManager*	itemPanel = [self->panelsByID objectForKey:itemIdentifier];
	
	
	// NOTE: no need to handle standard items here
	assert(nil != itemPanel);
	[result setLabel:[itemPanel panelName]];
	[result setImage:[itemPanel panelIcon]];
	[result setAction:[itemPanel panelDisplayAction]];
	[result setTarget:[itemPanel panelDisplayTarget]];
	
	return result;
}// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:


/*!
Returns the identifiers for the kinds of items that can appear
in the given toolbar.

(4.1)
*/
- (NSArray*)
toolbarAllowedItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	NSArray*	result = [[self->panelIDArray copy] autorelease];
	
	
	return result;
}// toolbarAllowedItemIdentifiers:


/*!
Returns the identifiers for the items that will appear in the
given toolbar whenever the user has not customized it.  Since
this toolbar is not customized, the result is the same as
"toolbarAllowedItemIdentifiers:".

(4.1)
*/
- (NSArray*)
toolbarDefaultItemIdentifiers:(NSToolbar*)	toolbar
{
	return [self toolbarAllowedItemIdentifiers:toolbar];
}// toolbarDefaultItemIdentifiers:


/*!
Returns the identifiers for the items that may appear in a
“selected” state (which is all of them).

(4.1)
*/
- (NSArray*)
toolbarSelectableItemIdentifiers:(NSToolbar*)	toolbar
{
	return [self toolbarAllowedItemIdentifiers:toolbar];
}// toolbarSelectableItemIdentifiers:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.1)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != windowFirstResponder);
	assert(nil != windowLastResponder);
	assert(nil != containerTabView);
	assert(nil != sourceListContainer);
	assert(nil != sourceListTableView);
	assert(nil != sourceListSegmentedControl);
	assert(nil != horizontalDividerView);
	
	NSRect const	kOriginalContainerFrame = [self->containerTabView frame];
	
	
	// programmatically remove the toolbar button
	if ([[self window] respondsToSelector:@selector(setShowsToolbarButton:)])
	{
		[[self window] setShowsToolbarButton:NO];
	}
	
	// create all panels
	{
		Panel_ViewManager*		newViewMgr = nil;
		
		
		// “General” panel
		self->generalPanel = [[PrefPanelGeneral_ViewManager alloc] init];
		newViewMgr = self->generalPanel;
		[newViewMgr setPanelDisplayAction:@selector(performDisplayPrefPanelGeneral:)];
		[newViewMgr setPanelDisplayTarget:self];
		[self->panelIDArray addObject:[newViewMgr panelIdentifier]];
		[self->panelsByID setObject:newViewMgr forKey:[newViewMgr panelIdentifier]];
		[newViewMgr release], newViewMgr = nil;
		
		// “Formats” panel
		self->formatsPanel = [[PrefPanelFormats_ViewManager alloc] init];
		newViewMgr = self->formatsPanel;
		[newViewMgr setPanelDisplayAction:@selector(performDisplayPrefPanelFormats:)];
		[newViewMgr setPanelDisplayTarget:self];
		[self->panelIDArray addObject:[newViewMgr panelIdentifier]];
		[self->panelsByID setObject:newViewMgr forKey:[newViewMgr panelIdentifier]];
		[newViewMgr release], newViewMgr = nil;
		
		// “Translations” panel
		self->translationsPanel = [[PrefPanelTranslations_ViewManager alloc] init];
		newViewMgr = self->translationsPanel;
		[newViewMgr setPanelDisplayAction:@selector(performDisplayPrefPanelTranslations:)];
		[newViewMgr setPanelDisplayTarget:self];
		[self->panelIDArray addObject:[newViewMgr panelIdentifier]];
		[self->panelsByID setObject:newViewMgr forKey:[newViewMgr panelIdentifier]];
		[newViewMgr release], newViewMgr = nil;
		
		// “Full Screen” panel
		self->fullScreenPanel = [[PrefPanelFullScreen_ViewManager alloc] init];
		newViewMgr = self->fullScreenPanel;
		[newViewMgr setPanelDisplayAction:@selector(performDisplayPrefPanelFullScreen:)];
		[newViewMgr setPanelDisplayTarget:self];
		[self->panelIDArray addObject:[newViewMgr panelIdentifier]];
		[self->panelsByID setObject:newViewMgr forKey:[newViewMgr panelIdentifier]];
		[newViewMgr release], newViewMgr = nil;
		
		// other panels TBD
	}
	
	// create toolbar; has to be done programmatically, because
	// IB only supports them in 10.5; which makes sense, you know,
	// since toolbars have only been in the OS since 10.0, and
	// hardly any applications would have found THOSE useful...
	{
		NSString*		toolbarID = @"PreferencesToolbar"; // do not ever change this; that would only break user preferences
		NSToolbar*		windowToolbar = [[[NSToolbar alloc] initWithIdentifier:toolbarID] autorelease];
		
		
		[windowToolbar setAllowsUserCustomization:NO];
		[windowToolbar setAutosavesConfiguration:NO];
		[windowToolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
		[windowToolbar setSizeMode:NSToolbarSizeModeRegular];
		[windowToolbar setDelegate:self];
		[[self window] setToolbar:windowToolbar];
	}
	
	// “indent” the toolbar items slightly so they are further away
	// from the window’s controls (such as the close button)
	for (UInt16 i = 0; i < 2/* arbitrary */; ++i)
	{
		[[[self window] toolbar] insertItemWithItemIdentifier:NSToolbarSpaceItemIdentifier atIndex:0];
	}
	
	// remember how much bigger the window’s content is than the container view
	{
		NSRect	contentFrame = [[self window] contentRectForFrameRect:[[self window] frame]];
		
		
		self->extraWindowContentSize = NSMakeSize(NSWidth(contentFrame) - NSWidth(kOriginalContainerFrame),
													NSHeight(contentFrame) - NSHeight(kOriginalContainerFrame));
	}
	
	// update the user interface using the new panels
	{
		NSEnumerator*	eachObject = [self->panelIDArray objectEnumerator];
		
		
		// add each panel as a subview and hide all except the first;
		// give them all the initial size of the window (in reality
		// they will be changed to a different size before display)
		eachObject = [self->panelIDArray objectEnumerator];
		while (NSString* panelIdentifier = [eachObject nextObject])
		{
			Panel_ViewManager*	viewMgr = [self->panelsByID objectForKey:panelIdentifier];
			NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:panelIdentifier];
			NSView*				panelContainer = [viewMgr managedView];
			NSRect				panelFrame = kOriginalContainerFrame;
			NSSize				panelIdealSize = panelFrame.size;
			
			
			[[viewMgr delegate] panelViewManager:viewMgr requestingIdealSize:&panelIdealSize];
			
			// due to layout constraints, it is sufficient to make the
			// panel container match the parent view frame (except with
			// local origin); once the window is resized to its target
			// frame, the panel will automatically resize again
			panelFrame.origin.x = 0;
			panelFrame.origin.y = 0;
			[panelContainer setFrame:panelFrame];
			[self->panelsByID setObject:viewMgr forKey:panelIdentifier];
			
			// initialize the “remembered window size” for each panel
			// (this changes whenever the user resizes the window)
			{
				NSArray*	sizeArray = nil;
				NSSize		windowSize = NSMakeSize(panelIdealSize.width + self->extraWindowContentSize.width,
													panelIdealSize.height + self->extraWindowContentSize.height);
				
				
				// only inspector-style windows include space for a source list
				if (kPanel_EditTypeInspector != [viewMgr panelEditType])
				{
					windowSize.width -= NSWidth([self->sourceListContainer frame]);
				}
				
				// choose a frame size that uses the panel’s ideal size
				sizeArray = [NSArray arrayWithObjects:
										[NSNumber numberWithFloat:windowSize.width],
										[NSNumber numberWithFloat:windowSize.height],
										nil];
				[self->windowSizesByID setObject:sizeArray forKey:panelIdentifier];
				
				// also require (for now, at least) that the window be
				// no smaller than this initial size, whenever this
				// particular panel is displayed
				[self->windowMinSizesByID setObject:sizeArray forKey:panelIdentifier];
			}
			
			[tabItem setView:panelContainer];
			[tabItem setInitialFirstResponder:[viewMgr logicalFirstResponder]];
			
			[self->containerTabView addTabViewItem:tabItem];
			[tabItem release];
		}
	}
	
	// enable drag-and-drop in the source list
	[self->sourceListTableView registerForDraggedTypes:[NSArray arrayWithObjects:kMy_PrefsWindowSourceListDataType, nil]];
	
	// on Mac OS X versions prior to 10.5 there is no concept of
	// “bottom chrome” so a horizontal line is included to produce
	// a similar sort of divider; on later Mac OS X versions however
	// this should be hidden so that the softer gray can show through
	if (FlagManager_Test(kFlagOS10_5API))
	{
		[self->horizontalDividerView setHidden:YES];
	}
	
	// the “source list” style is not respected prior to Mac OS X 10.5
	// (and worse, it’s interpreted as a horrible background color);
	// programmatically override the NIB setting on older OS versions
	if (false == FlagManager_Test(kFlagOS10_5API))
	{
		// color should be fixed
		[self->sourceListTableView setBackgroundColor:[NSColor whiteColor]];
		
		// the appearance and size of the segmented control is different
		{
			NSRect	frame = [self->sourceListSegmentedControl frame];
			
			
			--(frame.origin.y);
			++(frame.size.height);
			[self->sourceListSegmentedControl setFrame:frame];
		}
	}
	
	// show the first panel while simultaneously resizing the
	// window to an appropriate frame and showing any auxiliary
	// interface that the panel requests (such as a source list)
	[self displayPanel:[self->panelsByID objectForKey:[self->panelIDArray objectAtIndex:0]] withAnimation:NO];
}// windowDidLoad


@end // PrefsWindow_Controller


@implementation PrefsWindow_Controller (PrefsWindow_ControllerInternal)


#pragma mark New Methods


/*!
Returns the class of preferences that are currently being
edited.  This determines the contents of the source list,
the type of context that is created when the user adds
something new, etc.

(4.1)
*/
- (Quills::Prefs::Class)
currentPreferencesClass
{
	Quills::Prefs::Class	result = [self->activePanel preferencesClass];
	
	
	return result;
}// currentPreferencesClass


/*!
Returns the context from which the user interface is
currently displayed (and the context that is changed when
the user modifies the interface).

For panels that use the source list this will match the
selection in the list.  For other panels it will be the
default context for the preferences class of the active
panel.

(4.1)
*/
- (Preferences_ContextRef)
currentPreferencesContext
{
	Preferences_ContextRef		result = nullptr;
	
	
	if (kPanel_EditTypeInspector == [self->activePanel panelEditType])
	{
		UInt16						selectedIndex = [self->sourceListTableView selectedRow];
		PrefsWindow_Collection*		selectedCollection = [self->currentPreferenceCollections objectAtIndex:selectedIndex];
		
		
		result = [selectedCollection preferencesContext];
	}
	else
	{
		Preferences_Result	prefsResult = Preferences_GetDefaultContext(&result, [self currentPreferencesClass]);
		
		
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	assert(Preferences_ContextIsValid(result));
	return result;
}// currentPreferencesContext


/*!
A selector of the form required by NSSavePanel; it responds to
an NSOKButton return code by saving the current preferences
context to a file.

(4.1)
*/
- (void)
didEndExportPanel:(NSSavePanel*)	aPanel
returnCode:(int)					aReturnCode
contextInfo:(void*)					aContextPtr
{
#pragma unused(aContextPtr)
	if (NSOKButton == aReturnCode)
	{
		// export the contents of the preferences context (the Preferences
		// module automatically ignores settings that are not part of the
		// right class, e.g. so that Default does not create a huge file)
		CFURLRef			asURL = BRIDGE_CAST([aPanel URL], CFURLRef);
		Preferences_Result	writeResult = Preferences_ContextSaveAsXMLFileURL([self currentPreferencesContext], asURL);
		
		
		if (kPreferences_ResultOK != writeResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteValueCFString, "unable to save file, URL",
							(CFStringRef)[((NSURL*)asURL) absoluteString]);
			Console_Warning(Console_WriteValue, "error", writeResult);
		}
	}
}// didEndExportPanel:returnCode:contextInfo:


/*!
A selector of the form required by NSOpenPanel; it responds to
an NSOKButton return code by opening the selected file(s).
The files are opened via Apple Events because there are already
event handlers defined that have the appropriate effect on
Preferences.

(4.1)
*/
- (void)
didEndImportPanel:(NSOpenPanel*)	aPanel
returnCode:(int)					aReturnCode
contextInfo:(void*)					aContextPtr
{
#pragma unused(aContextPtr)
	if (NSOKButton == aReturnCode)
	{
		// open the files via Apple Events
		NSArray*		toOpen = [aPanel URLs];
		NSEnumerator*	forURLs = [toOpen objectEnumerator];
		
		
		while (NSURL* currentFileURL = [forURLs nextObject])
		{
			FSRef		fileRef;
			OSStatus	error = noErr;
			
			
			if (CFURLGetFSRef((CFURLRef)currentFileURL, &fileRef))
			{
				error = FileUtilities_OpenDocument(fileRef);
			}
			else
			{
				error = kURLInvalidURLError;
			}
			
			if (noErr != error)
			{
				// TEMPORARY; should probably display a more user-friendly alert for this...
				Sound_StandardAlert();
				Console_Warning(Console_WriteValueCFString, "unable to open file, URL",
								(CFStringRef)[currentFileURL absoluteString]);
				Console_Warning(Console_WriteValue, "error", error);
			}
		}
	}
}// didEndImportPanel:returnCode:contextInfo:


/*!
If the specified panel is not the active panel, hides
the active panel and then displays the given panel.

The size of the window is synchronized with the new
panel, and the previous window size is saved (each
panel has a unique window size, allowing the user to
choose the ideal layout for each one).

(4.1)
*/
- (void)
displayPanel:(Panel_ViewManager*)	aPanel
withAnimation:(BOOL)				isAnimated
{
	if (aPanel != self->activePanel)
	{
		BOOL	wasShowingSourceList = [self isSourceListShowing];
		BOOL	willShowSourceList = wasShowingSourceList; // initially...
		
		
		if (nil != self->activePanel)
		{
			// remember the window size that the user wanted for the previous panel
			NSRect		contentRect = [[self window] contentRectForFrameRect:[[self window] frame]];
			NSSize		containerSize = NSMakeSize(NSWidth(contentRect), NSHeight(contentRect));
			NSArray*	sizeArray = [NSArray arrayWithObjects:
												[NSNumber numberWithFloat:containerSize.width],
												[NSNumber numberWithFloat:containerSize.height],
												nil];
			
			
			[self->windowSizesByID setObject:sizeArray forKey:[self->activePanel panelIdentifier]];
		}
		
		// show the new panel
		{
			Panel_ViewManager*	oldPanel = self->activePanel;
			Panel_ViewManager*	newPanel = aPanel;
			
			
			[[oldPanel delegate] panelViewManager:oldPanel willChangePanelVisibility:kPanel_VisibilityHidden];
			[[newPanel delegate] panelViewManager:newPanel willChangePanelVisibility:kPanel_VisibilityDisplayed];
			self->activePanel = newPanel;
			[[[self window] toolbar] setSelectedItemIdentifier:[newPanel panelIdentifier]];
			[self->containerTabView selectTabViewItemWithIdentifier:[newPanel panelIdentifier]];
			[[oldPanel delegate] panelViewManager:oldPanel didChangePanelVisibility:kPanel_VisibilityHidden];
			[[newPanel delegate] panelViewManager:newPanel didChangePanelVisibility:kPanel_VisibilityDisplayed];
		}
		
		// determine if the new panel needs a source list
		willShowSourceList = (kPanel_EditTypeInspector == [self->activePanel panelEditType]);
		
		// if necessary display the source list of available preference collections
		if (willShowSourceList != wasShowingSourceList)
		{
			// change the source list visibility
			[self updateUserInterfaceForSourceListTransition:[NSNumber numberWithBool:willShowSourceList]];
			[self setSourceListHidden:(NO == willShowSourceList)];
		}
		else
		{
			// source list is still needed, but it will have new content (note
			// that this step is implied if the visibility changes above)
			[self rebuildSourceList];
		}
		
		// set the window to the size for the new panel (if none, use the
		// size that was originally used for the panel in its NIB)
		{
			NSRect		originalFrame = [[self window] frame];
			NSRect		newWindowFrame = NSZeroRect; // set later
			NSRect		newContentRect = [[self window] contentRectForFrameRect:originalFrame]; // initially...
			NSSize		minSize = [[self window] minSize];
			NSArray*	sizeArray = [self->windowSizesByID objectForKey:[aPanel panelIdentifier]];
			NSArray*	minSizeArray = [self->windowMinSizesByID objectForKey:[aPanel panelIdentifier]];
			
			
			assert(nil != sizeArray);
			assert(nil != minSizeArray);
			
			// constrain the window appropriately for the panel that is displayed
			{
				assert(2 == [minSizeArray count]);
				Float32		newMinWidth = [[minSizeArray objectAtIndex:0] floatValue];
				Float32		newMinHeight = [[minSizeArray objectAtIndex:1] floatValue];
				
				
				minSize = NSMakeSize(newMinWidth, newMinHeight);
				[[self window] setContentMinSize:minSize];
			}
			
			// start with the restored size
			assert(2 == [sizeArray count]);
			newContentRect.size.width = MAX([[sizeArray objectAtIndex:0] floatValue], minSize.width);
			newContentRect.size.height = MAX([[sizeArray objectAtIndex:1] floatValue], minSize.height);
			
			// try to keep the window on screen; also constrain it to
			// its minimum size
			{
				NSRect	unconstrainedFrame = [[self window] frameRectForContentRect:newContentRect];
				
				
				newWindowFrame = unconstrainedFrame;
				
				newWindowFrame = NSIntersectionRect(newWindowFrame, [[[self window] screen] frame]);
				newWindowFrame.size.width = MAX(NSWidth(newWindowFrame), [[self window] minSize].width);
				newWindowFrame.size.height = MAX(NSHeight(newWindowFrame), [[self window] minSize].height);
				
				// since the coordinate system anchors the window at the
				// bottom-left corner and the intent is to keep the top-left
				// corner constant, calculate an appropriate new origin when
				// changing the size (don’t just change the size by itself);
				// do not do this if the window was constrained because the
				// origin and/or size will have changed for other reasons
				if (unconstrainedFrame.origin.y == newWindowFrame.origin.y)
				{
					newWindowFrame.origin.y += (NSHeight(originalFrame) - NSHeight(newWindowFrame));
				}
			}
			
			// resize the window
			[[self window] setFrame:newWindowFrame display:YES animate:isAnimated];
		}
	}
}// displayPanel:withAnimation:


/*!
Called when a monitored preference is changed.  See the
initializer for the set of events that is monitored.

(4.1)
*/
- (void)
model:(ListenerModel_Ref)				aModel
preferenceChange:(ListenerModel_Event)	anEvent
context:(void*)							aContext
{
#pragma unused(aModel, aContext)
	switch (anEvent)
	{
	case kPreferences_ChangeContextName:
		// a context has been renamed; refresh the list
		[self rebuildSourceList];
		break;
	
	case kPreferences_ChangeNumberOfContexts:
		// contexts were added or removed; destroy and rebuild the list
		[self rebuildSourceList];
		break;
	
	default:
		// ???
		break;
	}
}// model:preferenceChange:context:


/*!
Rebuilds the array that the source list is bound to, causing
potentially a new list of preferences contexts to be displayed.
The current panel’s preferences class is used to find an
appropriate list of contexts.

(4.1)
*/
- (void)
rebuildSourceList
{
	std::vector< Preferences_ContextRef >	contextList;
	NSIndexSet*								previousSelection = [self currentPreferenceCollectionIndexes];
	id										selectedObject = nil;
	
	
	if (nil == previousSelection)
	{
		previousSelection = [NSIndexSet indexSetWithIndex:0];
	}
	
	if ([previousSelection firstIndex] < [self->currentPreferenceCollections count])
	{
		selectedObject = [self->currentPreferenceCollections objectAtIndex:[previousSelection firstIndex]];
	}
	
	[self willChangeValueForKey:@"currentPreferenceCollections"];
	
	[self->currentPreferenceCollections removeAllObjects];
	if (Preferences_GetContextsInClass([self currentPreferencesClass], contextList))
	{
		// the first item in the returned list is always the default
		BOOL	haveAddedDefault = NO;
		
		
		for (std::vector< Preferences_ContextRef >::const_iterator toContext = contextList.begin();
				toContext != contextList.end(); ++toContext)
		{
			BOOL						isDefault = (NO == haveAddedDefault);
			PrefsWindow_Collection*		newCollection = [[PrefsWindow_Collection alloc]
															initWithPreferencesContext:(*toContext)
																						asDefault:isDefault];
			
			
			[self->currentPreferenceCollections addObject:newCollection];
			[newCollection release], newCollection = nil;
			haveAddedDefault = YES;
		}
	}
	
	[self didChangeValueForKey:@"currentPreferenceCollections"];
	
	// attempt to preserve the selection
	{
		unsigned int	newIndex = [self->currentPreferenceCollections indexOfObject:selectedObject];
		
		
		if (NSNotFound != newIndex)
		{
			[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:newIndex]];
		}
	}
}// rebuildSourceList


/*!
Performs user interface modifications appropriate for changing the
visibility of the source list that shows preference collections.

This is a separate method that accepts an object parameter so that
it can be easily called from the main thread using the NSObject
method "performSelectorOnMainThread:withObject:waitUntilDone:".
See "setSourceListHidden:".

NOTE:	Some interface changes (such as hiding or showing buttons)
		are achieved with Cocoa bindings to the appropriate states,
		so there are fewer adjustments here than you might expect.

(4.1)
*/
- (void)
updateUserInterfaceForSourceListTransition:(id)		anNSNumberBoolForNewVisibleState
{
#if 0
	UInt16 const	kIndentationLevels = 5; // arbitrary
#endif
	NSRect			newContainerFrame = [self->containerTabView frame];
	NSRect			windowContentFrame = [[self window] contentRectForFrameRect:[[self window] frame]];
	
	
	if ([anNSNumberBoolForNewVisibleState boolValue])
	{
	#if 0
		// “indent” the toolbar so that the items do not appear above the source list
		for (UInt16 i = 0; i < kIndentationLevels; ++i)
		{
			[[[self window] toolbar] insertItemWithItemIdentifier:NSToolbarSpaceItemIdentifier atIndex:0];
		}
	#endif
		
		// move the panel next to the source list
		newContainerFrame.origin.x = NSWidth([self->sourceListContainer frame]);
		newContainerFrame.size.width = (NSWidth(windowContentFrame) - NSWidth([self->sourceListContainer frame]));
		[self->containerTabView setFrame:newContainerFrame];
		
		// refresh the content (should not be needed, but this is a good place to do it;
		// note that notifications trigger rebuilds whenever preferences actually change)
		[self rebuildSourceList];
	}
	else
	{
	#if 0
		// remove the “indent” from the toolbar (IMPORTANT: repeat as many times as spaces were originally added, below)
		if ([[[[self window] toolbar] items] count] > kIndentationLevels)
		{
			for (UInt16 i = 0; i < kIndentationLevels; ++i)
			{
				[[[self window] toolbar] removeItemAtIndex:0];
			}
		}
	#endif
		
		// move the panel to the edge of the window; adjust the width to keep the
		// layout bindings from over-padding the opposite side however
		newContainerFrame.origin.x = 0;
		newContainerFrame.size.width = NSWidth(windowContentFrame);
		[self->containerTabView setFrame:newContainerFrame];
	}
}// updateUserInterfaceForSourceListTransition:


#pragma mark Actions


/*!
Brings the “Formats” panel to the front.

(4.1)
*/
- (void)
performDisplayPrefPanelFormats:(id)		sender
{
#pragma unused(sender)
	[self displayPanel:self->formatsPanel withAnimation:YES];
}// performDisplayPrefPanelFormats:


/*!
Brings the “Full Screen” panel to the front.

(4.1)
*/
- (void)
performDisplayPrefPanelFullScreen:(id)	sender
{
#pragma unused(sender)
	[self displayPanel:self->fullScreenPanel withAnimation:YES];
}// performDisplayPrefPanelFullScreen:


/*!
Brings the “General” panel to the front.

(4.1)
*/
- (void)
performDisplayPrefPanelGeneral:(id)		sender
{
#pragma unused(sender)
	[self displayPanel:self->generalPanel withAnimation:YES];
}// performDisplayPrefPanelGeneral:


/*!
Brings the “Full Screen” panel to the front.

(4.1)
*/
- (void)
performDisplayPrefPanelTranslations:(id)	sender
{
#pragma unused(sender)
	[self displayPanel:self->translationsPanel withAnimation:YES];
}// performDisplayPrefPanelTranslations:


@end // PrefsWindow_Controller (PrefsWindow_ControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
