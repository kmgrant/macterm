/*!	\file PrefPanelFormats.mm
	\brief Implements the Formats panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CocoaExtensions.objc++.h>
#import <ColorUtilities.h>
#import <Console.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "PreferenceValue.objc++.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "UIStrings.h"



#pragma mark Internal Method Prototypes
namespace {

Preferences_Result		copyColor	(Preferences_Tag, Preferences_ContextRef, Preferences_ContextRef,
									 Boolean = false, Boolean* = nullptr);

} // anonymous namespace

/*!
The private class interface.
*/
@interface PrefPanelFormats_GeneralViewManager (PrefPanelFormats_GeneralViewManagerInternal) //{

	- (NSArray*)
	colorBindingKeys;
	- (NSArray*)
	primaryDisplayBindingKeys;
	- (NSArray*)
	sampleDisplayBindingKeyPaths;
	- (void)
	setSampleAreaFromDefaultPreferences;
	- (void)
	setSampleAreaFromPreferences:(Preferences_ContextRef)_;
	- (void)
	setSampleAreaFromPreferences:(Preferences_ContextRef)_
	restrictedTag1:(Preferences_Tag)_
	restrictedTag2:(Preferences_Tag)_;

@end //}

/*!
The private class interface.
*/
@interface PrefPanelFormats_StandardColorsViewManager (PrefPanelFormats_StandardColorsViewManagerInternal) //{

	- (NSArray*)
	colorBindingKeys;
	- (void)
	copyColorWithPreferenceTag:(Preferences_Tag)_
	fromContext:(Preferences_ContextRef)_
	forKey:(NSString*)_
	failureFlag:(BOOL*)_;
	- (NSArray*)
	primaryDisplayBindingKeys;
	- (NSArray*)
	sampleDisplayBindingKeyPaths;
	- (void)
	setSampleAreaFromDefaultPreferences;
	- (void)
	setSampleAreaFromPreferences:(Preferences_ContextRef)_;
	- (void)
	setSampleAreaFromPreferences:(Preferences_ContextRef)_
	restrictedTag1:(Preferences_Tag)_
	restrictedTag2:(Preferences_Tag)_;
	- (BOOL)
	resetToFactoryDefaultColors;

@end //}



#pragma mark Public Methods

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
	tagList.push_back(kPreferences_TagTerminalColorCursorBackground);
	tagList.push_back(kPreferences_TagAutoSetCursorColor);
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
Copies a preference tag for a color from the specified
source context to the given destination.  All color
data has the same size, which is currently that of a
"CGDeviceColor".

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
	CGDeviceColor		colorValue;
	
	
	result = Preferences_ContextGetData(inSource, inSourceTag, sizeof(colorValue), &colorValue,
										inSearchDefaults, outIsDefaultOrNull);
	if (kPreferences_ResultOK == result)
	{
		result = Preferences_ContextSetData(inoutDestination, inSourceTag, sizeof(colorValue), &colorValue);
	}
	return result;
}// copyColor

} // anonymous namespace


#pragma mark -
@implementation PrefPanelFormats_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers = @[
										[[[PrefPanelFormats_GeneralViewManager alloc] init] autorelease],
										[[[PrefPanelFormats_StandardColorsViewManager alloc] init] autorelease],
									];
	
	
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


#pragma mark -
@implementation PrefPanelFormats_CharacterWidthContent


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
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
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, [self preferencesTag],
																		sizeof(result), &result,
																		true/* search defaults */, &isDefault);
		
		
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
	
	
	UNUSED_RETURN(Float32)[self readValueSeeIfDefault:&result];
	
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


