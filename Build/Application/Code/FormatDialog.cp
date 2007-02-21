/*###############################################################

	FormatDialog.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#define DIALOG_IS_SHEET TARGET_API_MAC_CARBON

#include "UniversalDefines.h"

// standard-C includes
#include <climits>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <ListUtilities.h>
#include <Localization.h>
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "DialogResources.h"
#include "StringResources.h"
#include "FormatDialog.r"

// MacTelnet includes
#include "ColorBox.h"
#include "ConstantsRegistry.h"
#include "DialogTransitions.h"
#include "DialogUtilities.h"
#include "EventInfoWindowScope.h"
#include "EventLoop.h"
#include "FormatDialog.h"
#include "HelpSystem.h"
#include "MenuBar.h"
#include "TextTranslation.h"



#pragma mark Constants

enum
{
	// dialog item indices
	iFormatOKButton = 1,
	iFormatCancelButton = 2,
	iFormatAppleGuideButton = 3,
	iFormatEditMenuPrimaryGroup = 4,
	iFormatEditMenuFontItemsPane = 5,
	iFormatEditMenuFontSizeItemsPane = 6,
	iFormatEditSizeLabel = 7,
	iFormatEditSizeField = 8,
	iFormatEditSizeArrows = 9,
	iFormatEditSizePopUpMenu = 10,
	iFormatEditCharacterSetList = 11,
	iFormatEditFontPopUpMenu = 12,
	iFormatEditMenuForeColorItemsPane = 13,
	iFormatForegroundColorBox = 14,
	iFormatForegroundColorLabel = 15,
	iFormatEditMenuBackColorItemsPane = 16,
	iFormatBackgroundColorBox = 17,
	iFormatBackgroundColorLabel = 18
};

#pragma mark Types

struct FormatDialog
{	
	SInt16							oldPanelIndex;				// previously displayed panel
	Boolean							isCustomSizeMenuItemAdded;	// does the size pop-up menu contain a custom size?
	WindowRef						screenWindow;				// the terminal window for which this dialog applies
	MenuRef							characterSetMenu;			// menu containing character set names
	FormatDialogSetupData			setupData;					// running values for colors, fonts, etc.
	DialogRef						dialog;						// the Mac OS dialog reference
	WindowInfoRef					windowInfo;					// auxiliary data for the dialog
	FormatDialogCloseNotifyProcPtr	closeNotifyProc;			// routine to call when the dialog is dismissed
#if DIALOG_IS_SHEET
	ListenerModel_ListenerRef		mainEventListener;			// listener for global application events
#endif
	FormatDialogRef					selfRef;					// identical to address of structure, but typed as ref
};
typedef FormatDialog*		FormatDialogPtr;
typedef FormatDialogPtr*	FormatDialogHandle;

typedef MemoryBlockHandleLocker< FormatDialogRef, FormatDialog >	FormatDialogHandleLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	FormatDialogPtr				gCurrentEventFormatDialogPtr = nullptr;	// used ONLY when necessary, to pass info to callbacks
	FormatDialogHandleLocker&	gFormatDialogHandleLocks()  { static FormatDialogHandleLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static void				checkSizeMenuSize			(FormatDialogPtr				inPtr,
													 MenuRef						inStandardSizeMenu,
													 SInt16							inSize);

static Boolean			customSizeMenuSize			(FormatDialogPtr				inPtr,
													 MenuRef						inStandardSizeMenu,
													 SInt16							inCustomSizeOrZero);

static void				disableItemGroupsAsNeeded	(FormatDialogPtr				inPtr,
													 FormatDialogFormatOptions		inBasedOnTheseOptions);

static void				fontUpdate					(FormatDialogPtr				inPtr);

static pascal Boolean	formatDialogEventFilter		(DialogRef						inDialog,
													 EventRecord*					inoutEventPtr,
													 short*							outItemIndex);

static ListHandle		getCharacterSetList			(FormatDialogPtr				inPtr);

static MenuRef			getFontMenu					(FormatDialogPtr				inPtr);

static MenuRef			getSizeMenu					(FormatDialogPtr				inPtr);

static void				handleItemHit				(FormatDialogPtr				inPtr,
													 DialogItemIndex*				inoutItemIndexPtr);

static pascal void		handleNewSize				(WindowRef						inWindow,
													 SInt32							inDeltaSizeX,
													 SInt32							inDeltaSizeY,
													 SInt32							inData);

#if DIALOG_IS_SHEET
static Boolean			mainEventLoopEvent			(ListenerModel_Ref				inUnusedModel,
													 ListenerModel_Event			inEventThatOccurred,
													 void*							inEventContextPtr,
													 void*							inListenerContextPtr);
#endif

static void				setFormatFromItems			(FormatDialogPtr				inPtr,
													 FormatDialogSetupDataPtr		inDataPtr,
													 UInt16							inFormatIndexToUse);

static void				setItemsForFormat			(FormatDialogPtr				inPtr,
													 FormatDialogSetupDataPtr		inDataPtr,
													 UInt16							inFormatIndexToUse);

static void				setSizeField				(FormatDialogPtr				inPtr,
													 SInt16							inNewSizeOrZero);

static void				setUpSizeMenuForFont		(MenuRef						inStandardSizeMenu,
													 ConstStringPtr					inFontName);



#pragma mark Public Methods

/*!
This method is used to initialize the Format dialog
box.  It creates the dialog box invisibly, and uses
the specified data to set up the corresponding
controls in the dialog box.  Do not pass nullptr for
"inSetupDataPtr".

(3.0)
*/
FormatDialogRef
FormatDialog_New	(FormatDialogSetupDataConstPtr		inSetupDataPtr,
					 FormatDialogCloseNotifyProcPtr		inCloseNotifyProcPtr)
{
	FormatDialogRef		result = (FormatDialogRef)Memory_NewHandle(sizeof(FormatDialog));
	
	
	if (result != nullptr)
	{
		FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(result);
		
		
		Cursors_DeferredUseWatch(30); // if it takes more than half a second to initialize, show the watch cursor
		
		// initialize data
		ptr->screenWindow = EventLoop_GetRealFrontWindow();
		GetNewDialogWithWindowInfo(kDialogIDFormat, &ptr->dialog, &ptr->windowInfo);
		ptr->closeNotifyProc = inCloseNotifyProcPtr;
		ptr->setupData = *inSetupDataPtr;
		ptr->oldPanelIndex = kFormatDialogIndexNormalText;
		ptr->isCustomSizeMenuItemAdded = false;
		ptr->selfRef = result;
		
		// set up the Help System
		HelpSystem_SetWindowKeyPhrase(GetDialogWindow(ptr->dialog), kHelpSystem_KeyPhraseFormatting);
		
		if (ptr->dialog != nullptr)
		{
		#if DIALOG_IS_SHEET
			// install a notification routine in the main event loop, to find out when dialog events occur
			ptr->mainEventListener = ListenerModel_NewBooleanListener(mainEventLoopEvent, result/* context */);
			EventLoop_StartMonitoringWindow(kEventLoop_WindowEventContentTrack, GetDialogWindow(ptr->dialog),
											ptr->mainEventListener);
			EventLoop_StartMonitoringWindow(kEventLoop_WindowEventKeyPress, GetDialogWindow(ptr->dialog),
											ptr->mainEventListener);
			EventLoop_StartMonitoringWindow(kEventLoop_WindowEventUpdate, GetDialogWindow(ptr->dialog),
											ptr->mainEventListener);
			
			// since resources can’t set resizability automatically, add this attribute manually here
			(OSStatus)ChangeWindowAttributes(GetDialogWindow(ptr->dialog),
												kWindowResizableAttribute/* set these attributes */,
												0/* clear these attributes */);
		#endif
			
			// adjust the OK and Cancel buttons so they are big enough for their titles
			SetDialogDefaultItem(ptr->dialog, iFormatOKButton);
			SetDialogCancelItem(ptr->dialog, iFormatCancelButton);
			(UInt16)Localization_AdjustDialogButtons(ptr->dialog, true/* OK */, true/* Cancel */);
			Localization_AdjustHelpButtonItem(ptr->dialog, iFormatAppleGuideButton);
			
			// set the static text controls (auto-sizing them) and the bottom checkbox (not changing its size)
			{
				ControlRef		control = nullptr;
				Str255			text;
				
				
				GetDialogItemAsControl(ptr->dialog, iFormatEditSizeLabel, &control);
				GetIndString(text, rStringsMiscellaneousStaticText, siStaticTextFormatSizeLabel);
				(UInt16)Localization_SetUpSingleLineTextControl(control, text,
																false/* is a checkbox or radio button */);
				
				GetDialogItemAsControl(ptr->dialog, iFormatForegroundColorLabel, &control);
				GetIndString(text, rStringsMiscellaneousStaticText, siStaticTextFormatForegroundColor);
				(UInt16)Localization_SetUpSingleLineTextControl(control, text,
																false/* is a checkbox or radio button */);
				
				GetDialogItemAsControl(ptr->dialog, iFormatBackgroundColorLabel, &control);
				GetIndString(text, rStringsMiscellaneousStaticText, siStaticTextFormatBackgroundColor);
				(UInt16)Localization_SetUpSingleLineTextControl(control, text,
																false/* is a checkbox or radio button */);
			}
			
			// fill in the items of the Character Set list
			{
				ListHandle		list = nullptr;
				Cell			cell = { 0, 0 };
				
				
				ListUtilities_GetDialogItemListHandle(ptr->dialog, iFormatEditCharacterSetList, &list);
				
				// specify how the list can be used
				SetListFlags(list, lDoVAutoscroll);
				SetListSelectionFlags(list, lOnlyOne | lNoDisjoint | lNoExtend | lUseSense);
				
				// Add Character Sets and highlight the first one.
				// IMPORTANT: A lot of code assumes this list contains
				//            ONLY character sets, no additional items.
				//!TMP!TextTranslation_AppendCharacterSetsToList(list);
				LSetSelect(true/* highlight */, cell, list);
			}
			
			// create the two color boxes
			//ColorBox_AttachToUserPaneDialogItem(ptr->dialog, iFormatForegroundColorBox, nullptr/* initial color */);
			//ColorBox_AttachToUserPaneDialogItem(ptr->dialog, iFormatBackgroundColorBox, nullptr/* initial color */);
			
			// prevent the user from typing non-digits into the Font Size field
			{
				ControlRef				sizeField = nullptr;
				ControlKeyFilterUPP		upp = NumericalLimiterKeyFilterUPP();
				
				
				GetDialogItemAsControl(ptr->dialog, iFormatEditSizeField, &sizeField);
				(OSStatus)SetControlData(sizeField, kControlNoPart, kControlKeyFilterTag,
											sizeof(upp), &upp);
			}
			
			// set up the little arrows control so it can span the font size range
			{
				ControlRef		littleArrows = nullptr;
				
				
				GetDialogItemAsControl(ptr->dialog, iFormatEditSizeArrows, &littleArrows);
				SetControl32BitMaximum(littleArrows, SHRT_MAX);
				SetControl32BitMinimum(littleArrows, 0);
			}
			
			// initialize the controls based on the default set
			setItemsForFormat(ptr, &ptr->setupData, 0);
			fontUpdate(ptr);
			
			// set up the Window Info information
			{
				WindowResizeResponderProcPtr	resizeProc = handleNewSize;
				
				
				WindowInfo_SetWindowDescriptor(ptr->windowInfo, kConstantsRegistry_WindowDescriptorFormat);
				WindowInfo_SetWindowResizeLimits(ptr->windowInfo,
														FORMAT_DIALOG_HT/* minimum height */,
														FORMAT_DIALOG_WD/* minimum width */,
														FORMAT_DIALOG_HT + 200/* arbitrary maximum height */,
														FORMAT_DIALOG_WD + 50/* arbitrary maximum width */);
				WindowInfo_SetWindowResizeResponder(ptr->windowInfo, resizeProc, 0L);
			}
			WindowInfo_SetForDialog(ptr->dialog, ptr->windowInfo);
		}
		gFormatDialogHandleLocks().releaseLock(result, &ptr);
	}
	return result;
}// New


