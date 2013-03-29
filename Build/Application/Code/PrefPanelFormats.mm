/*!	\file PrefPanelFormats.mm
	\brief Implements the Formats panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2013 by Kevin Grant.
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

#import "PrefPanelFormats.h"
#import <UniversalDefines.h>

// standard-C includes
#import <algorithm>
#import <cstring>
#import <utility>

// Unix includes
#import <strings.h>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventUtilities.template.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CocoaExtensions.objc++.h>
#import <ColorUtilities.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <DialogAdjust.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "ColorBox.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "GenericPanelTabs.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "PreferenceValue.objc++.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelsFavorites.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyCheckBoxDefaultFontName			= { 'XDFt', 0/* ID */ };
HIViewID const	idMyButtonFontName					= { 'Font', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultFontSize			= { 'XDFS', 0/* ID */ };
HIViewID const	idMyButtonFontSize					= { 'Size', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultCharacterWidth	= { 'XDCW', 0/* ID */ };
HIViewID const	idMySliderFontCharacterWidth		= { 'CWid', 0/* ID */ };
HIViewID const	idMyStaticTextNonMonospacedWarning	= { 'WMno', 0/* ID */ };
HIViewID const	idMyHelpTextCharacterWidth			= { 'HChW', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultNormalColors		= { 'XDNC', 0/* ID */ };
HIViewID const	idMyBevelButtonNormalText			= { 'NTxt', 0/* ID */ };
HIViewID const	idMyBevelButtonNormalBackground		= { 'NBkg', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultBoldColors		= { 'XDBC', 0/* ID */ };
HIViewID const	idMyBevelButtonBoldText				= { 'BTxt', 0/* ID */ };
HIViewID const	idMyBevelButtonBoldBackground		= { 'BBkg', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultBlinkingColors	= { 'XDBl', 0/* ID */ };
HIViewID const	idMyBevelButtonBlinkingText			= { 'BlTx', 0/* ID */ };
HIViewID const	idMyBevelButtonBlinkingBackground	= { 'BlBk', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultMatteColors		= { 'XDMC', 0/* ID */ };
HIViewID const	idMyBevelButtonMatteForeground		= { 'MtTx', 0/* ID */ };
HIViewID const	idMyBevelButtonMatteBackground		= { 'MtBk', 0/* ID */ };
HIViewID const	idMyUserPaneSampleTerminalView		= { 'Smpl', 0/* ID */ };
HIViewID const	idMyHelpTextSampleTerminalView		= { 'HSmp', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIBlackColors	= { 'XDAB', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalBlack		= { 'Cblk', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldBlack		= { 'CBlk', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIRedColors	= { 'XDAR', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalRed		= { 'Cred', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldRed			= { 'CRed', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIGreenColors	= { 'XDAG', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalGreen		= { 'Cgrn', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldGreen		= { 'CGrn', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIYellowColors	= { 'XDAY', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalYellow		= { 'Cyel', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldYellow		= { 'CYel', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIBlueColors	= { 'XDAb', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalBlue		= { 'Cblu', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldBlue			= { 'CBlu', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIMagentaColors= { 'XDAM', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalMagenta	= { 'Cmag', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldMagenta		= { 'CMag', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSICyanColors	= { 'XDAC', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalCyan		= { 'Ccyn', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldCyan			= { 'CCyn', 0/* ID */ };
HIViewID const	idMyCheckBoxDefaultANSIWhiteColors	= { 'XDAW', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalWhite		= { 'Cwht', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldWhite		= { 'CWht', 0/* ID */ };
HIViewID const	idMyHelpTextANSIColors				= { 'HANS', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Implements the “ANSI Colors” tab.
*/
struct My_FormatsPanelANSIColorsUI
{
	My_FormatsPanelANSIColorsUI		(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static void
	colorBoxChangeNotify	(HIViewRef, RGBColor const*, void*);
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	readPreferencesForBlack		(Preferences_ContextRef);
	
	void
	readPreferencesForRed		(Preferences_ContextRef);
	
	void
	readPreferencesForGreen		(Preferences_ContextRef);
	
	void
	readPreferencesForYellow	(Preferences_ContextRef);
	
	void
	readPreferencesForBlue		(Preferences_ContextRef);
	
	void
	readPreferencesForMagenta	(Preferences_ContextRef);
	
	void
	readPreferencesForCyan		(Preferences_ContextRef);
	
	void
	readPreferencesForWhite		(Preferences_ContextRef);
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	void
	setInheritanceCheckBox	(HIViewWrap, SInt32);
	
private:
	CarbonEventHandlerWrap				_buttonCommandsHandler;		//!< invoked when a button is clicked
	CommonEventHandlers_HIViewResizer	_containerResizer;
};
typedef My_FormatsPanelANSIColorsUI*	My_FormatsPanelANSIColorsUIPtr;

/*!
Implements the “Normal” tab.
*/
struct My_FormatsPanelNormalUI
{
	My_FormatsPanelNormalUI		(Panel_Ref, HIWindowRef);
	
	Panel_Ref			panel;				//!< the panel using this UI
	Float32				idealWidth;			//!< best size in pixels
	Float32				idealHeight;		//!< best size in pixels
	TerminalScreenRef	terminalScreen;		//!< used to store sample text
	TerminalViewRef		terminalView;		//!< used to render a sample
	HIViewWrap			terminalHIView;		//!< container of sample
	ControlActionUPP	sliderActionUPP;	//!< for live tracking
	HIViewWrap			mainView;
	
	static void
	colorBoxChangeNotify	(HIViewRef, RGBColor const*, void*);
	
	void
	loseFocus	();
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	readPreferencesForBlinkingColors	(Preferences_ContextRef, Boolean = true);
	
	void
	readPreferencesForBoldColors	(Preferences_ContextRef, Boolean = true);
	
	void
	readPreferencesForFontCharacterWidth	(Preferences_ContextRef, Boolean = true);
	
	void
	readPreferencesForFontName	(Preferences_ContextRef, Boolean = true);
	
	void
	readPreferencesForFontSize	(Preferences_ContextRef, Boolean = true);
	
	void
	readPreferencesForMatteColors	(Preferences_ContextRef, Boolean = true);
	
	void
	readPreferencesForNormalColors	(Preferences_ContextRef, Boolean = true);
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	void
	saveFieldPreferences	(Preferences_ContextRef);
	
	void
	setFontName		(CFStringRef, Boolean);
	
	void
	setFontSize		(SInt16, Boolean);
	
	void
	setFontWidthScaleFactor		(Float32, Boolean);
	
	void
	setSampleArea	(Preferences_ContextRef, Preferences_Tag = '----', Preferences_Tag = '----');

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	TerminalScreenRef
	createSampleTerminalScreen () const;
	
	void
	setInheritanceCheckBox	(HIViewWrap, SInt32);
	
	HIViewRef
	setUpSampleTerminalHIView	(TerminalViewRef, TerminalScreenRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	static void
	sliderProc	(HIViewRef, HIViewPartCode);

private:
	CarbonEventHandlerWrap				_buttonCommandsHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_fontPanelHandler;			//!< invoked when font panel events occur
	CarbonEventHandlerWrap				_windowFocusHandler;		//!< invoked when the window loses keyboard focus
	CommonEventHandlers_HIViewResizer	_containerResizer;
};
typedef My_FormatsPanelNormalUI*	My_FormatsPanelNormalUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_FormatsPanelANSIColorsData
{
	My_FormatsPanelANSIColorsData	();
	~My_FormatsPanelANSIColorsData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_FormatsPanelANSIColorsUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
	Boolean							isDefaultModel;	//!< true only if "dataModel" matches the class default context
};
typedef My_FormatsPanelANSIColorsData*	My_FormatsPanelANSIColorsDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_FormatsPanelNormalData
{
	My_FormatsPanelNormalData	();
	~My_FormatsPanelNormalData	();
	
	Panel_Ref					panel;			//!< the panel this data is for
	My_FormatsPanelNormalUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef		dataModel;		//!< source of initializations and target of changes
	Boolean						isDefaultModel;	//!< true only if "dataModel" matches the class default context
};
typedef My_FormatsPanelNormalData*	My_FormatsPanelNormalDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Preferences_Result	copyColor						(Preferences_Tag, Preferences_ContextRef, Preferences_ContextRef,
													 Boolean = false, Boolean* = nullptr);
Boolean				isMonospacedFont				(CFStringRef);
OSStatus			receiveFontChange				(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveWindowFocusChange		(EventHandlerCallRef, EventRef, void*);
void				resetANSIWarningCloseNotifyProc	(InterfaceLibAlertRef, SInt16, void*);
void				resetANSIWarningCloseNotifyProcCocoa	(AlertMessages_BoxRef, SInt16, void*);
void				setColorBox						(Preferences_ContextRef, Preferences_Tag, HIViewRef, Boolean&);
void				setInheritanceCheckBox			(HIViewWrap, SInt32, Boolean);

} // anonymous namespace

@interface PrefPanelFormats_GeneralViewManager (PrefPanelFormats_GeneralViewManagerInternal)

- (NSArray*)
colorBindingKeys;

- (NSArray*)
primaryDisplayBindingKeys;

- (NSArray*)
sampleDisplayBindingKeyPaths;

- (void)
setSampleAreaFromDefaultPreferences;

- (void)
setSampleAreaFromPreferences:(Preferences_ContextRef)_
restrictedTag1:(Preferences_Tag)_
restrictedTag2:(Preferences_Tag)_;

@end // PrefPanelFormats_GeneralViewManager (PrefPanelFormats_GeneralViewManagerInternal)

@interface PrefPanelFormats_StandardColorsViewManager (PrefPanelFormats_StandardColorsViewManagerInternal)

- (void)
copyColorWithPreferenceTag:(Preferences_Tag)_
fromContext:(Preferences_ContextRef)_
forKey:(NSString*)_
failureFlag:(BOOL*)_;

- (NSArray*)
primaryDisplayBindingKeys;

- (BOOL)
resetToFactoryDefaultColors;

@end // PrefPanelFormats_StandardColorsViewManager (PrefPanelFormats_StandardColorsViewManagerInternal)



#pragma mark Public Methods

/*!
Creates a new preference panel for the Formats category,
Destroy it using Panel_Dispose().

This is technically a tab container containing both the
panels from PrefPanelFormats_NewNormalPane() and
PrefPanelFormats_NewANSIColorsPane().  Since you did not
create those panels individually, you do not have to
dispose of them; destroying the tab container will delete
the tabs too.

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelFormats_New ()
{
	Panel_Ref				result = nullptr;
	CFStringRef				nameCFString = nullptr;
	GenericPanelTabs_List	tabList;
	
	
	tabList.push_back(PrefPanelFormats_NewNormalPane());
	tabList.push_back(PrefPanelFormats_NewANSIColorsPane());
	
	if (UIStrings_Copy(kUIStrings_PrefPanelFormatsCategoryName, nameCFString).ok())
	{
		result = GenericPanelTabs_New(nameCFString, kConstantsRegistry_PrefPanelDescriptorFormats, tabList);
		if (nullptr != result)
		{
			Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormats);
			Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
											AppResources_ReturnCreatorCode(),
											kConstantsRegistry_IconServicesIconPrefPanelFormats);
		}
		CFRelease(nameCFString), nameCFString = nullptr;
	}
	
	return result;
}// New


/*!
Creates only the Normal colors pane, which allows the user
to edit the font, font sizs, and basic terminal colors with
a preview.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelFormats_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelFormats_NewANSIColorsPane ()
{
	Panel_Ref	result = Panel_New(My_FormatsPanelANSIColorsUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_FormatsPanelANSIColorsDataPtr	dataPtr = new My_FormatsPanelANSIColorsData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorFormatsANSI);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormatsANSI);
		if (UIStrings_Copy(kUIStrings_PrefPanelFormatsANSIColorsTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFormats);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewANSIColorsPane


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelFormats_NewANSIColorsTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalColorANSIBlack);
	tagList.push_back(kPreferences_TagTerminalColorANSIBlackBold);
	tagList.push_back(kPreferences_TagTerminalColorANSIRed);
	tagList.push_back(kPreferences_TagTerminalColorANSIRedBold);
	tagList.push_back(kPreferences_TagTerminalColorANSIGreen);
	tagList.push_back(kPreferences_TagTerminalColorANSIGreenBold);
	tagList.push_back(kPreferences_TagTerminalColorANSIYellow);
	tagList.push_back(kPreferences_TagTerminalColorANSIYellowBold);
	tagList.push_back(kPreferences_TagTerminalColorANSIBlue);
	tagList.push_back(kPreferences_TagTerminalColorANSIBlueBold);
	tagList.push_back(kPreferences_TagTerminalColorANSIMagenta);
	tagList.push_back(kPreferences_TagTerminalColorANSIMagentaBold);
	tagList.push_back(kPreferences_TagTerminalColorANSICyan);
	tagList.push_back(kPreferences_TagTerminalColorANSICyanBold);
	tagList.push_back(kPreferences_TagTerminalColorANSIWhite);
	tagList.push_back(kPreferences_TagTerminalColorANSIWhiteBold);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewANSIColorsTagSet


/*!
Creates only the Normal colors pane, which allows the user
to edit the font, font sizs, and basic terminal colors with
a preview.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelFormats_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelFormats_NewNormalPane ()
{
	Panel_Ref	result = Panel_New(My_FormatsPanelNormalUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_FormatsPanelNormalDataPtr	dataPtr = new My_FormatsPanelNormalData();
		CFStringRef						nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorFormatsNormal);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormatsNormal);
		if (UIStrings_Copy(kUIStrings_PrefPanelFormatsNormalTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFormats);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewNormalPane


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelFormats_NewNormalTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagFontCharacterWidthMultiplier);
	tagList.push_back(kPreferences_TagFontName);
	tagList.push_back(kPreferences_TagFontSize);
	tagList.push_back(kPreferences_TagTerminalColorNormalForeground);
	tagList.push_back(kPreferences_TagTerminalColorNormalBackground);
	tagList.push_back(kPreferences_TagTerminalColorBoldForeground);
	tagList.push_back(kPreferences_TagTerminalColorBoldBackground);
	tagList.push_back(kPreferences_TagTerminalColorBlinkingForeground);
	tagList.push_back(kPreferences_TagTerminalColorBlinkingBackground);
	tagList.push_back(kPreferences_TagTerminalColorMatteBackground);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewNormalTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.0)
*/
Preferences_TagSetRef
PrefPanelFormats_NewTagSet ()
{
	Preferences_TagSetRef	result = Preferences_NewTagSet(30); // arbitrary initial capacity
	Preferences_TagSetRef	normalTags = PrefPanelFormats_NewNormalTagSet();
	Preferences_TagSetRef	standardColorTags = PrefPanelFormats_NewANSIColorsTagSet();
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_TagSetMerge(result, normalTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, standardColorTags);
	assert(kPreferences_ResultOK == prefsResult);
	
	Preferences_ReleaseTagSet(&normalTags);
	Preferences_ReleaseTagSet(&standardColorTags);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_FormatsPanelANSIColorsData structure.

(3.1)
*/
My_FormatsPanelANSIColorsData::
My_FormatsPanelANSIColorsData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr),
isDefaultModel(false)
{
}// My_FormatsPanelANSIColorsData default constructor


/*!
Tears down a My_FormatsPanelANSIColorsData structure.

(3.1)
*/
My_FormatsPanelANSIColorsData::
~My_FormatsPanelANSIColorsData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_FormatsPanelANSIColorsData destructor


/*!
Initializes a My_FormatsPanelANSIColorsUI structure.

(3.1)
*/
My_FormatsPanelANSIColorsUI::
My_FormatsPanelANSIColorsUI		(Panel_Ref		inPanel,
								 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_FormatsPanelANSIColorsUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_buttonCommandsHandler.isInstalled());
	assert(_containerResizer.isInstalled());
}// My_FormatsPanelANSIColorsUI 1-argument constructor


/*!
This routine is invoked whenever the color box value
is changed.  The panel responds by updating the color
preferences and updating all open session windows to
use the new color.

(3.1)
*/
void
My_FormatsPanelANSIColorsUI::
colorBoxChangeNotify	(HIViewRef			inColorBoxThatChanged,
						 RGBColor const*	inNewColor,
						 void*				inMyFormatsPanelANSIColorsUIPtr)
{
	My_FormatsPanelANSIColorsUIPtr		interfacePtr = REINTERPRET_CAST(inMyFormatsPanelANSIColorsUIPtr, My_FormatsPanelANSIColorsUIPtr);
	My_FormatsPanelANSIColorsDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																	My_FormatsPanelANSIColorsDataPtr);
	UInt32								colorID = 0;
	
	
	// command ID matches preferences tag
	if (noErr == GetControlCommandID(inColorBoxThatChanged, &colorID))
	{
		Boolean		isOK = false;
		
		
		if (nullptr == dataPtr->dataModel) isOK = false;
		else
		{
			HIWindowRef const		kOwningWindow = HIViewGetWindow(interfacePtr->mainView);
			Preferences_Tag const	kTag = STATIC_CAST(colorID, Preferences_Tag);
			Preferences_Result		prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kTag,
																				sizeof(*inNewColor), inNewColor);
			
			
			// update the inheritance checkbox
			// INCOMPLETE - for double-color cases, check to see if the other setting
			// is using the default, to determine whether to use “unchecked” or “mixed”
			switch (kTag)
			{
			case kPreferences_TagTerminalColorANSIBlack:
			case kPreferences_TagTerminalColorANSIBlackBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIBlackColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSIRed:
			case kPreferences_TagTerminalColorANSIRedBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIRedColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSIGreen:
			case kPreferences_TagTerminalColorANSIGreenBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIGreenColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSIYellow:
			case kPreferences_TagTerminalColorANSIYellowBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIYellowColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSIBlue:
			case kPreferences_TagTerminalColorANSIBlueBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIBlueColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSIMagenta:
			case kPreferences_TagTerminalColorANSIMagentaBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIMagentaColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSICyan:
			case kPreferences_TagTerminalColorANSICyanBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSICyanColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorANSIWhite:
			case kPreferences_TagTerminalColorANSIWhiteBold:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIWhiteColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			default:
				// ???
				break;
			}
			
			isOK = (kPreferences_ResultOK == prefsResult);
		}
		
		if (false == isOK) Sound_StandardAlert();
	}
}// My_FormatsPanelANSIColorsUI::colorBoxChangeNotify


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_FormatsPanelANSIColorsUI::
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
			(CFSTR("PrefPanelFormats"), CFSTR("ANSI"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// calculate the ideal size
	this->idealWidth = idealContainerBounds.right - idealContainerBounds.left;
	this->idealHeight = idealContainerBounds.bottom - idealContainerBounds.top;
	
	// augment the push buttons with the color box drawing handler, property and notifier;
	// TEMPORARY: in the future, this should probably be an HIView subclass of a button
	{
		HIViewID const		kButtonIDs[] =
							{
								idMyBevelButtonANSINormalBlack, idMyBevelButtonANSIBoldBlack,
								idMyBevelButtonANSINormalRed, idMyBevelButtonANSIBoldRed,
								idMyBevelButtonANSINormalGreen, idMyBevelButtonANSIBoldGreen,
								idMyBevelButtonANSINormalYellow, idMyBevelButtonANSIBoldYellow,
								idMyBevelButtonANSINormalBlue, idMyBevelButtonANSIBoldBlue,
								idMyBevelButtonANSINormalMagenta, idMyBevelButtonANSIBoldMagenta,
								idMyBevelButtonANSINormalCyan, idMyBevelButtonANSIBoldCyan,
								idMyBevelButtonANSINormalWhite, idMyBevelButtonANSIBoldWhite
							};
		
		
		for (size_t i = 0; i < sizeof(kButtonIDs) / sizeof(HIViewID); ++i)
		{
			HIViewWrap	button(kButtonIDs[i], inOwningWindow);
			
			
			assert(button.exists());
			ColorBox_AttachToBevelButton(button);
			ColorBox_SetColorChangeNotifyProc(button, My_FormatsPanelANSIColorsUI::colorBoxChangeNotify, this/* context */);
		}
	}
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	return result;
}// My_FormatsPanelANSIColorsUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_FormatsPanelANSIColorsUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const			kPanelWindow = HIViewGetWindow(inContainer);
	//My_FormatsPanelANSIColorsUI*	dataPtr = REINTERPRET_CAST(inContext, My_FormatsPanelANSIColorsUI*);
	
	HIViewWrap					viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextANSIColors, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_FormatsPanelANSIColorsUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_FormatsPanelANSIColorsUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorFormatsANSI, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelANSIColorsDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_FormatsPanelANSIColorsUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_FormatsPanelANSIColorsDataPtr));
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
			My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelANSIColorsDataPtr);
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
		result = kPanel_ResponseResizeNotNeeded;
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate view sizes
		{
			// this notification is currently ignored, but shouldn’t be...
		}
		break;
	
	case kPanel_MessageNewDataSet:
		{
			My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelANSIColorsDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::FORMAT);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext)
			{
				panelDataPtr->isDefaultModel = false;
				panelDataPtr->dataModel = defaultContext;
				panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
			}
			else
			{
				panelDataPtr->isDefaultModel = true;
			}
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
}// My_FormatsPanelANSIColorsUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// IMPORTANT: the tags read here should be in sync with those
		// returned by PrefPanelFormats_NewTagSet()
		this->readPreferencesForBlack(inSettings);
		this->readPreferencesForRed(inSettings);
		this->readPreferencesForGreen(inSettings);
		this->readPreferencesForYellow(inSettings);
		this->readPreferencesForBlue(inSettings);
		this->readPreferencesForMagenta(inSettings);
		this->readPreferencesForCyan(inSettings);
		this->readPreferencesForWhite(inSettings);
	}
}// My_FormatsPanelANSIColorsUI::readPreferences


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForBlack		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlack,
					HIViewWrap(idMyBevelButtonANSINormalBlack, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlackBold,
					HIViewWrap(idMyBevelButtonANSIBoldBlack, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIBlackColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForBlack


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForBlue		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlue,
					HIViewWrap(idMyBevelButtonANSINormalBlue, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlueBold,
					HIViewWrap(idMyBevelButtonANSIBoldBlue, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIBlueColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForBlue


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForCyan		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSICyan,
					HIViewWrap(idMyBevelButtonANSINormalCyan, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSICyanBold,
					HIViewWrap(idMyBevelButtonANSIBoldCyan, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSICyanColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForCyan


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForGreen		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIGreen,
					HIViewWrap(idMyBevelButtonANSINormalGreen, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIGreenBold,
					HIViewWrap(idMyBevelButtonANSIBoldGreen, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIGreenColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForGreen


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForMagenta	(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIMagenta,
					HIViewWrap(idMyBevelButtonANSINormalMagenta, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIMagentaBold,
					HIViewWrap(idMyBevelButtonANSIBoldMagenta, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIMagentaColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForMagenta


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForRed	(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIRed,
					HIViewWrap(idMyBevelButtonANSINormalRed, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIRedBold,
					HIViewWrap(idMyBevelButtonANSIBoldRed, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIRedColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForRed


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForWhite		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIWhite,
					HIViewWrap(idMyBevelButtonANSINormalWhite, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIWhiteBold,
					HIViewWrap(idMyBevelButtonANSIBoldWhite, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIWhiteColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForWhite


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferencesForYellow	(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIYellow,
					HIViewWrap(idMyBevelButtonANSINormalYellow, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIYellowBold,
					HIViewWrap(idMyBevelButtonANSIBoldYellow, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultANSIYellowColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
	}
}// My_FormatsPanelANSIColorsUI::readPreferencesForYellow


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
OSStatus
My_FormatsPanelANSIColorsUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyFormatsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_FormatsPanelANSIColorsUI*	interfacePtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelANSIColorsUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommandExtended	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if ((noErr == result) && (received.attributes & kHICommandFromControl))
		{
			HIViewRef	buttonHit = received.source.control;
			
			
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandResetANSIColors:
				// check with the user first!
				{
					AlertMessages_BoxRef	box = nullptr;
					UIStrings_Result		stringResult = kUIStrings_ResultOK;
					CFStringRef				dialogTextCFString = nullptr;
					CFStringRef				helpTextCFString = nullptr;
					
					
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowANSIColorsResetPrimaryText, dialogTextCFString);
					assert(stringResult.ok());
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowGenericCannotUndoHelpText, helpTextCFString);
					assert(stringResult.ok());
					
					box = Alert_NewWindowModal(HIViewGetWindow(interfacePtr->mainView), false/* is window close alert */,
												resetANSIWarningCloseNotifyProc, interfacePtr/* user data */);
					Alert_SetHelpButton(box, false);
					Alert_SetParamsFor(box, kAlert_StyleOKCancel);
					Alert_SetTextCFStrings(box, dialogTextCFString, helpTextCFString);
					Alert_SetType(box, kAlertCautionAlert);
					Alert_Display(box); // notifier disposes the alert when the sheet eventually closes
				}
				result = noErr; // event is handled
				break;
			
			case kCommandColorBlack:
			case kCommandColorBlackEmphasized:
			case kCommandColorRed:
			case kCommandColorRedEmphasized:
			case kCommandColorGreen:
			case kCommandColorGreenEmphasized:
			case kCommandColorYellow:
			case kCommandColorYellowEmphasized:
			case kCommandColorBlue:
			case kCommandColorBlueEmphasized:
			case kCommandColorMagenta:
			case kCommandColorMagentaEmphasized:
			case kCommandColorCyan:
			case kCommandColorCyanEmphasized:
			case kCommandColorWhite:
			case kCommandColorWhiteEmphasized:
				// see which of the color boxes was hit, display a color chooser
				// and then (if the user accepts a new color) update open windows
				ColorBox_UserSetColor(buttonHit);
				result = noErr; // event is handled
				break;
			
			case kCommandRestoreToDefault:
				// a preferences tag is encoded among the properties of the view
				// that sends this command
				assert(received.attributes & kHICommandFromControl);
				{
					HIViewRef const						kCheckBox = received.source.control;
					My_FormatsPanelANSIColorsDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																					My_FormatsPanelANSIColorsDataPtr);
					Preferences_ContextRef				targetContext = dataPtr->dataModel;
					Preferences_ContextRef				defaultContext = nullptr;
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					UInt32								actualSize = 0;
					Preferences_Tag						defaultPreferencesTag = '----';
					OSStatus							error = GetControlProperty
																(kCheckBox, AppResources_ReturnCreatorCode(),
																	kConstantsRegistry_ControlPropertyTypeDefaultPreferenceTag,
																	sizeof(defaultPreferencesTag), &actualSize,
																	&defaultPreferencesTag);
					
					
					assert_noerr(error);
					
					// since this destroys preferences under the assumption that the
					// default will fall through, make sure the current context is
					// not actually the default!
					{
						Quills::Prefs::Class const	kPrefsClass = Preferences_ContextReturnClass(targetContext);
						
						
						prefsResult = Preferences_GetDefaultContext(&defaultContext, kPrefsClass);
						assert(kPreferences_ResultOK == prefsResult);
					}
					
					if (defaultContext == targetContext)
					{
						// do nothing in this case, except to ensure that the
						// user cannot change the checkbox value
						if (kControlCheckBoxCheckedValue != GetControl32BitValue(kCheckBox))
						{
							interfacePtr->setInheritanceCheckBox(kCheckBox, kControlCheckBoxCheckedValue);
						}
					}
					else
					{
						result = noErr; // event is handled initially...
						
						// by convention, the property must match a known tag;
						// however, the tag only needs to refer to the “main”
						// preference in a section, not all preferences (for
						// example, both normal and bold ANSI black colors are
						// defaulted at the same time, but only the “normal”
						// tag is ever attached to a checkbox)
						switch (defaultPreferencesTag)
						{
						case kPreferences_TagTerminalColorANSIBlack:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIBlack);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIBlackBold);
							interfacePtr->readPreferencesForBlack(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSIRed:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIRed);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIRedBold);
							interfacePtr->readPreferencesForRed(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSIGreen:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIGreen);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIGreenBold);
							interfacePtr->readPreferencesForGreen(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSIYellow:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIYellow);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIYellowBold);
							interfacePtr->readPreferencesForYellow(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSIBlue:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIBlue);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIBlueBold);
							interfacePtr->readPreferencesForBlue(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSIMagenta:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIMagenta);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIMagentaBold);
							interfacePtr->readPreferencesForMagenta(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSICyan:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSICyan);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSICyanBold);
							interfacePtr->readPreferencesForCyan(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorANSIWhite:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIWhite);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorANSIWhiteBold);
							interfacePtr->readPreferencesForWhite(defaultContext);
							break;
						
						default:
							// ???
							result = eventNotHandledErr;
							break;
						}
					}
				}
				break;
			
			default:
				break;
			}
		}
		else
		{
			result = eventNotHandledErr;
		}
	}
	return result;
}// My_FormatsPanelANSIColorsUI::receiveHICommand


/*!
Calls the file-scope setInheritanceCheckBox(), automatically
setting the “is default” parameter to an appropriate value
based on the current data model.

(4.0)
*/
void
My_FormatsPanelANSIColorsUI::
setInheritanceCheckBox		(HIViewWrap		inCheckBox,
							 SInt32			inValue)
{
	My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(this->panel),
																		My_FormatsPanelANSIColorsDataPtr);
	
	
	::setInheritanceCheckBox(inCheckBox, inValue, panelDataPtr->isDefaultModel);
}// My_FormatsPanelANSIColorsUI::setInheritanceCheckBox


/*!
Initializes a My_FormatsPanelNormalData structure.

(3.1)
*/
My_FormatsPanelNormalData::
My_FormatsPanelNormalData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr),
isDefaultModel(false)
{
}// My_FormatsPanelNormalData default constructor


/*!
Tears down a My_FormatsPanelNormalData structure.

(3.1)
*/
My_FormatsPanelNormalData::
~My_FormatsPanelNormalData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_FormatsPanelNormalData destructor


/*!
Initializes a My_FormatsPanelNormalUI structure.

(3.1)
*/
My_FormatsPanelNormalUI::
My_FormatsPanelNormalUI		(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
terminalScreen			(createSampleTerminalScreen()),
terminalView			(TerminalView_NewHIViewBased(this->terminalScreen)),
terminalHIView			(setUpSampleTerminalHIView(this->terminalView, this->terminalScreen)),
sliderActionUPP			(NewControlActionUPP(sliderProc)),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_fontPanelHandler		(GetControlEventTarget(HIViewWrap(idMyButtonFontName, inOwningWindow)), receiveFontChange,
							CarbonEventSetInClass(CarbonEventClass(kEventClassFont), kEventFontPanelClosed, kEventFontSelection),
							this/* user data */),
_windowFocusHandler		(GetWindowEventTarget(inOwningWindow), receiveWindowFocusChange,
							CarbonEventSetInClass(CarbonEventClass(kEventClassWindow), kEventWindowFocusRelinquish),
							this/* user data */),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
									kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
							My_FormatsPanelNormalUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_buttonCommandsHandler.isInstalled());
	assert(_fontPanelHandler.isInstalled());
	assert(_windowFocusHandler.isInstalled());
	assert(_containerResizer.isInstalled());
	
	// this button is not used
	DeactivateControl(HIViewWrap(idMyBevelButtonMatteForeground, inOwningWindow));
}// My_FormatsPanelNormalUI 1-argument constructor


/*!
This routine is invoked whenever the color box value
is changed.  The panel responds by updating the color
preferences and updating all open session windows to
use the new color.

(3.1)
*/
void
My_FormatsPanelNormalUI::
colorBoxChangeNotify	(HIViewRef			inColorBoxThatChanged,
						 RGBColor const*	inNewColor,
						 void*				inMyFormatsPanelNormalUIPtr)
{
	My_FormatsPanelNormalUIPtr		interfacePtr = REINTERPRET_CAST(inMyFormatsPanelNormalUIPtr, My_FormatsPanelNormalUIPtr);
	My_FormatsPanelNormalDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																My_FormatsPanelNormalDataPtr);
	UInt32							colorID = 0;
	
	
	// command ID matches preferences tag
	if (noErr == GetControlCommandID(inColorBoxThatChanged, &colorID))
	{
		Boolean		isOK = false;
		
		
		if (nullptr == dataPtr->dataModel) isOK = false;
		else
		{
			HIWindowRef const		kOwningWindow = HIViewGetWindow(interfacePtr->mainView);
			Preferences_Tag const	kTag = STATIC_CAST(colorID, Preferences_Tag);
			Preferences_Result		prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kTag,
																				sizeof(*inNewColor), inNewColor);
			
			
			// update the sample area
			interfacePtr->setSampleArea(dataPtr->dataModel);
			
			// update the inheritance checkbox
			// INCOMPLETE - for double-color cases, check to see if the other setting
			// is using the default, to determine whether to use “unchecked” or “mixed”
			switch (kTag)
			{
			case kPreferences_TagTerminalColorNormalForeground:
			case kPreferences_TagTerminalColorNormalBackground:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultNormalColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorBoldForeground:
			case kPreferences_TagTerminalColorBoldBackground:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultBoldColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorBlinkingForeground:
			case kPreferences_TagTerminalColorBlinkingBackground:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultBlinkingColors, kOwningWindow),
														kControlCheckBoxMixedValue);
				break;
			
			case kPreferences_TagTerminalColorMatteBackground:
				interfacePtr->setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultMatteColors, kOwningWindow),
														kControlCheckBoxUncheckedValue);
				break;
			
			default:
				// ???
				break;
			}
			
			isOK = (kPreferences_ResultOK == prefsResult);
		}
		
		if (false == isOK) Sound_StandardAlert();
	}
}// My_FormatsPanelNormalUI::colorBoxChangeNotify


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_FormatsPanelNormalUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
{
	assert(nullptr != this->terminalHIView);
	
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	HIRect						sampleViewFrame;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	SetRect(&dummy, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	Panel_SetContainerView(inPanel, result);
	SetControlVisibility(result, false/* visible */, false/* draw */);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanelFormats"), CFSTR("Normal"), inOwningWindow,
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
	
	// augment the push buttons with the color box drawing handler, property and notifier;
	// TEMPORARY: in the future, this should probably be an HIView subclass of a button
	{
		HIViewID const		kButtonIDs[] =
							{
								idMyBevelButtonNormalText, idMyBevelButtonNormalBackground,
								idMyBevelButtonBoldText, idMyBevelButtonBoldBackground,
								idMyBevelButtonBlinkingText, idMyBevelButtonBlinkingBackground,
								idMyBevelButtonMatteBackground
							};
		
		
		for (size_t i = 0; i < sizeof(kButtonIDs) / sizeof(HIViewID); ++i)
		{
			HIViewWrap	button(kButtonIDs[i], inOwningWindow);
			
			
			assert(button.exists());
			ColorBox_AttachToBevelButton(button);
			ColorBox_SetColorChangeNotifyProc(button, My_FormatsPanelNormalUI::colorBoxChangeNotify, this/* context */);
		}
	}
	
	// install live tracking on the slider
	{
		HIViewWrap		slider(idMySliderFontCharacterWidth, inOwningWindow);
		
		
		error = SetControlProperty(slider, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeOwningPanel,
									sizeof(inPanel), &inPanel);
		if (noErr == error)
		{
			assert(nullptr != this->sliderActionUPP);
			SetControlAction(HIViewWrap(idMySliderFontCharacterWidth, inOwningWindow), this->sliderActionUPP);
		}
	}
	
	// find items of interest
	{
		std::vector< HIViewRef >::const_iterator	toView = viewList.end();
		
		
		// find the boundaries of the sample area; then throw it away
		toView = std::find_if(viewList.begin(), viewList.end(),
								DialogUtilities_HIViewIDEqualTo(idMyUserPaneSampleTerminalView));
		assert(viewList.end() != toView);
		error = HIViewGetFrame(*toView, &sampleViewFrame);
		assert_noerr(error);
		DisposeControl(*toView);
	}
	
	// use the original user pane only to define the boundaries of the new terminal view
	{
		error = HIViewAddSubview(result, this->terminalHIView);
		assert_noerr(error);
		error = HIViewSetFrame(this->terminalHIView, &sampleViewFrame);
		assert_noerr(error);
	}
	
	return result;
}// My_FormatsPanelNormalUI::createContainerView


/*!
Constructs the data storage and emulator for the
sample terminal screen view.

(3.1)
*/
TerminalScreenRef
My_FormatsPanelNormalUI::
createSampleTerminalScreen ()
const
{
	TerminalScreenRef			result = nullptr;
	Preferences_ContextWrap		terminalConfig(Preferences_NewContext(Quills::Prefs::TERMINAL), true/* is retained */);
	Preferences_ContextWrap		translationConfig(Preferences_NewContext(Quills::Prefs::TRANSLATION), true/* is retained */);
	Terminal_Result				screenCreationError = kTerminal_ResultOK;
	
	
	assert(terminalConfig.exists());
	if (terminalConfig.exists())
	{
		// set up the terminal
		Terminal_Emulator const		kTerminalType = kTerminal_EmulatorVT100;
		UInt16 const				kRowCount = 3;
		UInt32 const				kScrollbackSize = 0;
		Boolean const				kForceSave = false;
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSetData(terminalConfig.returnRef(), kPreferences_TagTerminalEmulatorType,
													sizeof(kTerminalType), &kTerminalType);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextSetData(terminalConfig.returnRef(), kPreferences_TagTerminalScreenRows,
													sizeof(kRowCount), &kRowCount);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextSetData(terminalConfig.returnRef(), kPreferences_TagTerminalScreenScrollbackRows,
													sizeof(kScrollbackSize), &kScrollbackSize);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextSetData(terminalConfig.returnRef(), kPreferences_TagTerminalClearSavesLines,
													sizeof(kForceSave), &kForceSave);
		assert(kPreferences_ResultOK == prefsResult);
	}
	assert(translationConfig.exists()); // just use defaults for this context
	
	// create the screen
	screenCreationError = Terminal_NewScreen(terminalConfig.returnRef(), translationConfig.returnRef(), &result);
	assert(kTerminal_ResultOK == screenCreationError);
	assert(nullptr != result);
	
	return result;
}// My_FormatsPanelNormalUI::createSampleTerminalScreen


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_FormatsPanelNormalUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		inDeltaY,
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_FormatsPanelNormalUI*	dataPtr = REINTERPRET_CAST(inContext, My_FormatsPanelNormalUI*);
	
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyButtonFontName, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyStaticTextNonMonospacedWarning, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyHelpTextCharacterWidth, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyUserPaneSampleTerminalView, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
	viewWrap = HIViewWrap(idMyHelpTextSampleTerminalView, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
}// My_FormatsPanelNormalUI::deltaSize


/*!
Deselects any font buttons that are active, in response to
a focus-lost event.

(3.1)
*/
void
My_FormatsPanelNormalUI::
loseFocus ()
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			fontNameButton(idMyButtonFontName, kOwningWindow);
	HIViewWrap			fontSizeButton(idMyButtonFontSize, kOwningWindow);
	
	
	SetControl32BitValue(fontNameButton, kControlCheckBoxUncheckedValue);
	SetControl32BitValue(fontSizeButton, kControlCheckBoxUncheckedValue);
}// My_FormatsPanelNormalUI::loseFocus


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_FormatsPanelNormalUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorFormatsNormal, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_FormatsPanelNormalDataPtr);
			WindowRef const*				windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_FormatsPanelNormalUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(inDataPtr, My_FormatsPanelNormalDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
			delete panelDataPtr;
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
			My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_FormatsPanelNormalDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_FormatsPanelNormalDataPtr);
			HISize&							newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != panelDataPtr->interfacePtr->idealWidth) && (0 != panelDataPtr->interfacePtr->idealHeight))
			{
				newLimits.width = panelDataPtr->interfacePtr->idealWidth;
				newLimits.height = panelDataPtr->interfacePtr->idealHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageGetUsefulResizeAxes: // request for panel to return the directions in which resizing makes sense
		result = kPanel_ResponseResizeBothAxes;
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate view sizes
		{
			// this notification is currently ignored, but shouldn’t be...
		}
		break;
	
	case kPanel_MessageNewDataSet:
		{
			My_FormatsPanelNormalDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelNormalDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::FORMAT);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext)
			{
				panelDataPtr->isDefaultModel = false;
				panelDataPtr->dataModel = defaultContext;
				panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
			}
			else
			{
				panelDataPtr->isDefaultModel = true;
			}
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
}// My_FormatsPanelNormalUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_FormatsPanelNormalUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// IMPORTANT: the tags read here should be in sync with those
		// returned by PrefPanelFormats_NewTagSet()
		
		// read font settings
		this->readPreferencesForFontName(inSettings, false/* update sample area */);
		this->readPreferencesForFontSize(inSettings, false/* update sample area */);
		this->readPreferencesForFontCharacterWidth(inSettings, false/* update sample area */);
		
		// read each color, skipping ones that are not defined
		this->readPreferencesForNormalColors(inSettings, false/* update sample area */);
		this->readPreferencesForBoldColors(inSettings, false/* update sample area */);
		this->readPreferencesForBlinkingColors(inSettings, false/* update sample area */);
		this->readPreferencesForMatteColors(inSettings, false/* update sample area */);
		
		// update the sample area
		this->setSampleArea(inSettings);
	}
}// My_FormatsPanelNormalUI::readPreferences


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForBlinkingColors	(Preferences_ContextRef		inSettings,
									 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorBlinkingForeground,
					HIViewWrap(idMyBevelButtonBlinkingText, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorBlinkingBackground,
					HIViewWrap(idMyBevelButtonBlinkingBackground, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultBlinkingColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagTerminalColorBlinkingForeground,
								kPreferences_TagTerminalColorBlinkingBackground);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForBlinkingColors


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForBoldColors	(Preferences_ContextRef		inSettings,
								 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorBoldForeground,
					HIViewWrap(idMyBevelButtonBoldText, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorBoldBackground,
					HIViewWrap(idMyBevelButtonBoldBackground, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultBoldColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagTerminalColorBoldForeground,
								kPreferences_TagTerminalColorBoldBackground);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForBoldColors


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForFontCharacterWidth	(Preferences_ContextRef		inSettings,
										 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		Float32				scaleFactor = 1.0;
		size_t				actualSize = 0;
		Boolean				isDefault = false;
		
		
		prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagFontCharacterWidthMultiplier,
													sizeof(scaleFactor), &scaleFactor, true/* search defaults too */,
													&actualSize, &isDefault);
		if (kPreferences_ResultOK == prefsResult)
		{
			this->setFontWidthScaleFactor(scaleFactor, isDefault);
		}
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagFontCharacterWidthMultiplier);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForFontCharacterWidth


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForFontName	(Preferences_ContextRef		inSettings,
							 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		CFStringRef			fontName = nullptr;
		size_t				actualSize = 0;
		Boolean				isDefault = false;
		
		
		prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagFontName, sizeof(fontName),
													&fontName, true/* search defaults too */, &actualSize,
													&isDefault);
		if (kPreferences_ResultOK == prefsResult)
		{
			this->setFontName(fontName, isDefault);
		}
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagFontName);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForFontName


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForFontSize	(Preferences_ContextRef		inSettings,
							 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		SInt16				fontSize = 0;
		size_t				actualSize = 0;
		Boolean				isDefault = false;
		
		
		prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagFontSize, sizeof(fontSize),
													&fontSize, true/* search defaults too */, &actualSize,
													&isDefault);
		if (kPreferences_ResultOK == prefsResult)
		{
			this->setFontSize(fontSize, isDefault);
		}
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagFontSize);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForFontSize


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForMatteColors	(Preferences_ContextRef		inSettings,
								 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorMatteBackground,
					HIViewWrap(idMyBevelButtonMatteBackground, kOwningWindow), isDefault);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultMatteColors, kOwningWindow),
								BooleanToCheckBoxValue(isDefault));
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagTerminalColorMatteBackground);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForMatteColors


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_FormatsPanelNormalUI::
readPreferencesForNormalColors	(Preferences_ContextRef		inSettings,
								 Boolean					inUpdateSample)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Boolean				isDefault = false;
		Boolean				isDefault2 = false;
		
		
		setColorBox(inSettings, kPreferences_TagTerminalColorNormalForeground,
					HIViewWrap(idMyBevelButtonNormalText, kOwningWindow), isDefault);
		setColorBox(inSettings, kPreferences_TagTerminalColorNormalBackground,
					HIViewWrap(idMyBevelButtonNormalBackground, kOwningWindow), isDefault2);
		setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultNormalColors, kOwningWindow),
								BooleansToCheckBoxValue(isDefault, isDefault2));
		
		if (inUpdateSample)
		{
			this->setSampleArea(inSettings, kPreferences_TagTerminalColorNormalForeground,
								kPreferences_TagTerminalColorNormalBackground);
		}
	}
}// My_FormatsPanelNormalUI::readPreferencesForNormalColors


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
OSStatus
My_FormatsPanelNormalUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyFormatsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_FormatsPanelNormalUI*	interfacePtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelNormalUI*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommandExtended	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if ((noErr == result) && (received.attributes & kHICommandFromControl))
		{
			HIViewRef	buttonHit = received.source.control;
			
			
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandEditFontAndSize:
				// select the button that was hit, and transmit font information
				{
					My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																					My_FormatsPanelNormalDataPtr);
					FontSelectionQDStyle			fontInfo;
					CFStringRef						fontName = nullptr;
					Boolean							releaseFontName = true;
					SInt16							fontSize = 0;
					size_t							actualSize = 0;
					Preferences_Result				prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextGetData(panelDataPtr->dataModel, kPreferences_TagFontName, sizeof(fontName),
																&fontName, false/* search defaults too */, &actualSize);
					if (kPreferences_ResultOK != prefsResult)
					{
						// error...pick an arbitrary value
						fontName = CFSTR("Monaco");
						releaseFontName = false;
					}
					prefsResult = Preferences_ContextGetData(panelDataPtr->dataModel, kPreferences_TagFontSize, sizeof(fontSize),
																&fontSize, false/* search defaults too */, &actualSize);
					if (kPreferences_ResultOK != prefsResult)
					{
						// error...pick an arbitrary value
						fontSize = 12;
					}
					
					bzero(&fontInfo, sizeof(fontInfo));
					fontInfo.version = kFontSelectionQDStyleVersionZero;
					{
						Str255		fontNamePascalString;
						
						
						if (CFStringGetPascalString(fontName, fontNamePascalString, sizeof(fontNamePascalString),
													kCFStringEncodingMacRoman))
						{
							fontInfo.instance.fontFamily = FMGetFontFamilyFromName(fontNamePascalString);
						}
					}
					fontInfo.instance.fontStyle = normal;
					fontInfo.size = fontSize;
					fontInfo.hasColor = false;
					// apparently this API can return paramErr even though it
					// successfully sets the desired font information...
					(OSStatus)SetFontInfoForSelection(kFontSelectionQDType, 1/* number of styles */, &fontInfo,
													// NOTE: This API is misdeclared in older headers, the last argument is supposed to
													// be an event target.  It is bastardized into HIObjectRef form for older compiles.
													#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
														GetControlEventTarget(buttonHit)
													#else
														REINTERPRET_CAST(buttonHit, HIObjectRef)
													#endif
													);
					if (1)
					{
						SetControl32BitValue(HIViewWrap(idMyButtonFontName, HIViewGetWindow(buttonHit)), kControlCheckBoxCheckedValue);
						SetControl32BitValue(HIViewWrap(idMyButtonFontSize, HIViewGetWindow(buttonHit)), kControlCheckBoxCheckedValue);
						if (false == FPIsFontPanelVisible())
						{
							result = FPShowHideFontPanel();
						}
					}
					
					if ((releaseFontName) && (nullptr != fontName))
					{
						CFRelease(fontName), fontName = nullptr;
					}
				}
				break;
			
			case kCommandColorMatteBackground:
			case kCommandColorBlinkingForeground:
			case kCommandColorBlinkingBackground:
			case kCommandColorBoldForeground:
			case kCommandColorBoldBackground:
			case kCommandColorNormalForeground:
			case kCommandColorNormalBackground:
				// see which of the color boxes was hit, display a color chooser
				// and then (if the user accepts a new color) update open windows
				ColorBox_UserSetColor(buttonHit);
				result = noErr; // event is handled
				break;
			
			case kCommandRestoreToDefault:
				// a preferences tag is encoded among the properties of the view
				// that sends this command
				assert(received.attributes & kHICommandFromControl);
				{
					HIViewRef const					kCheckBox = received.source.control;
					My_FormatsPanelNormalDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																				My_FormatsPanelNormalDataPtr);
					Preferences_ContextRef			targetContext = dataPtr->dataModel;
					Preferences_ContextRef			defaultContext = nullptr;
					Preferences_Result				prefsResult = kPreferences_ResultOK;
					UInt32							actualSize = 0;
					Preferences_Tag					defaultPreferencesTag = '----';
					OSStatus						error = GetControlProperty
															(kCheckBox, AppResources_ReturnCreatorCode(),
																kConstantsRegistry_ControlPropertyTypeDefaultPreferenceTag,
																sizeof(defaultPreferencesTag), &actualSize,
																&defaultPreferencesTag);
					
					
					assert_noerr(error);
					
					// since this destroys preferences under the assumption that the
					// default will fall through, make sure the current context is
					// not actually the default!
					{
						Quills::Prefs::Class const	kPrefsClass = Preferences_ContextReturnClass(targetContext);
						
						
						prefsResult = Preferences_GetDefaultContext(&defaultContext, kPrefsClass);
						assert(kPreferences_ResultOK == prefsResult);
					}
					
					if (defaultContext == targetContext)
					{
						// do nothing in this case, except to ensure that the
						// user cannot change the checkbox value
						if (kControlCheckBoxCheckedValue != GetControl32BitValue(kCheckBox))
						{
							interfacePtr->setInheritanceCheckBox(kCheckBox, kControlCheckBoxCheckedValue);
						}
					}
					else
					{
						result = noErr; // event is handled initially...
						
						// by convention, the property must match a known tag;
						// however, the tag only needs to refer to the “main”
						// preference in a section, not all preferences (for
						// example, both normal and bold ANSI black colors are
						// defaulted at the same time, but only the “normal”
						// tag is ever attached to a checkbox)
						switch (defaultPreferencesTag)
						{
						case kPreferences_TagFontName:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, defaultPreferencesTag);
							interfacePtr->readPreferencesForFontName(defaultContext);
							break;
						
						case kPreferences_TagFontSize:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, defaultPreferencesTag);
							interfacePtr->readPreferencesForFontSize(defaultContext);
							break;
						
						case kPreferences_TagFontCharacterWidthMultiplier:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, defaultPreferencesTag);
							interfacePtr->readPreferencesForFontCharacterWidth(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorNormalForeground:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorNormalForeground);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorNormalBackground);
							interfacePtr->readPreferencesForNormalColors(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorBoldForeground:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorBoldForeground);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorBoldBackground);
							interfacePtr->readPreferencesForBoldColors(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorBlinkingForeground:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorBlinkingForeground);
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, kPreferences_TagTerminalColorBlinkingBackground);
							interfacePtr->readPreferencesForBlinkingColors(defaultContext);
							break;
						
						case kPreferences_TagTerminalColorMatteBackground:
							(Preferences_Result)Preferences_ContextDeleteData
												(targetContext, defaultPreferencesTag);
							interfacePtr->readPreferencesForMatteColors(defaultContext);
							break;
						
						default:
							// ???
							result = eventNotHandledErr;
							break;
						}
					}
				}
				break;
			
			default:
				break;
			}
		}
		else
		{
			result = eventNotHandledErr;
		}
	}
	return result;
}// My_FormatsPanelNormalUI::receiveHICommand


