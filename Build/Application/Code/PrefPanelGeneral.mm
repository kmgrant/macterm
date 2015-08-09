/*!	\file PrefPanelGeneral.mm
	\brief Implements the General panel of Preferences.
	
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

#import "PrefPanelGeneral.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Unix includes
#import <strings.h>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <BoundName.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CFUtilities.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <ColorUtilities.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <GrowlSupport.h>
#import <HelpSystem.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "Keypads.h"
#import "Panel.h"
#import "Preferences.h"
#import "SessionFactory.h"
#import "TerminalView.h"
#import "TerminalWindow.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

#define NUMBER_OF_GENERAL_TABPANES		3
enum
{
	// must match tab order at creation, and be one-based
	kMy_TabIndexGeneralOptions			= 1,
	kMy_TabIndexGeneralSpecial			= 2,
	kMy_TabIndexGeneralNotification		= 3
};

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanels.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyCheckBoxDoNotAutoClose					= { 'DACW', 0/* ID */ };
static HIViewID const	idMyCheckBoxDoNotAutoCreateWindows			= { 'DCNW', 0/* ID */ };
static HIViewID const	idMyCheckBoxInvertSelectedText				= { 'ISel', 0/* ID */ };
static HIViewID const	idMyCheckBoxAutoCopySelectedText			= { 'ACST', 0/* ID */ };
static HIViewID const	idMyCheckBoxMoveCursorToDropArea			= { 'MCTD', 0/* ID */ };
static HIViewID const	idMyCheckBoxDoNotDimInactive				= { 'DDBW', 0/* ID */ };
static HIViewID const	idMyCheckBoxDoNotWarnOnPaste				= { 'NPWR', 0/* ID */ };
static HIViewID const	idMyCheckBoxMapBackquoteToEscape			= { 'MBQE', 0/* ID */ };
static HIViewID const	idMyCheckBoxFadeBackgroundWindows			= { 'FADE', 0/* ID */ };
static HIViewID const	idMyCheckBoxFocusFollowsMouse				= { 'FcFM', 0/* ID */ };
static HIViewID const	idMyLabelTerminalCursor						= { 'LCrs', 0/* ID */ };
static HIViewID const	idMyCheckBoxCursorFlashing					= { 'CurF', 0/* ID */ };
static HIViewID const	idMySegmentedViewCursorShape				= { 'CShp', 0/* ID */ };
static HIViewID const	idMyLabelWindowResizeEffect					= { 'LWRE', 0/* ID */ };
static HIViewID const	idMyRadioButtonResizeAffectsScreenSize		= { 'WRSS', 0/* ID */ };
static HIViewID const	idMyRadioButtonResizeAffectsFontSize		= { 'WRFS', 0/* ID */ };
static HIViewID const	idMyHelpTextResizeEffect					= { 'HTWR', 0/* ID */ };
static HIViewID const	idMyFieldCopyUsingSpacesForTabs				= { 'CUST', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNDefault				= { 'CNDf', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNShell				= { 'CNSh', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNLogInShell			= { 'CNLI', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNDialog				= { 'CNDg', 0/* ID */ };
static HIViewID const	idMyLabelBellType							= { 'LSnd', 0/* ID */ };
static HIViewID const	idMyPopUpMenuBellType						= { 'BSnd', 0/* ID */ };
static HIViewID const	idMyCheckBoxVisualBell						= { 'VisB', 0/* ID */ };
static HIViewID const	idMyHelpTextBellType						= { 'HTBT', 0/* ID */ };
static HIViewID const	idMyCheckBoxNotifyTerminalBeeps				= { 'BelN', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyDoNothing				= { 'NotN', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyBadgeDockIcon			= { 'NotD', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyBounceDockIcon			= { 'NotB', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyDisplayMessage			= { 'NotM', 0/* ID */ };
static HIViewID const	idMyLabelGrowlSettings						= { 'GLab', 0/* ID */ };
static HIViewID const	idMyButtonOpenGrowlPreferencesPane			= { 'GBut', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

struct My_GeneralPanelUI;

/*!
Implements the “Notification” tab.
*/
struct My_GeneralTabNotification:
public HIViewWrap
{
public:
	My_GeneralTabNotification	(HIWindowRef);

protected:
	void
	assignAccessibilityRelationships ();
	
	HIViewWrap
	createPaneView	(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
	HIViewWrap							labelBell;
	HIViewWrap							popupMenuBell;
};

/*!
Implements the “Options” tab.
*/
struct My_GeneralTabOptions:
public HIViewWrap
{
public:
	My_GeneralTabOptions	(HIWindowRef);

protected:
	HIViewWrap
	createPaneView	(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
};

/*!
Implements the “Special” tab.
*/
struct My_GeneralTabSpecial:
public HIViewWrap
{
public:
	My_GeneralTabSpecial	(My_GeneralPanelUI*, HIWindowRef);
	
	CarbonEventHandlerWrap		buttonCommandsHandler;		//!< invoked when a button is clicked

protected:
	void
	assignAccessibilityRelationships ();
	
	HIViewWrap
	createPaneView	(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CarbonEventHandlerWrap				fieldSpacesPerTabInputHandler;
	CommonEventHandlers_HIViewResizer	containerResizer;
	HIViewWrap							labelCursor;
	HIViewWrap							checkBoxCursorFlashing;
	HIViewWrap							labelWindowResizeEffect;
	HIViewWrap							radioButtonResizeAffectsScreenSize;
	HIViewWrap							radioButtonResizeAffectsFontSize;
};

/*!
Implements the entire panel user interface.
*/
struct My_GeneralPanelUI
{
public:
	My_GeneralPanelUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref							panel;				//!< the panel using this UI
	My_GeneralTabOptions				optionsTab;
	My_GeneralTabSpecial				specialTab;
	My_GeneralTabNotification			notificationTab;
	HIViewWrap							tabView;
	HIViewWrap							mainView;
	CommonEventHandlers_HIViewResizer	containerResizer;	//!< invoked when the panel is resized
	CarbonEventHandlerWrap				viewClickHandler;	//!< invoked when a tab is clicked

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
	
	HIViewWrap
	createTabsView	(HIWindowRef) const;
};
typedef My_GeneralPanelUI*			My_GeneralPanelUIPtr;
typedef My_GeneralPanelUI const*	My_GeneralPanelUIConstPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_GeneralPanelData
{
	My_GeneralPanelData ();
	~My_GeneralPanelData ();
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_GeneralPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef My_GeneralPanelData*	My_GeneralPanelDataPtr;

} // anonymous namespace


/*!
Implements an object wrapper for sound names, that allows them
to be correctly bound into user interface elements.  (On older
Mac OS X versions, raw strings do not bind properly.)
*/
@interface PrefPanelGeneral_SoundInfo : BoundName_Object //{
{
@private
	BOOL	isDefault;
	BOOL	isOff;
}

// initializers
	- (instancetype)
	initAsDefault;
	- (instancetype)
	initAsOff;
	- (instancetype)
	initWithDescription:(NSString*)_;
	- (instancetype)
	initWithDescription:(NSString*)_
	isDefault:(BOOL)_
	isOff:(BOOL)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (void)
	playSound;
	- (NSString*)
	preferenceString;

@end //}

#pragma mark Internal Method Prototypes
namespace {

void			changePreferenceUpdateScreenTerminalWindowOp	(TerminalWindowRef, void*, SInt32, void*);
void			deltaSizePanelContainerHIView					(HIViewRef, Float32, Float32, void*);
SInt32			panelChanged									(Panel_Ref, Panel_Message, void*);
OSStatus		receiveHICommand								(EventHandlerCallRef, EventRef, void*);
OSStatus		receiveFieldChanged								(EventHandlerCallRef, EventRef, void*);
OSStatus		receiveViewHit									(EventHandlerCallRef, EventRef, void*);
void			saveFieldPreferences							(My_GeneralPanelUIPtr);
void			showTabPane										(My_GeneralPanelUIPtr, UInt16);
Boolean			updateCheckBoxPreference						(My_GeneralPanelUIPtr, HIViewRef);

} // anonymous namespace


/*!
The private class interface.
*/
@interface PrefPanelGeneral_NotificationsViewManager (PrefPanelGeneral_NotificationsViewManagerInternal) //{

// new methods
	- (void)
	notifyDidChangeValueForBackgroundNotification;
	- (void)
	notifyWillChangeValueForBackgroundNotification;

// preference setting accessors
	- (SInt16)
	readBackgroundNotificationTypeWithDefaultValue:(SInt16)_;
	- (NSString*)
	readBellSoundNameWithDefaultValue:(NSString*)_;
	- (BOOL)
	writeBackgroundNotificationType:(SInt16)_;
	- (BOOL)
	writeBellSoundName:(NSString*)_;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelGeneral_OptionsViewManager (PrefPanelGeneral_OptionsViewManagerInternal) @end


/*!
The private class interface.
*/
@interface PrefPanelGeneral_SpecialViewManager (PrefPanelGeneral_SpecialViewManagerInternal) //{

// new methods
	- (void)
	notifyDidChangeValueForCursorShape;
	- (void)
	notifyWillChangeValueForCursorShape;
	- (void)
	notifyDidChangeValueForNewCommand;
	- (void)
	notifyWillChangeValueForNewCommand;
	- (void)
	notifyDidChangeValueForWindowResizeEffect;
	- (void)
	notifyWillChangeValueForWindowResizeEffect;
	- (NSArray*)
	primaryDisplayBindingKeys;

// preference setting accessors
	- (TerminalView_CursorType)
	readCursorTypeWithDefaultValue:(TerminalView_CursorType)_;
	- (UInt32)
	readNewCommandShortcutEffectWithDefaultValue:(UInt32)_;
	- (UInt16)
	readSpacesPerTabWithDefaultValue:(UInt16)_;
	- (BOOL)
	writeCursorType:(TerminalView_CursorType)_;
	- (BOOL)
	writeNewCommandShortcutEffect:(UInt32)_;
	- (BOOL)
	writeSpacesPerTab:(UInt16)_;

@end //}

#pragma mark Variables
namespace {

Float32		gIdealPanelWidth = 0.0;
Float32		gIdealPanelHeight = 0.0;
Float32		gMaximumTabPaneWidth = 0.0;
Float32		gMaximumTabPaneHeight = 0.0;

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new preference panel for the General
category, initializes it, and returns a reference
to it.  You must destroy the reference using
Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelGeneral_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (nullptr != result)
	{
		My_GeneralPanelDataPtr	dataPtr = new My_GeneralPanelData();
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorGeneral);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelGeneral);
		if (UIStrings_Copy(kUIStrings_PrefPanelGeneralCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelGeneralIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelGeneral);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// New


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewNotificationsTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagVisualBell);
	tagList.push_back(kPreferences_TagNotifyOfBeeps);
	tagList.push_back(kPreferences_TagBellSound);
	tagList.push_back(kPreferences_TagNotification);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewNotificationsTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewOptionsTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagDontAutoClose);
	tagList.push_back(kPreferences_TagDontDimBackgroundScreens);
	tagList.push_back(kPreferences_TagPureInverse);
	tagList.push_back(kPreferences_TagCopySelectedText);
	tagList.push_back(kPreferences_TagCursorMovesPriorToDrops);
	tagList.push_back(kPreferences_TagNoPasteWarning);
	tagList.push_back(kPreferences_TagMapBackquote);
	tagList.push_back(kPreferences_TagDontAutoNewOnApplicationReopen);
	tagList.push_back(kPreferences_TagFocusFollowsMouse);
	tagList.push_back(kPreferences_TagFadeBackgroundWindows);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewOptionsTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewSpecialTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalCursorType);
	tagList.push_back(kPreferences_TagCursorBlinks);
	tagList.push_back(kPreferences_TagTerminalResizeAffectsFontSize);
	tagList.push_back(kPreferences_TagCopyTableThreshold);
	tagList.push_back(kPreferences_TagWindowStackingOrigin);
	tagList.push_back(kPreferences_TagNewCommandShortcutEffect);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewSpecialTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewTagSet ()
{
	Preferences_TagSetRef	result = Preferences_NewTagSet(40); // arbitrary initial capacity
	Preferences_TagSetRef	optionTags = PrefPanelGeneral_NewOptionsTagSet();
	Preferences_TagSetRef	specialTags = PrefPanelGeneral_NewSpecialTagSet();
	Preferences_TagSetRef	notificationTags = PrefPanelGeneral_NewNotificationsTagSet();
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_TagSetMerge(result, optionTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, specialTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, notificationTags);
	assert(kPreferences_ResultOK == prefsResult);
	
	Preferences_ReleaseTagSet(&optionTags);
	Preferences_ReleaseTagSet(&specialTags);
	Preferences_ReleaseTagSet(&notificationTags);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_GeneralPanelData structure.

(3.1)
*/
My_GeneralPanelData::
My_GeneralPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr)
{
}// My_GeneralPanelData default constructor


/*!
Tears down a My_GeneralPanelData structure.

(4.0)
*/
My_GeneralPanelData::
~My_GeneralPanelData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_GeneralPanelData destructor


/*!
Initializes a My_GeneralPanelUI structure.

(3.1)
*/
My_GeneralPanelUI::
My_GeneralPanelUI	(Panel_Ref		inPanel,
					 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel				(inPanel),
optionsTab			(inOwningWindow),
specialTab			(this, inOwningWindow),
notificationTab		(inOwningWindow),
tabView				(createTabsView(inOwningWindow)
						<< HIViewWrap_AssertExists),
mainView			(createContainerView(inPanel, inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
								kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
						deltaSizePanelContainerHIView, this/* context */),
viewClickHandler	(HIViewGetEventTarget(this->tabView), receiveViewHit,
						CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
						this/* user data */)
{
	assert(containerResizer.isInstalled());
	assert(viewClickHandler.isInstalled());
}// My_GeneralPanelUI 2-argument constructor


/*!
Constructs the container for the panel.  Assumes that
the tabs view already exists.

(3.1)
*/
HIViewWrap
My_GeneralPanelUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
const
{
	assert(this->tabView.exists());
	assert(0 != gMaximumTabPaneWidth);
	assert(0 != gMaximumTabPaneHeight);
	
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
		
		gIdealPanelWidth = tabFrameTopLeft.h + tabPaneTopLeft.h + gMaximumTabPaneWidth +
							tabPaneBottomRight.h + tabFrameTopLeft.h/* right is same as left */;
		gIdealPanelHeight = tabFrameTopLeft.v + tabPaneTopLeft.v + gMaximumTabPaneHeight +
							tabPaneBottomRight.v + tabFrameTopLeft.v/* bottom is same as top */;
		
		// make the container big enough for the tabs
		{
			HIRect		containerFrame = CGRectMake(0, 0, gIdealPanelWidth, gIdealPanelHeight);
			
			
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
}// My_GeneralPanelUI::createContainerView


/*!
Constructs the container for all panel tab panes.
Assumes that all tabs already exist.

(3.1)
*/
HIViewWrap
My_GeneralPanelUI::
createTabsView	(HIWindowRef	inOwningWindow)
const
{
	assert(this->optionsTab.exists());
	assert(this->specialTab.exists());
	assert(this->notificationTab.exists());
	
	HIViewRef				result = nullptr;
	Rect					containerBounds;
	ControlTabEntry			tabInfo[NUMBER_OF_GENERAL_TABPANES];
	UIStrings_Result		stringResult = kUIStrings_ResultOK;
	OSStatus				error = noErr;
	
	
	// nullify or zero-fill everything, then set only what matters
	bzero(&tabInfo, sizeof(tabInfo));
	tabInfo[0].enabled =
		tabInfo[1].enabled =
		tabInfo[2].enabled = true;
	stringResult = UIStrings_Copy(kUIStrings_PrefPanelGeneralOptionsTabName,
									tabInfo[kMy_TabIndexGeneralOptions - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PrefPanelGeneralSpecialTabName,
									tabInfo[kMy_TabIndexGeneralSpecial - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PrefPanelGeneralNotificationTabName,
									tabInfo[kMy_TabIndexGeneralNotification - 1].name);
	bzero(&containerBounds, sizeof(containerBounds));
	error = CreateTabsControl(inOwningWindow, &containerBounds, kControlTabSizeLarge, kControlTabDirectionNorth,
								sizeof(tabInfo) / sizeof(ControlTabEntry)/* number of tabs */, tabInfo,
								&result);
	assert_noerr(error);
	for (size_t i = 0; i < sizeof(tabInfo) / sizeof(ControlTabEntry); ++i)
	{
		if (nullptr != tabInfo[i].name) CFRelease(tabInfo[i].name), tabInfo[i].name = nullptr;
	}
	
	// calculate the largest area required for all tabs, and keep this as the ideal size
	{
		Rect		optionsTabSize;
		Rect		specialTabSize;
		Rect		notificationTabSize;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// determine sizes of tabs from NIBs
		GetControlBounds(this->optionsTab, &optionsTabSize);
		GetControlBounds(this->specialTab, &specialTabSize);
		GetControlBounds(this->notificationTab, &notificationTabSize);
		
		// also include pane margin in panel size
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		// find the widest tab and the highest tab
		gMaximumTabPaneWidth = std::max(optionsTabSize.right - optionsTabSize.left,
										specialTabSize.right - specialTabSize.left);
		gMaximumTabPaneWidth = std::max(notificationTabSize.right - notificationTabSize.left,
										STATIC_CAST(gMaximumTabPaneWidth, int));
		gMaximumTabPaneHeight = std::max(optionsTabSize.bottom - optionsTabSize.top,
											specialTabSize.bottom - specialTabSize.top);
		gMaximumTabPaneHeight = std::max(notificationTabSize.bottom - notificationTabSize.top,
											STATIC_CAST(gMaximumTabPaneHeight, int));
		
		// make every tab pane match the ideal pane size
		{
			HIRect		containerFrame = CGRectMake(tabPaneTopLeft.h, tabPaneTopLeft.v,
													gMaximumTabPaneWidth, gMaximumTabPaneHeight);
			
			
			error = HIViewSetFrame(this->optionsTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->specialTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->notificationTab, &containerFrame);
			assert_noerr(error);
		}
		
		// make the tabs big enough for any of the panes
		{
			HIRect		containerFrame = CGRectMake(0, 0,
													tabPaneTopLeft.h + gMaximumTabPaneWidth + tabPaneBottomRight.h,
													tabPaneTopLeft.v + gMaximumTabPaneHeight + tabPaneBottomRight.v);
			
			
			error = HIViewSetFrame(result, &containerFrame);
			assert_noerr(error);
		}
		
		// embed every tab pane in the tabs view; done at this stage
		// so that the subsequent move of the tab frame (later) will
		// also offset the embedded panes; if embedding is done too
		// soon, then the panes have to know too much about where
		// they will physically reside within the window content area
		error = HIViewAddSubview(result, this->optionsTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->specialTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->notificationTab);
		assert_noerr(error);
	}
	
	return result;
}// My_GeneralPanelUI::createTabsView


/*!
Initializes a My_GeneralTabNotification structure.

(3.1)
*/
My_GeneralTabNotification::
My_GeneralTabNotification	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap				(createPaneView(inOwningWindow)
							<< HIViewWrap_AssertExists),
containerResizer		(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_GeneralTabNotification::deltaSize, this/* context */),
labelBell				(idMyLabelBellType, HIViewGetWindow(*this)),
popupMenuBell			(idMyPopUpMenuBellType, HIViewGetWindow(*this))
{
	assert(exists());
	assert(containerResizer.isInstalled());
	assert(labelBell.exists());
	assert(popupMenuBell.exists());
	assignAccessibilityRelationships();
}// My_GeneralTabNotification 2-argument constructor


/*!
Creates the necessary accessibility objects and assigns
relationships between things (for example, between
labels and the things they are labelling).

Since HIViewWrap automatically retains an accessibility
object, the views used for these relationships are
stored in this class permanently in order to preserve
their accessibility information.

(3.1)
*/
void
My_GeneralTabNotification::
assignAccessibilityRelationships ()
{
	// (removing this code for now to resolve static analyzer issues;
	// it is going to be replaced by Cocoa soon anyway)
}// My_GeneralTabNotification::assignAccessibilityRelationships


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_GeneralTabNotification::
createPaneView		(HIWindowRef	inOwningWindow)
const
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	bzero(&dummy, sizeof(dummy));
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("General/Notification"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// hide settings that are not useful, based on what is available on the user’s computer
	if (false == GrowlSupport_PreferencesPaneCanDisplay())
	{
		HIViewWrap	label(idMyLabelGrowlSettings, inOwningWindow);
		HIViewWrap	button(idMyButtonOpenGrowlPreferencesPane, inOwningWindow);
		
		
		label << HIViewWrap_SetVisibleState(false);
		button << HIViewWrap_SetVisibleState(false);
	}
	
	// initialize values
	{
		Boolean		visualBell = false;
		Boolean		notifyOfBeeps = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagVisualBell, sizeof(visualBell), &visualBell))
		{
			visualBell = false; // assume default, if preference can’t be found
		}
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNotifyOfBeeps, sizeof(notifyOfBeeps),
									&notifyOfBeeps))
		{
			notifyOfBeeps = false; // assume default, if preference can’t be found
		}
		
		// add all user sounds to the list of library sounds
		// (INCOMPLETE: should reinitialize this if it changes on disk)
		{
			CFArrayRef const	kSoundsListRef = CocoaBasic_ReturnUserSoundNames();
			HIViewWrap			soundsPopUpMenuButton(idMyPopUpMenuBellType, inOwningWindow);
			MenuRef				soundsMenu = GetControlPopupMenuRef(soundsPopUpMenuButton);
			MenuItemIndex		insertBelowIndex = 0;
			CFStringRef			userPreferredSoundName = nullptr;
			Boolean				releaseUserPreferredSoundName = true;
			
			
			// determine user preferred sound, to initialize the menu selection
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagBellSound, sizeof(userPreferredSoundName),
										&userPreferredSoundName))
			{
				userPreferredSoundName = CFSTR(""); // assume default, if preference can’t be found
				releaseUserPreferredSoundName = false;
			}
			
			// add every library sound to the menu
			if (noErr == GetIndMenuItemWithCommandID(soundsMenu, kCommandPrefBellSystemAlert, 1/* which instance to return */,
														nullptr/* menu */, &insertBelowIndex))
			{
				CFIndex const	kSoundCount = CFArrayGetCount(kSoundsListRef);
				
				
				SetControl32BitMaximum(soundsPopUpMenuButton, CountMenuItems(soundsMenu) + kSoundCount);
				for (CFIndex i = 0; i < kSoundCount; ++i)
				{
					CFStringRef		soundName = CFUtilities_StringCast(CFArrayGetValueAtIndex(kSoundsListRef, i));
					
					
					if (noErr == AppendMenuItemTextWithCFString(soundsMenu, soundName, kMenuItemAttrIgnoreMeta,
																kCommandPrefBellLibrarySound, nullptr/* new item ID */))
					{
						// if this is the saved user preference, select the item initially
						if (kCFCompareEqualTo == CFStringCompare(soundName, userPreferredSoundName, kCFCompareCaseInsensitive))
						{
							SetControl32BitValue(soundsPopUpMenuButton, insertBelowIndex + i + 1/* one-based */);
						}
					}
				}
			}
			
			// if the sound is "off", select that item instead (see "Preferences.h", the
			// "off" value is specially recognized)
			if (kCFCompareEqualTo == CFStringCompare(userPreferredSoundName, CFSTR("off"), kCFCompareCaseInsensitive))
			{
				MenuItemIndex	offIndex = 0;
				
				
				if (noErr == GetIndMenuItemWithCommandID(soundsMenu, kCommandPrefBellOff, 1/* which instance to return */,
															nullptr/* menu */, &offIndex))
				{
					SetControl32BitValue(soundsPopUpMenuButton, offIndex);
				}
			}
			
			if ((releaseUserPreferredSoundName) && (nullptr != userPreferredSoundName))
			{
				CFRelease(userPreferredSoundName), userPreferredSoundName = nullptr;
			}
		}
		
		SetControl32BitValue(HIViewWrap(idMyCheckBoxVisualBell, inOwningWindow), BooleanToCheckBoxValue(visualBell));
		SetControl32BitValue(HIViewWrap(idMyCheckBoxNotifyTerminalBeeps, inOwningWindow), BooleanToCheckBoxValue(notifyOfBeeps));
		
		// ...initialize radio buttons using user preferences
		{
			HIViewRef			selectedRadioButton = nullptr;
			HIViewWrap const	radioNotifyNothing(idMyRadioButtonNotifyDoNothing, inOwningWindow);
			HIViewWrap const	radioNotifyBadgeDockIcon(idMyRadioButtonNotifyBadgeDockIcon, inOwningWindow);
			HIViewWrap const	radioNotifyBounceDockIcon(idMyRadioButtonNotifyBounceDockIcon, inOwningWindow);
			HIViewWrap const	radioNotifyDisplayMessage(idMyRadioButtonNotifyDisplayMessage, inOwningWindow);
			UInt16				notificationPreferences = kAlert_NotifyDisplayDiamondMark;
			
			
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagNotification, sizeof(notificationPreferences),
										&notificationPreferences))
			{
				notificationPreferences = kAlert_NotifyDisplayDiamondMark; // assume default, if preference can’t be found
			}
			SetControl32BitValue(radioNotifyNothing, kControlRadioButtonUncheckedValue);
			SetControl32BitValue(radioNotifyBadgeDockIcon, kControlRadioButtonUncheckedValue);
			SetControl32BitValue(radioNotifyBounceDockIcon, kControlRadioButtonUncheckedValue);
			SetControl32BitValue(radioNotifyDisplayMessage, kControlRadioButtonUncheckedValue);
			switch (notificationPreferences)
			{
			case kAlert_NotifyDoNothing:
				selectedRadioButton = radioNotifyNothing;
				break;
			
			case kAlert_NotifyDisplayDiamondMark:
				selectedRadioButton = radioNotifyBadgeDockIcon;
				break;
			
			case kAlert_NotifyDisplayIconAndDiamondMark:
				selectedRadioButton = radioNotifyBounceDockIcon;
				break;
			
			case kAlert_NotifyAlsoDisplayAlert:
				selectedRadioButton = radioNotifyDisplayMessage;
				break;
			
			default:
				// ???
				break;
			}
			
			if (nullptr != selectedRadioButton) SetControl32BitValue(selectedRadioButton, kControlRadioButtonCheckedValue);
		}
	}
	
	return result;
}// My_GeneralTabNotification::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_GeneralTabNotification::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_GeneralTabNotification*	dataPtr = REINTERPRET_CAST(inContext, My_GeneralTabNotification*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextBellType, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_GeneralTabNotification::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_GeneralTabNotification::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_GeneralTabNotification::operator =


/*!
Initializes a My_GeneralTabOptions structure.

(3.1)
*/
My_GeneralTabOptions::
My_GeneralTabOptions	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap			(createPaneView(inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						My_GeneralTabOptions::deltaSize, this/* context */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_GeneralTabOptions 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_GeneralTabOptions::
createPaneView		(HIWindowRef	inOwningWindow)
const
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	Boolean						flag = false;
	
	
	// create the tab pane
	bzero(&dummy, sizeof(dummy));
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("General/Options"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	//
	// initialize checkboxes using user preferences
	//
	
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotAutoClose, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagDontAutoClose, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotDimInactive, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagDontDimBackgroundScreens, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxInvertSelectedText, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagPureInverse, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxAutoCopySelectedText, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCopySelectedText, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxMoveCursorToDropArea, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCursorMovesPriorToDrops, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotWarnOnPaste, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoPasteWarning, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxMapBackquoteToEscape, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagMapBackquote, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotAutoCreateWindows, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxFocusFollowsMouse, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagFocusFollowsMouse, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxFadeBackgroundWindows, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagFadeBackgroundWindows, sizeof(flag), &flag))
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	return result;
}// My_GeneralTabOptions::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_GeneralTabOptions::
deltaSize	(HIViewRef		UNUSED_ARGUMENT(inContainer),
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	// nothing needed
}// My_GeneralTabOptions::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_GeneralTabOptions::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_GeneralTabOptions::operator =


/*!
Initializes a My_GeneralTabSpecial structure.

(3.1)
*/
My_GeneralTabSpecial::
My_GeneralTabSpecial	(My_GeneralPanelUIPtr	inOwningUI,
						 HIWindowRef			inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap							(createPaneView(inOwningWindow)
										<< HIViewWrap_AssertExists),
buttonCommandsHandler				(GetWindowEventTarget(inOwningWindow), receiveHICommand,
										CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
										nullptr/* user data */),
fieldSpacesPerTabInputHandler		(HIViewGetEventTarget(HIViewWrap(idMyFieldCopyUsingSpacesForTabs, inOwningWindow)), receiveFieldChanged,
										CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
										inOwningUI/* user data */),
containerResizer					(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
										My_GeneralTabSpecial::deltaSize, this/* context */),
labelCursor							(idMyLabelTerminalCursor, HIViewGetWindow(*this)),
checkBoxCursorFlashing				(idMyCheckBoxCursorFlashing, HIViewGetWindow(*this)),
labelWindowResizeEffect				(idMyLabelWindowResizeEffect, HIViewGetWindow(*this)),
radioButtonResizeAffectsScreenSize	(idMyRadioButtonResizeAffectsScreenSize, HIViewGetWindow(*this)),
radioButtonResizeAffectsFontSize	(idMyRadioButtonResizeAffectsFontSize, HIViewGetWindow(*this))
{
	assert(exists());
	assert(buttonCommandsHandler.isInstalled());
	assert(containerResizer.isInstalled());
	assert(labelCursor.exists());
	assert(checkBoxCursorFlashing.exists());
	assert(labelWindowResizeEffect.exists());
	assert(radioButtonResizeAffectsScreenSize.exists());
	assert(radioButtonResizeAffectsFontSize.exists());
	assignAccessibilityRelationships();
	
	// hack in Yosemite edit-text font for now (will later transition to Cocoa)
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyFieldCopyUsingSpacesForTabs, inOwningWindow), kThemeSystemFont);
}// My_GeneralTabSpecial 2-argument constructor


/*!
Creates the necessary accessibility objects and assigns
relationships between things (for example, between
labels and the things they are labelling).

Since HIViewWrap automatically retains an accessibility
object, the views used for these relationships are
stored in this class permanently in order to preserve
their accessibility information.

(3.1)
*/
void
My_GeneralTabSpecial::
assignAccessibilityRelationships ()
{
	// (removing this code for now to resolve static analyzer issues;
	// it is going to be replaced by Cocoa soon anyway)
}// My_GeneralTabSpecial::assignAccessibilityRelationships


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_GeneralTabSpecial::
createPaneView		(HIWindowRef	inOwningWindow)
const
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	bzero(&dummy, sizeof(dummy));
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("General/Special"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// set up the cursor shape bevel buttons
	{
		HIViewWrap					segmentedView(idMySegmentedViewCursorShape, inOwningWindow);
		TerminalView_CursorType		terminalCursorType = kTerminalView_CursorTypeBlock;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagTerminalCursorType, sizeof(terminalCursorType),
									&terminalCursorType))
		{
			terminalCursorType = kTerminalView_CursorTypeBlock; // assume a block-shaped cursor, if preference can’t be found
		}
		
		// IMPORTANT: this must agree with what the NIB does
		UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 0/* value */);
		UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 0/* value */);
		UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 0/* value */);
		UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 0/* value */);
		UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 0/* value */);
		switch (terminalCursorType)
		{
		case kTerminalView_CursorTypeVerticalLine:
			UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeUnderscore:
			UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeThickVerticalLine:
			UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeThickUnderscore:
			UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeBlock:
		default:
			UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 1/* value */);
			break;
		}
	}
	{
		Boolean		cursorBlinks = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCursorBlinks, sizeof(cursorBlinks),
									&cursorBlinks))
		{
			cursorBlinks = false; // assume the cursor doesn’t flash, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxCursorFlashing, inOwningWindow), BooleanToCheckBoxValue(cursorBlinks));
	}
	
	// set window resize preferences
	{
		HIViewWrap	radioButtonScreenSize(idMyRadioButtonResizeAffectsScreenSize, inOwningWindow);
		HIViewWrap	radioButtonFontSize(idMyRadioButtonResizeAffectsFontSize, inOwningWindow);
		Boolean		affectsFontSize = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagTerminalResizeAffectsFontSize,
									sizeof(affectsFontSize), &affectsFontSize))
		{
			affectsFontSize = false; // assume default, if preference can’t be found
		}
		SetControl32BitValue(radioButtonScreenSize, (false == affectsFontSize)
													? kControlRadioButtonCheckedValue
													: kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonFontSize, (affectsFontSize)
													? kControlRadioButtonCheckedValue
													: kControlRadioButtonUncheckedValue);
	}
	
	// ...create “Copy Using Spaces For Tabs” controls and initialize using user preferences
	{
		HIViewWrap	fieldView(idMyFieldCopyUsingSpacesForTabs, inOwningWindow);
		UInt16		copyTableThreshold = 0;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCopyTableThreshold, sizeof(copyTableThreshold),
									&copyTableThreshold))
		{
			copyTableThreshold = 4; // assume 4 spaces per tab, if preference can’t be found
		}
		SetControlNumericalText(fieldView, copyTableThreshold);
		fieldView << HIViewWrap_InstallKeyFilter(NumericalLimiterKeyFilterUPP());
	}
	
	// ...initialize command-N radio buttons using user preferences
	{
		HIViewWrap	radioButtonDefault(idMyRadioButtonCommandNDefault, inOwningWindow);
		HIViewWrap	radioButtonShell(idMyRadioButtonCommandNShell, inOwningWindow);
		HIViewWrap	radioButtonLogInShell(idMyRadioButtonCommandNLogInShell, inOwningWindow);
		HIViewWrap	radioButtonDialog(idMyRadioButtonCommandNDialog, inOwningWindow);
		UInt32		newCommandShortcutEffect = kCommandNewSessionShell;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(newCommandShortcutEffect), &newCommandShortcutEffect))
		{
			newCommandShortcutEffect = kCommandNewSessionShell; // assume default, if preference can’t be found
		}
		SetControl32BitValue(radioButtonDefault, kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonShell, kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonLogInShell, kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonDialog, kControlRadioButtonUncheckedValue);
		switch (newCommandShortcutEffect)
		{
		case kCommandNewSessionDefaultFavorite:
			SetControl32BitValue(radioButtonDefault, kControlRadioButtonCheckedValue);
			break;
		
		case kCommandNewSessionShell:
			SetControl32BitValue(radioButtonShell, kControlRadioButtonCheckedValue);
			break;
		
		case kCommandNewSessionLoginShell:
			SetControl32BitValue(radioButtonLogInShell, kControlRadioButtonCheckedValue);
			break;
		
		case kCommandNewSessionDialog:
			SetControl32BitValue(radioButtonDialog, kControlRadioButtonCheckedValue);
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// My_GeneralTabSpecial::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_GeneralTabSpecial::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_GeneralTabSpecial*	dataPtr = REINTERPRET_CAST(inContext, My_GeneralTabSpecial*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextResizeEffect, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_GeneralTabSpecial::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_GeneralTabSpecial::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_GeneralTabSpecial::operator =


/*!
This method, of SessionFactory_TerminalWindowOpProcPtr
form, will update the specified window’s terminal
screens appropriately based on the value of the
preference indicated by the given General preference
tag.

Currently, this method only supports changing of
the “use background picture” or “don’t dim background
session windows” preferences.  If one of these tags
is given, then the specified screen’s port rectangle
is invalidated (which will cause the picture to
appear or disappear).  In the case of a background
picture, the per-screen flag is also updated to
reflect the preference value.

(3.0)
*/
void
changePreferenceUpdateScreenTerminalWindowOp	(TerminalWindowRef		inTerminalWindow,
												 void*					UNUSED_ARGUMENT(inUnusedData1),
												 SInt32					inDataPreferenceTag,
												 void*					UNUSED_ARGUMENT(inoutUndefinedResultPtr))
{
	switch (inDataPreferenceTag)
	{
	case kPreferences_TagDontDimBackgroundScreens:
	case kPreferences_TagPureInverse:
		// update the entire terminal window, which will reflect the change
		RegionUtilities_RedrawWindowOnNextUpdate(TerminalWindow_ReturnWindow(inTerminalWindow));
		break;
	
	default:
		// unrecognized tag
		break;
	}
}// changePreferenceUpdateScreenTerminalWindowOp


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
		//HIWindowRef				kPanelWindow = GetControlOwner(inView);
		My_GeneralPanelUIPtr	interfacePtr = REINTERPRET_CAST(inContext, My_GeneralPanelUIPtr);
		assert(nullptr != interfacePtr);
		
		
		interfacePtr->tabView << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		
		// due to event handlers, this will automatically resize subviews too
		interfacePtr->optionsTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->specialTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->notificationTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
	}
}// deltaSizePanelContainerHIView


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.0)
*/
SInt32
panelChanged	(Panel_Ref			inPanel,
				 Panel_Message		inMessage,
				 void*				inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorGeneral, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_GeneralPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_GeneralPanelDataPtr);
			WindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_GeneralPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
			showTabPane(panelDataPtr->interfacePtr, 1/* tab index */);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_GeneralPanelDataPtr	panelDataPtr = REINTERPRET_CAST(inDataPtr, My_GeneralPanelDataPtr);
			
			
			saveFieldPreferences(panelDataPtr->interfacePtr);
			delete panelDataPtr;
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
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			My_GeneralPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_GeneralPanelDataPtr);
			
			
			saveFieldPreferences(panelDataPtr->interfacePtr);
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
	
	return result;
}// panelChanged


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the fields in this panel by saving
their preferences when new text arrives.