/*!
Call this method to destroy a formatting dialog
box, and to fill in the data structure provided to
FormatDialog_New() to contain the user’s changes.

(3.0)
*/
void
FormatDialog_Dispose	(FormatDialogRef*	inoutDialogPtr)
{
	if (gFormatDialogHandleLocks().isLocked(*inoutDialogPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked format dialog; outstanding locks",
							gFormatDialogHandleLocks().returnLockCount(*inoutDialogPtr));
	}
	else
	{
		FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(*inoutDialogPtr);
		
		
		// clean up the Help System
		HelpSystem_SetWindowKeyPhrase(GetDialogWindow(ptr->dialog), kHelpSystem_KeyPhraseDefault);
		
		// release all memory occupied by the dialog
		//ColorBox_DetachFromUserPaneDialogItem(ptr->dialog, iFormatForegroundColorBox);
		//ColorBox_DetachFromUserPaneDialogItem(ptr->dialog, iFormatBackgroundColorBox);
		WindowInfo_Dispose(ptr->windowInfo);
		DisposeDialog(ptr->dialog), ptr->dialog = nullptr;
	#if DIALOG_IS_SHEET
		EventLoop_StopMonitoringWindow(kEventLoop_WindowEventContentTrack, GetDialogWindow(ptr->dialog),
										ptr->mainEventListener);
		EventLoop_StopMonitoringWindow(kEventLoop_WindowEventKeyPress, GetDialogWindow(ptr->dialog),
										ptr->mainEventListener);
		EventLoop_StopMonitoringWindow(kEventLoop_WindowEventUpdate, GetDialogWindow(ptr->dialog),
										ptr->mainEventListener);
		ListenerModel_ReleaseListener(&ptr->mainEventListener);
	#endif
		gFormatDialogHandleLocks().releaseLock(*inoutDialogPtr, &ptr);
		Memory_DisposeHandle(REINTERPRET_CAST(inoutDialogPtr, Handle*));
	}
}// Dispose


