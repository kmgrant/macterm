/*!	\file MacroManager.mm
	\brief An API for user-specified keyboard short-cuts.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#import "MacroManager.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cctype>
#import <cstdio>
#import <cstring>

// standard-C++ includes
#import <sstream>
#import <string>

// Mac includes
#import <Carbon/Carbon.h> // for kVK... virtual key codes (TEMPORARY; deprecated)
#import <Cocoa/Cocoa.h>

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <ContextSensitiveMenu.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "ConstantsRegistry.h"
#import "FindDialog.h"
#import "Network.h"
#import "Preferences.h"
#import "QuillsPrefs.h"
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "TerminalWindow.h"
#import "URL.h"
#import "UIStrings.h"



#pragma mark Internal Method Prototypes
namespace {

void						changeNotify						(MacroManager_Change, void*, Boolean = false);
void						macroSetChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void						preferenceChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
Preferences_ContextRef		returnDefaultMacroSet				(Boolean);
NSMenu*						returnMacrosMenu					();
CFStringRef					returnStringCopyWithSubstitutions	(CFStringRef, SessionRef);
unichar						virtualKeyToUnicode					(UInt16);

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_Ref&			gMacroManagerChangeListenerModel ()
							{
								static ListenerModel_Ref		_ = ListenerModel_New
																	(kListenerModel_StyleStandard,
																		kConstantsRegistry_ListenerModelDescriptorMacroChanges);
								
								
								return _;
							}
ListenerModel_ListenerRef&	gMacroSetMonitor ()		{ static ListenerModel_ListenerRef x = ListenerModel_NewStandardListener(macroSetChanged); return x; }
ListenerModel_ListenerRef&	gPreferencesMonitor ()	{ static ListenerModel_ListenerRef x = ListenerModel_NewStandardListener(preferenceChanged); return x; }
Preferences_ContextRef&		gCurrentMacroSet ()		{ static Preferences_ContextRef x = returnDefaultMacroSet(true/* retain */); return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Calls ContextSensitiveMenu_NewItemGroup() with a group
title and conditionally adds as many macro items as
possible to the given contextual menu.  The source
macros come from the specified macro set (if none is
given, the active set is used as a source).

Currently the only possible context is a terminal view
with a text selection.  If text is selected, the only
macros that appear are macros of type “Enter Text with
Substitutions” that contain at least one sequence that
depends on the current selection (\j, \q or \s).

WARNING:	This assumes that ContextSensitiveMenu_Init()
		has been called to start adding items to a
		menu, and it assumes that it is OK to start a
		new group of items (which will end any existing
		group).

*/
void
MacroManager_AddContextualMenuGroup		(NSMenu*					inoutContextualMenu,
					 					 Preferences_ContextRef		inMacroSetOrNullForActiveSet,
										 Boolean					inCheckDefaults)
{
	SessionRef			activeSession = SessionFactory_ReturnUserFocusSession();
	TerminalWindowRef	activeTerminalWindow = (Session_IsValid(activeSession))
												? Session_ReturnActiveTerminalWindow(activeSession)
												: nullptr;
	TerminalViewRef		activeTerminalView = TerminalWindow_IsValid(activeTerminalWindow)
												? TerminalWindow_ReturnViewWithFocus(activeTerminalWindow)
												: nullptr;
	
	
	if ((nullptr != activeTerminalView) && TerminalView_TextSelectionExists(activeTerminalView))
	{
		Preferences_ContextRef	prefsContext = (nullptr == inMacroSetOrNullForActiveSet)
												? gCurrentMacroSet()
												: inMacroSetOrNullForActiveSet;
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		MacroManager_Action		actionType = kMacroManager_ActionSendTextVerbatim;
		CFStringRef				contentsCFString = nullptr;
		CFStringRef				nameCFString = nullptr;
		CFStringRef				groupTitleCFString = nullptr;
		NSMenuItem*				newItem = nil;
		SEL						macroInvokerSelector = @selector(performActionForMacro:); // see Commands.pm
		id						targetForSelector = [Commands_Executor sharedExecutor]; // TEMPORARY during Carbon transition; see Commands.h
		
		
		UNUSED_RETURN(UIStrings_Result)UIStrings_Copy(kUIStrings_ContextualMenuGroupTitleMacros, groupTitleCFString);
		
		ContextSensitiveMenu_NewItemGroup(groupTitleCFString);
		
		// see which macros are applicable
		for (Preferences_Index i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
		{
			// retrieve action
			prefsResult = Preferences_ContextGetData
							(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, i),
								sizeof(actionType), &actionType, inCheckDefaults);
			if ((kPreferences_ResultOK == prefsResult) &&
				((kMacroManager_ActionSendTextProcessingEscapes == actionType) ||
					(kMacroManager_ActionFindTextProcessingEscapes == actionType)))
			{
				// retrieve contents
				prefsResult = Preferences_ContextGetData
								(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, i),
									sizeof(contentsCFString), &contentsCFString, inCheckDefaults);
				if (kPreferences_ResultOK == prefsResult)
				{
					NSString*	contentsNSString = BRIDGE_CAST(contentsCFString, NSString*);
					
					
					// if the macro performs substitutions and it contains any of the
					// sequences that deal with text selections then the macro should
					// be inserted into the contextual menu
					if (([contentsNSString rangeOfString:@"\\j"].length > 0) ||
						([contentsNSString rangeOfString:@"\\q"].length > 0) ||
						([contentsNSString rangeOfString:@"\\s"].length > 0))
					{
						// retrieve name
						prefsResult = Preferences_ContextGetData
										(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, i),
											sizeof(nameCFString), &nameCFString, inCheckDefaults);
						if (kPreferences_ResultOK == prefsResult)
						{
							newItem = [[NSMenuItem alloc] initWithTitle:BRIDGE_CAST(nameCFString, NSString*)
																		action:macroInvokerSelector
																		keyEquivalent:@""];
							[newItem setTarget:targetForSelector];
							[newItem setTag:i]; // IMPORTANT: selector relies on this to know which macro is being requested!!!
							ContextSensitiveMenu_AddItem(inoutContextualMenu, newItem);
							[newItem release];
						}
					}
				}
			}
		}
		
		if (nullptr != groupTitleCFString)
		{
			CFRelease(groupTitleCFString), groupTitleCFString = nullptr;
		}
	}
}// MacroManager_AddContextualMenuGroup