/*!
Saves every setting to the data model that may change outside
of easily-detectable means (e.g. not buttons, but things like
sliders).

(4.0)
*/
void
My_FormatsPanelNormalUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings)
{
	if (nullptr != inoutSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set character width scaling
		{
			SInt32		scaleDiscreteValue = GetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow));
			Float32		equivalentScale = 1.0;
			
			
			// warning, this MUST be consistent with the NIB and setFontWidthScaleFactor()
			if (1 == scaleDiscreteValue) equivalentScale = 0.6;
			else if (2 == scaleDiscreteValue) equivalentScale = 0.7;
			else if (3 == scaleDiscreteValue) equivalentScale = 0.8;
			else if (4 == scaleDiscreteValue) equivalentScale = 0.9;
			else if (6 == scaleDiscreteValue) equivalentScale = 1.1;
			else if (7 == scaleDiscreteValue) equivalentScale = 1.2;
			else if (8 == scaleDiscreteValue) equivalentScale = 1.3;
			else if (9 == scaleDiscreteValue) equivalentScale = 1.4;
			else equivalentScale = 1.0;
			
			prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagFontCharacterWidthMultiplier,
														sizeof(equivalentScale), &equivalentScale);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "failed to set font character width multiplier");
			}
		}
	}
}// My_FormatsPanelNormalUI::saveFieldPreferences