(4.0)
*/
OSStatus
receiveFieldChanged		(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					inMyGeneralPanelUIPtr)
{
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	My_GeneralPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMyGeneralPanelUIPtr, My_GeneralPanelUIPtr);
	assert(nullptr != interfacePtr);
	OSStatus				result = eventNotHandledErr;
	
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with preferences
	saveFieldPreferences(interfacePtr);
	
	return result;
}// receiveFieldChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
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
			case kCommandPrefCursorBlock:
			case kCommandPrefCursorUnderline:
			case kCommandPrefCursorVerticalBar:
			case kCommandPrefCursorThickUnderline:
			case kCommandPrefCursorThickVerticalBar:
				{
					assert(0 != (received.attributes & kHICommandFromControl));
					HIViewRef					segmentedView = received.source.control;
					TerminalView_CursorType		cursorType = kTerminalView_CursorTypeBlock;
					
					
					// IMPORTANT: this must agree with what the NIB does
					UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 0/* value */);
					UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 0/* value */);
					UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 0/* value */);
					UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 0/* value */);
					UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 0/* value */);
					if (kCommandPrefCursorBlock == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeBlock;
						UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorVerticalBar == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeVerticalLine;
						UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorUnderline == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeUnderscore;
						UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorThickVerticalBar == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeThickVerticalLine;
						UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorThickUnderline == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeThickUnderscore;
						UNUSED_RETURN(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 1/* value */);
					}
					Preferences_SetData(kPreferences_TagTerminalCursorType, sizeof(cursorType), &cursorType);
				}
				break;
			
			case kCommandPrefSetWindowLocation:
				Keypads_SetArrangeWindowPanelBinding(kPreferences_TagWindowStackingOrigin, typeQDPoint);
				Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
				result = noErr;
				break;
			
			case kCommandPrefBellOff:
				// do not “handle” the event so the pop-up menu is updated, etc.
				{
					CFStringRef		offCFString = CFSTR("off"); // see "Preferences.h"; this value is specially recognized
					
					
					Preferences_SetData(kPreferences_TagBellSound, sizeof(offCFString), &offCFString);
				}
				result = eventNotHandledErr;
				break;
			
			case kCommandPrefBellSystemAlert:
				// play the system alert sound for the user, but do not “handle”
				// the event so the pop-up menu is updated, etc.
				Sound_StandardAlert();
				{
					CFStringRef		emptyCFString = CFSTR("");
					
					
					Preferences_SetData(kPreferences_TagBellSound, sizeof(emptyCFString), &emptyCFString);
				}
				result = eventNotHandledErr;
				break;
			
			case kCommandPrefBellLibrarySound:
				// play the indicated sound for the user, but do not “handle”
				// the event so the pop-up menu is updated, etc.
				if (received.attributes & kHICommandFromMenu)
				{
					CFStringRef		itemTextCFString = nullptr;
					
					
					if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
															received.source.menu.menuItemIndex,
															&itemTextCFString))
					{
						CocoaBasic_PlaySoundByName(itemTextCFString);
						Preferences_SetData(kPreferences_TagBellSound, sizeof(itemTextCFString), &itemTextCFString);
						CFRelease(itemTextCFString), itemTextCFString = nullptr;
					}
				}
				result = eventNotHandledErr;
				break;
			
			case kCommandPrefOpenGrowlPreferencesPane:
				GrowlSupport_PreferencesPaneDisplay();
				result = noErr;
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
for this preferences panel.  Responds by changing
the currently selected tab, etc.