/*!
This method displays and handles events in the
Format dialog box.  If the user clicks OK,
"true" is returned; otherwise, "false" is
returned.

(3.0)
*/
void
FormatDialog_Display	(FormatDialogRef	inDialog)
{
	FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(inDialog);
	
	
	if (ptr == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// define this only for debugging; this lets you dump the dialog’s
		// control embedding hierarchy to a file, before it is displayed
		#if 0
		DebugSelectControlHierarchyDumpFile(GetDialogWindow(ptr->dialog));
		#endif
		
		// display the dialog
	#if DIALOG_IS_SHEET
		DialogTransitions_DisplaySheet(GetDialogWindow(ptr->dialog), ptr->screenWindow, true/* opaque */);
	#else
		DialogTransitions_Display(GetDialogWindow(ptr->dialog));
	#endif
		
		// set the focus
		FocusDialogItem(ptr->dialog, iFormatEditSizeField);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	#if !DIALOG_IS_SHEET
		gCurrentEventFormatDialogPtr = ptr;
		{
			ModalFilterUPP			filterProc = NewModalFilterUPP(formatDialogEventFilter);
			DialogItemIndex			itemIndex = 0;
			
			
			itemIndex = 0;
			do
			{
				ModalDialog(filterProc, &itemIndex);
				handleItemHit(ptr, &itemIndex);
			} while ((itemIndex != iFormatOKButton) && (itemIndex != iFormatCancelButton));
			DisposeModalFilterUPP(filterProc), filterProc = nullptr;
		}
	#endif
	}
	gFormatDialogHandleLocks().releaseLock(inDialog, &ptr);
}// Display


/*!
Fills in a data structure representing the current
display in the dialog box.  You usually use this in
a custom close notification method to figure out
what the user’s preferences are.

Returns "true" only if the data could be found.

(3.0)
*/
Boolean
FormatDialog_GetContents	(FormatDialogRef			inDialog,
							 FormatDialogSetupDataPtr	outSetupDataPtr)
{
	FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(inDialog);
	Boolean				result = false;
	
	
	if (ptr != nullptr)
	{
		*outSetupDataPtr = ptr->setupData;
		result = true;
	}
	gFormatDialogHandleLocks().releaseLock(inDialog, &ptr);
	return result;
}// GetContents


/*!
Fills in a data structure representing the current
display in the dialog box.  You usually use this in
a custom close notification method to figure out
what the user’s preferences are.

Returns nullptr if any problems occur.

(3.0)
*/
WindowRef
FormatDialog_GetParentWindow	(FormatDialogRef	inDialog)
{
	FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(inDialog);
	WindowRef			result = nullptr;
	
	
	if (ptr != nullptr)
	{
		result = ptr->screenWindow;
	}
	gFormatDialogHandleLocks().releaseLock(inDialog, &ptr);
	return result;
}// GetParentWindow


