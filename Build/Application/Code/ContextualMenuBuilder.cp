/*###############################################################

	ContextualMenuBuilder.cp
	
	This module generates context-based pop-up menu items
	appropriately for a particular object (such as a window).
	Contextual menus automatically take into account the state
	of menu items, so an item can appear in a contextual menu
	only if it exists in the menu bar and is enabled.  However,
	contextual menu items are allowed to use different item text
	to represent a command, to appear more context-sensitive
	(e.g. “Hide This Window”, not “Hide Frontmost Window”).
	
	IMPORTANT: Contextual menu implementation requires thought.
	           Ask yourself the following questions...
	
	- If you are adding an item to a contextual menu, is the
	  item available in a regular, menu bar menu as well?
			   
	- How many items are in this contextual menu?  Do NOT simply
	  add every *possible* item to a contextual menu!  Restrict
	  your choices to those items which will most certainly be
	  useful to the user, either because they are frequently
	  used, or because they are naturally suited to a pop-up
	  menu (such as “Copy”).
			   
	- With the *sole* exception of possibly the Help item, never
	  show disabled menu items in any contextual menu.  Items
	  which are not available just make menus unnecessarily
	  large, and do not help the user in any way.
	
	- How long does it take your context handler to do its work?
	  Remember, all of that processing time is taken up *after*
	  the user Control-clicks, and delays the display of the
	  menu to the user!
	
	MacTelnet
		© 1998-2010 by Kevin Grant.
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

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <ContextSensitiveMenu.h>
#include <MemoryBlocks.h>
#include <WindowInfo.h>

// resource includes
#include "MenuResources.h"

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "Clipboard.h"
#include "ContextualMenuBuilder.h"
#include "DialogUtilities.h"
#include "HelpSystem.h"
#include "MenuBar.h"
#include "TerminalView.h"
#include "TerminalWindow.h"
#include "UIStrings.h"



#pragma mark Variables
namespace {

UInt32	gHelpType = kCMHelpItemNoHelp;	//!< this could change depending on available system libraries at runtime

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void		buildAboutBoxContextualMenu				(MenuRef, HIWindowRef);
void		buildClipboardWindowContextualMenu		(MenuRef, HIWindowRef);
void		buildEmptyContextualMenu				(MenuRef, HIWindowRef);
void		buildSessionStatusWindowContextualMenu	(MenuRef, HIWindowRef);
void		buildTerminalWindowContextualMenu		(MenuRef, HIWindowRef);
OSStatus	populateMenuForView						(HIViewRef, MenuRef, ContextualMenuBuilder_AddItemsProcPtr, AEDesc&);
OSStatus	populateMenuForWindow					(HIWindowRef, WindowPartCode, MenuRef,
													 ContextualMenuBuilder_AddItemsProcPtr, AEDesc&);

} // anonymous namespace



#pragma mark Public Methods

/*!
Displays a contextual menu that is appropriate for the
specified view.  Call this method from a control
contextual menu click handler.

Most views do not allow contextual menus, and what you
really need is a menu for the entire window; see
ContextualMenuBuilder_DisplayMenuForWindow().

(3.1)
*/
OSStatus
ContextualMenuBuilder_DisplayMenuForView	(HIViewRef								inWhichView,
											 EventRef								inContextualMenuClickEvent,
											 ContextualMenuBuilder_AddItemsProcPtr	inItemAdderOrNull)
{
	MenuRef					menu = nullptr;
	HelpSystem_KeyPhrase	keyPhrase = kHelpSystem_KeyPhraseDefault; // determines the Help menu item text and the search string
	Boolean					failed = false;
	Boolean					makeCustomMenu = true;
	OSStatus				result = noErr;
	
	
	// WARNING: Do not call HandleControlContextualMenuClick() here!
	// This code is relied upon by contextual menu event handlers,
	// and HandleControlContextualMenuClick() triggers contextual menu
	// events.  Endless looping is...inconvenient.
	
	if (makeCustomMenu)
	{
		// create a menu with a unique menu ID in the allowed range
		menu = ContextSensitiveMenu_New((SInt16*)nullptr/* menu ID */);
		if (nullptr == menu)
		{
			failed = true;
			result = memFullErr;
		}
	}
	
	if ((makeCustomMenu) && (nullptr != menu))
	{
		OSStatus	error = noErr;
		AEDesc		contentsDesc;
		
		
		// add appropriate items
		error = populateMenuForView(inWhichView, menu, inItemAdderOrNull, contentsDesc);
		
		// now that all items have been added, notify the Contextual Menu modules to clean up
		ContextSensitiveMenu_DoneAddingItems(menu);
		
		if (noErr == error)
		{
			UInt32		selectionType = 0;
			SInt16		menuID = 0;
			UInt16		itemNumber = 0;
			Point		globalMouse;
			
			
			// determine the mouse location
			{
				HIViewRef	contentView = nullptr;
				
				
				error = HIViewFindByID(HIViewGetRoot(HIViewGetWindow(inWhichView)),
										kHIViewWindowContentID, &contentView);
				if (noErr == error)
				{
					HIPoint		mouseLocation = CGPointZero;
					
					
					// determine window-relative location, and convert it to content-local
					error = CarbonEventUtilities_GetEventParameter(inContextualMenuClickEvent,
																	kEventParamWindowMouseLocation,
																	typeHIPoint, mouseLocation);
					if (noErr == error)
					{
						error = HIViewConvertPoint(&mouseLocation, nullptr/* source view or null for window */,
													contentView/* destination view */);
						if (noErr == error)
						{
							// the content-local mouse coordinates match QuickDraw “local”
							// coordinates, and can finally be converted to a global point
							globalMouse.h = STATIC_CAST(mouseLocation.x, SInt16);
							globalMouse.v = STATIC_CAST(mouseLocation.y, SInt16);
							(Point*)QDLocalToGlobalPoint(GetWindowPort(GetControlOwner(inWhichView)), &globalMouse);
						}
					}
				}
			}
			
			// display and track the menu, getting the name of the Help item from the MacTelnet resource file
			if (noErr == error)
			{
				HelpSystem_Result	helpSystemResult = kHelpSystem_ResultOK;
				CFStringRef			helpCFString = nullptr;
				Str255				helpStringBuffer;
				ConstStringPtr		helpStringAddress = nullptr; // by default do not specify the help item
				
				
				// try to retrieve the name of the help system
				helpSystemResult = HelpSystem_CopyKeyPhraseCFString(keyPhrase, helpCFString);
				if (kHelpSystem_ResultOK == helpSystemResult)
				{
					// retrieved CFString; must convert to Pascal string due to limitations in ContextualMenuSelect()
					Boolean		conversionOK = false;
					
					
					conversionOK = CFStringGetPascalString(helpCFString, helpStringBuffer, sizeof(helpStringBuffer),
															CFStringGetSystemEncoding());
					if (conversionOK)
					{
						// success; set the pointer to the buffer location
						helpStringAddress = helpStringBuffer;
					}
					CFRelease(helpCFString), helpCFString = nullptr;
				}
				result = ContextualMenuSelect(menu, globalMouse, false/* reserved */, gHelpType,
												helpStringAddress, &contentsDesc, &selectionType, &menuID, &itemNumber);
			}
			
			if (userCanceledErr != result) failed = true;
			else
			{
				if (kCMNothingSelected != selectionType)
				{
					if (kCMShowHelpSelected == selectionType)
					{
						// TMP - just pick something to search for
						HelpSystem_DisplayHelpFromKeyPhrase(keyPhrase);
					}
					else if (kCMMenuItemSelected == selectionType)
					{
						UInt32		commandID = 0;
						
						
						if (kContextSensitiveMenu_DefaultMenuID == menuID)
						{
							if (noErr == GetMenuItemCommandID(menu, itemNumber, &commandID))
							{
								failed = (!MenuBar_HandleMenuCommandByID(commandID));
							}
							else failed = true;
						}
					}
					else failed = true;
				}
			}
		}
		ContextSensitiveMenu_Dispose(&menu);
		
		// do not allow focus to remain on drawers; set user focus
		// to the parent window, if appropriate
		{
			HIWindowRef		parentWindow = GetDrawerParent(GetUserFocusWindow());
			
			
			if (nullptr != parentWindow)
			{
				SetUserFocusWindow(parentWindow);
			}
		}
	}
	
	return result;
}// DisplayMenuForView