#pragma mark -
@implementation PrefPanelFormats_GeneralViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
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
	for (NSString* keyName in [self sampleDisplayBindingKeyPaths])
	{
		[self removeObserver:self forKeyPath:keyName];
	}
	
	[prefsMgr release];
	[byKey release];
	if (nullptr != sampleScreenBuffer)
	{
		Terminal_ReleaseScreen(&sampleScreenBuffer);
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
- (PreferenceValue_Flag*)
autoSetCursorColor
{
	return [self->byKey objectForKey:@"autoSetCursorColor"];
}// autoSetCursorColor


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Color*)
cursorBackgroundColor
{
	return [self->byKey objectForKey:@"cursorBackgroundColor"];
}// cursorBackgroundColor


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
	[[NSFontPanel sharedFontPanel] orderFront:sender];
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
#pragma unused(aKeyPath, anObject, aContext)
	if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
	{
		// TEMPORARY; refresh the terminal view for any change, without restrictions
		// (should fix to send only relevant preference tags, based on the key path)
		[self setSampleAreaFromDefaultPreferences]; // since a context may not have everything, make sure the rest uses Default values
		[self setSampleAreaFromPreferences:[self->prefsMgr currentContext]
											restrictedTag1:'----' restrictedTag2:'----'];
	}
}// observeValueForKeyPath:ofObject:change:context:


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
		Preferences_ContextWrap		terminalConfig(Preferences_NewContext(Quills::Prefs::TERMINAL),
													Preferences_ContextWrap::kAlreadyRetained);
		Preferences_ContextWrap		translationConfig(Preferences_NewContext(Quills::Prefs::TRANSLATION),
														Preferences_ContextWrap::kAlreadyRetained);
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
																	self->sampleScreenBuffer,
																	nullptr/* format */);
			if (nullptr == self->sampleScreenView)
			{
				Console_WriteLine("failed to create sample terminal view");
			}
			else
			{
				// force the view into normal display mode, because zooming will
				// mess up the font size
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetDisplayMode(self->sampleScreenView, kTerminalView_DisplayModeNormal);
				
				// ignore changes to certain preferences for this sample view, since
				// it is not meant to be an ordinary terminal view
				UNUSED_RETURN(TerminalView_Result)TerminalView_IgnoreChangesToPreference(self->sampleScreenView, kPreferences_TagTerminalResizeAffectsFontSize);
				
				// ignore user interaction, because text selections are not meant
				// to change
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetUserInteractionEnabled(self->sampleScreenView, false);
				
				// write some text in all major styles to the screen so that the
				// user can see examples of the selected font, size and colors
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
	for (NSString* keyPath in [self sampleDisplayBindingKeyPaths])
	{
		[self addObserver:self forKeyPath:keyPath options:0 context:nullptr];
	}
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
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
								initWithPreferencesTag:kPreferences_TagTerminalColorCursorBackground
														contextManager:self->prefsMgr] autorelease]
					forKey:@"cursorBackgroundColor"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagAutoSetCursorColor
														contextManager:self->prefsMgr] autorelease]
					forKey:@"autoSetCursorColor"];
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
	for (NSString* keyName in [self primaryDisplayBindingKeys])
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
	if (kPanel_VisibilityDisplayed == aVisibility)
	{
		[self setSampleAreaFromDefaultPreferences]; // since a context may not have everything, make sure the rest uses Default values
		[self setSampleAreaFromPreferences:[self->prefsMgr currentContext]];
	}
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
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// now apply the specified settings
	[self->prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
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


#pragma mark -
@implementation PrefPanelFormats_GeneralViewManager (PrefPanelFormats_GeneralViewManagerInternal)


/*!
Returns the names of key-value coding keys that have color values.

(4.1)
*/
- (NSArray*)
colorBindingKeys
{
	return @[
				@"normalBackgroundColor", @"normalForegroundColor",
				@"boldBackgroundColor", @"boldForegroundColor",
				@"blinkingBackgroundColor", @"blinkingForegroundColor",
				@"matteBackgroundColor",
				@"cursorBackgroundColor",
			];
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
	[result addObject:@"autoSetCursorColor"];
	
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
	
	
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[result addObject:[NSString stringWithFormat:@"%@.inherited", keyName]];
		[result addObject:[NSString stringWithFormat:@"%@.inheritEnabled", keyName]];
	}
	
	for (NSString* keyName in [self colorBindingKeys])
	{
		[result addObject:[NSString stringWithFormat:@"%@.colorValue", keyName]];
	}
	
	[result addObject:@"fontFamily.stringValue"];
	[result addObject:@"fontSize.stringValue"];
	[result addObject:@"characterWidth.floatValue"];
	[result addObject:@"autoSetCursorColor.numberValue"];
	
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
		[self setSampleAreaFromPreferences:defaultContext];
	}
}// setSampleAreaFromDefaultPreferences