/*!
Returns the macro set most recently made current with a
call to MacroManager_SetCurrentMacros().

(4.0)
*/
Preferences_ContextRef
MacroManager_ReturnCurrentMacros ()
{
	return gCurrentMacroSet();
}// ReturnCurrentMacros


/*!
Returns the default set of macros, which is identified as
such in the Preferences window.

This currently has no special significance other than its
position in the source list and the fact that the user
cannot delete it.  Even this set will be disabled if it
is active when the user chooses to turn off macros.

(4.0)
*/
Preferences_ContextRef
MacroManager_ReturnDefaultMacros ()
{
	// putting just-in-time monitoring here is a bit of a hack,
	// but it saves the module from requiring an initializer
	static Boolean		gDidInstallPrefsMonitor = false;
	if (false == gDidInstallPrefsMonitor)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring
						(gPreferencesMonitor(), kPreferences_ChangeNumberOfContexts,
							false/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		gDidInstallPrefsMonitor = true;
	}
	
	return returnDefaultMacroSet(false/* retain */);
}// ReturnDefaultMacros


/*!
Changes the current macro set, which affects the source
of future API calls such as MacroManager_UserInputMacro().
If "nullptr", then macros are turned off and future calls
to use them will silently have no effect.

You can retrieve this set later with a call to
MacroManager_ReturnCurrentMacros().

See also MacroManager_ReturnDefaultMacros(), which is a
convenient way to pass the default set to this function.

\retval kMacroManager_ResultOK
if no error occurred

\retval kMacroManager_ResultGenericFailure
if any error occurred

(4.0)
*/
MacroManager_Result
MacroManager_SetCurrentMacros	(Preferences_ContextRef		inMacroSetOrNullForNone)
{
	MacroManager_Result		result = kMacroManager_ResultGenericFailure;
	
	
	if (inMacroSetOrNullForNone == gCurrentMacroSet())
	{
		result = kMacroManager_ResultOK;
	}
	else
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// notify listeners
		changeNotify(kMacroManager_ChangeMacroSetFrom, gCurrentMacroSet());
		
		// perform last action for previous context
		if (nullptr != gCurrentMacroSet())
		{
			// remove monitors from the context that is about to be non-current
			for (Preferences_Index i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
			{
				UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																					Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, i));
				UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																					Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, i));
				UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																					Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, i));
				UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																					Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, i));
				UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																					Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, i));
			}
			
			Preferences_ReleaseContext(&(gCurrentMacroSet()));
		}
		
		// set the new current context
		gCurrentMacroSet() = inMacroSetOrNullForNone;
		
		// monitor the new current context so that caches and menus can be updated, etc.
		if (nullptr != gCurrentMacroSet())
		{
			Preferences_RetainContext(gCurrentMacroSet());
			
			// monitor Preferences for changes to macro settings that are important in the Macro Manager module
			for (Preferences_Index i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
			{
				prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																	Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, i),
																	true/* notify of initial value */);
				assert(kPreferences_ResultOK == prefsResult);
				prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																	Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, i),
																	true/* notify of initial value */);
				assert(kPreferences_ResultOK == prefsResult);
				prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																	Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, i),
																	true/* notify of initial value */);
				assert(kPreferences_ResultOK == prefsResult);
				prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																	Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, i),
																	true/* notify of initial value */);
				assert(kPreferences_ResultOK == prefsResult);
				prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																	Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, i),
																	true/* notify of initial value */);
				assert(kPreferences_ResultOK == prefsResult);
			}
		}
		
		// notify listeners
		changeNotify(kMacroManager_ChangeMacroSetTo, gCurrentMacroSet());
		
		result = kMacroManager_ResultOK;
	}
	
	return result;
}// SetCurrentMacros


