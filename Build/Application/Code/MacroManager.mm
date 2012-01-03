/*!	\file MacroManager.mm
	\brief An API for user-specified keyboard short-cuts.
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
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaBasic.h>
#import <Console.h>
#import <FileSelectionDialogs.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "ConstantsRegistry.h"
#import "MenuBar.h"
#import "Network.h"
#import "Preferences.h"
#import "QuillsPrefs.h"
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "URL.h"



#pragma mark Internal Method Prototypes
namespace {

void						macroSetChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void						preferenceChanged		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
Preferences_ContextRef		returnDefaultMacroSet	(Boolean);
NSMenu*						returnMacrosMenu		();
unichar						virtualKeyToUnicode		(UInt16);

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_ListenerRef&	gMacroSetMonitor ()		{ static ListenerModel_ListenerRef x = ListenerModel_NewStandardListener(macroSetChanged); return x; }
ListenerModel_ListenerRef&	gPreferencesMonitor ()	{ static ListenerModel_ListenerRef x = ListenerModel_NewStandardListener(preferenceChanged); return x; }
Preferences_ContextRef&		gCurrentMacroSet ()		{ static Preferences_ContextRef x = returnDefaultMacroSet(true/* retain */); return x; }

} // anonymous namespace



#pragma mark Public Methods

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
		
		
		// perform last action for previous context
		if (nullptr != gCurrentMacroSet())
		{
			// remove monitors from the context that is about to be non-current
			for (UInt16 i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
			{
				prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, i));
				prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, i));
				prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, i));
				prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
																Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, i));
				prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(),
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
			for (UInt16 i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
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
		
		result = kMacroManager_ResultOK;
	}
	
	return result;
}// SetCurrentMacros