/*!
Calls "setSampleAreaFromPreferences:restrictedTag1:restrictedTag2:"
without any tag constraints.  Use this to reread all possible
preferences, when you do not know which preferences have changed.

(2016.09)
*/
- (void)
setSampleAreaFromPreferences:(Preferences_ContextRef)	inSettingsToCopy
{
	[self setSampleAreaFromPreferences:inSettingsToCopy restrictedTag1:'----' restrictedTag2:'----'];
}// setSampleAreaFromPreferences:


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


#pragma mark -
@implementation PrefPanelFormats_StandardColorsViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
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
	AlertMessages_BoxWrap	box(Alert_NewWindowModal([[self managedView] window]),
								AlertMessages_BoxWrap::kAlreadyRetained);
	UIStrings_Result		stringResult = kUIStrings_ResultOK;
	CFRetainRelease			dialogTextCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowANSIColorsResetPrimaryText),
												CFRetainRelease::kAlreadyRetained);
	CFRetainRelease			helpTextCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowGenericCannotUndoHelpText),
												CFRetainRelease::kAlreadyRetained);
	
	
	assert(dialogTextCFString.exists());
	assert(helpTextCFString.exists());
	
	Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton1,
	^{
		// user consented to color reset; change all colors to defaults
		BOOL	resetOK = [self resetToFactoryDefaultColors];
		
		
		if (NO == resetOK)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to reset all the standard colors to default values!");
		}
	});
	Alert_SetHelpButton(box.returnRef(), false);
	Alert_SetParamsFor(box.returnRef(), kAlert_StyleOKCancel);
	Alert_SetTextCFStrings(box.returnRef(), dialogTextCFString.returnCFStringRef(),
							helpTextCFString.returnCFStringRef());
	Alert_Display(box.returnRef()); // retains alert until it is dismissed
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


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to bound values by updating the sample
terminal view.

