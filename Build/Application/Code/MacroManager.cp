/*###############################################################

	MacroManager.cp
	
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
#include <cctype>
#include <cstdio>
#include <cstring>

// standard-C++ includes
#include <sstream>
#include <string>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CocoaBasic.h>
#include <Console.h>
#include <FileSelectionDialogs.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// resource includes
#include "ApplicationVersion.h"
#include "DialogResources.h"
#include "GeneralResources.h"

// MacTelnet includes
#include "ConstantsRegistry.h"
#include "MacroManager.h"
#include "MenuBar.h"
#include "Network.h"
#include "Preferences.h"
#include "Session.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "URL.h"



#pragma mark Internal Method Prototypes
namespace {

void						macroSetChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
pascal OSStatus				receiveHICommand		(EventHandlerCallRef, EventRef, void*);
Preferences_ContextRef		returnDefaultMacroSet	();

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean						gShowMacrosMenu = false;
ListenerModel_ListenerRef&	gMacroSetMonitor ()		{ static ListenerModel_ListenerRef x = ListenerModel_NewStandardListener(macroSetChanged); return x; }
CarbonEventHandlerWrap&		gMacroCommandsHandler ()	{ static CarbonEventHandlerWrap	x(GetApplicationEventTarget(), receiveHICommand,
																							CarbonEventSetInClass(CarbonEventClass
																													(kEventClassCommand),
																													kEventCommandProcess,
																													kEventCommandUpdateStatus),
																							nullptr/* user data */); return x; }