/*!
Displays a contextual menu for the specified window
that is appropriate for the window.  Call this method
after you have determined that a contextual menu display
event has occurred in the user focus window, and you
know what part of the window was hit.

(3.1)
*/
OSStatus
ContextualMenuBuilder_DisplayMenuForWindow	(HIWindowRef							inWhichWindow,
											 EventRecord*							inoutEventPtr,
											 WindowPartCode							inWindowPart,
											 ContextualMenuBuilder_AddItemsProcPtr	inItemAdderOrNull)
{
	MenuRef					menu = nullptr;
	HelpSystem_KeyPhrase	keyPhrase = kHelpSystem_KeyPhraseDefault; // determines the Help menu item text and the search string
	Boolean					failed = false;
	Boolean					makeCustomMenu = true;
	OSStatus				result = noErr;
	
	
	// WARNING: Do not call HandleControlContextualMenuClick() here!
	// This code is relied upon by contextual menu event handlers,
	// and HandleControlContextualMenuClick() triggers contextual menu
	// events.  Endless looping is...inconvenient.
	
	if (makeCustomMenu)
	{
		// create a menu with a unique menu ID in the allowed range
		menu = ContextSensitiveMenu_New((SInt16*)nullptr/* menu ID */);
		if (nullptr == menu)
		{
			failed = true;
			result = memFullErr;
		}
	}
	
	if ((makeCustomMenu) && (nullptr != menu))
	{
		OSStatus	error = noErr;
		AEDesc		contentsDesc;
		Boolean		addedBalloonItem = false;
		
		
		// add a Balloon Help item for dialogs
		if (kDialogWindowKind == GetWindowKind(inWhichWindow))
		{
			ContextSensitiveMenu_Item	itemInfo;
			
			
			ContextSensitiveMenu_InitItem(&itemInfo);
			if (UIStrings_Copy(HMAreHelpTagsDisplayed()
								? kUIStrings_HelpSystemHideTagsCommandName
								: kUIStrings_HelpSystemShowTagsCommandName, itemInfo.commandText).ok())
			{
				addedBalloonItem = (noErr == ContextSensitiveMenu_AddItem(menu, &itemInfo));
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		// add appropriate items
		error = populateMenuForWindow(inWhichWindow, inWindowPart, menu, inItemAdderOrNull, contentsDesc);
		
		// now that all items have been added, notify the Contextual Menu modules to clean up
		ContextSensitiveMenu_DoneAddingItems(menu);
		
		if (noErr == error)
		{
			UInt32		selectionType = 0;
			SInt16		menuID = 0;
			UInt16		itemNumber = 0;
			
			
			// display and track the menu, getting the name of the Help item from the MacTelnet resource file
			{
				HelpSystem_Result	helpSystemResult = kHelpSystem_ResultOK;
				CFStringRef			helpCFString = nullptr;
				Str255				helpStringBuffer;
				ConstStringPtr		helpStringAddress = nullptr; // by default do not specify the help item
				
				
				// try to retrieve the name of the help system
				helpSystemResult = HelpSystem_CopyKeyPhraseCFString(keyPhrase, helpCFString);
				if (kHelpSystem_ResultOK == helpSystemResult)
				{
					// retrieved CFString; must convert to Pascal string due to limitations in ContextualMenuSelect()
					Boolean		conversionOK = false;
					
					
					conversionOK = CFStringGetPascalString(helpCFString, helpStringBuffer, sizeof(helpStringBuffer),
															CFStringGetSystemEncoding());
					if (conversionOK)
					{
						// success; set the pointer to the buffer location
						helpStringAddress = helpStringBuffer;
					}
					CFRelease(helpCFString), helpCFString = nullptr;
				}
				result = ContextualMenuSelect(menu, inoutEventPtr->where, false/* reserved */, gHelpType,
												helpStringAddress, &contentsDesc, &selectionType, &menuID, &itemNumber);
			}
			
			if (userCanceledErr != result) failed = true;
			else
			{
				if (kCMNothingSelected != selectionType)
				{
					if (kCMShowHelpSelected == selectionType)
					{
						// TMP - just pick something to search for
						HelpSystem_DisplayHelpFromKeyPhrase(keyPhrase);
					}
					else if (kCMMenuItemSelected == selectionType)
					{
						UInt32		commandID = 0;
						
						
						if (kContextSensitiveMenu_DefaultMenuID == menuID)
						{
							if ((addedBalloonItem) && (1 == itemNumber)) // the Balloon Help item was selected
							{
								HMSetHelpTagsDisplayed(!HMAreHelpTagsDisplayed());
							}
							else if (noErr == GetMenuItemCommandID(menu, itemNumber, &commandID)) // otherwise, let the handler take it
							{
								failed = (!MenuBar_HandleMenuCommandByID(commandID));
							}
							else failed = true;
						}
					}
					else failed = true;
				}
			}
		}
		ContextSensitiveMenu_Dispose(&menu);
	}
	
	return result;
}// DisplayMenuForWindow


/*!
Determines the kind of help item that will be
available in displayed contextual menus.  You
can specify this information using
ContextualMenuBuilder_SetMenuHelpType().

(3.0)
*/
UInt32
ContextualMenuBuilder_ReturnMenuHelpType ()
{
	return gHelpType;
}// ReturnMenuHelpType


/*!
Specifies the kind of help item that will be
available in displayed contextual menus.  You can
determine this information later using
ContextualMenuBuilder_ReturnMenuHelpType().

(3.0)
*/
void
ContextualMenuBuilder_SetMenuHelpType	(UInt32		inNewHelpType)
{
	gHelpType = inNewHelpType;
}// SetMenuHelpType


#pragma mark Internal Methods
namespace {

/*!
This routine will look at the frontmost window as being
the Standard About Box, and will construct a contextual
menu appropriate for it.  The given window must have
Window Info associated with it (see "WindowInfo.cp").

(3.0)
*/
void
buildAboutBoxContextualMenu		(MenuRef		inMenu,
								 WindowRef		UNUSED_ARGUMENT(inWhichWindow))
{
	ContextSensitiveMenu_Item	itemInfo;
	
	
	// window-related menu items
	ContextSensitiveMenu_NewItemGroup(inMenu);
	
	ContextSensitiveMenu_InitItem(&itemInfo);
	itemInfo.commandID = kCommandCloseConnection;
	if (Commands_IsCommandEnabled(itemInfo.commandID))
	{
		if (UIStrings_Copy(kUIStrings_ContextualMenuCloseThisWindow, itemInfo.commandText).ok())
		{
			(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Close This Window”
			CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
		}
	}
}// buildAboutBoxContextualMenu


/*!
This routine will look at the frontmost window as being
the Clipboard, and will construct a contextual menu
appropriate for it.  The given window must have
Window Info associated with it (see "WindowInfo.cp").

(3.0)
*/
void
buildClipboardWindowContextualMenu	(MenuRef		inMenu,
									 WindowRef		UNUSED_ARGUMENT(inWhichWindow))
{
	ContextSensitiveMenu_Item	itemInfo;
	
	
	// text-editing-related menu items
	ContextSensitiveMenu_NewItemGroup(inMenu);
	
	ContextSensitiveMenu_InitItem(&itemInfo);
	itemInfo.commandID = kCommandHandleURL;
	if (Commands_IsCommandEnabled(itemInfo.commandID))
	{
		if (UIStrings_Copy(kUIStrings_ContextualMenuOpenThisResource, itemInfo.commandText).ok())
		{
			(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Open This Resource (URL)”
			CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
		}
	}
	
	// window-related menu items
	ContextSensitiveMenu_NewItemGroup(inMenu);
	
	ContextSensitiveMenu_InitItem(&itemInfo);
	itemInfo.commandID = kCommandCloseConnection;
	if (Commands_IsCommandEnabled(itemInfo.commandID))
	{
		if (UIStrings_Copy(kUIStrings_ContextualMenuCloseThisWindow, itemInfo.commandText).ok())
		{
			(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Close This Window”
			CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
		}
	}
}// buildClipboardWindowContextualMenu


/*!
This routine will look at the frontmost window as one that
is not recognized, and will construct a contextual menu
appropriate for it.  The given window must have
Window Info associated with it (see "WindowInfo.cp").

(3.0)
*/
void
buildEmptyContextualMenu	(MenuRef		UNUSED_ARGUMENT(inMenu),
							 WindowRef		UNUSED_ARGUMENT(inWhichWindow))
{
	//ContextSensitiveMenu_Item	itemInfo;
	//MenuRef					menuBarMenu = nullptr;
	
	
	
}// buildEmptyContextualMenu


/*!
This routine will look at the frontmost window as being
the Session Status window, and will construct a contextual
menu appropriate for it.  The given window must have
Window Info associated with it (see "WindowInfo.cp").

(3.0)
*/
void
buildSessionStatusWindowContextualMenu	(MenuRef		inMenu,
										 WindowRef		UNUSED_ARGUMENT(inWhichWindow))
{
	ContextSensitiveMenu_Item	itemInfo;
	
	
	// window-related menu items
	ContextSensitiveMenu_NewItemGroup(inMenu);
	
	ContextSensitiveMenu_InitItem(&itemInfo);
	itemInfo.commandID = kCommandCloseConnection;
	if (Commands_IsCommandEnabled(itemInfo.commandID))
	{
		if (UIStrings_Copy(kUIStrings_ContextualMenuCloseThisWindow, itemInfo.commandText).ok())
		{
			(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Close This Window”
			CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
		}
	}
}// buildSessionStatusWindowContextualMenu


/*!
This routine will look at the frontmost window as a
terminal window, determine its characteristics and
construct a contextual menu appropriate for its
current context.  To keep menus short, this routine
builds two types of menus: one for text selections,
and one if nothing is selected.  If text is selected,
this routine assumes that the user will most likely
not want any of the more-general items, such as
“Set Window Title”.  Otherwise, these general items
are available if applicable.

(3.0)
*/
void
buildTerminalWindowContextualMenu	(MenuRef		inMenu,
									 WindowRef		inWhichWindow)
{
	ContextSensitiveMenu_Item	itemInfo;
	TerminalWindowRef			terminalWindow = TerminalWindow_ReturnFromWindow(inWhichWindow);
	TerminalViewRef				view =	TerminalWindow_ReturnViewWithFocus(terminalWindow);
	Boolean						isTextSelected = false;
	
	
	isTextSelected = TerminalView_TextSelectionExists(view);
	
	if (isTextSelected) // no need to keep checking for context, since the same context applies to all items!
	{
		Boolean		selectionIsURL = Commands_IsCommandEnabled(kCommandHandleURL);
		
		
		// URL commands
		ContextSensitiveMenu_NewItemGroup(inMenu);
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandHandleURL;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuOpenThisResource, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Open This Resource (URL)”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		// clipboard-related text selection commands
		ContextSensitiveMenu_NewItemGroup(inMenu);
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandCopy;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuCopyToClipboard, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Copy to Clipboard”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		// if the selection appears to be exactly a URL, it is highly
		// unlikely that normal text selection commands apply
		unless (selectionIsURL)
		{
			ContextSensitiveMenu_InitItem(&itemInfo);
			itemInfo.commandID = kCommandCopyTable;
			if (Commands_IsCommandEnabled(itemInfo.commandID))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuCopyUsingTabsForSpaces, itemInfo.commandText).ok())
				{
					(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Copy Using Tabs For Spaces”
					CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
				}
			}
			
			ContextSensitiveMenu_InitItem(&itemInfo);
			itemInfo.commandID = kCommandSaveText;
			if (Commands_IsCommandEnabled(itemInfo.commandID))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuSaveSelectedText, itemInfo.commandText).ok())
				{
					(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Save Selected Text…”
					CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
				}
			}
			
			// other text-selection-related commands
			ContextSensitiveMenu_NewItemGroup(inMenu);
			
			ContextSensitiveMenu_InitItem(&itemInfo);
			itemInfo.commandID = kCommandPrint;
			if (Commands_IsCommandEnabled(itemInfo.commandID))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuPrintSelectedText, itemInfo.commandText).ok())
				{
					(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Print Selected Text…”
					CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
				}
			}
			
			ContextSensitiveMenu_InitItem(&itemInfo);
			itemInfo.commandID = kCommandSpeakSelectedText;
			if (Commands_IsCommandEnabled(itemInfo.commandID))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuSpeakSelectedText, itemInfo.commandText).ok())
				{
					(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Speak Selected Text”
					CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
				}
			}
			
			ContextSensitiveMenu_InitItem(&itemInfo);
			itemInfo.commandID = kCommandStopSpeaking;
			if (Commands_IsCommandEnabled(itemInfo.commandID))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuStopSpeaking, itemInfo.commandText).ok())
				{
					(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Stop Speaking”
					CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
				}
			}
		}
	}
	else
	{
		// text-editing-related menu items
		ContextSensitiveMenu_NewItemGroup(inMenu);
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandPaste;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuPasteText, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Paste Text”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandFind;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuFindInThisWindow, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Find In This Window”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		// window arrangement menu items
		ContextSensitiveMenu_NewItemGroup(inMenu);
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandHideFrontWindow;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuHideThisWindow, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Hide This Window”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandStackWindows;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuArrangeAllInFront, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Stack Windows”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		// connection-related menu items
		ContextSensitiveMenu_NewItemGroup(inMenu);
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandSetScreenSize;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuCustomScreenDimensions, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Custom Screen Dimensions…”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandFormat;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuCustomFormat, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Custom Format…”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandSetKeys;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuSpecialKeySequences, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Special Key Sequences…”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandPrintScreen;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (Commands_CopyCommandName(itemInfo.commandID, kCommands_NameTypeDefault, itemInfo.commandText))
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Print Screen…”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
		
		ContextSensitiveMenu_InitItem(&itemInfo);
		itemInfo.commandID = kCommandChangeWindowTitle;
		if (Commands_IsCommandEnabled(itemInfo.commandID))
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuRenameThisWindow, itemInfo.commandText).ok())
			{
				(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Rename This Window…”
				CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
			}
		}
	}
}// buildTerminalWindowContextualMenu