/*!
Updates the font name display and its Default checkbox,
based on the given setting.

(3.1)
*/
void
My_FormatsPanelNormalUI::
setFontName		(CFStringRef	inFontName,
				 Boolean		inIsDefault)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControlTitleWithCFString(HIViewWrap(idMyButtonFontName, kOwningWindow), inFontName);
	HIViewSetVisible(HIViewWrap(idMyStaticTextNonMonospacedWarning, kOwningWindow), false == isMonospacedFont(inFontName));
	setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultFontName, kOwningWindow), BooleanToCheckBoxValue(inIsDefault));
}// My_FormatsPanelNormalUI::setFontName


/*!
Updates the font size display and its Default checkbox,
based on the given setting.

(3.1)
*/
void
My_FormatsPanelNormalUI::
setFontSize		(SInt16		inFontSize,
				 Boolean	inIsDefault)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControlNumericalTitle(HIViewWrap(idMyButtonFontSize, kOwningWindow), inFontSize);
	setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultFontSize, kOwningWindow), BooleanToCheckBoxValue(inIsDefault));
}// My_FormatsPanelNormalUI::setFontSize


/*!
Updates the font character width display and its Default
checkbox, based on the given setting.

If the value does not exactly match a slider tick, the
closest matching value will be chosen.

(4.0)
*/
void
My_FormatsPanelNormalUI::
setFontWidthScaleFactor		(Float32	inFontWidthScaleFactor,
							 Boolean	inIsDefault)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	Float32 const		kTolerance = 0.05;
	
	
	// floating point numbers are not exact values, they technically are
	// discrete at a very fine granularity; so do not use pure equality,
	// use a range with a small tolerance to choose a matching number
	if (inFontWidthScaleFactor < (0.6/* warning: must be consistent with NIB label and tick marks */ + kTolerance))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 1/* first slider value, 60% */);
	}
	else if ((inFontWidthScaleFactor >= (0.6/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (0.7/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 2/* next slider value */);
	}
	else if ((inFontWidthScaleFactor >= (0.7/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (0.8/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 3/* next slider value */);
	}
	else if ((inFontWidthScaleFactor >= (0.8/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (0.9/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 4/* next slider value */);
	}
	else if ((inFontWidthScaleFactor >= (0.9/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (1.0/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 5/* middle slider value */);
	}
	else if ((inFontWidthScaleFactor >= (1.0/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (1.1/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 6/* next slider value */);
	}
	else if ((inFontWidthScaleFactor >= (1.1/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (1.2/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 7/* next slider value */);
	}
	else if ((inFontWidthScaleFactor >= (1.2/* warning: must be consistent with NIB label */ + kTolerance)) &&
				(inFontWidthScaleFactor < (1.3/* warning: must be consistent with NIB label */ + kTolerance)))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 8/* next slider value */);
	}
	else if (inFontWidthScaleFactor >= (1.3/* warning: must be consistent with NIB label */ + kTolerance))
	{
		SetControl32BitValue(HIViewWrap(idMySliderFontCharacterWidth, kOwningWindow), 9/* last slider value, 140% */);
	}
	else
	{
		assert(false && "scale factor value should have triggered one of the preceding if/else cases");
	}
	setInheritanceCheckBox(HIViewWrap(idMyCheckBoxDefaultCharacterWidth, kOwningWindow),
							BooleanToCheckBoxValue(inIsDefault));
}// My_FormatsPanelNormalUI::setFontWidthScaleFactor


/*!
Calls the file-scope setInheritanceCheckBox(), automatically
setting the “is default” parameter to an appropriate value
based on the current data model.

(4.0)
*/
void
My_FormatsPanelNormalUI::
setInheritanceCheckBox		(HIViewWrap		inCheckBox,
							 SInt32			inValue)
{
	My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(this->panel),
																		My_FormatsPanelNormalDataPtr);
	
	
	::setInheritanceCheckBox(inCheckBox, inValue, panelDataPtr->isDefaultModel);
}// My_FormatsPanelNormalUI::setInheritanceCheckBox


/*!
Updates the sample terminal display using the settings in the
specified context.  All relevant settings are copied, unless
one or more valid restriction tags are given; when restricted,
only the settings indicated by the tags are used (and all other
parts of the display are unchanged).

(4.0)
*/
void
My_FormatsPanelNormalUI::
setSampleArea	(Preferences_ContextRef		inSettingsToCopy,
				 Preferences_Tag			inTagRestriction1,
				 Preferences_Tag			inTagRestriction2)
{
	Preferences_TagSetRef				tagSet = nullptr;
	std::vector< Preferences_Tag >		tags;
	
	
	if ('----' != inTagRestriction1) tags.push_back(inTagRestriction1);
	if ('----' != inTagRestriction2) tags.push_back(inTagRestriction2);
	if (false == tags.empty())
	{
		tagSet = Preferences_NewTagSet(tags);
		if (nullptr == tagSet)
		{
			Console_Warning(Console_WriteLine, "unable to create tag set, cannot update sample area");
		}
		else
		{
			Preferences_ContextCopy(inSettingsToCopy, TerminalView_ReturnFormatConfiguration(this->terminalView),
									tagSet);
			Preferences_ReleaseTagSet(&tagSet);
		}
	}
	else
	{
		Preferences_ContextCopy(inSettingsToCopy, TerminalView_ReturnFormatConfiguration(this->terminalView));
	}
}// My_FormatsPanelNormalUI::setSampleArea


/*!
Constructs the sample terminal screen HIView.

(3.1)
*/
HIViewRef
My_FormatsPanelNormalUI::
setUpSampleTerminalHIView	(TerminalViewRef	inTerminalView,
							 TerminalScreenRef	inTerminalScreen)
{
	HIViewRef	result = nullptr;
	OSStatus	error = noErr;
	
	
	result = TerminalView_ReturnContainerHIView(inTerminalView);
	assert(nullptr != result);
	
	// tag this view with an ID so it can be found easily later
	error = SetControlID(result, &idMyUserPaneSampleTerminalView);
	assert_noerr(error);
	
	// force the view into normal display mode, because zooming will
	// mess up the font size
	(TerminalView_Result)TerminalView_SetDisplayMode(inTerminalView, kTerminalView_DisplayModeNormal);
	
	// ignore changes to certain preferences for this sample view, since
	// it is not meant to be an ordinary terminal view
	(TerminalView_Result)TerminalView_IgnoreChangesToPreference(inTerminalView, kPreferences_TagTerminalResizeAffectsFontSize);
	
	// ignore user interaction, because text selections are not meant
	// to change
	(TerminalView_Result)TerminalView_SetUserInteractionEnabled(inTerminalView, false);
	
	// write some sample text to the view; NOTE that because of initial
	// size constraints, there may not be enough room to write more than
	// one line (if you need more than one line, defer to a later point)
	{
		// assumes VT100
		Terminal_EmulatorProcessCString(inTerminalScreen,
										"\033[2J\033[H"); // clear screen, home cursor
		Terminal_EmulatorProcessCString(inTerminalScreen,
										"sel find norm \033[1mbold\033[0m \033[5mblink\033[0m \033[3mital\033[0m \033[7minv\033[0m \033[4munder\033[0m"); // LOCALIZE THIS
		// the range selected here should be as long as the length of the word “sel” above
		TerminalView_SelectVirtualRange(inTerminalView, std::make_pair(std::make_pair(0, 0), std::make_pair(3, 1)/* exclusive end */));
		// the range selected here should be as long as the length of the word “find” above
		TerminalView_FindVirtualRange(inTerminalView, std::make_pair(std::make_pair(4, 0), std::make_pair(8, 1)/* exclusive end */));
	}
	
	return result;
}// My_FormatsPanelNormalUI::setUpSampleTerminalHIView


/*!
A standard control action routine for the slider; responds by
immediately saving the new value in the preferences context
and triggering updates to other views.

(4.0)
*/
void
My_FormatsPanelNormalUI::
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
		My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(panel),
																		My_FormatsPanelNormalDataPtr);
		
		
		panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
		{
			HIWindowRef const	kOwningWindow = HIViewGetWindow(panelDataPtr->interfacePtr->mainView);
			
			
			panelDataPtr->interfacePtr->setInheritanceCheckBox
										(HIViewWrap(idMyCheckBoxDefaultCharacterWidth, kOwningWindow),
											kControlCheckBoxUncheckedValue);
		}
		
		// update the sample area
		panelDataPtr->interfacePtr->setSampleArea(panelDataPtr->dataModel, kPreferences_TagFontCharacterWidthMultiplier);
	}
}// sliderProc


/*!
Copies a preference tag for a color from the specified
source context to the given destination.  All color
data has the same size, which is currently that of an
"RGBColor".

(4.0)
*/
Preferences_Result
copyColor	(Preferences_Tag			inSourceTag,
			 Preferences_ContextRef		inSource,
			 Preferences_ContextRef		inoutDestination,
			 Boolean					inSearchDefaults,
			 Boolean*					outIsDefaultOrNull)
{
	Preferences_Result	result = kPreferences_ResultOK;
	RGBColor			colorValue;
	size_t				actualSize = 0;
	
	
	result = Preferences_ContextGetData(inSource, inSourceTag, sizeof(colorValue), &colorValue,
										inSearchDefaults, &actualSize, outIsDefaultOrNull);
	if (kPreferences_ResultOK == result)
	{
		result = Preferences_ContextSetData(inoutDestination, inSourceTag, sizeof(colorValue), &colorValue);
	}
	return result;
}// copyColor


/*!
Determines if a font is monospaced.

DEPRECATED.  This will be removed when NSFont is used throughout.

(2.6)
*/
Boolean
isMonospacedFont	(CFStringRef	inFontName)
{
	Boolean		result = false;
	Boolean		doRomanTest = false;
	SInt32		numberOfScriptsEnabled = GetScriptManagerVariable(smEnabled);
	Str255		fontNamePascalString;
	
	
	(Boolean)CFStringGetPascalString(inFontName, fontNamePascalString, sizeof(fontNamePascalString),
										kCFStringEncodingMacRoman);
	
	if (numberOfScriptsEnabled > 1)
	{
		ScriptCode		scriptNumber = smRoman;
		FMFontFamily	fontID = FMGetFontFamilyFromName(fontNamePascalString);
		
		
		scriptNumber = FontToScript(fontID);
		if (scriptNumber != smRoman)
		{
			SInt32		thisScriptEnabled = GetScriptVariable(scriptNumber, smScriptEnabled);
			
			
			if (thisScriptEnabled)
			{
				// check if this font is the preferred monospaced font for its script
				SInt32		theSizeAndFontFamily = 0L;
				SInt16		thePreferredFontFamily = 0;
				
				
				theSizeAndFontFamily = GetScriptVariable(scriptNumber, smScriptMonoFondSize);
				thePreferredFontFamily = theSizeAndFontFamily >> 16; // high word is font family 
				result = (thePreferredFontFamily == fontID);
			}
			else result = false; // this font’s script isn’t enabled
		}
		else doRomanTest = true;
	}
	else doRomanTest = true;
		
	if (doRomanTest)
	{
		TextFontByName(fontNamePascalString);
		result = (CharWidth('W') == CharWidth('.'));
	}
	
	return result;
}// isMonospacedFont


/*!
Handles "kEventFontPanelClosed" and "kEventFontSelection" of
"kEventClassFont" for the font and size of the panel.

(3.1)
*/
OSStatus
receiveFontChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyFormatsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_FormatsPanelNormalUI*		normalDataPtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelNormalUI*);
	My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(normalDataPtr->panel),
																	My_FormatsPanelNormalDataPtr);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassFont);
	assert((kEventKind == kEventFontPanelClosed) || (kEventKind == kEventFontSelection));
	switch (kEventKind)
	{
	case kEventFontPanelClosed:
		// user has closed the panel; clear focus
		normalDataPtr->loseFocus();
		break;
	
	case kEventFontSelection:
		// user has accepted font changes in some way...update views
		// and internal preferences
		{
			Preferences_Result	prefsResult = kPreferences_ResultOK;
			FMFontFamily		fontFamily = kInvalidFontFamily;
			FMFontSize			fontSize = 0;
			OSStatus			error = noErr;
			
			
			result = noErr; // initially...
			
			// determine the font, if possible
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamFMFontFamily, typeFMFontFamily, fontFamily);
			if (noErr == error)
			{
				Str255		fontName;
				
				
				error = FMGetFontFamilyName(fontFamily, fontName);
				if (noErr != error) result = eventNotHandledErr;
				else
				{
					CFStringRef		fontNameCFString = CFStringCreateWithPascalString
														(kCFAllocatorDefault, fontName, kCFStringEncodingMacRoman);
					
					
					prefsResult = Preferences_ContextSetData(panelDataPtr->dataModel, kPreferences_TagFontName,
																sizeof(fontNameCFString), &fontNameCFString);
					if (kPreferences_ResultOK != prefsResult)
					{
						result = eventNotHandledErr;
					}
					else
					{
						// success!
						normalDataPtr->setFontName(fontNameCFString, false/* is default */);
					}
					
					if (nullptr != fontNameCFString)
					{
						CFRelease(fontNameCFString), fontNameCFString = nullptr;
					}
				}
			}
			
			// determine the font size, if possible
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamFMFontSize, typeFMFontSize, fontSize);
			if (noErr == error)
			{
				prefsResult = Preferences_ContextSetData(panelDataPtr->dataModel, kPreferences_TagFontSize,
															sizeof(fontSize), &fontSize);
				if (kPreferences_ResultOK != prefsResult)
				{
					result = eventNotHandledErr;
				}
				else
				{
					// success!
					normalDataPtr->setFontSize(fontSize, false/* is default */);
				}
			}
			
			// update the sample area
			panelDataPtr->interfacePtr->setSampleArea(panelDataPtr->dataModel);
			
			// TEMPORARY UGLY HACK: since the Cocoa font panel contains a slider with its
			// own event loop, it is possible for update routines to be invoked continuously
			// in such a way that the window is ERASED without an update; all attempts to
			// render the window in other ways have failed, but slightly RESIZING the window
			// performs a full redraw and is still fairly subtle (the user might notice a
			// slight shift on the right edge of the window, but this is still far better
			// than having the entire window turn white); it is not clear why this is
			// happening, other than the usual theory that Carbon and Cocoa just weren’t
			// meant to play nicely together and crap like this will never be truly fixed
			// until the user interface can migrate to Cocoa completely...
			{
				HIWindowRef		window = HIViewGetWindow(normalDataPtr->mainView);
				WindowClass		windowClass = kDocumentWindowClass;
				Rect			bounds;
				
				
				// for sheets, the PARENT window can sometimes be erased too...sigh
				error = GetWindowClass(window, &windowClass);
				if ((noErr == error) && (kSheetWindowClass == windowClass))
				{
					HIWindowRef		parentWindow;
					
					
					error = GetSheetWindowParent(window, &parentWindow);
					if (noErr == error)
					{
						(OSStatus)HIViewRender(HIViewGetRoot(parentWindow));
					}
				}
				
				// view rendering does not seem to work for the main window, so
				// use a resize hack to force a complete update
				error = GetWindowBounds(window, kWindowContentRgn, &bounds);
				if (noErr == error)
				{
					++bounds.right;
					(OSStatus)SetWindowBounds(window, kWindowContentRgn, &bounds);
					--bounds.right;
					(OSStatus)SetWindowBounds(window, kWindowContentRgn, &bounds);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// receiveFontChange


/*!
Embellishes "kEventWindowFocusRelinquish" of "kEventClassWindow"
for font buttons in this panel.

(3.1)
*/
OSStatus
receiveWindowFocusChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inMyFormatsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_FormatsPanelNormalUI*	normalDataPtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelNormalUI*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowFocusRelinquish);
	{
		HIWindowRef		windowLosingFocus = nullptr;
		
		
		// determine the window that is losing focus
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, windowLosingFocus);
		if (noErr == result)
		{
			normalDataPtr->loseFocus();
			
			// never completely handle this event
			result = eventNotHandledErr;
		}
		else
		{
			result = eventNotHandledErr;
		}
	}
	return result;
}// receiveWindowFocusChange


/*!
The responder to a closed “reset ANSI colors?” alert.
This routine resets the 16 displayed ANSI colors if
the item hit is the OK button, otherwise it does not
modify the displayed colors in any way.  The given
alert is destroyed.

(3.1)
*/
void
resetANSIWarningCloseNotifyProc		(InterfaceLibAlertRef	inAlertThatClosed,
									 SInt16					inItemHit,
									 void*					inMyFormatsPanelANSIColorsUIPtr)
{
	My_FormatsPanelANSIColorsUI*	interfacePtr = REINTERPRET_CAST(inMyFormatsPanelANSIColorsUIPtr,
																	My_FormatsPanelANSIColorsUI*);
	
	
	// if user consented to color reset, then change all colors to defaults
	if (kAlertStdAlertOKButton == inItemHit)
	{
		Preferences_ContextRef		defaultFormat = nullptr;
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_GetFactoryDefaultsContext(&defaultFormat);
		if (kPreferences_ResultOK == prefsResult)
		{
			My_FormatsPanelANSIColorsDataPtr	dataPtr = REINTERPRET_CAST
															(Panel_ReturnImplementation(interfacePtr->panel),
																My_FormatsPanelANSIColorsDataPtr);
			Preferences_ContextRef				targetContext = dataPtr->dataModel;
			
			
			// do not use Preferences_ContextCopy(), because the “factory defaults”
			// context contains more than just colors; copyColor() is called to
			// ensure that only color preferences are changed
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIBlack, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIBlackBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIRed, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIRedBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIGreen, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIGreenBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIYellow, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIYellowBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIBlue, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIBlueBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIMagenta, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIMagentaBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSICyan, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSICyanBold, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIWhite, defaultFormat, targetContext);
			prefsResult = copyColor(kPreferences_TagTerminalColorANSIWhiteBold, defaultFormat, targetContext);
			
			interfacePtr->readPreferences(defaultFormat);
		}
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// resetANSIWarningCloseNotifyProc


/*!
The responder to a closed “reset ANSI colors?” alert.
This routine resets the 16 displayed ANSI colors if
the item hit is the OK button, otherwise it does not
modify the displayed colors in any way.  The given
alert is destroyed.

(4.1)
*/
void
resetANSIWarningCloseNotifyProcCocoa	(AlertMessages_BoxRef	inAlertThatClosed,
										 SInt16					inItemHit,
										 void*					inStandardColorsViewManagerPtr)
{
	PrefPanelFormats_StandardColorsViewManager*		viewMgr = REINTERPRET_CAST
																(inStandardColorsViewManagerPtr,
																	PrefPanelFormats_StandardColorsViewManager*);
	
	
	// if user consented to color reset, then change all colors to defaults
	if (kAlertStdAlertOKButton == inItemHit)
	{
		BOOL	resetOK = [viewMgr resetToFactoryDefaultColors];
		
		
		if (NO == resetOK)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to reset all the standard colors to default values!");
		}
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// resetANSIWarningCloseNotifyProcCocoa


/*!
Updates the specified color box with the value (if any)
of the specified preference of type RGBColor.

The "outIsDefault" flag is set based on whether or not
the color box ended up with the default value; that is,
if the specified color did not exist in the specified
context.

(3.1)
*/
void
setColorBox		(Preferences_ContextRef		inSettings,
				 Preferences_Tag			inSourceTag,
				 HIViewRef					inDestinationBox,
				 Boolean&					outIsDefault)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	RGBColor				colorValue;
	size_t					actualSize = 0;
	
	
	// read each color, falling back to defaults for anything not defined
	prefsResult = Preferences_ContextGetData(inSettings, inSourceTag, sizeof(colorValue),
												&colorValue, true/* search defaults too */, &actualSize,
												&outIsDefault);
	if (kPreferences_ResultOK == prefsResult)
	{
		ColorBox_SetColor(inDestinationBox, &colorValue);
	}
}// setColorBox


/*!
Sets the value of a checkbox that represents an
inheritance.

Since inheritance is not reversed by clicking the
box, but by setting the associated preference to
a new value, the checkbox is disabled whenever
a “checked” value is given, and enabled otherwise.

When "inIsDefaultContext" is true, the value is
ignored, because all settings in the Default will
not have anything to inherit from.

(4.0)
*/
void
setInheritanceCheckBox		(HIViewWrap		inCheckBox,
							 SInt32			inValue,
							 Boolean		inIsDefaultContext)
{
	if (IsValidControlHandle(inCheckBox))
	{
		if (inIsDefaultContext)
		{
			SetControl32BitValue(inCheckBox, kControlCheckBoxUncheckedValue);
			inCheckBox << HIViewWrap_SetActiveState(false);
		}
		else
		{
			SetControl32BitValue(inCheckBox, inValue);
			inCheckBox << HIViewWrap_SetActiveState(kControlCheckBoxCheckedValue != inValue);
		}
	}
}// setInheritanceCheckBox

} // anonymous namespace


@implementation PrefPanelFormats_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	NSArray*	subViewManagers = [NSArray arrayWithObjects:
												[[[PrefPanelFormats_GeneralViewManager alloc] init] autorelease],
												[[[PrefPanelFormats_StandardColorsViewManager alloc] init] autorelease],
												nil];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Formats"
										localizedName:NSLocalizedStringFromTable(@"Formats", @"PrefPanelFormats",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelFormats"]
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


@end // PrefPanelFormats_ViewManager


@implementation PrefPanelFormats_CharacterWidthContent


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
	}
	return self;
}// initWithPreferencesTag:contextManager:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (Float32)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	Float32					result = 1.0;
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = [[self prefsMgr] currentContext];
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		size_t				actualSize = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, [self preferencesTag],
																		sizeof(result), &result,
																		true/* search defaults */, &actualSize,
																		&isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			result = 1.0; // assume a value if nothing is found
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (YES == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSNumber*)
numberValue
{
	BOOL		isDefault = NO;
	NSNumber*	result = [NSNumber numberWithFloat:[self readValueSeeIfDefault:&isDefault]];
	
	
	return result;
}
- (void)
setNumberValue:(NSNumber*)		aNumber
{
	[self willSetPreferenceValue];
	
	if (nil == aNumber)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [[self prefsMgr] deleteDataForPreferenceTag:[self preferencesTag]];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteLine, "failed to remove character width preference");
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = [[self prefsMgr] currentContext];
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			Float32				scaleFactor = [aNumber floatValue];
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, [self preferencesTag],
																			sizeof(scaleFactor), &scaleFactor);
			
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save character width preference");
		}
	}
	
	[self didSetPreferenceValue];
}// setNumberValue:


#pragma mark PreferenceValue_InheritedSingleTag


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	(Float32)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setNumberValue:nil];
}// setNilPreferenceValue


@end // PrefPanelFormats_CharacterWidthContent


@implementation PrefPanelFormats_GeneralViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelFormatGeneralCocoa" delegate:self context:nullptr];
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
	// unregister observers that were used to update the sample area
	{
		NSEnumerator*	eachKey = [[self sampleDisplayBindingKeyPaths] objectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self removeObserver:self forKeyPath:keyName];
		}
	}
	
	[prefsMgr release];
	[byKey release];
	if (nullptr != sampleScreenBuffer)
	{
		Terminal_DisposeScreen(sampleScreenBuffer);
	}
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
normalBackgroundColor
{
	return [self->byKey objectForKey:@"normalBackgroundColor"];
}// normalBackgroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
normalForegroundColor
{
	return [self->byKey objectForKey:@"normalForegroundColor"];
}// normalForegroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
boldBackgroundColor
{
	return [self->byKey objectForKey:@"boldBackgroundColor"];
}// boldBackgroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
boldForegroundColor
{
	return [self->byKey objectForKey:@"boldForegroundColor"];
}// boldForegroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
blinkingBackgroundColor
{
	return [self->byKey objectForKey:@"blinkingBackgroundColor"];
}// blinkingBackgroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
blinkingForegroundColor
{
	return [self->byKey objectForKey:@"blinkingForegroundColor"];
}// blinkingForegroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
matteBackgroundColor
{
	return [self->byKey objectForKey:@"matteBackgroundColor"];
}// matteBackgroundColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
fontFamily
{
	return [self->byKey objectForKey:@"fontFamily"];
}// fontFamily


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
fontSize
{
	return [self->byKey objectForKey:@"fontSize"];
}// fontSize