/*!
If you only need a close notification procedure
for the purpose of disposing of the Format Dialog
reference (and don’t otherwise care when a Format
Dialog closes), you can pass this standard routine
to FormatDialog_New() as your notification procedure.

(3.0)
*/
void
FormatDialog_StandardCloseNotifyProc	(FormatDialogRef	inDialogThatClosed,
										 Boolean			UNUSED_ARGUMENT(inOKButtonPressed))
{
	FormatDialog_Dispose(&inDialogThatClosed);
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
To find the item in the Size pop-up menu that
corresponds to the given size and automatically
make that the current value of the pop-up menu
(“checking” that size in the menu), use this
method.

(3.0)
*/
static void
checkSizeMenuSize		(FormatDialogPtr	inPtr,
						 MenuRef			inStandardSizeMenu,
						 SInt16				inSize)
{
	register SInt16 	i = 0;
	SInt32				itemFontSize = 0L;
	Str255 				itemString;
	
	
	for (i = 1; i <= CountMenuItems(inStandardSizeMenu); i++)
	{
		GetMenuItemText(inStandardSizeMenu, i, itemString);
		StringToNum(itemString, &itemFontSize);
		if (itemFontSize == inSize)
		{
			SetDialogItemValue(inPtr->dialog, iFormatEditSizePopUpMenu, i);
			break;
		}
	}
}// checkSizeMenuSize


/*!
To add or remove a custom size item from a Size
menu, use this method.  Pass 0 as the size to
remove any custom item (has no effect if a
custom item is not there).  If a custom item is
already in place, the new size you specify will
replace the custom size.  This routine will
automatically check the numerical values in the
Size menu you provide, and if any of them
matches the given size, no custom item is added
and any existing custom item is automatically
removed.

If a custom item is in the menu when this
routine is finished, "true" is returned; if not,
"false" is returned.

IMPORTANT:	This method may not produce the
			expected results if you change your
			Size menu outside of the control of
			this routine.

(3.0)
*/
static Boolean
customSizeMenuSize		(FormatDialogPtr	inPtr,
						 MenuRef			inStandardSizeMenu,
						 SInt16				inCustomSizeOrZero)
{
	SInt16 			i = 0;
	SInt32			itemFontSize = 0L;
	Str255 			itemString;
	ControlRef		sizeMenuButton = nullptr;
	Boolean			result = false;
	
	
	// get the size pop-up menu button, because its “maximum” must change when items are added or removed
	(OSStatus)GetDialogItemAsControl(inPtr->dialog, iFormatEditSizePopUpMenu, &sizeMenuButton);
	
	if (inPtr->isCustomSizeMenuItemAdded)
	{
		// delete last two appended items (the divider and custom size)
		DeleteMenuItem(inStandardSizeMenu, CountMenuItems(inStandardSizeMenu));
		DeleteMenuItem(inStandardSizeMenu, CountMenuItems(inStandardSizeMenu));
		SetControl32BitMaximum(sizeMenuButton, GetControlMaximum(sizeMenuButton) - 2);
		inPtr->isCustomSizeMenuItemAdded = false;
	}
	
	if (inCustomSizeOrZero)
	{
		// check Size menu for a number identical to the given size
		inPtr->isCustomSizeMenuItemAdded = true;
		for (i = 1; i <= CountMenuItems(inStandardSizeMenu); i++)
		{
			GetMenuItemText(inStandardSizeMenu, i, itemString);
			StringToNum(itemString, &itemFontSize);
			if (itemFontSize == inCustomSizeOrZero) inPtr->isCustomSizeMenuItemAdded = false;
		}
	}
	else
	{
		inPtr->isCustomSizeMenuItemAdded = false;
	}
	
	if (inPtr->isCustomSizeMenuItemAdded)
	{
		NumToString(inCustomSizeOrZero, itemString);
		AppendMenu(inStandardSizeMenu, "\p(-");
		AppendMenu(inStandardSizeMenu, itemString);
		SetControl32BitMaximum(sizeMenuButton, GetControlMaximum(sizeMenuButton) + 2);
	}
	result = inPtr->isCustomSizeMenuItemAdded;
	
	return result;
}// customSizeMenuSize


/*!
To disable portions of the formatting controls based on format
dialog options, use this method.  You can individually disable
groups of controls related to setting the font, the font size,
the foreground color, and the background color.

IMPORTANT:	Any group that you do not explicitly identify as
			being disabled will become enabled after calling
			this routine.  To enable all item groups, mask off
			"kFormatDialogFormatOptionMaskDisableAllItems" in
			the given format options.

(3.0)
*/
static void
disableItemGroupsAsNeeded		(FormatDialogPtr			inPtr,
								 FormatDialogFormatOptions	inBasedOnTheseOptions)
{
	// initially enable everything
	ActivateDialogItem(inPtr->dialog, iFormatEditMenuFontItemsPane);
	ActivateDialogItem(inPtr->dialog, iFormatEditMenuFontSizeItemsPane);
	ActivateDialogItem(inPtr->dialog, iFormatEditMenuForeColorItemsPane);
	ActivateDialogItem(inPtr->dialog, iFormatEditMenuBackColorItemsPane);
	
	// now disable whatever was requested by the options
	if (inBasedOnTheseOptions & kFormatDialogFormatOptionDisableFontItems)
	{
		DeactivateDialogItem(inPtr->dialog, iFormatEditMenuFontItemsPane);
	}
	if (inBasedOnTheseOptions & kFormatDialogFormatOptionDisableFontSizeItems)
	{
		DeactivateDialogItem(inPtr->dialog, iFormatEditMenuFontSizeItemsPane);
	}
	if (inBasedOnTheseOptions & kFormatDialogFormatOptionDisableForeColorItems)
	{
		DeactivateDialogItem(inPtr->dialog, iFormatEditMenuForeColorItemsPane);
	}
	if (inBasedOnTheseOptions & kFormatDialogFormatOptionDisableBackColorItems)
	{
		DeactivateDialogItem(inPtr->dialog, iFormatEditMenuBackColorItemsPane);
	}
}// disableItemGroupsAsNeeded


/*!
To make the Font pop-up menu button use the font
of its current selection, use this method.  As a
convenience, this routine also invokes the
getSizeMenu() method, to make sure it is up to
date.

(3.0)
*/
static void
fontUpdate	(FormatDialogPtr	inPtr)
{
	ControlFontStyleRec		styleRecord;
	ControlRef				fontMenuButton = nullptr;
	MenuRef					fontMenu = getFontMenu(inPtr);
	SInt16					fontID = 0;
	SInt16					value = 0;
	Str255					itemText;
	
	
	(OSStatus)GetDialogItemAsControl(inPtr->dialog, iFormatEditFontPopUpMenu, &fontMenuButton);
	value = GetControlValue(fontMenuButton);
	
	GetMenuItemText(fontMenu, value, itemText);
#if TARGET_API_MAC_OS8
	GetFNum(itemText, &fontID);
#else
	fontID = FMGetFontFamilyFromName(itemText);
#endif
	styleRecord.flags = kControlUseFontMask;
	styleRecord.font = fontID;
	(OSStatus)SetControlFontStyle(fontMenuButton, &styleRecord);
	DrawOneControl(fontMenuButton);
	
	setUpSizeMenuForFont(getSizeMenu(inPtr), itemText);
}// fontUpdate


/*!
Handles special key events in the Format dialog box.

IMPORTANT:	The global variable "gCurrentEventFormatDialogPtr"
			must always be set prior to invoking this routine.

(3.0)
*/
static pascal Boolean
formatDialogEventFilter		(DialogRef		inDialog,
							 EventRecord*	inoutEventPtr,
							 short*			outItemIndex)
{
	Boolean		result = pascal_false,
				isButtonAndShouldFlash = false; // simplifies handling of multiple key-to-button maps
	
	
	// this rigamorole is necessary to distinguish between up-arrow and down-arrow clicks!
	if (inoutEventPtr->what == mouseDown)
	{
		GrafPtr				oldPort = nullptr;
		Point				localMouse = inoutEventPtr->where;
		ControlRef			controlHit = nullptr;
		ControlRef			arrows = nullptr;
		ControlRef			sizeMenuControl = nullptr;
		ControlPartCode		part = kControlNoPart;
		
		
		GetPort(&oldPort);
		SetPortWindowPort(GetDialogWindow(inDialog));
		GlobalToLocal(&localMouse);
		controlHit = FindControlUnderMouse(localMouse, GetDialogWindow(inDialog), &part);
		GetDialogItemAsControl(inDialog, iFormatEditSizeArrows, &arrows);
		GetDialogItemAsControl(inDialog, iFormatEditSizePopUpMenu, &sizeMenuControl);
		if (controlHit == sizeMenuControl)
		{
			// ensure menu is updated prior to display, but don’t otherwise intercept the event
			setSizeField(gCurrentEventFormatDialogPtr, 0);
		}
		else if (controlHit == arrows)
		{
			// ensure arrow value is updated prior to use
			setSizeField(gCurrentEventFormatDialogPtr, 0);
			
			// determine which arrow was clicked, and update controls
			part = HandleControlClick(controlHit, localMouse, inoutEventPtr->modifiers, (ControlActionUPP)nullptr);
			switch (part)
			{
			case kControlUpButtonPart:
			case kControlDownButtonPart:
				{
					SInt16		fontSize = 0;
					
					
					// change arrows value...
					fontSize = GetControlValue(arrows);
					if ((part == kControlUpButtonPart) && (fontSize < GetControlMaximum(arrows)))
					{
						fontSize++;
					}
					else if ((part == kControlDownButtonPart) && (fontSize > GetControlMinimum(arrows)))
					{
						fontSize--;
					}
					setSizeField(gCurrentEventFormatDialogPtr, fontSize);
				}
				result = pascal_true;
				break;
			
			default:
				// ???
				break;
			}
		}
		SetPort(oldPort);
	}
	
	if (!result) result = (StandardDialogEventFilter(inDialog, inoutEventPtr, outItemIndex));
	else if (isButtonAndShouldFlash) FlashButton(inDialog, *outItemIndex);
	
	return result;
}// formatDialogEventFilter


/*!
To acquire a handle to the list displaying all the
available Character Sets, use this convenient method.

(3.0)
*/
static ListHandle
getCharacterSetList		(FormatDialogPtr	inPtr)
{
	ListHandle		result = nullptr;
	
	
	ListUtilities_GetDialogItemListHandle(inPtr->dialog, iFormatEditCharacterSetList, &result);
	return result;
}// getCharacterSetMenu


/*!
To acquire a handle to the menu from the Font
pop-up menu button, use this convenient method.

(3.0)
*/
static MenuRef
getFontMenu		(FormatDialogPtr	inPtr)
{
	MenuRef			result = nullptr;
	ControlRef		fontMenuButton = nullptr;
	Size			actualSize = 0L;
	
	
	GetDialogItemAsControl(inPtr->dialog, iFormatEditFontPopUpMenu, &fontMenuButton);
	(OSStatus)GetControlData(fontMenuButton, kControlMenuPart,
								kControlPopupButtonMenuHandleTag,
								sizeof(result), (Ptr)&result, &actualSize);
	return result;
}// getFontMenu


/*!
To acquire a handle to the menu from the Size
pop-up menu button, use this convenient method.

(3.0)
*/
static MenuRef
getSizeMenu		(FormatDialogPtr	inPtr)
{
	MenuRef			result = nullptr;
	ControlRef		sizeMenuButton = nullptr;
	Size			actualSize = 0L;
	
	
	GetDialogItemAsControl(inPtr->dialog, iFormatEditSizePopUpMenu, &sizeMenuButton);
	(OSStatus)GetControlData(sizeMenuButton, kControlMenuPart,
								kControlPopupButtonMenuHandleTag,
								sizeof(result), (Ptr)&result, &actualSize);
	return result;
}// getSizeMenu


/*!
Responds to control selections in the dialog box.
If a button is pressed, the window is closed and
the close notifier routine is called; otherwise,
an adjustment is made to the display.

(3.0)
*/
static void
handleItemHit	(FormatDialogPtr		inPtr,
				 DialogItemIndex*		inoutItemIndexPtr)
{
	switch (*inoutItemIndexPtr)
	{
	case iFormatOKButton:
		// undo any “damage” to this menu
		(Boolean)customSizeMenuSize(inPtr, getSizeMenu(inPtr), 0);
		
		// get the information from the dialog and then kill it
		setFormatFromItems(inPtr, &inPtr->setupData, inPtr->oldPanelIndex);
	#if DIALOG_IS_SHEET
		DialogTransitions_CloseSheet(GetDialogWindow(inPtr->dialog), inPtr->screenWindow);
	#else
		DialogTransitions_Close(GetDialogWindow(inPtr->dialog));
	#endif
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			InvokeFormatDialogCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK pressed */);
		}
		break;
	
	case iFormatCancelButton:
		// user cancelled - close the dialog with an appropriate transition for cancelling
	#if DIALOG_IS_SHEET
		DialogTransitions_CloseSheet(GetDialogWindow(inPtr->dialog), inPtr->screenWindow);
	#else
		DialogTransitions_CloseCancel(GetDialogWindow(inPtr->dialog));
	#endif
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			InvokeFormatDialogCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK pressed */);
		}
		break;
	
	case iFormatAppleGuideButton:
		HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseFormatting);
		break;
	
	case iFormatEditMenuPrimaryGroup:
		{
			SInt16		value = GetDialogItemValue(inPtr->dialog, iFormatEditMenuPrimaryGroup);
			
			
			switch (--value) // indices are zero-based, item numbers are one-based
			{
			case kFormatDialogIndexNormalText:
			case kFormatDialogIndexBoldText:
			case kFormatDialogIndexBlinkingText:
				if (value != inPtr->oldPanelIndex)
				{
					setFormatFromItems(inPtr, &inPtr->setupData, inPtr->oldPanelIndex); // save old
					setItemsForFormat(inPtr, &inPtr->setupData, value); // draw new
					inPtr->oldPanelIndex = value;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
		break;
	
	case iFormatEditCharacterSetList:
		// update the keyboard script to match the selected encoding, unless
		// the user says not to synchronize the key script with the font
		unless (GetScriptManagerVariable(smGenFlags) & smfDisableKeyScriptSyncMask)
		{
			Cell			selectedCell = { 0, 0 };
			TextEncoding	encoding = kTextEncodingMacRoman;
			ScriptCode		scriptCode = smRoman;
			OSStatus		error = noErr;
			
			
			LGetSelect(true/* get nearest */, &selectedCell/* on input, cell to start at; on output, selected cell */,
						getCharacterSetList(inPtr));
			encoding = TextTranslation_GetIndexedCharacterSet(1 + selectedCell.v/* one-based set index */);
			error = RevertTextEncodingToScriptInfo(encoding, &scriptCode, nullptr/* language */, nullptr/* region */);
			if (error == noErr) KeyScript(scriptCode);
			else KeyScript(smKeySysScript); // on error, revert to the default script
		}
		break;
	
	case iFormatEditFontPopUpMenu:
		fontUpdate(inPtr);
		break;
	
	case iFormatEditSizeArrows:
		// necessarily handled in the event filter
		break;
	
	case iFormatEditSizePopUpMenu:
		{
			SInt16		value = GetDialogItemValue(inPtr->dialog, iFormatEditSizePopUpMenu);
			
			
			switch (value)
			{
			default:
				// simply copy the menu item text to the text field
				{
					Str255			itemText;
					MenuRef			sizeMenu = nullptr;
					ControlRef		sizeMenuButton = nullptr;
					Size			actualSize = 0L;
					
					
					GetDialogItemAsControl(inPtr->dialog, iFormatEditSizePopUpMenu, &sizeMenuButton);
					if (GetControlData(sizeMenuButton, kControlMenuPart,
										kControlPopupButtonMenuHandleTag,
										sizeof(sizeMenu), (Ptr)&sizeMenu, &actualSize) == noErr)
					{
						SInt32		fontSize = 0L;
						
						
						GetMenuItemText(sizeMenu, value, itemText);
						StringToNum(itemText, &fontSize);
						setSizeField(inPtr, fontSize);
					}
				}
				break;
			}
		}
		break;
	
	case iFormatForegroundColorBox:
	case iFormatBackgroundColorBox:
		{
			ControlRef		control = nullptr;
			
			
			(OSStatus)GetDialogItemAsControl(inPtr->dialog, *inoutItemIndexPtr, &control);
			ColorBox_UserSetColor(control);
		}
		break;
	
	default:
		break;
	}
}// handleItemHit


/*!
This method moves and resizes controls in response to
a resize of the “Format” movable modal dialog box.

(3.0)
*/
static pascal void
handleNewSize		(WindowRef		inWindow,
					 SInt32			inDeltaSizeX,
					 SInt32			inDeltaSizeY,
					 SInt32			UNUSED_ARGUMENT(inData))
{
	DialogAdjust_BeginDialogItemAdjustment(GetDialogFromWindow(inWindow));
	{
		SInt32		amountH = 0L;
		
		
		if ((inDeltaSizeX) || (inDeltaSizeY))
		{
			// controls which are moved horizontally
			if (Localization_IsLeftToRight())
			{
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatOKButton, inDeltaSizeX);
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatCancelButton, inDeltaSizeX);
			}
			else
			{
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatAppleGuideButton, inDeltaSizeX);
			}
			amountH = INTEGER_HALVED(inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatEditSizePopUpMenu, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatEditSizeArrows, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatForegroundColorLabel, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iFormatBackgroundColorLabel, amountH);
			
			// controls which are moved vertically
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatOKButton, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatCancelButton, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatAppleGuideButton, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatEditMenuBackColorItemsPane, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatEditMenuForeColorItemsPane, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatEditMenuFontSizeItemsPane, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveV, iFormatEditFontPopUpMenu, inDeltaSizeY);
			
			// controls which are resized horizontally and vertically
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditMenuFontItemsPane, inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeV, iFormatEditMenuFontItemsPane, inDeltaSizeY);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditCharacterSetList, inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeV, iFormatEditCharacterSetList, inDeltaSizeY);
			
			// controls which are resized horizontally
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditMenuPrimaryGroup, inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditMenuFontSizeItemsPane, inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditMenuForeColorItemsPane, inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditMenuBackColorItemsPane, inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditFontPopUpMenu, inDeltaSizeX);
			amountH = INTEGER_HALVED(inDeltaSizeX);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatEditSizeField, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatForegroundColorBox, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatForegroundColorLabel, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatBackgroundColorBox, amountH);
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iFormatBackgroundColorLabel, amountH);
			
			// controls which are resized vertically
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeV, iFormatEditMenuPrimaryGroup, inDeltaSizeY);
		}
	}
	DialogAdjust_EndAdjustment(inDeltaSizeX, inDeltaSizeY); // moves and resizes controls properly
	
	// remove keyboard focus to avoid glitch when resizing list
	(OSStatus)ClearKeyboardFocus(inWindow);
	
	// resize selection rectangle to match new list boundaries
	{
		ControlRef		listBox = nullptr;
		
		
		if (GetDialogItemAsControl(GetDialogFromWindow(inWindow), iFormatEditCharacterSetList, &listBox) == noErr)
		{
			ListHandle		list = nullptr;
			
			
			ListUtilities_SynchronizeListBoundsWithControlBounds(listBox);
			ListUtilities_GetControlListHandle(listBox, &list);
			{
				Point		newCellDimensions;
				Rect		listBounds;
				
				
				GetControlBounds(listBox, &listBounds);
				SetPt(&newCellDimensions, listBounds.right - listBounds.left, 18);
				LSetDrawingMode(false, list);
				LCellSize(newCellDimensions, list);
				LSetDrawingMode(true, list);
				RegionUtilities_RedrawControlOnNextUpdate(listBox);
			}
		}
	}
}// handleNewSize