/*!
Appends contextual items to a menu that are appropriate
for the specified view and event (mouse location, modifier
keys, etc.).

A descriptor for the contents of the specified view may
also be constructed; you can use this (for instance) to
tell the Contextual Menu Manager what is in the view so
that appropriate plug-ins can add their own items.  You
must call AEDisposeDesc() on this result.

Most views do not allow contextual menus, and what you
really need is a menu for the entire window; see
populateMenuForWindow().

(3.1)
*/
OSStatus
populateMenuForView		(HIViewRef								inWhichView,
						 MenuRef								inoutMenu,
						 ContextualMenuBuilder_AddItemsProcPtr	inItemAdderOrNull,
						 AEDesc&								inoutViewContentsDesc)
{
	Boolean		failed = false;
	OSStatus	result = noErr;
	
	
	result = AppleEventUtilities_InitAEDesc(&inoutViewContentsDesc);
	assert_noerr(result);
	
	if (nullptr != inoutMenu)
	{
		// if there is a callback, invoke it
		if (nullptr != inItemAdderOrNull)
		{
			(*inItemAdderOrNull)(inoutMenu, REINTERPRET_CAST(inWhichView, HIObjectRef), inoutViewContentsDesc);
		}
		
		// determine what else should go in the menu
		// UNIMPLEMENTED
		failed = true;
		
		unless (failed)
		{
			// Depending upon the view type, there may be data that can be used by contextual
			// menu plug-ins.  The method of getting that data also depends on the view type.
			// See what kind of view the contextual-menu-display event was for, and send that
			// view’s data (usually the current selection) to the plug-ins via a new Apple
			// Event descriptor.
			// UNIMPLEMENTED
			
			// now that all items have been added, notify the Contextual Menu modules to clean up
			ContextSensitiveMenu_DoneAddingItems(inoutMenu);
		}
	}
	
	return result;
}// populateMenuForView