/*!
Accessor.

(4.1)
*/
- (PrefPanelFormats_CharacterWidthContent*)
characterWidth
{
	return [self->byKey objectForKey:@"characterWidth"];
}// characterWidth


#pragma mark Actions


/*!
Displays a font panel asking the user to set the font family
and font size to use for general formatting.

(4.1)
*/
- (IBAction)
performFontSelection:(id)	sender
{
#pragma unused(sender)
	[[NSFontPanel sharedFontPanel] makeKeyAndOrderFront:sender];
}// performFontSelection:


#pragma mark NSColorPanel


/*!
Responds to user selections in the system’s Colors panel.
Due to Cocoa Bindings, no action is necessary; color changes
simply trigger the apppropriate calls to color-setting
methods.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
#pragma unused(sender)
	// no action is necessary due to Cocoa Bindings
}// changeColor:


#pragma mark NSFontPanel


/*!
Updates the “font family” and “font size” preferences based on
the user’s selection in the system’s Fonts panel.

(4.1)
*/
- (void)
changeFont:(id)	sender
{
	NSFont*		oldFont = [NSFont fontWithName:[[self fontFamily] stringValue]
												size:[[[self fontSize] numberStringValue] floatValue]];
	NSFont*		newFont = [sender convertFont:oldFont];
	
	
	[[self fontFamily] setStringValue:[newFont familyName]];
	[[self fontSize] setNumberStringValue:[[NSNumber numberWithFloat:[newFont pointSize]] stringValue]];
}// changeFont:


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to bound values by updating the sample
terminal view.