Also, temporarily, this handles clicks in other
controls.  Eventually, the buttons will simply have
commands associated with them, and a command handler
will be installed instead.

(3.1)
*/
OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyGeneralPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	My_GeneralPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMyGeneralPanelUIPtr, My_GeneralPanelUIPtr);
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
			
			if (view == interfacePtr->tabView)
			{
				showTabPane(interfacePtr, GetControl32BitValue(view));
				result = noErr; // completely handled
			}
			else
			{
				ControlKind		controlKind;
				OSStatus		error = GetControlKind(view, &controlKind);
				
				
				if ((noErr == error) && (kControlKindSignatureApple == controlKind.signature) &&
					((kControlKindCheckBox == controlKind.kind) || (kControlKindRadioButton == controlKind.kind)))
				{
					UNUSED_RETURN(Boolean)updateCheckBoxPreference(interfacePtr, view);
				}
			}
		}
	}
	
	return result;
}// receiveViewHit


/*!
Updates the preferences in memory to reflect the
information in the specified text field.  This
should be done if the focus of the window changes
(by hitting the tab key or clicking in another
text field).

(3.0)
*/
void
saveFieldPreferences	(My_GeneralPanelUIPtr	inInterfacePtr)
{
	if (nullptr != inInterfacePtr)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(inInterfacePtr->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// save spaces-per-tab
		{
			HIViewWrap	fieldSpaces(idMyFieldCopyUsingSpacesForTabs, kOwningWindow);
			assert(fieldSpaces.exists());
			SInt32		value = 0L;
			UInt16		threshold = 0;
			
			
			GetControlNumericalText(fieldSpaces, &value);
			threshold = value;
			prefsResult = Preferences_SetData(kPreferences_TagCopyTableThreshold, sizeof(threshold), &threshold);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "failed to set spaces-per-tab");
			}
		}
	}
}// savePreferenceForField