/*!
Arranges for a callback to be invoked for the given type
of change.

In addition, for "kMacroManager_ChangeMacroSetTo", the
callback is invoked immediately with the current macro set
as the context.

\retval kMacroManager_ResultOK
if no error occurs

\retval kMacroManager_ResultGenericFailure
if the specified change type is not valid

(2020.05)
*/
MacroManager_Result
MacroManager_StartMonitoring	(MacroManager_Change		inForWhatChange,
								 ListenerModel_ListenerRef	inListener)
{
	MacroManager_Result		result = kMacroManager_ResultGenericFailure;
	
	
	switch (inForWhatChange)
	{
	// Keep this in sync with MacroManager_StopMonitoring().
	case kMacroManager_ChangeMacroSetFrom:
	case kMacroManager_ChangeMacroSetTo:
		{
			Boolean		addOK = false;
			
			
			addOK = ListenerModel_AddListenerForEvent(gMacroManagerChangeListenerModel(), inForWhatChange,
														inListener);
			if (false == addOK)
			{
				result = kMacroManager_ResultGenericFailure;
			}
			else
			{
				result = kMacroManager_ResultOK;
				
				if (kMacroManager_ChangeMacroSetTo == inForWhatChange)
				{
					// notify of initial value
					changeNotify(kMacroManager_ChangeMacroSetTo, gCurrentMacroSet());
				}
			}
		}
		break;
	
	default:
		// unsupported tag for notifiers
		result = kMacroManager_ResultGenericFailure;
		break;
	}
	
	return result;
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked when
the specified change occurs.

\retval kMacroManager_ResultOK
if no error occurs

\retval kMacroManager_ResultGenericFailure
if the specified change type is not valid

(2020.05)
*/
MacroManager_Result
MacroManager_StopMonitoring	(MacroManager_Change		inForWhatChange,
							 ListenerModel_ListenerRef	inListener)
{
	MacroManager_Result		result = kMacroManager_ResultGenericFailure;
	
	
	switch (inForWhatChange)
	{
	// Keep this in sync with MacroManager_StartMonitoring().
	case kMacroManager_ChangeMacroSetFrom:
	case kMacroManager_ChangeMacroSetTo:
		ListenerModel_RemoveListenerForEvent(gMacroManagerChangeListenerModel(), inForWhatChange, inListener);
		result = kMacroManager_ResultOK;
		break;
	
	default:
		// unsupported tag for notifiers
		result = kMacroManager_ResultGenericFailure;
		break;
	}
	
	return result;
}// StopMonitoring


/*!
Sets the title and key equivalent of the specified menu item
based on the given macro in the set.

Given information on the state of the application (terminal
window in front, etc.), a hint is returned to indicate whether
or not the macro item should be enabled.  The state is not
directly changed here because the enabled state of a menu item
is typically handled at the command level. 

(4.0)
*/
Boolean
MacroManager_UpdateMenuItem		(NSMenuItem*				inMenuItem,
								 UInt16						inOneBasedMacroIndex,
								 Boolean					inIsTerminalWindowActive,
								 Preferences_ContextRef		inMacroSetOrNullForActiveSet,
								 Boolean					inCheckDefaults)
{
	Preferences_Index const	kMacroPrefIndex = STATIC_CAST(inOneBasedMacroIndex, Preferences_Index);
	Preferences_ContextRef	defaultContext = returnDefaultMacroSet(false/* retain */);
	Preferences_ContextRef	prefsContext = (nullptr == inMacroSetOrNullForActiveSet)
											? gCurrentMacroSet()
											: inMacroSetOrNullForActiveSet;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFStringRef				nameCFString = nullptr;
	MacroManager_Action		macroAction = kMacroManager_ActionSendTextVerbatim;
	MacroManager_KeyID		macroKeyID = 0;
	UInt32					modifiers = 0;
	Boolean					isMenuDisplayingDefault = (prefsContext == defaultContext);
	Boolean					isDefault = false; // see below
	Boolean					result = false;
	
	
	// retrieve name
	prefsResult = Preferences_ContextGetData
					(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, kMacroPrefIndex),
						sizeof(nameCFString), &nameCFString, inCheckDefaults, &isDefault);
	if (kPreferences_ResultOK == prefsResult)
	{
		if ((isDefault) && (false == isMenuDisplayingDefault))
		{
			NSString*	templateNSString = NSLocalizedString(@" (Default)  %@",
																@"used in Macros menu when non-Default set is displaying macro inherited from Default");
			
			
			[inMenuItem setTitle:[NSString stringWithFormat:templateNSString, BRIDGE_CAST(nameCFString, NSString*)]];
		}
		else
		{
			[inMenuItem setTitle:BRIDGE_CAST(nameCFString, NSString*)];
		}
		CFRelease(nameCFString), nameCFString = nullptr;
	}
	else
	{
		// set a dummy value
		[inMenuItem setTitle:@""];
	}
	
	// reset menu item
	[inMenuItem setKeyEquivalent:@""];
	
	// retrieve key code
	prefsResult = Preferences_ContextGetData
					(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, kMacroPrefIndex),
						sizeof(macroKeyID), &macroKeyID, inCheckDefaults, &isDefault);
	if (kPreferences_ResultOK == prefsResult)
	{
		UInt16 const	kKeyCode = MacroManager_KeyIDKeyCode(macroKeyID);
		Boolean const	kIsVirtualKey = MacroManager_KeyIDIsVirtualKey(macroKeyID);
		unichar			keyEquivalentUnicode = '\0';
		
		
		// apply new key equivalent
		if (kIsVirtualKey)
		{
			keyEquivalentUnicode = virtualKeyToUnicode(kKeyCode);
			[inMenuItem setKeyEquivalent:[NSString stringWithCharacters:&keyEquivalentUnicode length:1]];
		}
		else
		{
			keyEquivalentUnicode = kKeyCode;
			[inMenuItem setKeyEquivalent:[NSString stringWithCharacters:&keyEquivalentUnicode length:1]];
		}
	}
	
	// retrieve key modifiers
	prefsResult = Preferences_ContextGetData
					(prefsContext,
						Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, kMacroPrefIndex),
						sizeof(modifiers), &modifiers, inCheckDefaults, &isDefault);
	if (kPreferences_ResultOK == prefsResult)
	{
		unsigned int	abbreviatedModifiers = 0;
		
		
		// NOTE: if a command key has not been assigned to the item,
		// setting its modifiers will have no visible effect
		if (modifiers & kMacroManager_ModifierKeyMaskCommand) abbreviatedModifiers |= NSEventModifierFlagCommand;
		if (modifiers & kMacroManager_ModifierKeyMaskControl) abbreviatedModifiers |= NSEventModifierFlagControl;
		if (modifiers & kMacroManager_ModifierKeyMaskOption) abbreviatedModifiers |= NSEventModifierFlagOption;
		if (modifiers & kMacroManager_ModifierKeyMaskShift) abbreviatedModifiers |= NSEventModifierFlagShift;
		[inMenuItem setKeyEquivalentModifierMask:abbreviatedModifiers];
	}
	else
	{
		// set a dummy value
		[inMenuItem setKeyEquivalentModifierMask:0];
	}
	
	// retrieve action
	prefsResult = Preferences_ContextGetData
					(prefsContext,
						Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, kMacroPrefIndex),
						sizeof(macroAction), &macroAction, inCheckDefaults, &isDefault);
	if (kPreferences_ResultOK == prefsResult)
	{
		result = true; // initially...
		
		// determine if this type of macro should be enabled, given
		// the information on the current context
		switch (macroAction)
		{
		case kMacroManager_ActionHandleURL:
		case kMacroManager_ActionNewWindowWithCommand:
		case kMacroManager_ActionSelectMatchingWindow:
			// these types of actions can always be taken
			result = true;
			break;
		
		case kMacroManager_ActionSendTextVerbatim:
		case kMacroManager_ActionSendTextProcessingEscapes:
		case kMacroManager_ActionFindTextVerbatim:
		case kMacroManager_ActionFindTextProcessingEscapes:
			// these types of actions require a terminal window
			result = inIsTerminalWindowActive;
			break;
		
		default:
			break;
		}
	}
	else
	{
		// set a default value
		result = inIsTerminalWindowActive;
	}
	
	return result;
}// UpdateMenuItem