(2016.09)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
#pragma unused(aKeyPath, anObject, aContext)
	if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
	{
		// TEMPORARY; refresh the terminal view for any change, without restrictions
		// (should fix to send only relevant preference tags, based on the key path)
		[self setSampleAreaFromDefaultPreferences]; // since a context may not have everything, make sure the rest uses Default values
		[self setSampleAreaFromPreferences:[self->prefsMgr currentContext]
											restrictedTag1:'----' restrictedTag2:'----'];
	}
}// observeValueForKeyPath:ofObject:change:context:


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
		Preferences_ContextWrap		terminalConfig(Preferences_NewContext(Quills::Prefs::TERMINAL),
													Preferences_ContextWrap::kAlreadyRetained);
		Preferences_ContextWrap		translationConfig(Preferences_NewContext(Quills::Prefs::TRANSLATION),
														Preferences_ContextWrap::kAlreadyRetained);
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
																	self->sampleScreenBuffer,
																	nullptr/* format */);
			if (nullptr == self->sampleScreenView)
			{
				Console_WriteLine("failed to create sample terminal view");
			}
			else
			{
				// force the view into normal display mode, because zooming will
				// mess up the font size
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetDisplayMode(self->sampleScreenView, kTerminalView_DisplayModeNormal);
				
				// ignore changes to certain preferences for this sample view, since
				// it is not meant to be an ordinary terminal view
				UNUSED_RETURN(TerminalView_Result)TerminalView_IgnoreChangesToPreference(self->sampleScreenView, kPreferences_TagTerminalResizeAffectsFontSize);
				
				// ignore user interaction, because text selections are not meant
				// to change
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetUserInteractionEnabled(self->sampleScreenView, false);
				
				// write some text in all base colors to the screen so that the
				// user can see examples of the selected colors (normal and bold)
				Terminal_EmulatorProcessCString(self->sampleScreenBuffer,
												"\033[2J\033[H"); // clear screen, home cursor (assumes VT100)
				Terminal_EmulatorProcessCString(self->sampleScreenBuffer,
												"\033[40m b \033[41m r \033[42m g \033[43m y \033[44m b \033[45m m \033[46m c \033[47m w \033[0m \033[30m b \033[31m r \033[32m g \033[33m y \033[34m b \033[35m m \033[36m c \033[37m w \033[0m");
				Terminal_EmulatorProcessCString(self->sampleScreenBuffer,
												"\r\n\033[1m\033[40m b \033[41m r \033[42m g \033[43m y \033[44m b \033[45m m \033[46m c \033[47m w \033[0m \033[1m\033[30m b \033[31m r \033[32m g \033[33m y \033[34m b \033[35m m \033[36m c \033[37m w \033[0m");
			}
		}
	}
	
	// observe all properties that can affect the sample display area
	for (NSString* keyPath in [self sampleDisplayBindingKeyPaths])
	{
		[self addObserver:self forKeyPath:keyPath options:0 context:nullptr];
	}
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
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
	if (kPanel_VisibilityDisplayed == aVisibility)
	{
		[self setSampleAreaFromDefaultPreferences]; // since a context may not have everything, make sure the rest uses Default values
		[self setSampleAreaFromPreferences:[self->prefsMgr currentContext]];
	}
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
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	[self->prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
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


#pragma mark -
@implementation PrefPanelFormats_StandardColorsViewManager (PrefPanelFormats_StandardColorsViewManagerInternal)


/*!
Returns the names of key-value coding keys that have color values.

(2016.09)
*/
- (NSArray*)
colorBindingKeys
{
	return @[
				@"blackNormalColor", @"blackBoldColor",
				@"redNormalColor", @"redBoldColor",
				@"greenNormalColor", @"greenBoldColor",
				@"yellowNormalColor", @"yellowBoldColor",
				@"blueNormalColor", @"blueBoldColor",
				@"magentaNormalColor", @"magentaBoldColor",
				@"cyanNormalColor", @"cyanBoldColor",
				@"whiteNormalColor", @"whiteBoldColor",
			];
}// colorBindingKeys


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

(2016.09)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	NSMutableArray*		result = [NSMutableArray arrayWithArray:[self colorBindingKeys]];
	
	
	return result;
}// primaryDisplayBindingKeys


/*!
Returns the key paths of all settings that, if changed, would
require the sample terminal to be refreshed.

(2016.09)
*/
- (NSArray*)
sampleDisplayBindingKeyPaths
{
	NSMutableArray*		result = [NSMutableArray arrayWithCapacity:20/* arbitrary */];
	
	
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[result addObject:[NSString stringWithFormat:@"%@.inherited", keyName]];
		[result addObject:[NSString stringWithFormat:@"%@.inheritEnabled", keyName]];
	}
	
	for (NSString* keyName in [self colorBindingKeys])
	{
		[result addObject:[NSString stringWithFormat:@"%@.colorValue", keyName]];
	}
	
	return result;
}// sampleDisplayBindingKeyPaths


/*!
Updates the sample terminal display using Default settings.
This is occasionally necessary as a reset step prior to
adding new settings, because a context does not have to
define every value (the display should fall back on the
defaults, instead of whatever setting happened to be in
effect beforehand).

(2016.09)
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
Calls "setSampleAreaFromPreferences:restrictedTag1:restrictedTag2:"
without any tag constraints.  Use this to reread all possible
preferences, when you do not know which preferences have changed.

(2016.09)
*/
- (void)
setSampleAreaFromPreferences:(Preferences_ContextRef)	inSettingsToCopy
{
	[self setSampleAreaFromPreferences:inSettingsToCopy restrictedTag1:'----' restrictedTag2:'----'];
}// setSampleAreaFromPreferences:


/*!
Updates the sample terminal display using the settings in the
specified context.  All relevant settings are copied, unless
one or more valid restriction tags are given; when restricted,
only the settings indicated by the tags are used (and all other
parts of the display are unchanged).

(2016.09)
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