/*!
Displays the specified tab pane and hides the others.

(3.1)
*/
void
showTabPane		(My_GeneralPanelUIPtr	inUIPtr,
				 UInt16					inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	
	
	if (kMy_TabIndexGeneralOptions == inTabIndex)
	{
		selectedTabPane = inUIPtr->optionsTab;
		assert_noerr(HIViewSetVisible(inUIPtr->specialTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->notificationTab, false/* visible */));
	}
	else if (kMy_TabIndexGeneralSpecial == inTabIndex)
	{
		selectedTabPane = inUIPtr->specialTab;
		assert_noerr(HIViewSetVisible(inUIPtr->optionsTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->notificationTab, false/* visible */));
	}
	else if (kMy_TabIndexGeneralNotification == inTabIndex)
	{
		selectedTabPane = inUIPtr->notificationTab;
		assert_noerr(HIViewSetVisible(inUIPtr->optionsTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->specialTab, false/* visible */));
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


/*!
Call this method when a click in a checkbox or
radio button has been detected AND the control
value has been toggled to its new value.

The appropriate preference will have been updated
in memory using Preferences_SetData().

If the specified view is “known”, "true" is
returned to indicate that the click was handled.
Otherwise, "false" is returned.

(3.0)
*/
Boolean
updateCheckBoxPreference	(My_GeneralPanelUIPtr	inInterfacePtr,
							 HIViewRef				inCheckBoxClicked)
{
	Boolean		result = false;
	
	
	if (nullptr != inInterfacePtr)
	{
		WindowRef const		kWindow = GetControlOwner(inCheckBoxClicked);
		assert(IsValidWindowRef(kWindow));
		HIViewIDWrap		viewID(HIViewWrap(inCheckBoxClicked).identifier());
		Boolean				checkBoxFlagValue = (GetControl32BitValue(inCheckBoxClicked) == kControlCheckBoxCheckedValue);
		UInt16				chosenTabIndex = STATIC_CAST(GetControl32BitValue(inInterfacePtr->tabView), UInt16);
		
		
		switch (chosenTabIndex)
		{
		case kMy_TabIndexGeneralOptions:
			if (HIViewIDWrap(idMyCheckBoxDoNotAutoClose) == viewID)
			{
				Preferences_SetData(kPreferences_TagDontAutoClose,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxDoNotDimInactive) == viewID)
			{
				Preferences_SetData(kPreferences_TagDontDimBackgroundScreens,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				SessionFactory_ForEveryTerminalWindowDo(changePreferenceUpdateScreenTerminalWindowOp,
														nullptr/* undefined */, kPreferences_TagDontDimBackgroundScreens,
														nullptr/* undefined */);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxInvertSelectedText) == viewID)
			{
				Preferences_SetData(kPreferences_TagPureInverse,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				// there’s no real reason to do this, since background window text selections will not
				// have an inverted appearance anyway; re-rendering them all wouldn’t change anything
				//SessionFactory_ForEveryTerminalWindowDo(changePreferenceUpdateScreenTerminalWindowOp,
				//										nullptr/* undefined */, kPreferences_TagPureInverse,
				//										nullptr/* undefined */);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxAutoCopySelectedText) == viewID)
			{
				Preferences_SetData(kPreferences_TagCopySelectedText,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxMoveCursorToDropArea) == viewID)
			{
				Preferences_SetData(kPreferences_TagCursorMovesPriorToDrops,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxDoNotWarnOnPaste) == viewID)
			{
				Preferences_SetData(kPreferences_TagNoPasteWarning,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxMapBackquoteToEscape) == viewID)
			{
				Preferences_SetData(kPreferences_TagMapBackquote,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxDoNotAutoCreateWindows) == viewID)
			{
				Preferences_SetData(kPreferences_TagDontAutoNewOnApplicationReopen,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxFocusFollowsMouse) == viewID)
			{
				Preferences_SetData(kPreferences_TagFocusFollowsMouse,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxFadeBackgroundWindows) == viewID)
			{
				Preferences_SetData(kPreferences_TagFadeBackgroundWindows,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			break;
		
		case kMy_TabIndexGeneralSpecial:
			if (HIViewIDWrap(idMyCheckBoxCursorFlashing) == viewID)
			{
				// save the new value
				Preferences_SetData(kPreferences_TagCursorBlinks,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else
			{
				// check if any of the radio buttons was hit; if not, check for other control hits
				HIViewWrap		radioButtonCommandNDefault(idMyRadioButtonCommandNDefault, kWindow);
				HIViewWrap		radioButtonCommandNShell(idMyRadioButtonCommandNShell, kWindow);
				HIViewWrap		radioButtonCommandNLogInShell(idMyRadioButtonCommandNLogInShell, kWindow);
				HIViewWrap		radioButtonCommandNDialog(idMyRadioButtonCommandNDialog, kWindow);
				UInt32			newCommandShortcutEffect = 0;
				
				
				if (HIViewIDWrap(radioButtonCommandNDefault.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionDefaultFavorite;
					SetControl32BitValue(radioButtonCommandNShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNLogInShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNDialog, kControlRadioButtonUncheckedValue);
				}
				else if (HIViewIDWrap(radioButtonCommandNShell.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionShell;
					SetControl32BitValue(radioButtonCommandNDefault, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNLogInShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNDialog, kControlRadioButtonUncheckedValue);
				}
				else if (HIViewIDWrap(radioButtonCommandNLogInShell.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionLoginShell;
					SetControl32BitValue(radioButtonCommandNDefault, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNDialog, kControlRadioButtonUncheckedValue);
				}
				else if (HIViewIDWrap(radioButtonCommandNDialog.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionDialog;
					SetControl32BitValue(radioButtonCommandNDefault, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNLogInShell, kControlRadioButtonUncheckedValue);
				}
				else
				{
					// unrecognized radio button
					newCommandShortcutEffect = 0;
				}
				
				if (0 != newCommandShortcutEffect)
				{
					Preferences_SetData(kPreferences_TagNewCommandShortcutEffect,
										sizeof(newCommandShortcutEffect), &newCommandShortcutEffect);
					result = true;
				}
				
				HIViewWrap		radioButtonResizeAffectsScreenSize(idMyRadioButtonResizeAffectsScreenSize, kWindow);
				HIViewWrap		radioButtonResizeAffectsFontSize(idMyRadioButtonResizeAffectsFontSize, kWindow);
				Boolean			resizeAffectsFontSize = false;
				
				
				if (HIViewIDWrap(radioButtonResizeAffectsScreenSize.identifier()) == viewID)
				{
					resizeAffectsFontSize = false;
					Preferences_SetData(kPreferences_TagTerminalResizeAffectsFontSize,
										sizeof(resizeAffectsFontSize), &resizeAffectsFontSize);
					SetControl32BitValue(radioButtonResizeAffectsFontSize, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonResizeAffectsFontSize.identifier()) == viewID)
				{
					resizeAffectsFontSize = true;
					Preferences_SetData(kPreferences_TagTerminalResizeAffectsFontSize,
										sizeof(resizeAffectsFontSize), &resizeAffectsFontSize);
					SetControl32BitValue(radioButtonResizeAffectsScreenSize, kControlRadioButtonUncheckedValue);
					result = true;
				}
			}
			break;
		
		case kMy_TabIndexGeneralNotification:
			{
				HIViewWrap		radioButtonNotificationNone(idMyRadioButtonNotifyDoNothing, kWindow);
				HIViewWrap		radioButtonNotificationBadge(idMyRadioButtonNotifyBadgeDockIcon, kWindow);
				HIViewWrap		radioButtonNotificationBounce(idMyRadioButtonNotifyBounceDockIcon, kWindow);
				HIViewWrap		radioButtonNotificationDialog(idMyRadioButtonNotifyDisplayMessage, kWindow);
				
				
				if (HIViewIDWrap(radioButtonNotificationNone.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyDoNothing;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationBadge, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBounce, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationDialog, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonNotificationBadge.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyDisplayDiamondMark;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationNone, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBounce, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationDialog, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonNotificationBounce.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyDisplayIconAndDiamondMark;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationNone, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBadge, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationDialog, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonNotificationDialog.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyAlsoDisplayAlert;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationNone, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBadge, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBounce, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewWrap(idMyCheckBoxVisualBell, kWindow) == HIViewWrap(inCheckBoxClicked))
				{
					// save the new value
					Preferences_SetData(kPreferences_TagVisualBell,
										sizeof(checkBoxFlagValue), &checkBoxFlagValue);
					result = true;
				}
				else if (HIViewWrap(idMyCheckBoxNotifyTerminalBeeps, kWindow) == HIViewWrap(inCheckBoxClicked))
				{
					// save the new value
					Preferences_SetData(kPreferences_TagNotifyOfBeeps,
										sizeof(checkBoxFlagValue), &checkBoxFlagValue);
					result = true;
				}
			}
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// updateCheckBoxPreference

} // anonymous namespace


#pragma mark -
@implementation PrefPanelGeneral_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers = @[
										[[[PrefPanelGeneral_OptionsViewManager alloc] init] autorelease],
										[[[PrefPanelGeneral_SpecialViewManager alloc] init] autorelease],
										[[[PrefPanelGeneral_NotificationsViewManager alloc] init] autorelease],
									];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.General"
										localizedName:NSLocalizedStringFromTable(@"General", @"PrefPanelGeneral",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelGeneral"]
										viewManagerArray:subViewManagers];
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


@end // PrefPanelGeneral_ViewManager


#pragma mark -
@implementation PrefPanelGeneral_SoundInfo


/*!
Creates an object representing the user’s system-wide
default alert sound.

(4.1)
*/
- (instancetype)
initAsDefault
{
	self = [self initWithDescription:nil isDefault:YES isOff:NO];
	if (nil != self)
	{
	}
	return self;
}// initAsDefault


/*!
Creates an object representing the sound-off state.

(4.1)
*/
- (instancetype)
initAsOff
{
	self = [self initWithDescription:nil isDefault:NO isOff:YES];
	if (nil != self)
	{
	}
	return self;
}// initAsOff


/*!
Creates an object representing a particular sound name.

(4.1)
*/
- (instancetype)
initWithDescription:(NSString*)		aDescription
{
	self = [self initWithDescription:aDescription isDefault:NO isOff:NO];
	if (nil != self)
	{
	}
	return self;
}// initWithDescription:


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithDescription:(NSString*)		aDescription
isDefault:(BOOL)					aDefaultFlag
isOff:(BOOL)						anOffFlag
{
	self = [super initWithBoundName:aDescription];
	if (nil != self)
	{
		self->isDefault = aDefaultFlag;
		self->isOff = anOffFlag;
		if (aDefaultFlag)
		{
			[self setDescription:NSLocalizedStringFromTable(@"Default", @"PrefPanelGeneral",
															@"label for the default terminal bell sound")];
		}
		else if (anOffFlag)
		{
			[self setDescription:NSLocalizedStringFromTable(@"Off", @"PrefPanelGeneral",
															@"label for turning off the terminal bell sound")];
		}
		else
		{
			[self setDescription:aDescription];
		}
	}
	return self;
}// initWithDescription:isDefault:isOff:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Plays the sound (if any) that is represented by this object.

(4.1)
*/
- (void)
playSound
{
	if (self->isDefault)
	{
		NSBeep();
	}
	else if (self->isOff)
	{
		// do nothing
	}
	else
	{
		CocoaBasic_PlaySoundByName((CFStringRef)[self boundName]);
	}
}// playSound


/*!
Returns the string that should be used when storing this setting
in user preferences.  This is usually the name of the sound, but
for the Off and Default objects it is different.

(4.0)
*/
- (NSString*)
preferenceString
{
	if (self->isDefault)
	{
		return @"";
	}
	else if (self->isOff)
	{
		return @"off";
	}
	return [self boundName];
}// preferenceString


@end // PrefPanelGeneral_SoundInfo


#pragma mark -
@implementation PrefPanelGeneral_NotificationsViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralNotificationsCocoa" delegate:self context:nullptr];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
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
	[prefsMgr release];
	[soundNameIndexes release];
	[soundNames release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
alwaysUseVisualBell
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagVisualBell defaultValue:NO];
}
- (void)
setAlwaysUseVisualBell:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagVisualBell];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save visual-bell preference");
	}
}// setAlwaysUseVisualBell:


/*!
Accessor.

(4.1)
*/
- (BOOL)
backgroundBellsSendNotifications
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagNotifyOfBeeps defaultValue:NO];
}
- (void)
setBackgroundBellsSendNotifications:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagNotifyOfBeeps];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save background-bell-notification preference");
	}
}// setBackgroundBellsSendNotifications:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationNone
{
	return (kAlert_NotifyDoNothing == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationNone
{
	return NO;
}
- (void)
setBackgroundNotificationNone:(BOOL)	aFlag
{
	if ([self isBackgroundNotificationNone] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyDoNothing];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-none preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationNone:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationChangeDockIcon
{
	return (kAlert_NotifyDisplayDiamondMark == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationChangeDockIcon
{
	return NO;
}
- (void)
setBackgroundNotificationChangeDockIcon:(BOOL)	aFlag
{
	if ([self isBackgroundNotificationChangeDockIcon] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyDisplayDiamondMark];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-change-Dock-icon preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationChangeDockIcon:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationAnimateIcon
{
	return (kAlert_NotifyDisplayIconAndDiamondMark == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationAnimateIcon
{
	return NO;
}
- (void)
setBackgroundNotificationAnimateIcon:(BOOL)		aFlag
{
	if ([self isBackgroundNotificationAnimateIcon] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyDisplayIconAndDiamondMark];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-animate-icon preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationAnimateIcon:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationDisplayMessage
{
	return (kAlert_NotifyAlsoDisplayAlert == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationDisplayMessage
{
	return NO;
}
- (void)
setBackgroundNotificationDisplayMessage:(BOOL)		aFlag
{
	if ([self isBackgroundNotificationDisplayMessage] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyAlsoDisplayAlert];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-display-message preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationDisplayMessage:


/*!
Accessor.

(4.0)
*/
- (NSArray*)
soundNames
{
	return [[soundNames retain] autorelease];
}


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
soundNameIndexes
{
	return [[soundNameIndexes retain] autorelease];
}
+ (BOOL)
automaticallyNotifiesObserversOfSoundNameIndexes
{
	return NO;
}
- (void)
setSoundNameIndexes:(NSIndexSet*)	indexes
{
	unsigned int					newIndex = (nil != indexes)
												? [indexes firstIndex]
												: 0;
	PrefPanelGeneral_SoundInfo*		info = ((NSNotFound != newIndex) && (newIndex < [[self soundNames] count]))
											? [[self soundNames] objectAtIndex:newIndex]
											: nil;
	
	
	if (indexes != soundNameIndexes)
	{
		[self willChangeValueForKey:@"soundNameIndexes"];
		
		[soundNameIndexes release];
		soundNameIndexes = [indexes retain];
		
		// save the new preference
		{
			BOOL		writeOK = NO;
			NSString*	savedName = [info preferenceString];
			
			
			if (nil != savedName)
			{
				writeOK = [self writeBellSoundName:savedName];
				if (writeOK)
				{
					[info playSound];
				}
			}
			
			if (NO == writeOK)
			{
				Console_Warning(Console_WriteLine, "failed to save bell-sound preference");
			}
		}
		
		[self didChangeValueForKey:@"soundNameIndexes"];
	}
	else
	{
		// play the sound either way, even if the user chooses the same item again
		[info playSound];
	}
}// setSoundNameIndexes:


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
#pragma unused(aViewManager, aContext)
	self->prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	// set up the array of bell sounds
	{
		NSArray*		soundNamesOnly = BRIDGE_CAST(CocoaBasic_ReturnUserSoundNames(), NSArray*);
		NSString*		savedName = [self readBellSoundNameWithDefaultValue:@""];
		unsigned int	currentIndex = 0;
		unsigned int	initialIndex = 0;
		
		
		[self willChangeValueForKey:@"soundNames"];
		self->soundNames = [[NSMutableArray alloc] initWithCapacity:[soundNamesOnly count]];
		[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc] initAsOff] autorelease]];
		if ([savedName isEqualToString:[[self->soundNames lastObject] preferenceString]])
		{
			initialIndex = currentIndex;
		}
		++currentIndex;
		[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc] initAsDefault] autorelease]];
		if ([savedName isEqualToString:[[self->soundNames lastObject] preferenceString]])
		{
			initialIndex = currentIndex;
		}
		++currentIndex;
		for (NSString* soundName in soundNamesOnly)
		{
			[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc]
												initWithDescription:soundName]
											autorelease]];
			if ([savedName isEqualToString:[[self->soundNames lastObject] preferenceString]])
			{
				initialIndex = currentIndex;
			}
			++currentIndex;
		}
		[self didChangeValueForKey:@"soundNames"];
		
		[self willChangeValueForKey:@"soundNameIndexes"];
		self->soundNameIndexes = [[NSIndexSet indexSetWithIndex:initialIndex] retain];
		[self didChangeValueForKey:@"soundNameIndexes"];
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
}// panelViewManager:requestingIdealSize:


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager, sender)
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
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
}// panelViewManager:didChangePanelVisibility:


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
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.Notifications";
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
	return NSLocalizedStringFromTable(@"Notifications", @"PrefPanelGeneral", @"the name of this panel");
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


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end // PrefPanelGeneral_NotificationsViewManager


#pragma mark -
@implementation PrefPanelGeneral_NotificationsViewManager (PrefPanelGeneral_NotificationsViewManagerInternal)


/*!
Send when changing any of the properties that affect the
command-N key mapping (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForBackgroundNotification
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"backgroundNotificationNone"];
	[self didChangeValueForKey:@"backgroundNotificationChangeDockIcon"];
	[self didChangeValueForKey:@"backgroundNotificationAnimateIcon"];
	[self didChangeValueForKey:@"backgroundNotificationDisplayMessage"];
}
- (void)
notifyWillChangeValueForBackgroundNotification
{
	[self willChangeValueForKey:@"backgroundNotificationDisplayMessage"];
	[self willChangeValueForKey:@"backgroundNotificationAnimateIcon"];
	[self willChangeValueForKey:@"backgroundNotificationChangeDockIcon"];
	[self willChangeValueForKey:@"backgroundNotificationNone"];
}// notifyWillChangeValueForBackgroundNotification


/*!
Returns the current user preference for background notifications,
or the specified default value if no preference exists.  The
result will match a "kAlert_Notify..." constant.

(4.1)
*/
- (SInt16)
readBackgroundNotificationTypeWithDefaultValue:(SInt16)		aDefaultValue
{
	SInt16					result = aDefaultValue;
	Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagNotification,
																sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readBackgroundNotificationTypeWithDefaultValue:


/*!
Returns the current user preference for the bell sound.

This is the base name of a sound file, or an empty string
(to indicate the system’s default alert sound is played)
or the special string "off" to indicate no sound at all.

(4.1)
*/
- (NSString*)
readBellSoundNameWithDefaultValue:(NSString*)	aDefaultValue
{
	NSString*			result = nil;
	CFStringRef			soundName = nullptr;
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	BOOL				releaseSoundName = YES;
	
	
	// determine user’s preferred sound
	prefsResult = Preferences_GetData(kPreferences_TagBellSound, sizeof(soundName),
										&soundName);
	if (kPreferences_ResultOK != prefsResult)
	{
		soundName = BRIDGE_CAST(aDefaultValue, CFStringRef);
		releaseSoundName = NO;
	}
	
	result = [[((NSString*)soundName) retain] autorelease];
	if (releaseSoundName)
	{
		CFRelease(soundName), soundName = nullptr;
	}
	
	return result;
}// readBellSoundNameWithDefaultValue:


/*!
Writes a new user preference for background notifications and
returns YES only if this succeeds.  The given value must match
a "kAlert_Notify..." constant.

(4.1)
*/
- (BOOL)
writeBackgroundNotificationType:(SInt16)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagNotification,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeBackgroundNotificationType:


/*!
Writes a new user preference for the bell sound.  For details
on what the string can be, see the documentation for the
"readBellSoundNameWithDefaultValue" method.

(4.1)
*/
- (BOOL)
writeBellSoundName:(NSString*)	aValue
{
	CFStringRef			asCFStringRef = BRIDGE_CAST(aValue, CFStringRef);
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagBellSound,
															sizeof(asCFStringRef), &asCFStringRef);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeBellSoundName:


@end // PrefPanelGeneral_NotificationsViewManager (PrefPanelGeneral_NotificationsViewManagerInternal)


#pragma mark -
@implementation PrefPanelGeneral_OptionsViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralOptionsCocoa" delegate:self context:nullptr];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
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
	[prefsMgr release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
noWindowCloseOnProcessExit
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagDontAutoClose defaultValue:NO];
}
- (void)
setNoWindowCloseOnProcessExit:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagDontAutoClose];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save no-auto-close preference");
	}
}// setNoWindowCloseOnProcessExit:


/*!
Accessor.

(4.1)
*/
- (BOOL)
noAutomaticNewWindows
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagDontAutoNewOnApplicationReopen defaultValue:NO];
}
- (void)
setNoAutomaticNewWindows:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagDontAutoNewOnApplicationReopen];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save no-auto-new preference");
	}
}// setNoAutomaticNewWindows:


/*!
Accessor.

(4.1)
*/
- (BOOL)
fadeInBackground
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagFadeBackgroundWindows defaultValue:NO];
}
- (void)
setFadeInBackground:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagFadeBackgroundWindows];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save fade-in-background preference");
	}
}// setFadeInBackground:


/*!
Accessor.

(4.1)
*/
- (BOOL)
invertSelectedText
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagPureInverse defaultValue:NO];
}
- (void)
setInvertSelectedText:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagPureInverse];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save invert-selected-text preference");
	}
}// setInvertSelectedText:


/*!
Accessor.

(4.1)
*/
- (BOOL)
automaticallyCopySelectedText
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagCopySelectedText defaultValue:NO];
}
- (void)
setAutomaticallyCopySelectedText:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagCopySelectedText];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save auto-copy-selected-text preference");
	}
}// setAutomaticallyCopySelectedText:


/*!
Accessor.

(4.1)
*/
- (BOOL)
moveCursorToTextDropLocation
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagCursorMovesPriorToDrops defaultValue:NO];
}
- (void)
setMoveCursorToTextDropLocation:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagCursorMovesPriorToDrops];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save move-cursor-to-drop-location preference");
	}
}// setMoveCursorToTextDropLocation:


/*!
Accessor.

(4.1)
*/
- (BOOL)
doNotDimBackgroundTerminalText
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagDontDimBackgroundScreens defaultValue:NO];
}
- (void)
setDoNotDimBackgroundTerminalText:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagDontDimBackgroundScreens];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save do-not-dim preference");
	}
}// setDoNotDimBackgroundTerminalText:


/*!
Accessor.

(4.1)
*/
- (BOOL)
doNotWarnAboutMultiLinePaste
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagNoPasteWarning defaultValue:NO];
}
- (void)
setDoNotWarnAboutMultiLinePaste:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagNoPasteWarning];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save do-not-warn-about-multi-line-paste preference");
	}
}// setDoNotWarnAboutMultiLinePaste:


/*!
Accessor.

(4.1)
*/
- (BOOL)
treatBackquoteLikeEscape
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagMapBackquote defaultValue:NO];
}
- (void)
setTreatBackquoteLikeEscape:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagMapBackquote];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save backquote-is-escape preference");
	}
}// setTreatBackquoteLikeEscape:


/*!
Accessor.

(4.1)
*/
- (BOOL)
focusFollowsMouse
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagFocusFollowsMouse defaultValue:NO];
}
- (void)
setFocusFollowsMouse:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagFocusFollowsMouse];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save focus-follows-mouse preference");
	}
}// setFocusFollowsMouse:


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
#pragma unused(aViewManager, aContext)
	self->prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
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
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
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
}// panelViewManager:didChangePanelVisibility:


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
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.Options";
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
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelGeneral", @"the name of this panel");
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


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end // PrefPanelGeneral_OptionsViewManager


#pragma mark -
@implementation PrefPanelGeneral_OptionsViewManager (PrefPanelGeneral_OptionsViewManagerInternal)