(4.1)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
#pragma unused(anObject, aContext)
	if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
	{
		// TEMPORARY; refresh the terminal view for any change, without restrictions
		// (should fix to send only relevant preference tags, based on the key path)
		[self setSampleAreaFromDefaultPreferences]; // since a context may not have everything, make sure the rest uses Default values
		[self setSampleAreaFromPreferences:[self->prefsMgr currentContext]
											restrictedTag1:'----' restrictedTag2:'----'];
	}
}// observeValueForKeyPath:ofObject:change:context:


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
	self->prefsMgr = [[PrefsContextManager_Object alloc] init];
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:16/* arbitrary; number of colors */];
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
	*outEditType = kPanel_EditTypeInspector;
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
	assert(nil != terminalSampleBackgroundView);
	assert(nil != terminalSamplePaddingView);
	assert(nil != terminalSampleContentView);
	
	
	// remember frame from XIB (it might be changed later)
	self->idealFrame = [aContainerView frame];
	
	{
		Preferences_ContextWrap		terminalConfig(Preferences_NewContext
													(Quills::Prefs::TERMINAL), true/* is retained */);
		Preferences_ContextWrap		translationConfig(Preferences_NewContext
														(Quills::Prefs::TRANSLATION), true/* is retained */);
		Terminal_Result				bufferResult = Terminal_NewScreen(terminalConfig.returnRef(),
																		translationConfig.returnRef(),
																		&self->sampleScreenBuffer);
		
		
		if (kTerminal_ResultOK != bufferResult)
		{
			Console_Warning(Console_WriteValue, "failed to create sample terminal screen buffer, error", bufferResult);
		}
		else
		{
			self->sampleScreenView = TerminalView_NewNSViewBased(self->terminalSampleContentView,
																	self->terminalSamplePaddingView,
																	self->terminalSampleBackgroundView,
																	sampleScreenBuffer, nullptr/* format */);
			if (nullptr == self->sampleScreenView)
			{
				Console_WriteLine("failed to create sample terminal view");
			}
			else
			{
				// write some text in various styles to the screen (happens to be a
				// copy of what the sample view does); this will help with testing
				// the new Cocoa-based renderer as it is implemented
				Terminal_EmulatorProcessCString(self->sampleScreenBuffer,
												"\033[2J\033[H"); // clear screen, home cursor (assumes VT100)
				Terminal_EmulatorProcessCString(self->sampleScreenBuffer,
												"sel find norm \033[1mbold\033[0m \033[5mblink\033[0m \033[3mital\033[0m \033[7minv\033[0m \033[4munder\033[0m");
				// the range selected here should be as long as the length of the word “sel” above
				TerminalView_SelectVirtualRange
				(self->sampleScreenView, std::make_pair(std::make_pair(0, 0),
														std::make_pair(3, 1)/* exclusive end */));
				// the range selected here should be as long as the length of the word “find” above
				TerminalView_FindVirtualRange
				(self->sampleScreenView, std::make_pair(std::make_pair(4, 0),
														std::make_pair(8, 1)/* exclusive end */));
			}
		}
	}
	
	// observe all properties that can affect the sample display area
	{
		NSEnumerator*	eachKey = [[self sampleDisplayBindingKeyPaths] objectEnumerator];
		
		
		while (NSString* keyPath = [eachKey nextObject])
		{
			[self addObserver:self forKeyPath:keyPath options:0 context:nullptr];
		}
	}
	
	// note that all current values will change
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] objectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self willChangeValueForKey:keyName];
		}
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorNormalBackground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"normalBackgroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorNormalForeground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"normalForegroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorBoldBackground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"boldBackgroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorBoldForeground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"boldForegroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorBlinkingBackground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"blinkingBackgroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorBlinkingForeground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"blinkingForegroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorMatteBackground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"matteBackgroundColor"];
	[self->byKey setObject:[[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagFontName
														contextManager:self->prefsMgr] autorelease]
					forKey:@"fontFamily"];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagFontSize
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16] autorelease]
					forKey:@"fontSize"];
	[self->byKey setObject:[[[PrefPanelFormats_CharacterWidthContent alloc]
								initWithPreferencesTag:kPreferences_TagFontCharacterWidthMultiplier
														contextManager:self->prefsMgr] autorelease]
					forKey:@"characterWidth"];
	
	// note that all values have changed (causes the display to be refreshed)
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] reverseObjectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self didChangeValueForKey:keyName];
		}
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
	*outIdealSize = self->idealFrame.size;
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
	(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
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

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	// note that all current values will change
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] objectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self willChangeValueForKey:keyName];
		}
	}
	
	// now apply the specified settings
	[self->prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// note that all values have changed (causes the display to be refreshed)
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] reverseObjectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self didChangeValueForKey:keyName];
		}
	}
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
	return [NSImage imageNamed:@"IconForPrefPanelFormats"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Formats.General";
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
	return NSLocalizedStringFromTable(@"General", @"PrefPanelFormats", @"the name of this panel");
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
	return kPanel_ResizeConstraintBothAxes;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::FORMAT;
}// preferencesClass