/*!
Sets the title and key equivalent of the specified menu item
based on the given macro in the set.

(4.0)
*/
void
MacroManager_UpdateMenuItem		(NSMenuItem*				inMenuItem,
								 UInt16						inOneBasedMacroIndex,
								 Preferences_ContextRef		inMacroSetOrNullForActiveSet)
{
	Preferences_ContextRef	prefsContext = (nullptr == inMacroSetOrNullForActiveSet)
											? gCurrentMacroSet()
											: inMacroSetOrNullForActiveSet;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFStringRef				nameCFString = nullptr;
	MacroManager_KeyID		macroKeyID = 0;
	UInt32					modifiers = 0;
	size_t					actualSize = 0;
	
	
	// retrieve name
	prefsResult = Preferences_ContextGetData
					(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, inOneBasedMacroIndex),
						sizeof(nameCFString), &nameCFString, false/* search defaults too */, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		[inMenuItem setTitle:(NSString*)nameCFString];
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
					(prefsContext, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, inOneBasedMacroIndex),
						sizeof(macroKeyID), &macroKeyID, false/* search defaults too */, &actualSize);
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
						Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, inOneBasedMacroIndex),
						sizeof(modifiers), &modifiers, false/* search defaults too */, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		unsigned int	abbreviatedModifiers = 0;
		
		
		// NOTE: if a command key has not been assigned to the item,
		// setting its modifiers will have no visible effect
		if (modifiers & cmdKey) abbreviatedModifiers |= NSCommandKeyMask;
		if (modifiers & controlKey) abbreviatedModifiers |= NSControlKeyMask;
		if (modifiers & optionKey) abbreviatedModifiers |= NSAlternateKeyMask;
		if (modifiers & shiftKey) abbreviatedModifiers |= NSShiftKeyMask;
		[inMenuItem setKeyEquivalentModifierMask:abbreviatedModifiers];
	}
	else
	{
		// set a dummy value
		[inMenuItem setKeyEquivalentModifierMask:0];
	}
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
	
	
	if ((nullptr != context) && (nullptr != session))
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		CFStringRef				actionCFString = nullptr;
		MacroManager_Action		actionPerformed = kMacroManager_ActionSendTextProcessingEscapes;
		
		
		// retrieve action type
		// TEMPORARY - it is smarter to query this only as preferences
		// actually change, i.e. in the monitor callback, and rely
		// only on a cached array at this point
		prefsResult = Preferences_ContextGetData
						(context, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, inZeroBasedMacroIndex + 1),
							sizeof(actionPerformed), &actionPerformed, true/* search defaults too */, &actualSize);
		if (kPreferences_ResultOK == prefsResult)
		{
			// retrieve action text
			prefsResult = Preferences_ContextGetData
							(context, Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, inZeroBasedMacroIndex + 1),
								sizeof(actionCFString), &actionCFString, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				switch (actionPerformed)
				{
				case kMacroManager_ActionSendTextVerbatim:
					// send string to the session as-is
					Session_UserInputCFString(session, actionCFString);
					result = kMacroManager_ResultOK;
					break;
				
				case kMacroManager_ActionSendTextProcessingEscapes:
					{
						// TEMPORARY - translate everything here for now, but the plan is to
						// eventually monitor preference changes and pre-scan and cache a
						// mostly-processed string (e.g. static escape sequences translated)
						// so that very little has to be done at this point
						CFIndex const			kLength = CFStringGetLength(actionCFString);
						CFStringInlineBuffer	charBuffer;
						CFRetainRelease			finalCFString(CFStringCreateMutable(kCFAllocatorDefault, 0/* limit */), true/* is retained */);
						Boolean					substitutionError = false;
						UniChar					octalSequenceCharCode = '\0'; // overwritten each time a \0nn is processed
						SInt16					readOctal = -1;		// if 0, a \0 was read, and the first "n" (in \0nn) might be next;
																	// if 1, a \1 was read, and the first "n" (in \1nn) might be next;
						
						
						CFStringInitInlineBuffer(actionCFString, &charBuffer, CFRangeMake(0, kLength));
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
									if (readOctal == 1) octalSequenceCharCode = '\100';
									else octalSequenceCharCode = '\0';
									
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
											TerminalWindowRef const		kTerminalWindow = Session_ReturnActiveTerminalWindow(session);
											
											
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
																									CFSTR("%u"), kLineCount), true/* is retained */);
													
													
													CFStringAppend(finalCFString.returnCFMutableStringRef(), numberCFString.returnCFStringRef());
													substitutionError = false;
												}
											}
										}
										++i; // skip special sequence character
										break;
									
									case 'e':
										// escape; equivalent to \033
										CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\033"));
										++i; // skip special sequence character
										break;
									
									case 'i':
										// IP address
										{
											std::vector< CFRetainRelease >		addressList;
											
											
											if (Network_CopyIPAddresses(addressList))
											{
												// TEMPORARY - should there be a way to select among available addresses?
												assert(false == addressList.empty());
												CFStringAppend(finalCFString.returnCFMutableStringRef(), addressList[0].returnCFStringRef());
											}
											else
											{
												substitutionError = true;
											}
										}
										++i; // skip special sequence character
										break;
									
									case 'n':
										// new-line
										// TEMPORARY - should send the sanctioned new-line sequence for the session
										CFStringAppend(finalCFString.returnCFMutableStringRef(), CFSTR("\n"));
										++i; // skip special sequence character
										break;
									
									case 'r':
										// carriage return without line feed
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
										Console_Warning(Console_WriteValueCharacter, "unrecognized backslash escape", nextChar);
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
						
						if (substitutionError)
						{
							Console_WriteLine("macro was not handled due to substitution errors");
						}
						else
						{
							// at last, send the edited string to the session!
							Session_UserInputCFString(session, finalCFString.returnCFStringRef());
							result = kMacroManager_ResultOK;
						}
					}
					break;
				
				case kMacroManager_ActionHandleURL:
					{
						OSStatus	error = URL_ParseCFString(actionCFString);
						
						
						if (noErr == error) result = kMacroManager_ResultOK;
					}
					result = kMacroManager_ResultOK;
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
	size_t						actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeNumberOfContexts:
		// if the current macro set has been destroyed, stop using it!
		if (false == Preferences_ContextIsValid(MacroManager_ReturnCurrentMacros()))
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
	NSMenuItem*		macrosMenuItem = [mainMenu itemWithTag:kMenuIDMacros];
	assert(nil != macrosMenuItem);
	NSMenu*			result = [macrosMenuItem submenu];
	assert(nil != result);
	
	
	return result;
}// returnMacrosMenu


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
	case 0x24:
		result = NSCarriageReturnCharacter;
		break;
	
	case 0x33:
		result = NSBackspaceCharacter;
		break;
	
	case 0x35:
		result = 0x001B; // the “escape” key
		break;
	
	case 0x47:
		result = 0x2327; // the “clear” key
		break;
	
	case 0x4C:
		result = NSEnterCharacter;
		break;
	
	case 0x60:
		result = NSF5FunctionKey;
		break;
	
	case 0x61:
		result = NSF6FunctionKey;
		break;
	
	case 0x62:
		result = NSF7FunctionKey;
		break;
	
	case 0x63:
		result = NSF3FunctionKey;
		break;
	
	case 0x64:
		result = NSF8FunctionKey;
		break;
	
	case 0x65:
		result = NSF9FunctionKey;
		break;
	
	case 0x67:
		result = NSF11FunctionKey;
		break;
	
	case 0x69:
		result = NSF13FunctionKey;
		break;
	
	case 0x6A:
		result = NSF16FunctionKey;
		break;
	
	case 0x6B:
		result = NSF14FunctionKey;
		break;
	
	case 0x6D:
		result = NSF10FunctionKey;
		break;
	
	case 0x6F:
		result = NSF12FunctionKey;
		break;
	
	case 0x71:
		result = NSF15FunctionKey;
		break;
	
	case 0x73:
		result = NSHomeFunctionKey;
		break;
	
	case 0x74:
		result = NSPageUpFunctionKey;
		break;
	
	case 0x75:
		result = NSDeleteCharacter;
		break;
	
	case 0x76:
		result = NSF4FunctionKey;
		break;
	
	case 0x77:
		result = NSEndFunctionKey;
		break;
	
	case 0x78:
		result = NSF2FunctionKey;
		break;
	
	case 0x79:
		result = NSPageDownFunctionKey;
		break;
	
	case 0x7A:
		result = NSF1FunctionKey;
		break;
	
	case 0x7B:
		result = NSLeftArrowFunctionKey;
		break;
	
	case 0x7C:
		result = NSRightArrowFunctionKey;
		break;
	
	case 0x7D:
		result = NSDownArrowFunctionKey;
		break;
	
	case 0x7E:
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