#if DIALOG_IS_SHEET
/*!
Processes events from the main event loop, used when
the Format Dialog is actually window-modal (a sheet)
and not a modal dialog.  Mac OS X only.

(3.0)
*/
static Boolean
mainEventLoopEvent		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inEventThatOccurred,
						 void*					inEventContextPtr,
						 void*					inListenerContextPtr)
{
	FormatDialogRef		ref = REINTERPRET_CAST(inListenerContextPtr, FormatDialogRef);
	Boolean				result = true;
	
	
	switch (inEventThatOccurred)
	{
	case kEventLoop_WindowEventContentTrack:
		// if the window whose content region was clicked in is a Format Dialog, respond to the click
		{
			EventInfoWindowScope_ContentClickPtr	clickInfoPtr = REINTERPRET_CAST
																	(inEventContextPtr,
																		EventInfoWindowScope_ContentClickPtr);
			
			
			if (ref != nullptr)
			{
				FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(ref);
				DialogRef			dialogHit = nullptr;
				DialogItemIndex		itemHit = 0;
				
				
				gCurrentEventFormatDialogPtr = ptr;
				if (formatDialogEventFilter(ptr->dialog, &clickInfoPtr->event, &itemHit) ||
					(DialogSelect(&clickInfoPtr->event, &dialogHit, &itemHit) &&
						(dialogHit == ptr->dialog)))
				{
					handleItemHit(ptr, &itemHit);
				}
				result = true;
				gFormatDialogHandleLocks().releaseLock(ref, &ptr);
			}
		}
		break;
	
	case kEventLoop_WindowEventKeyPress:
		// if the window containing the keyboard focus is a Format Dialog, respond to the key press
		{
			EventInfoWindowScope_KeyPressPtr	keyPressInfoPtr = REINTERPRET_CAST
																	(inEventContextPtr,
																		EventInfoWindowScope_KeyPressPtr);
			
			
			if (ref != nullptr)
			{
				FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(ref);
				DialogRef			dialogHit = nullptr;
				DialogItemIndex		itemHit = 0;
				
				
				gCurrentEventFormatDialogPtr = ptr;
				if (formatDialogEventFilter(ptr->dialog, &keyPressInfoPtr->event, &itemHit) ||
					(DialogSelect(&keyPressInfoPtr->event, &dialogHit, &itemHit) &&
						(dialogHit == ptr->dialog)))
				{
					handleItemHit(ptr, &itemHit);
				}
				result = true;
				gFormatDialogHandleLocks().releaseLock(ref, &ptr);
			}
		}
		break;
	
	case kEventLoop_WindowEventUpdate:
		// if the window that needs updating is a Format Dialog, react accordingly
		{
			EventInfoWindowScope_UpdatePtr	updateInfoPtr = REINTERPRET_CAST
															(inEventContextPtr, EventInfoWindowScope_UpdatePtr);
			
			
			if (ref != nullptr)
			{
				FormatDialogPtr		ptr = gFormatDialogHandleLocks().acquireLock(ref);
				
				
				BeginUpdate(updateInfoPtr->window);
				UpdateDialog(ptr->dialog, updateInfoPtr->visibleRegion);
				EndUpdate(updateInfoPtr->window);
				result = true;
				gFormatDialogHandleLocks().releaseLock(ref, &ptr);
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// mainEventLoopEvent
#endif


/*!
To set up the specified format using the values of
appropriate controls in the dialog box, use this
method.

(3.0)
*/
static void
setFormatFromItems		(FormatDialogPtr			inPtr,
						 FormatDialogSetupDataPtr	inDataPtr,
						 UInt16						inFormatIndexToUse)
{
	Boolean		didSet = true;
	
	
	if (inDataPtr != nullptr)
	{
		if (inFormatIndexToUse < kFormatDialogNumberOfFormats)
		{
			RGBColor*		colorPtr = nullptr;
			ControlRef		control = nullptr;
			
			
			// read colors
			colorPtr = &inDataPtr->format[inFormatIndexToUse].colors.foreground;
			GetDialogItemAsControl(inPtr->dialog, iFormatForegroundColorBox, &control);
			ColorBox_GetColor(control, colorPtr);
			
			colorPtr = &inDataPtr->format[inFormatIndexToUse].colors.background;
			GetDialogItemAsControl(inPtr->dialog, iFormatBackgroundColorBox, &control);
			ColorBox_GetColor(control, colorPtr);
			
			// read font name
			GetDialogItemAsControl(inPtr->dialog, iFormatEditFontPopUpMenu, &control);
			GetMenuItemText(getFontMenu(inPtr), GetControlValue(control),
							inDataPtr->format[inFormatIndexToUse].font.familyName);
			
			// read font size
			{
				SInt32		number = 0L;
				
				
				GetDialogItemAsControl(inPtr->dialog, iFormatEditSizeField, &control);
				GetControlNumericalText(control, &number);
				inDataPtr->format[inFormatIndexToUse].font.size = number;
			}
		}
		else didSet = false;
	}
	else didSet = false;
	
	if (!didSet) Sound_StandardAlert();
}// setFormatFromItems


/*!
To set up the values of all controls in the group box
to use the information in the specified format, use
this method.

(3.0)
*/
static void
setItemsForFormat		(FormatDialogPtr			inPtr,
						 FormatDialogSetupDataPtr	inDataPtr,
						 UInt16						inFormatIndexToUse)
{
	Boolean		didSet = true;
	
	
	if (inDataPtr != nullptr)
	{
		if (inFormatIndexToUse < kFormatDialogNumberOfFormats)
		{
			ControlRef		control = nullptr;
			RGBColor*		foreColorPtr = &inDataPtr->format[inFormatIndexToUse].colors.foreground;
			RGBColor*		backColorPtr = &inDataPtr->format[inFormatIndexToUse].colors.background;
			StringPtr		fontName = inDataPtr->format[inFormatIndexToUse].font.familyName;
			SInt16			fontSize = inDataPtr->format[inFormatIndexToUse].font.size;
			
			
			// update foreground color
			GetDialogItemAsControl(inPtr->dialog, iFormatForegroundColorBox, &control);
			ColorBox_SetColor(control, foreColorPtr);
			DrawOneDialogItem(inPtr->dialog, iFormatForegroundColorBox);
			
			// update background color
			GetDialogItemAsControl(inPtr->dialog, iFormatBackgroundColorBox, &control);
			ColorBox_SetColor(control, backColorPtr);
			DrawOneDialogItem(inPtr->dialog, iFormatBackgroundColorBox);
			
			setSizeField(inPtr, fontSize); // update font size
			
			// update character set menu
			{
				//MenuItemIndex		index = 0;
				
				
				// - find font text encoding
				// - find text encoding in menu using TextTranslation_GetCharacterSetIndex()
				//MenuBar_ReturnMenuItemIndexByItemText(getCharacterSetList(inPtr), fontName);
				//SetDialogItemValue(inPtr->dialog, iFormatEditFontPopUpMenu, index);
				//DrawOneDialogItem(inPtr->dialog, iFormatEditFontPopUpMenu);
			}
			
			// update font menu
			{
				MenuItemIndex		itemIndex = MenuBar_ReturnMenuItemIndexByItemText(getFontMenu(inPtr), fontName);
				
				
				SetDialogItemValue(inPtr->dialog, iFormatEditFontPopUpMenu, itemIndex);
				DrawOneDialogItem(inPtr->dialog, iFormatEditFontPopUpMenu);
			}
			
			disableItemGroupsAsNeeded(inPtr, inDataPtr->format[inFormatIndexToUse].options);
			fontUpdate(inPtr);
		}
		else didSet = false;
	}
	else didSet = false;
	
	if (!didSet) Sound_StandardAlert();
}// setItemsForFormat


/*!
Call this method to update all controls related
to the font size (such as the size menu and the
arrows).  Pass 0 for "inNewSize" if you want
the text field to be left alone (presumably
because you are responding to the user typing
something); this causes the current value of
the text field to be used to update all other
controls.

(3.0)
*/
static void
setSizeField		(FormatDialogPtr	inPtr,
					 SInt16				inNewSizeOrZero)
{
	ControlRef		control = nullptr;
	SInt32			newSize = inNewSizeOrZero;
	
	
	GetDialogItemAsControl(inPtr->dialog, iFormatEditSizeField, &control);
	if (newSize != 0)
	{
		// set the field to the given size
		SetControlNumericalText(control, newSize);
		DrawOneControl(control);
	}
	else
	{
		// 0 means “read the current field value”
		GetControlNumericalText(control, &newSize);
	}
	GetDialogItemAsControl(inPtr->dialog, iFormatEditSizeArrows, &control);
	SetControl32BitValue(control, newSize);
	customSizeMenuSize(inPtr, getSizeMenu(inPtr), newSize);
	checkSizeMenuSize(inPtr, getSizeMenu(inPtr), newSize); // synchronizes the pop-up menu value
}// setSizeField


/*!
This method outlines the appropriate menu items
in the Size menu for the font indicated.

(3.0)
*/
static void
setUpSizeMenuForFont	(MenuRef			inStandardSizeMenu,
						 ConstStringPtr		inFontName)
{
	SInt16 		i = 0,
				fontID = 0;
	SInt32		itemFontSize = 0L;
	Str255 		itemString;
	
	
#if TARGET_API_MAC_OS8
	GetFNum(inFontName, &fontID);
#else
	fontID = FMGetFontFamilyFromName(inFontName);
#endif
	
	// set up Size menu
	for (i = 1; i <= CountMenuItems(inStandardSizeMenu); i++)
	{
		GetMenuItemText(inStandardSizeMenu, i, itemString);
		StringToNum(itemString, &itemFontSize);
		
		if (RealFont(fontID, (short)itemFontSize)) SetItemStyle(inStandardSizeMenu, i, outline);
		else SetItemStyle(inStandardSizeMenu, i, normal);
	}
}// setUpSizeMenuForFont

// BELOW IS REQUIRED NEWLINE TO END FILE