@end // PrefPanelFormats_GeneralViewManager


@implementation PrefPanelFormats_GeneralViewManager (PrefPanelFormats_GeneralViewManagerInternal)


/*!
Returns the names of key-value coding keys that have color values.

(4.1)
*/
- (NSArray*)
colorBindingKeys
{
	return [NSArray arrayWithObjects:
						@"normalBackgroundColor", @"normalForegroundColor",
						@"boldBackgroundColor", @"boldForegroundColor",
						@"blinkingBackgroundColor", @"blinkingForegroundColor",
						@"matteBackgroundColor",
						nil];
}// colorBindingKeys


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	NSMutableArray*		result = [NSMutableArray arrayWithArray:[self colorBindingKeys]];
	
	
	[result addObject:@"fontFamily"];
	[result addObject:@"fontSize"];
	[result addObject:@"characterWidth"];
	
	return result;
}// primaryDisplayBindingKeys


/*!
Returns the key paths of all settings that, if changed, would
require the sample terminal to be refreshed.

(4.1)
*/
- (NSArray*)
sampleDisplayBindingKeyPaths
{
	NSMutableArray*		result = [NSMutableArray arrayWithCapacity:20/* arbitrary */];
	NSEnumerator*		eachKey = nil;
	
	
	eachKey = [[self primaryDisplayBindingKeys] objectEnumerator];
	while (NSString* keyName = [eachKey nextObject])
	{
		[result addObject:[NSString stringWithFormat:@"%@.inherited", keyName]];
		[result addObject:[NSString stringWithFormat:@"%@.inheritEnabled", keyName]];
	}
	
	eachKey = [[self colorBindingKeys] objectEnumerator];
	while (NSString* keyName = [eachKey nextObject])
	{
		[result addObject:[NSString stringWithFormat:@"%@.colorValue", keyName]];
	}
	
	[result addObject:@"fontFamily.stringValue"];
	[result addObject:@"fontSize.stringValue"];
	[result addObject:@"characterWidth.floatValue"];
	
	return result;
}// sampleDisplayBindingKeyPaths