@end // PrefPanelGeneral_OptionsViewManager (PrefPanelGeneral_OptionsViewManagerInternal)


#pragma mark -
@implementation PrefPanelGeneral_SpecialViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralSpecialCocoa" delegate:self context:nullptr];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
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
	[prefsMgr release];
	[byKey release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorFlashes
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagCursorBlinks defaultValue:NO];
}
- (void)
setCursorFlashes:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagCursorBlinks];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save cursor-flashing preference");
	}
}// setCursorFlashes:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsBlock
{
	return (kTerminalView_CursorTypeBlock == [self readCursorTypeWithDefaultValue:kTerminalView_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsBlock
{
	return NO;
}
- (void)
setCursorShapeIsBlock:(BOOL)	aFlag
{
	if ([self cursorShapeIsBlock] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminalView_CursorTypeBlock];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-block preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsBlock:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsThickUnderline
{
	return (kTerminalView_CursorTypeThickUnderscore == [self readCursorTypeWithDefaultValue:kTerminalView_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsThickUnderline
{
	return NO;
}
- (void)
setCursorShapeIsThickUnderline:(BOOL)	aFlag
{
	if ([self cursorShapeIsThickUnderline] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminalView_CursorTypeThickUnderscore];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-thick-underline preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsThickUnderline:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsThickVerticalBar
{
	return (kTerminalView_CursorTypeThickVerticalLine == [self readCursorTypeWithDefaultValue:kTerminalView_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsThickVerticalBar
{
	return NO;
}
- (void)
setCursorShapeIsThickVerticalBar:(BOOL)		aFlag
{
	if ([self cursorShapeIsThickVerticalBar] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminalView_CursorTypeThickVerticalLine];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-thick-vertical-bar preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsThickVerticalBar:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsUnderline
{
	return (kTerminalView_CursorTypeUnderscore == [self readCursorTypeWithDefaultValue:kTerminalView_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsUnderline
{
	return NO;
}
- (void)
setCursorShapeIsUnderline:(BOOL)	aFlag
{
	if ([self cursorShapeIsUnderline] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminalView_CursorTypeUnderscore];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-underline preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsUnderline:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsVerticalBar
{
	return (kTerminalView_CursorTypeVerticalLine == [self readCursorTypeWithDefaultValue:kTerminalView_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsVerticalBar
{
	return NO;
}
- (void)
setCursorShapeIsVerticalBar:(BOOL)		aFlag
{
	if ([self cursorShapeIsVerticalBar] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminalView_CursorTypeVerticalLine];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-vertical-bar preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsVerticalBar:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isWindowResizeEffectTerminalScreenSize
{
	BOOL	preferenceFlag = [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize
																		defaultValue:NO];
	
	
	return (NO == preferenceFlag);
}
- (void)
setWindowResizeEffectTerminalScreenSize:(BOOL)	aFlag
{
	if ([self isWindowResizeEffectTerminalScreenSize] != aFlag)
	{
		[self notifyWillChangeValueForWindowResizeEffect];
		
		BOOL	preferenceFlag = (NO == aFlag);
		BOOL	writeOK = [self->prefsMgr writeFlag:preferenceFlag
													forPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save window-resize-affects-font-size preference");
		}
		
		[self notifyDidChangeValueForWindowResizeEffect];
	}
}// setWindowResizeEffectTerminalScreenSize:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isWindowResizeEffectTextSize
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize defaultValue:NO];
}
- (void)
setWindowResizeEffectTextSize:(BOOL)	aFlag
{
	if ([self isWindowResizeEffectTextSize] != aFlag)
	{
		[self notifyWillChangeValueForWindowResizeEffect];
		
		BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save window-resize-affects-font-size preference");
		}
		
		[self notifyDidChangeValueForWindowResizeEffect];
	}
}// setWindowResizeEffectTextSize:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandCustomNewSession
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kCommandNewSessionDefaultFavorite];
	
	
	return (kCommandNewSessionDialog == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandCustomNewSession
{
	return NO;
}
- (void)
setNewCommandCustomNewSession:(BOOL)	aFlag
{
	if ([self isNewCommandCustomNewSession] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kCommandNewSessionDialog];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-custom-session preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandCustomNewSession:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandDefaultSessionFavorite
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kCommandNewSessionDefaultFavorite];
	
	
	return (kCommandNewSessionDefaultFavorite == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandDefaultSessionFavorite
{
	return NO;
}
- (void)
setNewCommandDefaultSessionFavorite:(BOOL)	aFlag
{
	if ([self isNewCommandDefaultSessionFavorite] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kCommandNewSessionDefaultFavorite];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-default-favorite preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandDefaultSessionFavorite:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandLogInShell
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kCommandNewSessionDefaultFavorite];
	
	
	return (kCommandNewSessionLoginShell == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandLogInShell
{
	return NO;
}
- (void)
setNewCommandLogInShell:(BOOL)	aFlag
{
	if ([self isNewCommandLogInShell] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kCommandNewSessionLoginShell];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-log-in-shell preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandLogInShell:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandShell
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kCommandNewSessionDefaultFavorite];
	
	
	return (kCommandNewSessionShell == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandShell
{
	return NO;
}
- (void)
setNewCommandShell:(BOOL)	aFlag
{
	if ([self isNewCommandShell] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kCommandNewSessionShell];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-shell preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandLogInShell:


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
spacesPerTab
{
	return [self->byKey objectForKey:@"spacesPerTab"];
}// spacesPerTab


#pragma mark Actions


/*!
Responds to a request to set the window stacking origin.

(4.1)
*/
- (IBAction)
performSetWindowStackingOrigin:(id)	sender
{
#pragma unused(sender)
	Keypads_SetArrangeWindowPanelBinding(kPreferences_TagWindowStackingOrigin, typeQDPoint);
	Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
}// performSetWindowStackingOrigin:


#pragma mark Validators


/*!
Validates the number of spaces entered by the user, returning an
appropriate error (and a NO result) if the number is incorrect.

(4.1)
*/
- (BOOL)
validateSpacesPerTab:(id*/* NSString* */)	ioValue
error:(NSError**)						outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		// first strip whitespace
		*ioValue = [[*ioValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] retain];
		
		// while an NSNumberFormatter is more typical for validation,
		// the requirements for this numerical value are quite simple
		NSScanner*	scanner = [NSScanner scannerWithString:*ioValue];
		int			value = 0;
		
		
		// the error message below should agree with the range enforced here...
		if ([scanner scanInt:&value] && [scanner isAtEnd] && (value >= 1) && (value <= 24/* arbitrary */))
		{
			result = YES;
		}
		else
		{
			if (nil != outError) result = NO;
			else result = YES; // cannot return NO when the error instance is undefined
		}
		
		if (NO == result)
		{
			*outError = [NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
							code:kConstantsRegistry_NSErrorBadUserID
							userInfo:@{
											NSLocalizedDescriptionKey: NSLocalizedStringFromTable
																		(@"“Copy with Tab Substitution” requires a count from 1 to 24 spaces.",
																			@"PrefPanelGeneral"/* table */,
																			@"message displayed for bad numbers"),
										}];
		}
	}
	return result;
}// validateSpacesPerTab:error:


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
#pragma unused(aViewManager, aContext)
	self->prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:8/* arbitrary; number of expected settings */];
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
	assert(nil != byKey);
	assert(nil != prefsMgr);
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagCopyTableThreshold
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16] autorelease]
					forKey:@"spacesPerTab"];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
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
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
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
}// panelViewManager:didChangePanelVisibility:


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
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.Special";
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
	return NSLocalizedStringFromTable(@"Special", @"PrefPanelGeneral", @"the name of this panel");
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


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end // PrefPanelGeneral_SpecialViewManager


#pragma mark -
@implementation PrefPanelGeneral_SpecialViewManager (PrefPanelGeneral_SpecialViewManagerInternal)


/*!
Send when changing any of the properties that affect the
shape of the cursor (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForCursorShape
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"cursorShapeIsVerticalBar"];
	[self didChangeValueForKey:@"cursorShapeIsUnderline"];
	[self didChangeValueForKey:@"cursorShapeIsThickVerticalBar"];
	[self didChangeValueForKey:@"cursorShapeIsThickUnderline"];
	[self didChangeValueForKey:@"cursorShapeIsBlock"];
}
- (void)
notifyWillChangeValueForCursorShape
{
	[self willChangeValueForKey:@"cursorShapeIsBlock"];
	[self willChangeValueForKey:@"cursorShapeIsThickUnderline"];
	[self willChangeValueForKey:@"cursorShapeIsThickVerticalBar"];
	[self willChangeValueForKey:@"cursorShapeIsUnderline"];
	[self willChangeValueForKey:@"cursorShapeIsVerticalBar"];
}// notifyWillChangeValueForCursorShape


/*!
Send when changing any of the properties that affect the
command-N key mapping (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForNewCommand
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"newCommandShell"];
	[self didChangeValueForKey:@"newCommandLogInShell"];
	[self didChangeValueForKey:@"newCommandDefaultSessionFavorite"];
	[self didChangeValueForKey:@"newCommandCustomNewSession"];
}
- (void)
notifyWillChangeValueForNewCommand
{
	[self willChangeValueForKey:@"newCommandCustomNewSession"];
	[self willChangeValueForKey:@"newCommandDefaultSessionFavorite"];
	[self willChangeValueForKey:@"newCommandLogInShell"];
	[self willChangeValueForKey:@"newCommandShell"];
}// notifyWillChangeValueForNewCommand


/*!
Send when changing any of the properties that affect the
window resize effect (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForWindowResizeEffect
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"windowResizeEffectTextSize"];
	[self didChangeValueForKey:@"windowResizeEffectTerminalScreenSize"];
}
- (void)
notifyWillChangeValueForWindowResizeEffect
{
	[self willChangeValueForKey:@"windowResizeEffectTerminalScreenSize"];
	[self willChangeValueForKey:@"windowResizeEffectTextSize"];
}// notifyWillChangeValueForWindowResizeEffect


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[
				@"spacesPerTab",
			];
}// primaryDisplayBindingKeys


/*!
Returns the current user preference for the cursor shape,
or the specified default value if no preference exists.

(4.1)
*/
- (TerminalView_CursorType)
readCursorTypeWithDefaultValue:(TerminalView_CursorType)	aDefaultValue
{
	TerminalView_CursorType		result = aDefaultValue;
	Preferences_Result			prefsResult = Preferences_GetData(kPreferences_TagTerminalCursorType,
																	sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readCursorTypeWithDefaultValue:


/*!
Returns the current user preference for “command-N mapping”,
or the specified default value if no preference exists.

(4.1)
*/
- (UInt32)
readNewCommandShortcutEffectWithDefaultValue:(UInt32)	aDefaultValue
{
	UInt32				result = aDefaultValue;
	Preferences_Result	prefsResult = Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
															sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readNewCommandShortcutEffectWithDefaultValue:


/*!
Returns the current user preference for spaces per tab,
or the specified default value if no preference exists.

(4.1)
*/
- (UInt16)
readSpacesPerTabWithDefaultValue:(UInt16)	aDefaultValue
{
	UInt16				result = aDefaultValue;
	Preferences_Result	prefsResult = Preferences_GetData(kPreferences_TagCopyTableThreshold,
															sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readSpacesPerTabWithDefaultValue:


/*!
Writes a new user preference for the cursor shape and
returns YES only if this succeeds.

(4.1)
*/
- (BOOL)
writeCursorType:(TerminalView_CursorType)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagTerminalCursorType,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeCursorType:


/*!
Writes a new user preference for “command-N mapping” and
returns YES only if this succeeds.

(4.1)
*/
- (BOOL)
writeNewCommandShortcutEffect:(UInt32)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagNewCommandShortcutEffect,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeNewCommandShortcutEffect:


/*!
Writes a new user preference for spaces per tab and
returns YES only if this succeeds.

(4.1)
*/
- (BOOL)
writeSpacesPerTab:(UInt16)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagCopyTableThreshold,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeSpacesPerTab:


@end // PrefPanelGeneral_SpecialViewManager (PrefPanelGeneral_SpecialViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
