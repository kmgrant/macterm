/*!	\file PrefsWindow.mm
	\brief Implements the shell of the Preferences window.
	
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
#import <FileSelectionDialogs.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <IconManager.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <SoundSystem.h>
#import <WindowInfo.h>

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
NSString*	kMy_PrefsWindowSourceListDataType = @"net.macterm.MacTerm.prefswindow.sourcelistdata";

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
@interface PrefsWindow_Collection : NSObject< NSCopying > //{
{
@private
	Preferences_ContextRef		preferencesContext;
	BOOL						isDefault;
}

// initializers
	- (instancetype)
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

// NSCopying
	- (id)
	copyWithZone:(NSZone*)_;

// NSObject
	- (unsigned int)
	hash;
	- (BOOL)
	isEqual:(id)_;

@end //}

#pragma mark Internal Method Prototypes
namespace {

OSStatus				accessDataBrowserItemData		(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
														 DataBrowserItemDataRef, Boolean);
void					chooseContext					(Preferences_ContextRef);
void					choosePanel						(UInt16);
Boolean					createCollection				(CFStringRef = nullptr);
CFDictionaryRef			createSearchDictionary			();
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

/*!
The private class interface.
*/
@interface PrefsWindow_Controller (PrefsWindow_ControllerInternal) //{

// class methods
	+ (void)
	popUpResultsForQuery:(NSString*)_
	inSearchField:(NSSearchField*)_;
	+ (BOOL)
	queryString:(NSString*)_
	matchesTargetPhrase:(NSString*)_;
	+ (BOOL)
	queryString:(NSString*)_
	withWordArray:(NSArray*)_
	matchesTargetPhrase:(NSString*)_;

// new methods
	- (void)
	displayPanel:(Panel_ViewManager< PrefsWindow_PanelInterface >*)_
	withAnimation:(BOOL)_;
	- (void)
	displayPanelOrTabWithIdentifier:(NSString*)_
	withAnimation:(BOOL)_;
	- (void)
	rebuildSourceList;
	- (void)
	setSourceListHidden:(BOOL)_
	newWindowFrame:(NSRect)_;

// accessors
	- (Quills::Prefs::Class)
	currentPreferencesClass;
	- (Preferences_ContextRef)
	currentPreferencesContext;

// methods of the form required by NSSavePanel
	- (void)
	didEndExportPanel:(NSSavePanel*)_
	returnCode:(int)_
	contextInfo:(void*)_;

// methods of the form required by NSOpenPanel
	- (void)
	didEndImportPanel:(NSOpenPanel*)_
	returnCode:(int)_
	contextInfo:(void*)_;

@end //}

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
NSDictionary*							gSearchDataDictionary ()	{ static CFDictionaryRef x = createSearchDictionary(); return BRIDGE_CAST(x, NSDictionary*); }
Preferences_ContextRef					gCurrentDataSet = nullptr;
Float32									gBottomMargin = 0;

} // anonymous namespace


#pragma mark Functors

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
		// clean up the Help System
		HelpSystem_SetWindowKeyPhrase(gPreferencesWindow, kHelpSystem_KeyPhraseDefault);
		
		// disable preferences callbacks
		Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts);
		Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeContextName);
		ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
		
		// disable event callbacks and destroy the window
		RemoveEventHandler(gPreferencesWindowClosingHandler), gPreferencesWindowClosingHandler = nullptr;
		DisposeEventHandlerUPP(gPreferencesWindowClosingUPP), gPreferencesWindowClosingUPP = nullptr;
		for (auto dataPtr : gPanelList())
		{
			// dispose each panel
			if (nullptr != dataPtr)
			{
				Panel_Dispose(&dataPtr->panel);
				delete dataPtr, dataPtr = nullptr;
			}
		}
		for (auto itemRef : gCategoryToolbarItems())
		{
			// release each item
			if (nullptr != itemRef)
			{
				CFRelease(itemRef);
			}
		}
		UNUSED_RETURN(OSStatus)SetDrawerParent(gDrawerWindow, nullptr/* parent */);
		DisposeWindow(gDrawerWindow), gDrawerWindow = nullptr;
		DisposeWindow(gPreferencesWindow), gPreferencesWindow = nullptr; // automatically destroys all controls
		WindowInfo_Dispose(gPreferencesWindowInfo);
	}
}// Done