/*!
Updates the sample terminal display using Default settings.
This is occasionally necessary as a reset step prior to
adding new settings, because a context does not have to
define every value (the display should fall back on the
defaults, instead of whatever setting happened to be in
effect beforehand).

(4.1)
*/
- (void)
setSampleAreaFromDefaultPreferences
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef	defaultContext = nullptr;
	
	
	prefsResult = Preferences_GetDefaultContext(&defaultContext, [self preferencesClass]);
	if (kPreferences_ResultOK == prefsResult)
	{
		[self setSampleAreaFromPreferences:defaultContext restrictedTag1:'----' restrictedTag2:'----'];
	}
}// setSampleAreaFromDefaultPreferences


/*!
Updates the sample terminal display using the settings in the
specified context.  All relevant settings are copied, unless
one or more valid restriction tags are given; when restricted,
only the settings indicated by the tags are used (and all other
parts of the display are unchanged).

(4.1)
*/
- (void)
setSampleAreaFromPreferences:(Preferences_ContextRef)	inSettingsToCopy
restrictedTag1:(Preferences_Tag)						inTagRestriction1
restrictedTag2:(Preferences_Tag)						inTagRestriction2
{
	Preferences_TagSetRef				tagSet = nullptr;
	std::vector< Preferences_Tag >		tags;
	
	
	if ('----' != inTagRestriction1) tags.push_back(inTagRestriction1);
	if ('----' != inTagRestriction2) tags.push_back(inTagRestriction2);
	if (false == tags.empty())
	{
		tagSet = Preferences_NewTagSet(tags);
		if (nullptr == tagSet)
		{
			Console_Warning(Console_WriteLine, "unable to create tag set, cannot update sample area");
		}
		else
		{
			Preferences_ContextCopy(inSettingsToCopy, TerminalView_ReturnFormatConfiguration(self->sampleScreenView),
									tagSet);
			Preferences_ReleaseTagSet(&tagSet);
		}
	}
	else
	{
		Preferences_ContextCopy(inSettingsToCopy, TerminalView_ReturnFormatConfiguration(self->sampleScreenView));
	}
}// setSampleAreaFromPreferences:restrictedTag1:restrictedTag2:


@end // PrefPanelFormats_GeneralViewManager (PrefPanelFormats_GeneralViewManagerInternal)


@implementation PrefPanelFormats_StandardColorsViewManager


/*!
Designated initializer.

(4.1)
*/
- (id)
init
{
	self = [super initWithNibNamed:@"PrefPanelFormatStandardColorsCocoa" delegate:self context:nullptr];
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
- (PreferenceValue_Color*)
blackBoldColor
{
	return [self->byKey objectForKey:@"blackBoldColor"];
}// blackBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
blackNormalColor
{
	return [self->byKey objectForKey:@"blackNormalColor"];
}// blackNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
redBoldColor
{
	return [self->byKey objectForKey:@"redBoldColor"];
}// redBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
redNormalColor
{
	return [self->byKey objectForKey:@"redNormalColor"];
}// redNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
greenBoldColor
{
	return [self->byKey objectForKey:@"greenBoldColor"];
}// greenBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
greenNormalColor
{
	return [self->byKey objectForKey:@"greenNormalColor"];
}// greenNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
yellowBoldColor
{
	return [self->byKey objectForKey:@"yellowBoldColor"];
}// yellowBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
yellowNormalColor
{
	return [self->byKey objectForKey:@"yellowNormalColor"];
}// yellowNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
blueBoldColor
{
	return [self->byKey objectForKey:@"blueBoldColor"];
}// blueBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
blueNormalColor
{
	return [self->byKey objectForKey:@"blueNormalColor"];
}// blueNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
magentaBoldColor
{
	return [self->byKey objectForKey:@"magentaBoldColor"];
}// magentaBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
magentaNormalColor
{
	return [self->byKey objectForKey:@"magentaNormalColor"];
}// magentaNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
cyanBoldColor
{
	return [self->byKey objectForKey:@"cyanBoldColor"];
}// cyanBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
cyanNormalColor
{
	return [self->byKey objectForKey:@"cyanNormalColor"];
}// cyanNormalColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
whiteBoldColor
{
	return [self->byKey objectForKey:@"whiteBoldColor"];
}// whiteBoldColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
whiteNormalColor
{
	return [self->byKey objectForKey:@"whiteNormalColor"];
}// whiteNormalColor


#pragma mark Actions


/*!
Displays a warning asking the user to confirm a reset
of all 16 ANSI colors to their “factory defaults”, and
performs the reset if the user allows it.

(4.1)
*/
- (IBAction)
performResetStandardColors:(id)		sender
{
#pragma unused(sender)
	// check with the user first!
	AlertMessages_BoxRef	box = nullptr;
	UIStrings_Result		stringResult = kUIStrings_ResultOK;
	CFStringRef				dialogTextCFString = nullptr;
	CFStringRef				helpTextCFString = nullptr;
	
	
	stringResult = UIStrings_Copy(kUIStrings_AlertWindowANSIColorsResetPrimaryText, dialogTextCFString);
	assert(stringResult.ok());
	stringResult = UIStrings_Copy(kUIStrings_AlertWindowGenericCannotUndoHelpText, helpTextCFString);
	assert(stringResult.ok());
	
	box = Alert_NewWindowModal([[self managedView] window], false/* is window close alert */,
								resetANSIWarningCloseNotifyProcCocoa, self/* user data */);
	Alert_SetHelpButton(box, false);
	Alert_SetParamsFor(box, kAlert_StyleOKCancel);
	Alert_SetTextCFStrings(box, dialogTextCFString, helpTextCFString);
	Alert_SetType(box, kAlertCautionAlert);
	Alert_Display(box); // notifier disposes the alert when the sheet eventually closes
}// performResetStandardColors:


#pragma mark NSColorPanel


/*!
Responds to user selections in the system’s Colors panel.
Due to Cocoa Bindings, no action is necessary; color changes
simply trigger the apppropriate calls to color-setting
methods.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
#pragma unused(sender)
	// no action is necessary due to Cocoa Bindings
}// changeColor:


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
	self->prefsMgr = [[PrefsContextManager_Object alloc] init];
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:16/* arbitrary; number of colors */];
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
	*outEditType = kPanel_EditTypeInspector;
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
	
	
	// remember frame from XIB (it might be changed later)
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] objectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self willChangeValueForKey:keyName];
		}
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIBlack
														contextManager:self->prefsMgr] autorelease]
					forKey:@"blackNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIBlackBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"blackBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIRed
														contextManager:self->prefsMgr] autorelease]
					forKey:@"redNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIRedBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"redBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIGreen
														contextManager:self->prefsMgr] autorelease]
					forKey:@"greenNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIGreenBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"greenBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIYellow
														contextManager:self->prefsMgr] autorelease]
					forKey:@"yellowNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIYellowBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"yellowBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIBlue
														contextManager:self->prefsMgr] autorelease]
					forKey:@"blueNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIBlueBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"blueBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIMagenta
														contextManager:self->prefsMgr] autorelease]
					forKey:@"magentaNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIMagentaBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"magentaBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSICyan
														contextManager:self->prefsMgr] autorelease]
					forKey:@"cyanNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSICyanBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"cyanBoldColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIWhite
														contextManager:self->prefsMgr] autorelease]
					forKey:@"whiteNormalColor"];
	[self->byKey setObject:[[[PreferenceValue_Color alloc]
								initWithPreferencesTag:kPreferences_TagTerminalColorANSIWhiteBold
														contextManager:self->prefsMgr] autorelease]
					forKey:@"whiteBoldColor"];
	
	// note that all values have changed (causes the display to be refreshed)
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] reverseObjectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self didChangeValueForKey:keyName];
		}
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
	*outIdealSize = self->idealFrame.size;
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
	(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
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

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	// note that all current values will change
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] objectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self willChangeValueForKey:keyName];
		}
	}
	
	[self->prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// note that all values have changed (causes the display to be refreshed)
	{
		NSEnumerator*	eachKey = [[self primaryDisplayBindingKeys] reverseObjectEnumerator];
		
		
		while (NSString* keyName = [eachKey nextObject])
		{
			[self didChangeValueForKey:keyName];
		}
	}
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
	return [NSImage imageNamed:@"IconForPrefPanelFormats"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Formats.StandardColors";
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
	return NSLocalizedStringFromTable(@"Standard Colors", @"PrefPanelFormats", @"the name of this panel");
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
	return Quills::Prefs::FORMAT;
}// preferencesClass


@end // PrefPanelFormats_StandardColorsViewManager


@implementation PrefPanelFormats_StandardColorsViewManager (PrefPanelFormats_StandardColorsViewManagerInternal)


/*!
Copies a color from the specified source context to the
current context.

Since color properties are significant to key-value coding,
the specified key name is used to automatically call
"willChangeValueForKey:" before the change is made and
"didChangeValueForKey:" after the change has been made.

This is expected to be called repeatedly for various colors
so "aFlagPtr" is used to note any failures if any occur (if
there is no failure the value DOES NOT CHANGE).  This allows
you to initialize a flag to NO, call this routine to copy
various colors, and then check once at the end to see if
any color failed to copy.

(4.1)
*/
- (void)
copyColorWithPreferenceTag:(Preferences_Tag)	aTag
fromContext:(Preferences_ContextRef)			aSourceContext
forKey:(NSString*)								aKeyName
failureFlag:(BOOL*)								aFlagPtr
{
	Preferences_ContextRef	targetContext = [self->prefsMgr currentContext];
	BOOL					failed = YES;
	
	
	if (Preferences_ContextIsValid(targetContext))
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		[self willChangeValueForKey:aKeyName];
		prefsResult = copyColor(aTag, aSourceContext, targetContext);
		[self didChangeValueForKey:aKeyName];
		if (kPreferences_ResultOK == prefsResult)
		{
			failed = NO;
		}
	}
	
	// the flag is only set on failure; this allows a sequence of calls to
	// occur where only a single check at the end is done to trap any issue
	if (failed)
	{
		*aFlagPtr = YES;
	}
}// copyColorWithPreferenceTag:fromContext:forKey:failureFlag:


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return [NSArray arrayWithObjects:
						@"blackNormalColor", @"blackBoldColor",
						@"blackNormalColorInherited", @"blackBoldColorInherited",
						@"blackNormalColorInheritEnabled", @"blackBoldColorInheritEnabled",
						@"redNormalColor", @"redBoldColor",
						@"greenNormalColor", @"greenBoldColor",
						@"yellowNormalColor", @"yellowBoldColor",
						@"blueNormalColor", @"blueBoldColor",
						@"magentaNormalColor", @"magentaBoldColor",
						@"cyanNormalColor", @"cyanBoldColor",
						@"whiteNormalColor", @"whiteBoldColor",
						nil];
}// primaryDisplayBindingKeys


/*!
Using the “factory default” colors, replaces all of the standard
ANSI color settings in the currently-selected preferences context.

IMPORTANT:	This is a low-level routine that performs the reset
			without warning.  It is called AFTER displaying a
			confirmation alert to the user!

(4.1)
*/
- (BOOL)
resetToFactoryDefaultColors
{
	BOOL					result = NO;
	Preferences_ContextRef	defaultFormat = nullptr;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_GetFactoryDefaultsContext(&defaultFormat);
	if (kPreferences_ResultOK == prefsResult)
	{
		Preferences_Tag		currentTag = '----';
		NSString*			currentKey = @"";
		BOOL				anyFailure = NO;
		
		
		currentTag = kPreferences_TagTerminalColorANSIBlack;
		currentKey = @"blackNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIBlackBold;
		currentKey = @"blackBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIRed;
		currentKey = @"redNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIRedBold;
		currentKey = @"redBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIGreen;
		currentKey = @"greenNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIGreenBold;
		currentKey = @"greenBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIYellow;
		currentKey = @"yellowNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIYellowBold;
		currentKey = @"yellowBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIBlue;
		currentKey = @"blueNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIBlueBold;
		currentKey = @"blueBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIMagenta;
		currentKey = @"magentaNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIMagentaBold;
		currentKey = @"magentaBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSICyan;
		currentKey = @"cyanNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSICyanBold;
		currentKey = @"cyanBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIWhite;
		currentKey = @"whiteNormalColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		currentTag = kPreferences_TagTerminalColorANSIWhiteBold;
		currentKey = @"whiteBoldColor";
		[self copyColorWithPreferenceTag:currentTag fromContext:defaultFormat forKey:currentKey failureFlag:&anyFailure];
		
		if (NO == anyFailure)
		{
			result = YES;
		}
	}
	return result;
}// resetToFactoryDefaultColors


@end // PrefPanelFormats_StandardColorsViewManager (PrefPanelFormats_StandardColorsViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