/*!
Retrieves the action and associated text for the specified
macro of the given set (or the active set, if no context
is provided), and performs the macro action.

If the action needs to target a session, such as inserting
text, then the specified session is used (or the active
session, if none is provided).

\retval kMacroManager_ResultOK
if no error occurred

\retval kMacroManager_ResultGenericFailure
if any error occurred

(4.0)
*/
MacroManager_Result
MacroManager_UserInputMacro		(UInt16						inZeroBasedMacroIndex,
								 SessionRef					inTargetSessionOrNullForActiveSession,
								 Preferences_ContextRef		inMacroSetOrNullForActiveSet)
{
	MacroManager_Result		result = kMacroManager_ResultGenericFailure;
	SessionRef				session = (nullptr == inTargetSessionOrNullForActiveSession)
										? SessionFactory_ReturnUserRecentSession()
										: inTargetSessionOrNullForActiveSession;
	Preferences_ContextRef	context = (nullptr == inMacroSetOrNullForActiveSet)
										? gCurrentMacroSet()
										: inMacroSetOrNullForActiveSet;
	
	
	if (nullptr != context)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		CFStringRef				actionCFString = nullptr;
		MacroManager_Action		actionPerformed = kMacroManager_ActionSendTextProcessingEscapes;
		
		
		// retrieve action type
		// TEMPORARY - it is smarter to query this only as preferences
		// actually change, i.e. in the monitor callback, and rely
		// only on a cached array at this point
		prefsResult = Preferences_ContextGetData
						(context, Preferences_ReturnTagVariantForIndex
									(kPreferences_TagIndexedMacroAction, STATIC_CAST(inZeroBasedMacroIndex + 1, Preferences_Index)),
							sizeof(actionPerformed), &actionPerformed, true/* search defaults too */);
		if (kPreferences_ResultOK == prefsResult)
		{
			// retrieve action text
			prefsResult = Preferences_ContextGetData
							(context, Preferences_ReturnTagVariantForIndex
										(kPreferences_TagIndexedMacroContents, STATIC_CAST(inZeroBasedMacroIndex + 1, Preferences_Index)),
								sizeof(actionCFString), &actionCFString, true/* search defaults too */);
			if (kPreferences_ResultOK == prefsResult)
			{
				switch (actionPerformed)
				{
				case kMacroManager_ActionSendTextVerbatim:
					// send string to the session as-is
					if (nullptr != session)
					{
						Session_UserInputCFString(session, actionCFString);
						result = kMacroManager_ResultOK;
					}
					break;
				
				case kMacroManager_ActionFindTextVerbatim:
					// find string as-is without performing substitutions
					if (nullptr != session)
					{
						UNUSED_RETURN(NSUInteger)FindDialog_SearchWithoutDialog(actionCFString, Session_ReturnActiveTerminalWindow(session),
																				kFindDialog_OptionsAllOff);
						result = kMacroManager_ResultOK;
					}
					break;
				
				case kMacroManager_ActionSendTextProcessingEscapes:
					if (nullptr != session)
					{
						CFRetainRelease		finalCFString(returnStringCopyWithSubstitutions(actionCFString, session), CFRetainRelease::kAlreadyRetained);
						
						
						if (false == finalCFString.exists())
						{
							Console_WriteLine("macro was not handled due to substitution errors");
						}
						else
						{
							// send the edited string to the session!
							Session_UserInputCFString(session, finalCFString.returnCFStringRef());
							result = kMacroManager_ResultOK;
						}
					}
					break;
				
				case kMacroManager_ActionFindTextProcessingEscapes:
					// find string after performing substitutions
					if (nullptr != session)
					{
						CFRetainRelease		finalCFString(returnStringCopyWithSubstitutions(actionCFString, session), CFRetainRelease::kAlreadyRetained);
						
						
						if (false == finalCFString.exists())
						{
							Console_WriteLine("macro was not handled due to substitution errors");
							result = kMacroManager_ResultGenericFailure;
						}
						else
						{
							// perform a search with the edited string
							UNUSED_RETURN(NSUInteger)FindDialog_SearchWithoutDialog(finalCFString.returnCFStringRef(), Session_ReturnActiveTerminalWindow(session),
																					kFindDialog_OptionsAllOff);
							result = kMacroManager_ResultOK;
						}
					}
					break;
				
				case kMacroManager_ActionHandleURL:
					if (URL_ParseCFString(actionCFString))
					{
						result = kMacroManager_ResultOK;
					}
					break;
				
				case kMacroManager_ActionNewWindowWithCommand:
					{
						CFArrayRef		argsCFArray = CFStringCreateArrayBySeparatingStrings
														(kCFAllocatorDefault, actionCFString, CFSTR(" ")/* LOCALIZE THIS? */);
						
						
						if (nullptr != argsCFArray)
						{
							TerminalWindowRef		terminalWindow = SessionFactory_NewTerminalWindowUserFavorite();
							Preferences_ContextRef	workspaceContext = nullptr;
							SessionRef				newSession = nullptr;
							
							
							newSession = SessionFactory_NewSessionArbitraryCommand(terminalWindow, argsCFArray, nullptr/* session */,
																					false/* reconfigure window from session */,
																					workspaceContext, 0/* window index */);
							if (nullptr != newSession) result = kMacroManager_ResultOK;
							
							CFRelease(argsCFArray), argsCFArray = nullptr;
						}
					}
					break;
				
				case kMacroManager_ActionSelectMatchingWindow:
					{
						TerminalWindowRef							activeTerminalWindow = TerminalWindow_ReturnFromMainWindow();
						__block std::vector< TerminalWindowRef >	windowList;
						
						
						SessionFactory_ForEachTerminalWindow
						(^(TerminalWindowRef	inTerminalWindow,
						   Boolean&				UNUSED_ARGUMENT(outStop))
						{
							windowList.push_back(inTerminalWindow);
						});
						
						if (windowList.size() > 0)
						{
							NSString*			actionNSString = BRIDGE_CAST(actionCFString, NSString*);
							TerminalWindowRef	wrapAroundMatch = nullptr;
							TerminalWindowRef	matchingWindow = nullptr;
							Boolean				foundActive = false;
							
							
							// start from the current window and search for another
							// window in the list that has a matching title (while
							// searching the window list for the current window,
							// also find the first matching window from the front
							// in case the search has to wrap around)
							for (auto iterTerminalWindow : windowList)
							{
								if (iterTerminalWindow == activeTerminalWindow)
								{
									foundActive = true;
								}
								else
								{
									// see if this window’s title matches the query
									NSWindow*	asWindow = TerminalWindow_ReturnNSWindow(iterTerminalWindow);
									NSString*	windowTitle = [asWindow title];
									
									
									if ([windowTitle rangeOfString:actionNSString options:NSCaseInsensitiveSearch].length > 0)
									{
										// the window’s title sufficiently matches the macro’s content string
										if (foundActive)
										{
											// the active window was already found in the window list
											// so this matching window is the next window to select
											matchingWindow = iterTerminalWindow;
											break;
										}
										else if (nullptr == wrapAroundMatch)
										{
											// the active window has not been found in the list iteration
											// yet; although this window matches, it is only going to be
											// the final target window if no other match can be found
											// *beyond* the active window in the current list iteration
											wrapAroundMatch = iterTerminalWindow;
										}
										else
										{
											// a wrap-around match has been found, do not overwrite it;
											// continue searching for the active window however
										}
									}
								}
							}
							
							// if no match was found beyond the active window, use the wrap-around match
							if ((nullptr == matchingWindow) && (nullptr != wrapAroundMatch))
							{
								matchingWindow = wrapAroundMatch;
							}
							
							// if the macro succeeds, select the window; otherwise, emit an error tone
							if (nullptr != matchingWindow)
							{
								TerminalWindow_Select(matchingWindow);
							}
							else
							{
								Sound_StandardAlert();
							}
						}
					}
					break;
				
				default:
					// ???
					break;
				}
				CFRelease(actionCFString), actionCFString = nullptr;
			}
		}
	}
	return result;
}// UserInputMacro