Preferences_ContextRef&		gCurrentMacroSet ()			{ static Preferences_ContextRef x = returnDefaultMacroSet(); return x; }

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
	return returnDefaultMacroSet();
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
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	// install a command handler if none exists
	gMacroCommandsHandler();
	
	// perform last action for previous context
	if (nullptr == gCurrentMacroSet())
	{
		MenuRef		macrosMenu = MenuBar_ReturnMacrosMenu();
		
		
		// e.g. previous set was None; now, show the Macros menu for the new set
		if (gShowMacrosMenu)
		{
			(OSStatus)ChangeMenuAttributes(macrosMenu, 0/* attributes to set */, kMenuAttrHidden/* attributes to clear */);
		}
		else
		{
			// user preference is to always hide
			(OSStatus)ChangeMenuAttributes(macrosMenu, kMenuAttrHidden/* attributes to set */, 0/* attributes to clear */);
		}
	}
	else
	{
		// remove monitors from the context that is about to be non-current
		prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroAction);
		prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroContents);
		prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroName);
		prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroKeyModifiers);
		prefsResult = Preferences_ContextStopMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroKey);
	}
	
	// set the new current context
	gCurrentMacroSet() = inMacroSetOrNullForNone;
	
	// monitor the new current context so that caches and menus can be updated, etc.
	if (nullptr == gCurrentMacroSet())
	{
		MenuRef		macrosMenu = MenuBar_ReturnMacrosMenu();
		
		
		// e.g. the user selected the None macro set; hide the Macros menu entirely
		(OSStatus)ChangeMenuAttributes(macrosMenu, kMenuAttrHidden/* attributes to set */, 0/* attributes to clear */);
	}
	else
	{
		// technically this only has to be installed once
		static Boolean		gMacroMenuVisibleMonitorInstalled = false;
		if (false == gMacroMenuVisibleMonitorInstalled)
		{
			prefsResult = Preferences_StartMonitoring(gMacroSetMonitor(), kPreferences_TagMacrosMenuVisible, true/* notify of initial value */);
			gMacroMenuVisibleMonitorInstalled = true;
		}
		
		// monitor Preferences for changes to macro settings that are important in the Macro Manager module
		prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroAction,
															true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroContents,
															true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroName,
															true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroKeyModifiers,
															true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_ContextStartMonitoring(gCurrentMacroSet(), gMacroSetMonitor(), kPreferences_TagIndexedMacroKey,
															true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	result = kMacroManager_ResultOK;
	return result;
}// SetCurrentMacros


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
										? SessionFactory_ReturnUserFocusSession()
										: inTargetSessionOrNullForActiveSession;
	Preferences_ContextRef	context = (nullptr == inMacroSetOrNullForActiveSet)
										? gCurrentMacroSet()
										: inMacroSetOrNullForActiveSet;
	
	
	if ((nullptr != context) && (nullptr != session))
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		CFStringRef				actionCFString = nullptr;
		MacroManager_Action		actionPerformed = kMacroManager_ActionSendText;
		
		
		// retrieve action type
		// TEMPORARY - it is smarter to query this only as preferences
		// actually change, i.e. in the monitor callback, and rely
		// only on a cached array at this point
		prefsResult = Preferences_ContextGetDataAtIndex(context, kPreferences_TagIndexedMacroAction,
														inZeroBasedMacroIndex + 1/* one-based */,
														sizeof(actionCFString), &actionCFString,
														true/* search defaults too */, &actualSize);
		if (kPreferences_ResultOK == prefsResult)
		{
			// retrieve action text
			prefsResult = Preferences_ContextGetDataAtIndex(context, kPreferences_TagIndexedMacroContents,
															inZeroBasedMacroIndex + 1/* one-based */,
															sizeof(actionCFString), &actionCFString,
															true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				switch (actionPerformed)
				{
				case kMacroManager_ActionSendText:
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
										Console_WriteValueCharacter("warning, unrecognized backslash escape", nextChar);
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
							SessionRef		newSession = nullptr;
							
							
							newSession = SessionFactory_NewSessionArbitraryCommand(nullptr/* terminal window */, argsCFArray);
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
	MenuRef						macrosMenu = MenuBar_ReturnMacrosMenu();
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	size_t						actualSize = 0;
	
	
	if (kPreferences_TagMacrosMenuVisible == inPreferenceTagThatChanged)
	{
		// global preference
		Boolean		flag = false;
		
		
		prefsResult = Preferences_GetData(inPreferenceTagThatChanged, sizeof(flag), &flag, &actualSize);
		gShowMacrosMenu = flag;
		if (gShowMacrosMenu)
		{
			// the menu should remain hidden if the current macro set is None
			// (it will be shown again if the user switches the macro set)
			if (nullptr != gCurrentMacroSet())
			{
				(OSStatus)ChangeMenuAttributes(macrosMenu, 0/* attributes to set */, kMenuAttrHidden/* attributes to clear */);
			}
		}
		else
		{
			// hide menu immediately
			(OSStatus)ChangeMenuAttributes(macrosMenu, kMenuAttrHidden/* attributes to set */, 0/* attributes to clear */);
		}
	}
	else if (nullptr == prefsContext)
	{
		Console_WriteLine("warning, callback was invoked for nonexistent macro set");
	}
	else
	{
		switch (inPreferenceTagThatChanged)
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
			// update every menu item to match the macro names of the set
			// TEMPORARY: it would be nicer to have a notifier that returns a specific index, too...
			for (UInt32 i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
			{
				CFStringRef		nameCFString = nullptr;
				
				
				// retrieve name
				prefsResult = Preferences_ContextGetDataAtIndex(prefsContext, inPreferenceTagThatChanged,
																i/* one-based index */, sizeof(nameCFString), &nameCFString,
																true/* search defaults too */, &actualSize);
				if (kPreferences_ResultOK == prefsResult)
				{
					(OSStatus)SetMenuItemTextWithCFString(macrosMenu, i, nameCFString);
					CFRelease(nameCFString), nameCFString = nullptr;
				}
			}
			break;
		
		case kPreferences_TagIndexedMacroKey:
			// update every menu item to match the macro keys of the set
			// TEMPORARY: it would be nicer to have a notifier that returns a specific index, too...
			for (UInt32 i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
			{
				MacroManager_KeyID		macroKeyID = 0;
				
				
				// retrieve key modifiers
				prefsResult = Preferences_ContextGetDataAtIndex(prefsContext, inPreferenceTagThatChanged,
																i/* one-based index */, sizeof(macroKeyID), &macroKeyID,
																true/* search defaults too */, &actualSize);
				if (kPreferences_ResultOK == prefsResult)
				{
					UInt16 const	kKeyCode = MacroManager_KeyIDKeyCode(macroKeyID);
					Boolean const	kIsVirtualKey = MacroManager_KeyIDIsVirtualKey(macroKeyID);
					
					
					// reset menu item
					(OSStatus)ChangeMenuItemAttributes(macrosMenu, i, 0/* attributes to set */,
														kMenuItemAttrUseVirtualKey/* attributes to clear */);
					(OSStatus)SetMenuItemCommandKey(macrosMenu, i, false/* virtual key */, '\0'/* character */);
					(OSStatus)SetMenuItemKeyGlyph(macrosMenu, i, kMenuNullGlyph);
					
					// apply new key equivalent
					(OSStatus)SetMenuItemCommandKey(macrosMenu, i, kIsVirtualKey, kKeyCode);
				}
			}
			break;
		
		case kPreferences_TagIndexedMacroKeyModifiers:
			// update every menu item to match the macro key modifiers of the set
			// TEMPORARY: it would be nicer to have a notifier that returns a specific index, too...
			for (UInt32 i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
			{
				UInt32		modifiers = 0;
				
				
				// retrieve key modifiers
				prefsResult = Preferences_ContextGetDataAtIndex(prefsContext, inPreferenceTagThatChanged,
																i/* one-based index */, sizeof(modifiers), &modifiers,
																true/* search defaults too */, &actualSize);
				if (kPreferences_ResultOK == prefsResult)
				{
					UInt8		abbreviatedModifiers = kMenuNoCommandModifier;
					
					
					// NOTE: if a command key has not been assigned to the item,
					// setting its modifiers will have no visible effect
					if (modifiers & cmdKey) abbreviatedModifiers &= ~kMenuNoCommandModifier;
					if (modifiers & controlKey) abbreviatedModifiers |= kMenuControlModifier;
					if (modifiers & optionKey) abbreviatedModifiers |= kMenuOptionModifier;
					if (modifiers & shiftKey) abbreviatedModifiers |= kMenuShiftModifier;
					(OSStatus)SetMenuItemModifiers(macrosMenu, i, abbreviatedModifiers);
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
}// macroSetChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand" for
macro set selection commands.

(4.0)
*/
pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// donÕt claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// Determine if a menu command needs updating.
				switch (received.commandID)
				{
				case kCommandMacroSetNone:
					{
						MacroManager_Result		macrosResult = kMacroManager_ResultOK;
						
						
						macrosResult = MacroManager_SetCurrentMacros(nullptr);
						if (false == macrosResult.ok())
						{
							Sound_StandardAlert();
						}
						result = noErr;
					}
					break;
				
				case kCommandMacroSetDefault:
					{
						MacroManager_Result		macrosResult = kMacroManager_ResultOK;
						
						
						macrosResult = MacroManager_SetCurrentMacros(MacroManager_ReturnDefaultMacros());
						if (false == macrosResult.ok())
						{
							Sound_StandardAlert();
						}
						result = noErr;
					}
					break;
				
				case kCommandMacroSetByFavoriteName:
					{
						// use the specified preferences
						Boolean		isError = true;
						
						
						if (received.attributes & kHICommandFromMenu)
						{
							CFStringRef		collectionName = nullptr;
							
							
							if (noErr == CopyMenuItemTextAsCFString(received.menu.menuRef, received.menu.menuItemIndex, &collectionName))
							{
								Preferences_ContextRef		namedSettings = Preferences_NewContextFromFavorites
																			(kPreferences_ClassMacroSet, collectionName);
								
								
								if (nullptr != namedSettings)
								{
									MacroManager_Result		macrosResult = kMacroManager_ResultOK;
									
									
									macrosResult = MacroManager_SetCurrentMacros(namedSettings);
									isError = (false == macrosResult.ok());
								}
								CFRelease(collectionName), collectionName = nullptr;
							}
						}
						
						if (isError)
						{
							// failed...
							Sound_StandardAlert();
						}
						
						result = noErr;
					}
					break;
				
				default:
					// ???
					break;
				}
				break;
			
			case kEventCommandUpdateStatus:
				// Execute a command selected from a menu (or sent from a control, etc.).
				//
				// IMPORTANT: This could imply ANY form of menu selection, whether from
				//            the menu bar, from a contextual menu, or from a pop-up menu!
				switch (received.commandID)
				{
				case kCommandMacroSetNone:
					if (received.attributes & kHICommandFromMenu)
					{
						MenuRef			menu = received.menu.menuRef;
						MenuItemIndex	itemIndex = received.menu.menuItemIndex;
						
						
						CheckMenuItem(menu, itemIndex, (nullptr == gCurrentMacroSet()));
						
						// fall through to the other handler (MenuBar module), which
						// will also fix the active state of this item
						result = eventNotHandledErr;
					}
					break;
				
				case kCommandMacroSetDefault:
					if (received.attributes & kHICommandFromMenu)
					{
						MenuRef			menu = received.menu.menuRef;
						MenuItemIndex	itemIndex = received.menu.menuItemIndex;
						
						
						CheckMenuItem(menu, itemIndex, (returnDefaultMacroSet() == gCurrentMacroSet()));
						
						// fall through to the other handler (MenuBar module), which
						// will also fix the active state of this item
						result = eventNotHandledErr;
					}
					break;
				
				case kCommandMacroSetByFavoriteName:
					if (received.attributes & kHICommandFromMenu)
					{
						MenuRef			menu = received.menu.menuRef;
						MenuItemIndex	itemIndex = received.menu.menuItemIndex;
						Boolean			isChecked = false;
						
						
						if (nullptr != gCurrentMacroSet())
						{
							CFStringRef		collectionName = nullptr;
							CFStringRef		menuItemName = nullptr;
							
							
							if (noErr == CopyMenuItemTextAsCFString(menu, itemIndex, &menuItemName))
							{
								// the context name should not be released
								Preferences_Result		prefsResult = Preferences_ContextGetName(gCurrentMacroSet(), collectionName);
								
								
								if (kPreferences_ResultOK == prefsResult)
								{
									isChecked = (kCFCompareEqualTo == CFStringCompare(collectionName, menuItemName, 0/* options */));
								}
								CFRelease(menuItemName), menuItemName = nullptr;
							}
						}
						CheckMenuItem(menu, itemIndex, isChecked);
						
						// fall through to the other handler (MenuBar module), which
						// will also fix the active state of this item
						result = eventNotHandledErr;
					}
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
In order to initialize a static (global) variable immediately,
this function was written to return a value.  Otherwise, it is
no different than calling Preferences_GetDefaultContext()
directly for a macro set class.

(4.0)
*/
Preferences_ContextRef
returnDefaultMacroSet ()
{
	Preferences_ContextRef		result = nullptr;
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_GetDefaultContext(&result, kPreferences_ClassMacroSet);
	assert(kPreferences_ResultOK == prefsResult);
	return result;
}// returnDefaultMacroSet


} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