/*!
Appends contextual items to a menu that are appropriate
for the specified window.  If you want the menu to also
be appropriate for a specific event (mouse location,
modifier keys, window part), provide those parameters.

A descriptor for the contents of the specified window
may also be constructed; you can use this (for instance)
to tell the Contextual Menu Manager what is in the window
so that appropriate plug-ins can add their own items.
You must call AEDisposeDesc() on this result.

Sometimes, a contextual menu is constructed that will
not pop up at the user focus.  For this case, you can
pass nullptr for the event record and "inNoWindow" for
the window part; the contents of the menu will be found
based solely on the active window and any selection in
that window; the mouse location and state of modifier
keys will be ignored.

(3.0)
*/
OSStatus
populateMenuForWindow	(HIWindowRef							inWhichWindow,
						 WindowPartCode							inWindowPart,
						 MenuRef								inoutMenu,
						 ContextualMenuBuilder_AddItemsProcPtr	inItemAdderOrNull,
						 AEDesc&								inoutWindowContentsDesc)
{
	WindowInfo_Ref			windowFeaturesRef = nullptr;
	WindowInfo_Descriptor	windowDescriptor = kWindowInfo_InvalidDescriptor;
	HelpSystem_KeyPhrase	keyPhrase = kHelpSystem_KeyPhraseDefault; // determines the Help menu item text and the search string
	Boolean					failed = false;
	Boolean					isConnectionWindow = false;
	OSStatus				result = noErr;
	
	
	result = AppleEventUtilities_InitAEDesc(&inoutWindowContentsDesc);
	assert_noerr(result);
	
	windowFeaturesRef = WindowInfo_ReturnFromWindow(inWhichWindow);
	if (nullptr != windowFeaturesRef) windowDescriptor = WindowInfo_ReturnWindowDescriptor(windowFeaturesRef);
	isConnectionWindow = TerminalWindow_ExistsFor(inWhichWindow);
	
	if (nullptr != inoutMenu)
	{
		// if there is a callback, invoke it
		if (nullptr != inItemAdderOrNull)
		{
			(*inItemAdderOrNull)(inoutMenu, REINTERPRET_CAST(inWhichWindow, HIObjectRef), inoutWindowContentsDesc);
		}
		
		// determine what else should go in the menu
		switch (inWindowPart)
		{
		//case inSysWindow:
		case inStructure:
		case inContent:
		case inDrag:
		case inGrow:
		case inGoAway:
		case inZoomIn:
		case inZoomOut:
		case inCollapseBox:
		case inProxyIcon:
		case inToolbarButton:
		case inNoWindow:
			switch (windowDescriptor)
			{
			case kConstantsRegistry_WindowDescriptorClipboard:
				buildClipboardWindowContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseDefault;
				break;
			
			case kConstantsRegistry_WindowDescriptorCommandLine:
				buildEmptyContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseCommandLine;
				break;
			
			case kConstantsRegistry_WindowDescriptorConnectionStatus:
				buildSessionStatusWindowContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseDefault;
				break;
			
			case kConstantsRegistry_WindowDescriptorFind:
				buildEmptyContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseFind;
				break;
			
			case kConstantsRegistry_WindowDescriptorFormat:
				buildEmptyContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseFormatting;
				break;
			
			case kConstantsRegistry_WindowDescriptorPreferences:
				buildEmptyContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhrasePreferences;
				break;
			
			case kConstantsRegistry_WindowDescriptorScreenSize:
				buildEmptyContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseScreenSize;
				break;
			
			case kConstantsRegistry_WindowDescriptorStandardAboutBox:
				buildAboutBoxContextualMenu(inoutMenu, inWhichWindow);
				keyPhrase = kHelpSystem_KeyPhraseDefault;
				break;
			
			default:
				if (isConnectionWindow)
				{
					buildTerminalWindowContextualMenu(inoutMenu, inWhichWindow);
					keyPhrase = kHelpSystem_KeyPhraseTerminals;
				}
				else
				{
					buildEmptyContextualMenu(inoutMenu, inWhichWindow);
					keyPhrase = kHelpSystem_KeyPhraseDefault;
				}
				break;
			}
			break;
		
		default:
			failed = true;
			break;
		}
		
		unless (failed)
		{
			// Depending upon the window type, there may be data that can be used by contextual
			// menu plug-ins.  The method of getting that data also depends on the window type.
			// See what kind of window the contextual-menu-display event was for, and send that
			// window’s data (usually the current selection) to the plug-ins via a new Apple
			// Event descriptor.
			if (isConnectionWindow)
			{
				// Let the Contextual Menu Manager know what data is selected in the
				// frontmost window, so that Contextual Menu Plug-Ins can parse it.
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(inWhichWindow);
				TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
				
				
				if (TerminalView_TextSelectionExists(view))
				{
					Handle		handle = nullptr;
					
					
					handle = TerminalView_ReturnSelectedTextAsNewHandle(view, 0/* tab info */, 0/* flags */);
					if (nullptr != handle)
					{
						(OSStatus)AECreateDesc(typeChar, *handle, GetHandleSize(handle), &inoutWindowContentsDesc);
						Memory_DisposeHandle(&handle);
					}
				}
			}
			else if (kConstantsRegistry_WindowDescriptorClipboard == windowDescriptor)
			{
				// Let the Contextual Menu Manager know what data is on the clipboard,
				// so that any installed Contextual Menu Plug-Ins can parse it.
				(OSStatus)Clipboard_CreateContentsAEDesc(&inoutWindowContentsDesc);
			}
			
			// now that all items have been added, notify the Contextual Menu modules to clean up
			ContextSensitiveMenu_DoneAddingItems(inoutMenu);
		}
	}
	
	return result;
}// populateMenuForWindow

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