#pragma mark Internal Methods
namespace {

/*!
Notifies all listeners for the specified macro manager
change, passing the given context to the listener.

(2020.05)
*/
void
changeNotify	(MacroManager_Change	inWhatChanged,
				 void*					inContextOrNull,
				 Boolean				inIsInitialValue)
{
#pragma unused(inIsInitialValue)
	// invoke listener callback routines appropriately, from the macro manager listener model
	ListenerModel_NotifyListenersOfEvent(gMacroManagerChangeListenerModel(), inWhatChanged, inContextOrNull);
}// changeNotify


/*!
The Preferences module calls this routine whenever a
monitored macro setting is changed.  This allows cached
strings to be recalculated, menus to be updated, etc.

(4.0)
*/
void
macroSetChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inPreferencesContext,
					 void*					UNUSED_ARGUMENT(inListenerContext))
{
	Preferences_ContextRef		prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	
	
	if (nullptr == prefsContext)
	{
		Console_Warning(Console_WriteLine, "callback was invoked for nonexistent macro set");
	}
	else
	{
		Preferences_Tag const		kTagWithoutIndex = Preferences_ReturnTagFromVariant(inPreferenceTagThatChanged);
		//Preferences_Index const		kIndexFromTag = Preferences_ReturnTagIndex(inPreferenceTagThatChanged);
		
		
		switch (kTagWithoutIndex)
		{
		case kPreferences_TagIndexedMacroAction:
			// TEMPORARY - do nothing for now, but the plan is to eventually use
			// this opportunity to cache all actions in an array so that no time
			// is wasted on a query at macro invocation time
			break;
		
		case kPreferences_TagIndexedMacroContents:
			// TEMPORARY - do nothing for now, but the plan is to eventually use
			// this opportunity to pre-scan and cache a mostly-processed string
			// (e.g. static escape sequences translated) for repeated use at
			// macro invocation time
			break;
		
		case kPreferences_TagIndexedMacroName:
		case kPreferences_TagIndexedMacroKey:		
		case kPreferences_TagIndexedMacroKeyModifiers:
			// immediately update the entire menu, because key equivalents must
			// always match correctly (even if the user does not open the menu)
			[returnMacrosMenu() update];
			// NOTE: this could perhaps do caching that would make it more
			// efficient to call MacroManager_UpdateMenuItem() later
			break;
		
		default:
			// ???
			break;
		}
	}
}// macroSetChanged


/*!
Invoked whenever a monitored preference value is changed (see
MacroManager_ReturnCurrentMacros() to see which preferences are
monitored).  This routine responds by ensuring that the current
macro set is still valid; if the macros were destroyed, the
active set is changed to something else.

(4.0)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	//Preferences_ChangeContext*	contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeNumberOfContexts:
		// if the current macro set has been destroyed, stop using it!
		if ((nullptr != MacroManager_ReturnCurrentMacros()) && (false == Preferences_ContextIsValid(MacroManager_ReturnCurrentMacros())))
		{
			MacroManager_SetCurrentMacros(nullptr);
		}
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
In order to initialize a static (global) variable immediately,
this function was written to return a value.  Otherwise, it is
no different than calling Preferences_GetDefaultContext()
directly for a macro set class.

If "inRetain" is true, then the context is retained before it
is returned, so that it may be safely released.

(4.0)
*/
Preferences_ContextRef
returnDefaultMacroSet	(Boolean	inRetain)
{
	Preferences_ContextRef		result = nullptr;
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_GetDefaultContext(&result, Quills::Prefs::MACRO_SET);
	assert(kPreferences_ResultOK == prefsResult);
	
	if (inRetain)
	{
		Preferences_RetainContext(result);
	}
	
	return result;
}// returnDefaultMacroSet