/*!
Copies the specified settings into a new global preferences
collection of the appropriate type.

If you want the user to immediately see the new settings,
pass an appropriate nonzero command ID that will be used to
request the display of a specific panel in the window.

(4.1)
*/
void
PrefsWindow_AddCollection		(Preferences_ContextRef		inReferenceContextToCopy,
								 Preferences_TagSetRef		inTagSetOrNull,
								 UInt32						inPrefPanelShowCommandIDOrZero)
{
	// create and immediately save a new named context, which
	// triggers callbacks to update Favorites lists, etc.
	Preferences_ContextWrap		newContext(Preferences_NewContextFromFavorites
											(Preferences_ContextReturnClass(inReferenceContextToCopy),
												nullptr/* name, or nullptr to auto-generate */),
											true/* is retained */);
	
	
	if (false == newContext.exists())
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "unable to create a new Favorite for copying local changes");
	}
	else
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSave(newContext.returnRef());
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unable to save the new context!");
		}
		else
		{
			prefsResult = Preferences_ContextCopy(inReferenceContextToCopy, newContext.returnRef(), inTagSetOrNull);
			if (kPreferences_ResultOK != prefsResult)
			{
				Sound_StandardAlert();
				Console_Warning(Console_WriteLine, "unable to copy local changes into the new Favorite!");
			}
			else
			{
				if (0 != inPrefPanelShowCommandIDOrZero)
				{
					// trigger an automatic switch to focus a related part of the Preferences window
					Commands_ExecuteByIDUsingEvent(inPrefPanelShowCommandIDOrZero);
				}
			}
		}
	}
}// AddCollection


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
		
		Panel_SendMessageNewVisibility(gCurrentPanel, false);
	}
	
	// write all the preference data in memory to disk
	UNUSED_RETURN(Preferences_Result)Preferences_Save();
	
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
	{
		assert(false == gCategoryToolbarItems().empty());
		for (auto itemRef : gCategoryToolbarItems())
		{
			UNUSED_RETURN(OSStatus)HIToolbarItemChangeAttributes(itemRef, 0/* attributes to set */,
																	kHIToolbarItemSelected/* attributes to clear */);
		}
		UNUSED_RETURN(OSStatus)HIToolbarItemChangeAttributes(gCategoryToolbarItems()[inZeroBasedPanelNumber],
																kHIToolbarItemSelected/* attributes to set */,
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
				UNUSED_RETURN(OSStatus)selectCollection();
				
				// if the panel is an inspector type, show the collections drawer
				if (kPanel_ResponseEditTypeInspector == Panel_SendMessageGetEditType(newPanelDataPtr->panel))
				{
					UNUSED_RETURN(OSStatus)OpenDrawer(gDrawerWindow, kWindowEdgeDefault, true/* asynchronously */);
				}
				else
				{
					UNUSED_RETURN(OSStatus)CloseDrawer(gDrawerWindow, true/* asynchronously */);
				}
				
				// grow box appearance
				{
					HIViewWrap		growBox(kHIViewWindowGrowBoxID, gPreferencesWindow);
					
					
				#if 1
					// since a footer is now present, panels are no longer abutted to the bottom
					// of the window, so their grow box preferences are irrelevant
					UNUSED_RETURN(OSStatus)HIGrowBoxViewSetTransparent(growBox, true);
				#else
					// make the grow box transparent by default, which looks better most of the time;
					// however, give the panel the option to change this appearance
					if (kPanel_ResponseGrowBoxOpaque == Panel_SendMessageGetGrowBoxLook(newPanelDataPtr->panel))
					{
						UNUSED_RETURN(OSStatus)HIGrowBoxViewSetTransparent(growBox, false);
					}
					else
					{
						UNUSED_RETURN(OSStatus)HIGrowBoxViewSetTransparent(growBox, true);
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
		if (nullptr != nowInvisibleContainer)
		{
			assert_noerr(HIViewSetVisible(nowInvisibleContainer, false));
		}
		assert_noerr(HIViewSetVisible(nowVisibleContainer, true));
		UNUSED_RETURN(OSStatus)HIViewSetVisible(nowVisibleContainer, true/* visible */);
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
Creates and returns a Core Foundation dictionary that can be used
to match search keywords to parts of the Preferences window (as
defined in the application bundle’s "PreferencesSearch.plist").

Returns nullptr if unsuccessful for any reason.

(4.1)
*/
CFDictionaryRef
createSearchDictionary ()
{
	CFDictionaryRef		result = nullptr;
	CFRetainRelease		fileCFURLObject(CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), CFSTR("PreferencesSearch"),
																CFSTR("plist")/* type string */, nullptr/* subdirectory path */),
										true/* is retained */);
	
	
	if (fileCFURLObject.exists())
	{
		CFURLRef	fileURL = REINTERPRET_CAST(fileCFURLObject.returnCFTypeRef(), CFURLRef);
		CFDataRef   fileData = nullptr;
		SInt32		errorCode = 0;
		
		
		unless (CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL, &fileData, 
															nullptr/* properties */, nullptr/* desired properties */, &errorCode))
		{
			// Not all data was successfully retrieved, but let the caller determine if anything
			// important is missing.
			// NOTE: Technically, the error code returned in "errorCode" is not an OSStatus.
			//       If negative, it is an Apple error, and if positive it is scheme-specific.
			Console_WriteValue("error reading raw data of 'PreferencesSearch.plist'", (SInt32)errorCode);
		}
		
		{
			CFPropertyListRef   	propertyList = nullptr;
			CFPropertyListFormat	actualFormat = kCFPropertyListXMLFormat_v1_0;
			CFErrorRef				errorObject = nullptr;
			
			
			propertyList = CFPropertyListCreateWithData(kCFAllocatorDefault, fileData, kCFPropertyListImmutable, &actualFormat, &errorObject);
			if (nullptr != errorObject)
			{
				CFRetainRelease		searchErrorCFString(CFErrorCopyDescription(errorObject), true/* is retained */);
				
				
				Console_WriteValueCFString("failed to create dictionary from 'PreferencesSearch.plist', error", searchErrorCFString.returnCFStringRef());
				Console_WriteValue("actual format of file is type", (SInt32)actualFormat);
				CFRelease(errorObject), errorObject = nullptr;
			}
			else
			{
				// the file actually contains a dictionary
				result = CFUtilities_DictionaryCast(propertyList);
			}
		}
		
		// finally, release the file data
		CFRelease(fileData), fileData = nullptr;
	}
	
	return result;
}// createSearchDictionary


/*!
Focuses the collections drawer and displays an editable
text area for the specified item.

(3.1)
*/
void
displayCollectionRenameUI	(DataBrowserItemID		inItemToRename)
{
	// focus the drawer
	UNUSED_RETURN(OSStatus)SetUserFocusWindow(gDrawerWindow);
	
	// focus the data browser itself
	UNUSED_RETURN(OSStatus)DialogUtilities_SetKeyboardFocus(gDataBrowserForCollections);
	
	// open the new item for editing
	UNUSED_RETURN(OSStatus)SetDataBrowserEditItem(gDataBrowserForCollections, inItemToRename,
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
	
	[[NSCursor arrowCursor] set];
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
	UNUSED_RETURN(OSStatus)SetDrawerParent(drawerWindow, mainWindow);
	
	if (nullptr != gPreferencesWindow)
	{
		Rect		panelBounds;
		
		
		// although the API is available on 10.4, the Spaces-related flags
		// will only work on Leopard
		UNUSED_RETURN(OSStatus)HIWindowChangeAvailability
								(gPreferencesWindow,
									kHIWindowMoveToActiveSpace/* attributes to set */,
									0/* attributes to clear */);
		
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
		UNUSED_RETURN(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(gPreferencesWindow, true);
		
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
						UNUSED_RETURN(OSStatus)SetControlTitleWithCFString(gCollectionAddMenuButton, CFSTR(""));
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
						UNUSED_RETURN(OSStatus)SetControlTitleWithCFString(gCollectionRemoveButton, CFSTR(""));
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
						UNUSED_RETURN(OSStatus)SetControlTitleWithCFString(gCollectionManipulateMenuButton, CFSTR(""));
					}
				}
				IconManager_DisposeIcon(&buttonIcon);
			}
		}
		
		// set accessibility relationships, if possible
		{
			CFStringRef		accessibilityDescCFString = nullptr;
			
			
			if (UIStrings_Copy(kUIStrings_ButtonAddAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionAddMenuButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to set accessibility description of button in Preferences window, error", error);
				}
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
			if (UIStrings_Copy(kUIStrings_ButtonRemoveAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionRemoveButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to set accessibility description of button in Preferences window, error", error);
				}
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
			if (UIStrings_Copy(kUIStrings_ButtonMoveUpAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionMoveUpButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to set accessibility description of button in Preferences window, error", error);
				}
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
			if (UIStrings_Copy(kUIStrings_ButtonMoveDownAccessibilityDesc, accessibilityDescCFString).ok())
			{
				OSStatus		error = noErr;
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(gCollectionMoveDownButton.returnHIObjectRef(), 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to set accessibility description of button in Preferences window, error", error);
				}
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
		}
		
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
				
				UNUSED_RETURN(OSStatus)SetWindowToolbar(gPreferencesWindow, categoryIcons);
				UNUSED_RETURN(OSStatus)ShowHideWindowToolbar(gPreferencesWindow, true/* show */, false/* animate */);
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
			gPrefsFooterActiveStateHandler.install(HIViewGetEventTarget(footerView), receiveFooterActiveStateChange,
													CarbonEventSetInClass(CarbonEventClass(kEventClassControl),
																			kEventControlActivate, kEventControlDeactivate),
													nullptr/* user data */);
			
			// install a callback to render the footer background
			gPrefsFooterDrawHandler.install(HIViewGetEventTarget(footerView), receiveFooterDraw,
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
			float		leadingOffset = 51/* arbitrary; the current height of a standard toolbar */;
			float		trailingOffset = kWindowOffsetUnchanged;
			OSStatus	error = noErr;
			
			
			error = SetDrawerOffsets(gDrawerWindow, leadingOffset, trailingOffset);
			if (noErr != error)
			{
				Console_Warning(Console_WriteValue, "failed to set drawer offsets in Preferences window, error", error);
			}
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
		[[NSCursor arrowCursor] set];
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
			if (noErr != error)
			{
				Console_Warning(Console_WriteValue, "failed to complete save for Preferences window export, error", error);
			}
		}
		break;
	
	case kNavCBTerminate:
		// clean up
		NavDialogDispose(inParameters->context);
		UNUSED_RETURN(OSStatus)ChangeWindowAttributes
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
				
				
				if (0 == dataPtr->listIndex)
				{
					itemOptions |= kHIToolbarItemSelected;
				}
				if (noErr == HIToolbarItemCreate(Panel_ReturnKind(inPanel), itemOptions, &newItem))
				{
					CFStringRef		descriptionCFString = nullptr;
					
					
					// PrefsWindow_Done() should clean up this retain
					CFRetain(newItem), gCategoryToolbarItems().push_back(newItem);
					
					// set the icon, label and tooltip
					UNUSED_RETURN(OSStatus)Panel_SetToolbarItemIconAndLabel(newItem, inPanel);
					Panel_GetDescription(inPanel, descriptionCFString);
					if (nullptr != descriptionCFString)
					{
						UNUSED_RETURN(OSStatus)HIToolbarItemSetHelpText(newItem, descriptionCFString, nullptr/* long version */);
					}
					
					// add the item to the toolbar
					UNUSED_RETURN(OSStatus)HIToolbarAppendItem(categoryIcons, newItem);
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
	Quills::Prefs::Class const				kCurrentPreferencesClass = returnCurrentPreferencesClass();
	std::vector< Preferences_ContextRef >	contextList;
	
	
	if (Preferences_GetContextsInClass(kCurrentPreferencesClass, contextList))
	{
		// start by destroying all items in the list
		UNUSED_RETURN(OSStatus)RemoveDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
														0/* size of array */, nullptr/* items array; nullptr = destroy all */,
														kDataBrowserItemNoProperty/* pre-sort property */);
		
		// now acquire contexts for all available names in this class,
		// and add data browser items for each of them
		for (auto prefsContextRef : contextList)
		{
			DataBrowserItemID	ids[] = { REINTERPRET_CAST(prefsContextRef, DataBrowserItemID) };
			
			
			UNUSED_RETURN(OSStatus)AddDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
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
				
				// set dark background
				bzero(&backgroundInfo, sizeof(backgroundInfo));
				backgroundInfo.version = 0;
				backgroundInfo.state = IsControlActive(footerView) ? kThemeStateActive : kThemeStateInactive;
				backgroundInfo.kind = kThemeBackgroundMetal;
				error = HIThemeDrawBackground(&floatBounds, &backgroundInfo, drawingContext, kHIThemeOrientationNormal);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to draw footer of Preferences window, error", error);
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
								
								prefsResult = Preferences_ContextDeleteFromFavorites(deletedContext), deletedContext = nullptr;
								if (kPreferences_ResultOK != prefsResult) isError = true;
								else
								{
									// success!
									result = noErr;
									
									// select something else...
									UNUSED_RETURN(OSStatus)selectCollection();
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
						UNUSED_RETURN(Boolean)CocoaBasic_FileOpenPanelDisplay(promptCFString, titleCFString, fileTypes.returnCFArrayRef());
						
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
						UNUSED_RETURN(OSStatus)ChangeWindowAttributes
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
								if (noErr != error)
								{
									Console_Warning(Console_WriteValue, "failed to display save panel for Preferences window export, error", error);
								}
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
	UNUSED_RETURN(OSStatus)UpdateDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
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
	UNUSED_RETURN(OSStatus)SetDataBrowserEditItem(gDataBrowserForCollections, kDataBrowserNoItem, kDataBrowserItemNoProperty);
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


#pragma mark -
@implementation PrefsWindow_Collection


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
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


#pragma mark NSCopying


/*!
Copies this instance by retaining the original context.

(4.1)
*/
- (id)
copyWithZone:(NSZone*)	aZone
{
	PrefsWindow_Collection*		result = [[self.class allocWithZone:aZone] init];
	
	
	if (nil != result)
	{
		result->preferencesContext = self->preferencesContext;
		Preferences_RetainContext(result->preferencesContext);
		result->isDefault = self->isDefault;
	}
	return result;
}// copyWithZone


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
	
	
	if ([anObject isKindOfClass:self.class])
	{
		PrefsWindow_Collection*		asCollection = (PrefsWindow_Collection*)anObject;
		
		
		result = ([asCollection preferencesContext] == [self preferencesContext]);
	}
	return result;
}// isEqual:


@end // PrefsWindow_Collection


#pragma mark -
@implementation PrefsWindow_Controller


static PrefsWindow_Controller*	gPrefsWindow_Controller = nil;


@synthesize searchText = _searchText;


/*!
Returns the singleton.

(4.1)
*/
+ (id)
sharedPrefsWindowController
{
	if (nil == gPrefsWindow_Controller)
	{
		gPrefsWindow_Controller = [[self.class allocWithZone:NULL] init];
	}
	return gPrefsWindow_Controller;
}// sharedPrefsWindowController


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
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
		self->activePanel = nil;
		self->_searchText = [@"" copy];
		
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
	[self ignoreWhenObjectsPostNotes];
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
																kPreferences_ChangeContextName);
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
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
		NSUInteger					oldIndex = [self->currentPreferenceCollectionIndexes firstIndex];
		NSUInteger					newIndex = [indexes firstIndex];
		PrefsWindow_Collection*		oldCollection = ((NSNotFound != oldIndex) && (oldIndex < currentPreferenceCollections.count))
													? [currentPreferenceCollections objectAtIndex:oldIndex]
													: nil;
		PrefsWindow_Collection*		newCollection = ((NSNotFound != newIndex) && (newIndex < currentPreferenceCollections.count))
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
		
		// write all the preference data in memory to disk
		UNUSED_RETURN(Preferences_Result)Preferences_Save();
		
		// notify the panel that a new data set has been selected
		[self->activePanel.delegate panelViewManager:self->activePanel
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
+ (BOOL)
automaticallyNotifiesObserversOfCurrentPreferenceCollections
{
	return NO;
}// automaticallyNotifiesObserversOfCurrentPreferenceCollections


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
	NSArray*		allowedTypes = @[
										@"com.apple.property-list",
										@"plist"/* redundant, needed for older systems */,
										@"xml",
									];
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
			Preferences_Result		prefsResult = kPreferences_ResultOK;
			
			
			// NOTE: notifications are sent when a context is deleted from Favorites
			// so the source list should automatically resynchronize
			prefsResult = Preferences_ContextDeleteFromFavorites(deadContext), deadCollection = nil, deadContext = nullptr;
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
Responds to the user typing in the search field.

(4.1)
*/
- (IBAction)
performSearch:(id)		sender
{
	if ([sender isKindOfClass:[NSSearchField class]])
	{
		NSSearchField*		searchField = REINTERPRET_CAST(sender, NSSearchField*);
		
		
		[PrefsWindow_Controller popUpResultsForQuery:self.searchText inSearchField:searchField];
	}
	else
	{
		Console_Warning(Console_WriteLine, "received 'performSearch:' message from unexpected sender");
	}
}// performSearch:


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
		NSSegmentedControl*		segments = REINTERPRET_CAST(sender, NSSegmentedControl*);
		
		
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


#pragma mark NSTableDataSource


/*!
Returns the number of rows in the source list.  This is not
strictly necessary with bindings on later Mac OS X versions.
This is however a “required” method in the original protocol
so the table views will not work on older Mac OS X versions
unless this method is implemented.

(4.1)
*/
- (int)
numberOfRowsInTableView:(NSTableView*)	aTableView
{
#pragma unused(aTableView)
	int		result = [self->currentPreferenceCollections count];
	
	
	return result;
}// numberOfRowsInTableView:


/*!
Returns the object for the given source list row.  This is not
strictly necessary with bindings on later Mac OS X versions.
This is however a “required” method in the original protocol
so the table views will not work on older Mac OS X versions
unless this method is implemented.

(4.1)
*/
- (id)
tableView:(NSTableView*)					aTableView
objectValueForTableColumn:(NSTableColumn*)	aTableColumn
row:(int)									aRowIndex
{
#pragma unused(aTableView, aTableColumn)
	id		result = [self->currentPreferenceCollections objectAtIndex:aRowIndex];
	
	
	return result;
}// tableView:objectValueForTableColumn:row:


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
	BOOL				result = YES; // initially...
	NSMutableIndexSet*	indexSet = [[NSMutableIndexSet alloc] init];
	
	
	// drags are confined to the table, so it’s only necessary to copy row numbers;
	// note that the Mac OS X 10.3 version gives arrays of NSNumber and the actual
	// representation (in "tableView:writeRowIndexes:toPasteboard:") is NSIndexSet*
	for (NSNumber* numberObject in aRowArray)
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
		
		
		[aPasteboard declareTypes:@[kMy_PrefsWindowSourceListDataType] owner:self];
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
		
		
		[aPasteboard declareTypes:@[kMy_PrefsWindowSourceListDataType] owner:self];
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


#pragma mark NSTableViewDelegate


/*!
Informed when the selection in the source list changes.

This is not strictly necessary with bindings, on later versions
of Mac OS X.  On at least Panther however it appears that panels
are not switched properly through bindings alone; a notification
must be used as a backup.

(4.1)
*/
- (void)
tableViewSelectionDidChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self setCurrentPreferenceCollectionIndexes:[self->sourceListTableView selectedRowIndexes]];
}// tableViewSelectionDidChange:


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
	[result setAction:itemPanel.panelDisplayAction];
	[result setTarget:itemPanel.panelDisplayTarget];
	
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
	assert(nil != sourceListBackdrop);
	assert(nil != sourceListContainer);
	assert(nil != sourceListTableView);
	assert(nil != sourceListSegmentedControl);
	assert(nil != sourceListHelpButton);
	assert(nil != mainViewHelpButton);
	assert(nil != verticalSeparator);
	
	NSRect const	kOriginalContainerFrame = [self->containerTabView frame];
	
	
	// create all panels
	{
		Panel_ViewManager*		newViewMgr = nil;
		
		
		for (Class viewMgrClass in
				@[
					[PrefPanelGeneral_ViewManager class],
					[PrefPanelWorkspaces_ViewManager class],
					[PrefPanelSessions_ViewManager class],
					[PrefPanelTerminals_ViewManager class],
					[PrefPanelFormats_ViewManager class],
					[PrefPanelTranslations_ViewManager class],
					[PrefPanelFullScreen_ViewManager class]
					// other panels TBD
				])
		{
			newViewMgr = [[viewMgrClass alloc] init];
			[self->panelIDArray addObject:[newViewMgr panelIdentifier]];
			[self->panelsByID setObject:newViewMgr forKey:[newViewMgr panelIdentifier]]; // retains allocated object
			newViewMgr.panelDisplayAction = @selector(performDisplaySelfThroughParent:);
			newViewMgr.panelDisplayTarget = newViewMgr;
			newViewMgr.panelParent = self;
			if ([newViewMgr conformsToProtocol:@protocol(Panel_Parent)])
			{
				for (Panel_ViewManager* childViewMgr in [REINTERPRET_CAST(newViewMgr, id< Panel_Parent >) panelParentEnumerateChildViewManagers])
				{
					childViewMgr.panelDisplayAction = @selector(performDisplaySelfThroughParent:);
					childViewMgr.panelDisplayTarget = childViewMgr;
				}
			}
			[newViewMgr release], newViewMgr = nil;
		}
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
	
	// disable the zoom button (Full Screen is graphically glitchy)
	{
		// you would think this should be NSWindowFullScreenButton but since Apple
		// redefined the Zoom button for Full Screen on later OS versions, it is
		// necessary to disable Zoom instead
		NSButton*		zoomButton = [self.window standardWindowButton:NSWindowZoomButton];
		
		
		zoomButton.enabled = NO;
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
	
	// add each panel as a subview and hide all except the first;
	// give them all the initial size of the window (in reality
	// they will be changed to a different size before display)
	for (NSString* panelIdentifier in self->panelIDArray)
	{
		Panel_ViewManager*	viewMgr = [self->panelsByID objectForKey:panelIdentifier];
		NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:panelIdentifier];
		NSView*				panelContainer = [viewMgr managedView];
		NSRect				panelFrame = kOriginalContainerFrame;
		NSSize				panelIdealSize = panelFrame.size;
		
		
		[viewMgr.delegate panelViewManager:viewMgr requestingIdealSize:&panelIdealSize];
		
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
			sizeArray = @[
							[NSNumber numberWithFloat:windowSize.width],
							[NSNumber numberWithFloat:windowSize.height],
						];
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
	
	// enable drag-and-drop in the source list
	[self->sourceListTableView registerForDraggedTypes:@[kMy_PrefsWindowSourceListDataType]];
	
	// be notified of source list changes; not strictly necessary on
	// newer OS versions (where bindings work perfectly) but it seems
	// to be necessary for correct behavior on Panther at least
	[self whenObject:self->sourceListTableView postsNote:NSTableViewSelectionDidChangeNotification
						performSelector:@selector(tableViewSelectionDidChange:)];
	
	// show the first panel while simultaneously resizing the
	// window to an appropriate frame and showing any auxiliary
	// interface that the panel requests (such as a source list)
	[self displayPanel:[self->panelsByID objectForKey:[self->panelIDArray objectAtIndex:0]] withAnimation:NO];
	
	// find out when the window closes
	[self whenObject:self.window postsNote:NSWindowWillCloseNotification
					performSelector:@selector(windowWillClose:)];
}// windowDidLoad


#pragma mark NSWindowDelegate


/*!
Responds to a close of the preferences window by saving
any changes.

(4.1)
*/
- (void)
windowWillClose:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	Preferences_Result	prefsResult = Preferences_Save();
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "failed to save preferences to disk, error", prefsResult);
	}
	
	// a window does not receive a “did close” message so there is
	// no choice except to notify a panel of both “will” and ”did”
	// messages at the same time
	[self->activePanel.delegate panelViewManager:self->activePanel willChangePanelVisibility:kPanel_VisibilityHidden];
	[self->activePanel.delegate panelViewManager:self->activePanel didChangePanelVisibility:kPanel_VisibilityHidden];
}// windowWillClose:


#pragma mark Panel_Parent


/*!
Switches to the specified panel.

(4.1)
*/
- (void)
panelParentDisplayChildWithIdentifier:(NSString*)	anIdentifier
withAnimation:(BOOL)								isAnimated
{
	[self displayPanelOrTabWithIdentifier:anIdentifier withAnimation:isAnimated];
}// panelParentDisplayChildWithIdentifier:withAnimation:


/*!
Returns the primary panels displayed in this window.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	// TEMPORARY; this is not an ordered list (it should be)
	return [self->panelsByID objectEnumerator];
}// panelParentEnumerateChildViewManagers


@end // PrefsWindow_Controller


#pragma mark -
@implementation PrefsWindow_Controller (PrefsWindow_ControllerInternal)


#pragma mark Class Methods


/*!
Tries to match the given phrase against the search data for
preferences panels and shows the user which panels (and
possibly tabs) are relevant.

To cancel the search, pass an empty or "nil" string.

(4.1)
*/
+ (void)
popUpResultsForQuery:(NSString*)	aQueryString
inSearchField:(NSSearchField*)		aSearchField
{
	NSUInteger const			kSetCapacity = 25; // IMPORTANT: set to value greater than number of defined panels and tabs!
	NSMutableSet*				matchingPanelIDs = [[NSMutableSet alloc] initWithCapacity:kSetCapacity];
	NSMutableArray*				orderedMatchingPanels = [NSMutableArray arrayWithCapacity:kSetCapacity];
	NSDictionary*				searchData = gSearchDataDictionary();
	NSArray*					queryStringWords = [aQueryString componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]; // LOCALIZE THIS
	PrefsWindow_Controller*		controller = [self sharedPrefsWindowController];
	
	
	// TEMPORARY; do the lazy thing and iterate over the data in
	// the exact form it is stored in the bundle dictionary; the
	// data is ordered in the way most convenient for writing so
	// it’s not optimal for searching; if performance proves to
	// be bad, the data could be indexed in memory but for now
	// it is probably fine this way
	for (NSString* candidatePanelIdentifier in searchData)
	{
		id		candidatePanelData = [searchData objectForKey:candidatePanelIdentifier];
		
		
		if (NO == [candidatePanelData respondsToSelector:@selector(objectAtIndex:)])
		{
			Console_Warning(Console_WriteValueCFString, "panel search data should map to an array, for key",
							BRIDGE_CAST(candidatePanelIdentifier, CFStringRef));
		}
		else
		{
			NSArray*	searchPhraseArray = REINTERPRET_CAST(candidatePanelData, NSArray*);
			
			
			for (id object in searchPhraseArray)
			{
				// currently, every object in the array must be a string
				if (NO == [object respondsToSelector:@selector(characterAtIndex:)])
				{
					Console_Warning(Console_WriteValueCFString, "array in panel search data should contain only string values, for key",
									BRIDGE_CAST(candidatePanelIdentifier, CFStringRef));
				}
				else
				{
					// TEMPORARY; looking up localized strings on every query is
					// wasteful; since the search data ought to be reverse-indexed
					// anyway, a better solution is to reverse-index the data and
					// simultaneously capture all localized phrases
					NSString*			asKeyPhrase = REINTERPRET_CAST(object, NSString*);
					CFRetainRelease		localizedKeyPhrase(CFBundleCopyLocalizedString(AppResources_ReturnApplicationBundle(),
																						BRIDGE_CAST(asKeyPhrase, CFStringRef), nullptr/* default value */,
																						CFSTR("PreferencesSearch")/* base name of ".strings" file */),
															true/* is retained */);
					
					
					if (localizedKeyPhrase.exists())
					{
						asKeyPhrase = BRIDGE_CAST(localizedKeyPhrase.returnCFStringRef(), NSString*);
					}
					
					if ([self queryString:aQueryString withWordArray:queryStringWords matchesTargetPhrase:asKeyPhrase])
					{
						[matchingPanelIDs addObject:candidatePanelIdentifier];
					}
				}
			}
		}
	}
	
	// find the panel objects that correspond to the matching IDs;
	// display each panel only once, and iterate over panels in
	// the order they appear in the window so that results are
	// nicely sorted (this assumes that tabs also iterate in the
	// order that tabs are displayed; see "GenericPanelTabs.mm")
	for (NSString* mainPanelIdentifier in controller->panelIDArray)
	{
		Panel_ViewManager*		mainPanel = [controller->panelsByID objectForKey:mainPanelIdentifier];
		
		
		if ([matchingPanelIDs containsObject:mainPanelIdentifier])
		{
			// match applies to an entire category
			[orderedMatchingPanels addObject:mainPanel];
		}
		else
		{
			// target panel is not a whole category; search for any
			// tabs that were included in the matched set
			if ([mainPanel conformsToProtocol:@protocol(Panel_Parent)])
			{
				id< Panel_Parent >	asParent = REINTERPRET_CAST(mainPanel, id< Panel_Parent >);
				
				
				for (Panel_ViewManager* subPanel in [asParent panelParentEnumerateChildViewManagers])
				{
					if ([matchingPanelIDs containsObject:[subPanel panelIdentifier]])
					{
						[orderedMatchingPanels addObject:subPanel];
					}
				}
			}
		}
	}
	[matchingPanelIDs release];
	
	// display a menu with search results
	{
		static NSMenu*		popUpResultsMenu = nil;
		
		
		if (nil != popUpResultsMenu)
		{
			// clear current menu
			[popUpResultsMenu removeAllItems];
		}
		else
		{
			// create new menu
			popUpResultsMenu = [[NSMenu alloc] initWithTitle:@""];
		}
		
		// pop up a menu of results whose commands will display the matching panels
		for (Panel_ViewManager* panelObject in orderedMatchingPanels)
		{
			NSImage*				panelIcon = [[panelObject panelIcon] copy];
			NSString*				panelName = [panelObject panelName];
			NSString*				displayItemName = (nil != panelName)
														? panelName
														: @"";
			NSMenuItem*				panelDisplayItem = [[NSMenuItem alloc] initWithTitle:displayItemName
																							action:panelObject.panelDisplayAction
																							keyEquivalent:@""];
			
			
			panelIcon.size = NSMakeSize(24, 24); // shrink default image, which is too large
			[panelDisplayItem setImage:panelIcon];
			[panelDisplayItem setTarget:panelObject.panelDisplayTarget]; // action is set in initializer above
			[popUpResultsMenu addItem:panelDisplayItem];
			[panelIcon release];
			[panelDisplayItem release];
		}
		
		if ([orderedMatchingPanels count] > 0)
		{
			NSPoint		menuLocation = NSMakePoint(4/* arbitrary */, NSHeight([aSearchField bounds]) + 4/* arbitrary */);
			
			
			[popUpResultsMenu popUpMenuPositioningItem:nil/* menu item */ atLocation:menuLocation inView:aSearchField];
		}
	}
}// popUpResultsForQuery:inSearchField:


/*!
Returns YES if the specified strings match according to
the rules of searching preferences.

(4.1)
*/
+ (BOOL)
queryString:(NSString*)				aQueryString
matchesTargetPhrase:(NSString*)		aSetPhrase
{
	NSRange		lowestCommonRange = NSMakeRange(0, MIN([aSetPhrase length], [aQueryString length]));
	BOOL		result = NO;
	
	
	// for now, search against the beginning of phrases; could later
	// add more intelligence such as introducing synonyms,
	// misspellings and other “fuzzy” logic
	if (NSOrderedSame == [aSetPhrase compare:aQueryString options:(NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch)
												range:lowestCommonRange])
	{
		result = YES;
	}
	
	return result;
}// queryString:matchesTargetPhrase:


/*!
Returns YES if the specified strings match according to
the rules of searching preferences.  The given array of
words should be consistent with the query string but
split into tokens (e.g. on whitespace).

(4.1)
*/
+ (BOOL)
queryString:(NSString*)				aQueryString
withWordArray:(NSArray*)			anArrayOfWordsInQueryString
matchesTargetPhrase:(NSString*)		aSetPhrase
{
	BOOL	result = [self queryString:aQueryString matchesTargetPhrase:aSetPhrase];
	
	
	if (NO == result)
	{
		// if the phrase doesn’t exactly match, try to
		// match individual words from the query string
		// (in the future, might want to return a “rank”
		// to indicate how closely the result matched)
		for (NSString* word in anArrayOfWordsInQueryString)
		{
			if ([self queryString:word matchesTargetPhrase:aSetPhrase])
			{
				result = YES;
				break;
			}
		}
	}
	
	return result;
}// queryString:withWordArray:matchesTargetPhrase:


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
		for (NSURL* currentFileURL in [aPanel URLs])
		{
			FSRef		fileRef;
			OSStatus	error = noErr;
			
			
			if (CFURLGetFSRef(BRIDGE_CAST(currentFileURL, CFURLRef), &fileRef))
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
								BRIDGE_CAST([currentFileURL absoluteString], CFStringRef));
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
displayPanel:(Panel_ViewManager< PrefsWindow_PanelInterface >*)		aPanel
withAnimation:(BOOL)												isAnimated
{
	if (aPanel != self->activePanel)
	{
		NSRect	newWindowFrame = NSZeroRect; // set later
		BOOL	wasShowingSourceList = ((NO == self->sourceListContainer.isHidden) &&
										(self->sourceListContainer.frame.size.width > 0));
		BOOL	willShowSourceList = (kPanel_EditTypeInspector == [aPanel panelEditType]); // initially...
		
		
		if (nil != self->activePanel)
		{
			// remember the window size that the user wanted for the previous panel
			NSRect		contentRect = [[self window] contentRectForFrameRect:[[self window] frame]];
			NSSize		containerSize = NSMakeSize(NSWidth(contentRect), NSHeight(contentRect));
			NSArray*	sizeArray = @[
										[NSNumber numberWithFloat:containerSize.width],
										[NSNumber numberWithFloat:containerSize.height],
									];
			
			
			[self->windowSizesByID setObject:sizeArray forKey:[self->activePanel panelIdentifier]];
		}
		
		// show the new panel
		{
			Panel_ViewManager< PrefsWindow_PanelInterface >*	oldPanel = self->activePanel;
			Panel_ViewManager< PrefsWindow_PanelInterface >*	newPanel = aPanel;
			
			
			[oldPanel.delegate panelViewManager:oldPanel willChangePanelVisibility:kPanel_VisibilityHidden];
			[newPanel.delegate panelViewManager:newPanel willChangePanelVisibility:kPanel_VisibilityDisplayed];
			self->activePanel = newPanel;
			[[[self window] toolbar] setSelectedItemIdentifier:[newPanel panelIdentifier]];
			[self->containerTabView selectTabViewItemWithIdentifier:[newPanel panelIdentifier]];
			[oldPanel.delegate panelViewManager:oldPanel didChangePanelVisibility:kPanel_VisibilityHidden];
			[newPanel.delegate panelViewManager:newPanel didChangePanelVisibility:kPanel_VisibilityDisplayed];
		}
		
		// set the window to the size for the new panel (if none, use the
		// size that was originally used for the panel in its NIB)
		if (NO == EventLoop_IsWindowFullScreen(self.window))
		{
			NSRect		originalFrame = self.window.frame;
			NSRect		newContentRect = [self.window contentRectForFrameRect:originalFrame]; // initially...
			NSSize		minSize = [self.window minSize];
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
				[self.window setContentMinSize:minSize];
			}
			
			// start with the restored size but respect the minimum size
			assert(2 == [sizeArray count]);
			newContentRect.size.width = MAX([[sizeArray objectAtIndex:0] floatValue], minSize.width);
			newContentRect.size.height = MAX([[sizeArray objectAtIndex:1] floatValue], minSize.height);
			
			// since the coordinate system anchors the window at the
			// bottom-left corner and the intent is to keep the top-left
			// corner constant, calculate an appropriate new origin when
			// changing the size (don’t just change the size by itself);
			// this calculation must be performed prior to seeking the
			// intersection with the screen rectangle so that a proposed
			// offscreen location is seen by the intersection check
			newWindowFrame = [[self window] frameRectForContentRect:newContentRect];
			newWindowFrame.origin.y += (NSHeight(originalFrame) - NSHeight(newWindowFrame));
			
			// try to keep the window on screen
			{
				NSRect		onScreenFrame = NSIntersectionRect(newWindowFrame, self.window.screen.visibleFrame);
				
				
				// see if the window is going to remain entirely on the screen...
				if (NO == NSEqualRects(onScreenFrame, newWindowFrame))
				{
					// window will become partially offscreen; try to move it onscreen
					if ((newWindowFrame.origin.x + NSWidth(newWindowFrame)) >
						(self.window.screen.visibleFrame.origin.x + NSWidth(self.window.screen.visibleFrame)))
					{
						// adjust to the left
						newWindowFrame.origin.x = (onScreenFrame.origin.x + NSWidth(onScreenFrame) - NSWidth(newWindowFrame));
					}
					else
					{
						// adjust to the right
						newWindowFrame.origin.x = onScreenFrame.origin.x;
					}
					newWindowFrame.origin.y = onScreenFrame.origin.y;
				}
			}
		}
		else
		{
			// full-screen; size is not changing
			newWindowFrame = self.window.frame;
		}
		
		// if necessary display the source list of available preference collections
		if (willShowSourceList != wasShowingSourceList)
		{
			if (willShowSourceList)
			{
				[self rebuildSourceList];
			}
			
			// resize the window and change the source list visibility
			// at the same time for fluid animation
			[self setSourceListHidden:(NO == willShowSourceList) newWindowFrame:newWindowFrame];
		}
		else
		{
			// resize the window normally
			[self.window setFrame:newWindowFrame display:YES animate:isAnimated];
			
			// source list is still needed, but it will have new content (note
			// that this step is implied if the visibility changes above)
			[self rebuildSourceList];
		}
	}
}// displayPanel:withAnimation:


/*!
If the specified identifier exactly matches that of a
top category panel, this method behaves the same as
"displayPanel:withAnimation:".

If however the given identifier can only be resolved
by the child panels of the main categories, this
method will first display the parent panel (with or
without animation, as indicated) and then issue a
request to switch to the appropriate tab.  In this
way, you can pinpoint a specific part of the window.

(4.1)
*/
- (void)
displayPanelOrTabWithIdentifier:(NSString*)		anIdentifier
withAnimation:(BOOL)							isAnimated
{
	Panel_ViewManager< Panel_Parent, PrefsWindow_PanelInterface >*	mainPanel = nil;
	Panel_ViewManager< Panel_Parent, PrefsWindow_PanelInterface >*	candidateMainPanel = nil;
	Panel_ViewManager*												childPanel = nil;
	Panel_ViewManager*												candidateChildPanel = nil;
	
	
	// given the way panels are currently indexed, it is
	// necessary to search main categories and then
	// search panel children (there is no direct map)
	for (NSString* panelIdentifier in self->panelIDArray)
	{
		if ([panelIdentifier isEqualToString:anIdentifier])
		{
			mainPanel = [self->panelsByID objectForKey:anIdentifier];
			break;
		}
	}
	
	if (nil == mainPanel)
	{
		// requested identifier does not exactly match a top-level
		// category; search for a child panel that matches
		for (candidateMainPanel in [self->panelsByID objectEnumerator])
		{
			for (candidateChildPanel in [candidateMainPanel panelParentEnumerateChildViewManagers])
			{
				NSString*	childIdentifier = [candidateChildPanel panelIdentifier];
				
				
				if ([childIdentifier isEqualToString:anIdentifier])
				{
					mainPanel = candidateMainPanel;
					childPanel = candidateChildPanel;
					break;
				}
			}
		}
	}
	
	if (nil != mainPanel)
	{
		[self displayPanel:mainPanel withAnimation:isAnimated];
	}
	
	if (nil != childPanel)
	{
		[childPanel performDisplaySelfThroughParent:nil];
	}
}// displayPanelOrTabWithIdentifier:withAnimation:


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
		
		
		for (auto prefsContextRef : contextList)
		{
			BOOL						isDefault = (NO == haveAddedDefault);
			PrefsWindow_Collection*		newCollection = [[PrefsWindow_Collection alloc]
															initWithPreferencesContext:prefsContextRef
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
Changes the visibility of the source list and the size of
the window, animating any other views to match.

(4.1)
*/
- (void)
setSourceListHidden:(BOOL)		aHiddenFlag
newWindowFrame:(NSRect)			aNewWindowFrame
{
	NSRect		newWindowContentFrame = [self.window contentRectForFrameRect:aNewWindowFrame];
	NSRect		listFrame;
	NSRect		panelFrame;
	
	
	// the darker backdrop behind the list makes the
	// slide animation less jarring
	self->sourceListBackdrop.hidden = aHiddenFlag;
	
	// calculate the new size and location of the panel
	panelFrame = NSMakeRect((aHiddenFlag) ? 0 : NSWidth(sourceListContainer.frame), containerTabView.frame.origin.y,
							NSWidth(newWindowContentFrame) - ((aHiddenFlag) ? 0 : self->extraWindowContentSize.width),
							NSHeight(newWindowContentFrame) - self->extraWindowContentSize.height);
	
	// calculate the new size and location of the list,
	// potentially moving it to an invisible location (after
	// the animation ends, the list is hidden anyway)
	listFrame = sourceListContainer.frame;
	listFrame.origin.x = ((aHiddenFlag) ? -self->extraWindowContentSize.width : 0); // “slide” effect
	listFrame.size.height = NSHeight(panelFrame);
	
	// animate changes to other views (NOTE: can replace this with a
	// more advanced block-based approach when using a later SDK)
	dispatch_async(dispatch_get_main_queue(),
	^{
		// all of the changes in this block will animate simultaneously
		[NSAnimationContext beginGrouping];
		[[NSAnimationContext currentContext] setDuration:0.22];
		{
			[self.window.animator setFrame:aNewWindowFrame display:YES];
			[[self->containerTabView animator] setFrame:panelFrame];
			if (NO == aHiddenFlag)
			{
				sourceListContainer.hidden = aHiddenFlag;
				sourceListSegmentedControl.hidden = aHiddenFlag;
				sourceListHelpButton.hidden = (NO == aHiddenFlag);
				verticalSeparator.hidden = aHiddenFlag;
				mainViewHelpButton.hidden = aHiddenFlag;
			}
			[sourceListContainer.animator setFrame:listFrame];
			[sourceListContainer.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[sourceListSegmentedControl.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[verticalSeparator.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[mainViewHelpButton.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[sourceListHelpButton.animator setAlphaValue:((aHiddenFlag) ? 1.0 : 0.0)]; // this alternate help button appears when there is no list
			if (aHiddenFlag)
			{
				sourceListContainer.hidden = aHiddenFlag;
				sourceListSegmentedControl.hidden = aHiddenFlag;
				sourceListHelpButton.hidden = (NO == aHiddenFlag);
				verticalSeparator.hidden = aHiddenFlag;
				mainViewHelpButton.hidden = aHiddenFlag;
			}
		}
		[NSAnimationContext endGrouping];
	});
	// after the animation above completes, refresh the final view
	// (it is unclear why this step is needed; the symptom is that
	// when the source list is hidden, the tab view of the next panel
	// is sometimes not completely redrawn on its left side)
	dispatch_after(dispatch_time(0, 0.32 * NSEC_PER_SEC), dispatch_get_main_queue(),
	^{
		[containerTabView setNeedsDisplay:YES];
	});
}// setSourceListHidden:newWindowFrame:


@end // PrefsWindow_Controller (PrefsWindow_ControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