/*!
Returns the menu from the menu bar that contains macro
commands.  There must be an item in this menu with a tag
of "1", with subsequent macros having increasing numerical
tags.

(4.0)
*/
NSMenu*
returnMacrosMenu ()
{
	assert(nil != NSApp);
	NSMenu*			mainMenu = [NSApp mainMenu];
	assert(nil != mainMenu);
	NSMenuItem*		macrosMenuItem = [mainMenu itemWithTag:kCommands_MenuIDMacros];
	assert(nil != macrosMenuItem);
	NSMenu*			result = [macrosMenuItem submenu];
	assert(nil != result);
	
	
	return result;
}// returnMacrosMenu


/*!
Returns a new string that is at least a copy of the given
string.  If any substitution characters are present (all
two-character sequences starting with a backslash are
reserved) then they are stripped and replaced with the
appropriate values.

If there are substitution errors, they are flagged and
the resulting string will be nullptr.  Specific error
information is currently sent only to the console.

(4.1)
*/
CFStringRef
returnStringCopyWithSubstitutions	(CFStringRef	inBaseString,
									 SessionRef		inTargetSession)
{
	// TEMPORARY - translate everything here for now but the plan is to
	// eventually monitor preference changes and pre-scan and cache a
	// mostly-processed string (e.g. static escape sequences translated)
	// so that very little has to be done at this point
	CFIndex const			kLength = CFStringGetLength(inBaseString);
	CFStringRef				result = nullptr;
	CFStringInlineBuffer	charBuffer;
	CFRetainRelease			finalCFString(CFStringCreateMutable(kCFAllocatorDefault, 0/* limit */),
											CFRetainRelease::kAlreadyRetained);
	__block Boolean			substitutionError = false;
	UniChar					octalSequenceCharCode = '\0'; // overwritten each time a \0nn is processed
	SInt16					readOctal = -1;		// if 0, a \0 was read, and the first "n" (in \0nn) might be next;
												// if 1, a \1 was read, and the first "n" (in \1nn) might be next;
	
	
	CFStringInitInlineBuffer(inBaseString, &charBuffer, CFRangeMake(0, kLength));
	for (CFIndex i = 0; i < kLength; ++i)
	{
		UniChar		thisChar = CFStringGetCharacterFromInlineBuffer(&charBuffer, i);
		
		
		if (i == (kLength - 1))
		{
			CFStringAppendCharacters(finalCFString.returnCFMutableStringRef(), &thisChar, 1/* count */);
		}
		else
		{
			UniChar		nextChar = CFStringGetCharacterFromInlineBuffer(&charBuffer, i + 1);
			
			
			if (readOctal >= 0)
			{
				if (readOctal == 1)
				{
					octalSequenceCharCode = '\100';
				}
				else
				{
					octalSequenceCharCode = '\0';
				}
				
				switch (thisChar)
				{
				case '0':
					break;
				
				case '1':
					octalSequenceCharCode += '\010';
					break;
				
				case '2':
					octalSequenceCharCode += '\020';
					break;
				
				case '3':
					octalSequenceCharCode += '\030';
					break;
				
				case '4':
					octalSequenceCharCode += '\040';
					break;
				
				case '5':
					octalSequenceCharCode += '\050';
					break;
				
				case '6':
					octalSequenceCharCode += '\060';
					break;
				
				case '7':
					octalSequenceCharCode += '\070';
					break;
				
				default:
					// ???
					Console_WriteLine("non-octal-numeric character found while handling a \\0nn sequence");
					substitutionError = true;
					readOctal = -1; // flag error
					break;
				}
				
				switch (nextChar)
				{
				case '0':
					break;
				
				case '1':
					octalSequenceCharCode += '\001';
					break;
				
				case '2':
					octalSequenceCharCode += '\002';
					break;
				
				case '3':
					octalSequenceCharCode += '\003';
					break;
				
				case '4':
					octalSequenceCharCode += '\004';
					break;
				
				case '5':
					octalSequenceCharCode += '\005';
					break;
				
				case '6':
					octalSequenceCharCode += '\006';
					break;
				
				case '7':
					octalSequenceCharCode += '\007';
					break;
				
				default:
					// ???
					Console_WriteLine("non-octal-numeric character found while handling a \\0nn sequence");
					substitutionError = true;
					readOctal = -1; // flag error
					break;
				}
				
				// this is set to false to flag errors in the above switches...
				if (readOctal >= 0)
				{
					++i; // skip the 2nd digit (1st digit is current)
					CFStringAppendCharacters(finalCFString.returnCFMutableStringRef(),
												&octalSequenceCharCode, 1/* count */);
				}
				
				readOctal = -1;
			}
			else if (thisChar == '\\')
			{
				// process escape sequence
				switch (nextChar)
				{
				case '\\':
					// an escaped backslash; send a single backslash
					CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\\"));
					++i; // skip special sequence character
					break;
				
				case '"':
					// an escaped double-quote; send a double-quote (for legacy reasons, this is allowed)
					CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\""));
					++i; // skip special sequence character
					break;
				
				case '#':
					// number of terminal lines
					{
						TerminalWindowRef const		kTerminalWindow = Session_ReturnActiveTerminalWindow(inTargetSession);
						
						
						substitutionError = true;
						if (nullptr == kTerminalWindow)
						{
							Console_WriteLine("unexpected error finding the terminal window, while handling \\# sequence");
						}
						else
						{
							TerminalScreenRef const		kTerminalScreen = TerminalWindow_ReturnScreenWithFocus(kTerminalWindow);
							
							
							if (nullptr == kTerminalScreen)
							{
								Console_WriteLine("unexpected error finding the terminal screen, while handling \\# sequence");
							}
							else
							{
								unsigned int const			kLineCount = Terminal_ReturnRowCount(kTerminalScreen);
								CFRetainRelease				numberCFString(CFStringCreateWithFormat
																			(kCFAllocatorDefault, nullptr/* options */,
																				CFSTR("%u"), kLineCount), CFRetainRelease::kAlreadyRetained);
								
								
								CFStringAppend(finalCFString.returnCFMutableStringRef(), numberCFString.returnCFStringRef());
								substitutionError = false;
							}
						}
					}
					++i; // skip special sequence character
					break;
				
				case 'b':
					// backspace; equivalent to \010
					CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\010"));
					++i; // skip special sequence character
					break;
				
				case 'e':
					// escape; equivalent to \033
					CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\033"));
					++i; // skip special sequence character
					break;
				
				case 'i':
				case 'I':
					// an arbitrary IP address ('i') or space-separated full list ('I');
					// the setup for both of these cases is mostly the same so they
					// are combined
					{
						bool const			sendSingleAddress = ('i' == nextChar);
						__block CFArrayRef	localIPAddresses = nullptr;
						__block Boolean		isComplete = false;
						auto				copyAddressesBlock =
											^{
												Network_CopyLocalHostAddresses(localIPAddresses, &isComplete);
											};
						
						
						copyAddressesBlock();
						if ((false == isComplete) || (0 == CFArrayGetCount(localIPAddresses)))
						{
							auto	targetQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0/* flags */);
							
							
							// release copy created by first call above
							CFRelease(localIPAddresses), localIPAddresses = nullptr;
							
							// if the list is not ready, incur a short synchronous delay
							// and retry once to see if the macro can then be completed
							Console_Warning(Console_WriteLine, "macro: address list incomplete; waiting briefly, will retry");
							// TEMPORARY; can replace with dispatch_barrier_sync() in later SDK
						#if 0
							dispatch_barrier_sync(targetQueue,
													^{
														CocoaExtensions_RunLaterInQueue(targetQueue, 3/* seconds */,
									 													copyAddressesBlock);
													});
						#else
							{
								dispatch_semaphore_t		doneSignal = dispatch_semaphore_create(0);
								
								
								dispatch_async(targetQueue,
												^{
													CocoaExtensions_RunLaterInQueue
													(targetQueue, 5/* seconds */,
														^{
															copyAddressesBlock();
															// return value ignored because this block does
															// not need to know if a thread was awoken
															UNUSED_RETURN(long)dispatch_semaphore_signal(doneSignal);
														});
												});
								// return value ignored because wait time is “forever”
								UNUSED_RETURN(long)dispatch_semaphore_wait(doneSignal, DISPATCH_TIME_FOREVER);
								dispatch_release(doneSignal), doneSignal = nullptr;
							}
						#endif
							if (false == isComplete)
							{
								Console_Warning(Console_WriteLine, "macro: address list is still incomplete, using as-is");
							}
							else
							{
								Console_Warning(Console_WriteLine, "macro: address list is now complete");
							}
						}
						
						if (nullptr == localIPAddresses)
						{
							substitutionError = true;
						}
						else
						{
							NSArray*	addressList = BRIDGE_CAST(localIPAddresses, NSArray*);
							
							
							if (0 == addressList.count)
							{
								// TEMPORARY; determine if it is sensible to consider this an error
								// (while nothing can be substituted, there was also nothing found
								// and that may be a meaningful case)
								substitutionError = true;
							}
							else if (sendSingleAddress)
							{
								// send one address; the rules for selecting it are arbitrary
								CFStringRef		arbitraryAddress = BRIDGE_CAST([addressList objectAtIndex:0], CFStringRef);
								
								
								if (nullptr == arbitraryAddress)
								{
									substitutionError = true;
								}
								else
								{
									CFStringAppend(finalCFString.returnCFMutableStringRef(), arbitraryAddress);
								}
							}
							else
							{
								// send the entire list of addresses (space-separated); NOTE that
								// this assumes individual addresses never contain spaces, no
								// escaping of spaces in address strings is performed…
								NSUInteger	addressIndex = 0;
								
								
								for (id object : addressList)
								{
									assert([object isKindOfClass:NSString.class]);
									NSString*	asString = STATIC_CAST(object, NSString*);
									
									
									CFStringAppend(finalCFString.returnCFMutableStringRef(), BRIDGE_CAST(asString, CFStringRef));
									++addressIndex;
									if (addressIndex != addressList.count)
									{
										// space-separated values (no space at end)
										CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR(" "));
									}
								}
							}
						}
						
						if (nullptr != localIPAddresses)
						{
							CFRelease(localIPAddresses), localIPAddresses = nullptr;
						}
					}
					++i; // skip special sequence character
					break;
				
				case 'j':
				case 'q':
				case 's':
					// currently-selected text, optionally joined together as a single line and with quoting
					{
						TerminalWindowRef const		kTerminalWindow = Session_ReturnActiveTerminalWindow(inTargetSession);
						
						
						if (TerminalWindow_IsValid(kTerminalWindow))
						{
							TerminalViewRef const	kTerminalView = TerminalWindow_ReturnViewWithFocus(kTerminalWindow);
							
							
							if (nullptr != kTerminalView)
							{
								CFStringRef			selectedText = TerminalView_ReturnSelectedTextCopyAsUnicode
																	(kTerminalView, 0/* spaces to replace with tabs */,
																		(('s' == nextChar)
																			? 0
																			: kTerminalView_TextFlagInline));
								
								
								if (nullptr != selectedText)
								{
									CFRetainRelease		workArea;
									
									
									if ('q' == nextChar)
									{
										// copy the string and insert escapes at certain locations to perform quoting
										workArea.setMutableWithNoRetain
													(CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* max. length or zero */,
																				selectedText));
										if (workArea.exists())
										{
											CFRange const	kWholeString = CFRangeMake
																			(0, CFStringGetLength
																				(workArea.returnCFStringRef()));
											
											
											selectedText = workArea.returnCFStringRef();
											
											// escape spaces and tabs
											UNUSED_RETURN(CFIndex)CFStringFindAndReplace
																	(workArea.returnCFMutableStringRef(),
																		CFSTR(" "), CFSTR("\\ "), kWholeString,
																		0/* flags */);
											UNUSED_RETURN(CFIndex)CFStringFindAndReplace
																	(workArea.returnCFMutableStringRef(),
																		CFSTR("\t"), CFSTR("\\\t"), kWholeString,
																		0/* flags */);
											
											// TEMPORARY; no other escapes are performed (might add more in
											// the future, or make this extensible somehow)
										}
									}
									
									// add the appropriate text to the macro expansion
									if (nullptr != selectedText)
									{
										CFStringAppend(finalCFString.returnCFMutableStringRef(), selectedText);
									}
								}
							}
						}
					}
					++i; // skip special sequence character
					break;
				
				case 'n':
					// new-line
					{
						Session_EventKeys		sessionEventKeys = Session_ReturnEventKeys(inTargetSession);
						
						
						// send the sanctioned new-line sequence for the session
						switch (sessionEventKeys.newline)
						{
						case kSession_NewlineModeMapCR:
							CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\015"));
							break;
						
						case kSession_NewlineModeMapCRLF:
							CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\015\012"));
							break;
						
						case kSession_NewlineModeMapCRNull:
							CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\015\000"));
							break;
						
						case kSession_NewlineModeMapLF:
							CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\012"));
							break;
						
						default:
							Console_Warning(Console_WriteValue,
											"macro new-line sequence does not handle mode", sessionEventKeys.newline);
							CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\n"));
							break;
						}
					}
					++i; // skip special sequence character
					break;
				
				case 'r':
					// carriage return without line feed; equivalent to \015
					CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\r"));
					++i; // skip special sequence character
					break;
				
				case 't':
					// horizontal tabulation
					CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\t"));
					++i; // skip special sequence character
					break;
				
				case '0':
					// possibly an arbitrary character code substitution
					readOctal = 0;
					++i; // skip special sequence character
					break;
				
				case '1':
					// possibly an arbitrary character code substitution
					readOctal = 1;
					++i; // skip special sequence character
					break;
				
				default:
					// ???
					Console_Warning(Console_WriteValueCharacter, "unrecognized backslash escape", STATIC_CAST(nextChar, UInt8));
					substitutionError = true;
					break;
				}
			}
			else 
			{
				// not an escape sequence
				CFStringAppendCharacters(finalCFString.returnCFMutableStringRef(), &thisChar, 1/* count */);
			}
		}
	}
	
	if (false == substitutionError)
	{
		result = finalCFString.returnCFStringRef();
		CFRetain(result);
	}
	
	return result;
}// returnStringCopyWithSubstitutions



/*!
Returns a Unicode character that is a reasonable description
of the specified virtual key code, as returned by the Carbon
event manager.

INCOMPLETE.

Virtual key codes are poorly documented, but they are
described in older Inside Macintosh books!

(4.0)
*/
unichar
virtualKeyToUnicode		(UInt16		inVirtualKeyCode)
{
	unichar		result = '\0';
	
	
	// INCOMPLETE!!!
	// (yes, these really are assigned as bizarrely as they seem...)
	switch (inVirtualKeyCode)
	{
	case kVK_Return: // 0x24
		result = NSCarriageReturnCharacter;
		break;
	
	case kVK_Delete: // 0x33
		result = NSBackspaceCharacter;
		break;
	
	case kVK_Escape: // 0x35
		result = 0x001B; // the “escape” key
		break;
	
	case kVK_ANSI_KeypadClear: // 0x47
		result = 0x2327; // the “clear” key
		break;
	
	case kVK_ANSI_KeypadEnter: // 0x4C
		result = NSEnterCharacter;
		break;
	
	case kVK_F5: // 0x60
		result = NSF5FunctionKey;
		break;
	
	case kVK_F6: // 0x61
		result = NSF6FunctionKey;
		break;
	
	case kVK_F7: // 0x62
		result = NSF7FunctionKey;
		break;
	
	case kVK_F3: // 0x63
		result = NSF3FunctionKey;
		break;
	
	case kVK_F8: // 0x64
		result = NSF8FunctionKey;
		break;
	
	case kVK_F9: // 0x65
		result = NSF9FunctionKey;
		break;
	
	case kVK_F11: // 0x67
		result = NSF11FunctionKey;
		break;
	
	case kVK_F13: // 0x69
		result = NSF13FunctionKey;
		break;
	
	case kVK_F16: // 0x6A
		result = NSF16FunctionKey;
		break;
	
	case kVK_F14: // 0x6B
		result = NSF14FunctionKey;
		break;
	
	case kVK_F10: // 0x6D
		result = NSF10FunctionKey;
		break;
	
	case kVK_F12: // 0x6F
		result = NSF12FunctionKey;
		break;
	
	case kVK_F15: // 0x71
		result = NSF15FunctionKey;
		break;
	
	case kVK_Home: // 0x73
		result = NSHomeFunctionKey;
		break;
	
	case kVK_PageUp: // 0x74
		result = NSPageUpFunctionKey;
		break;
	
	case kVK_ForwardDelete: // 0x75
		result = NSDeleteCharacter;
		break;
	
	case kVK_F4: // 0x76
		result = NSF4FunctionKey;
		break;
	
	case kVK_End: // 0x77
		result = NSEndFunctionKey;
		break;
	
	case kVK_F2: // 0x78
		result = NSF2FunctionKey;
		break;
	
	case kVK_PageDown: // 0x79
		result = NSPageDownFunctionKey;
		break;
	
	case kVK_F1: // 0x7A
		result = NSF1FunctionKey;
		break;
	
	case kVK_LeftArrow: // 0x7B
		result = NSLeftArrowFunctionKey;
		break;
	
	case kVK_RightArrow: // 0x7C
		result = NSRightArrowFunctionKey;
		break;
	
	case kVK_DownArrow: // 0x7D
		result = NSDownArrowFunctionKey;
		break;
	
	case kVK_UpArrow: // 0x7E
		result = NSUpArrowFunctionKey;
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// virtualKeyToUnicode

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
