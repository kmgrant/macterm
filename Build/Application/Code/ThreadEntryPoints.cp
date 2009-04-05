/*###############################################################

	ThreadEntryPoints.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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

// standard-C++ includes
#include <map>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <Cursors.h>
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>

// resource includes
#include "ApplicationVersion.h"
#include "MenuResources.h"

// MacTelnet includes
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "MenuBar.h"
#include "ThreadEntryPoints.h"



#pragma mark Internal Method Prototypes

static void			clearMenu			(MenuRef);
static void			constructFontMenu	(MenuRef);
static Boolean		isMonospacedFont	(Str255);



#pragma mark Public Methods

/*!
This thread reads the font information from the
preferences file, compares it against the fonts
currently installed, and then rebuilds the font
list if necessary.  This is done at startup
time.  Since the font menu is being constructed
here, the Format item in the Edit menu is not
available until this thread unlocks it.

In MacTelnet 3.0, this routine (finally) uses the
partial resource management APIs to properly
construct manageable resource handles.  In
previous versions of MacTelnet, handles would be
expanded continuously, until eventually memory
got trampled (if too many fonts were installed
or there were many fonts with long names).
Now that Mac OS 9 and beyond allow even more
fonts to be installed, this problem is all the
more likely to occur.  MacTelnet 3.0 fixes it.
(And by the way, only in MacTelnet 3.0 was this
process ever multithreaded!  Thank you very
much.)

(3.0)
*/
pascal voidPtr
ThreadEntryPoints_InitializeFontMenu	(void*		inMenuRefPosingAsVoidPointer)
{
	MenuItemIndex	numFontsInMenu = 0;
	MenuRef			theMenu = REINTERPRET_CAST(inMenuRefPosingAsVoidPointer, MenuRef);
	
	
	constructFontMenu(theMenu);
	numFontsInMenu = CountMenuItems(theMenu);
	//Console_WriteValue("Font menu created - initial cardinality", numFontsInMenu);
	
	// remove proportional fonts from the menu
#if 1
	// NOTE:	Proportional fonts don’t have to be stripped (MacTelnet is
	//       now able to render them if they are used); however, they
	//       are SLOW.  Not only will the extra fonts add time opening
	//       dialogs that have font menus (such as Terminal Editor), but
	//       terminal text simply does not render as quickly when it has
	//       to write one character at once to preserve cell spacing.
	{
		SInt16		i = 0;
		Str255		fontName;
		
		
		for (i = 1; i <= numFontsInMenu; ++i)
		{
			GetMenuItemText(theMenu, i, fontName);
			unless (isMonospacedFont(fontName))
			{
				DeleteMenuItem(theMenu, i);
				--numFontsInMenu;
				--i; // offset index into menu to account for deleted item
			}
		}
	}
#endif
	//Console_WriteValue("Font menu filtered to monospace-only - new cardinality", numFontsInMenu);
	
	// make the remaining fonts appear in their own typeface
	{
		SInt16		i = 0;
		SInt16		fontNum = 0;
		Str255		fontName;
		
		
		for (i = 1; i <= numFontsInMenu; ++i)
		{
			GetMenuItemText(theMenu, i, fontName);
			
			// set font for menu item
			fontNum = FMGetFontFamilyFromName(fontName);
			SetMenuItemFontID(theMenu, i, fontNum);
		}
	}
	
	MenuBar_SetFontMenuAvailable(true); // now that the list rebuilding is finished, it is safe to access font menus
	
	//Console_WriteLine("Shutting down thread.");
	return nullptr;
}// InitializeFontMenu


#pragma mark Internal Methods

/*!
Uses the quickest API available to delete all
items from the specified menu.

(3.0)
*/
static void
clearMenu	(MenuRef	inoutMenu)
{
	(OSStatus)DeleteMenuItems(inoutMenu, 1/* first item */, CountMenuItems(inoutMenu));
}// clearMenu


/*!
Uses the most advanced API available to build
a menu of all available fonts - if possible,
with fonts from the current script, as well
as the system script (if different) and the
most recent key script (if different).

(3.0)
*/
static void
constructFontMenu	(MenuRef	inoutFontMenu)
{
#if 1
	{
		ATSFontContext const		kMyFontIterationContext = kATSFontContextLocal;
		ATSOptionFlags const		kMyFontIterationOptions = 0L;
		ATSFontFilter const* const	kMyFontIterationFilter = nullptr;
		void* const					kMyFontIterationRefCon = nullptr;
		ATSFontIterator				toFont = nullptr;
		OSStatus					error = noErr;
		
		
		clearMenu(inoutFontMenu);
		error = ATSFontIteratorCreate(kMyFontIterationContext, kMyFontIterationFilter,
										kMyFontIterationRefCon, kMyFontIterationOptions,
										&toFont);
		if (noErr == error)
		{
			while (noErr == error)
			{
				ATSFontRef		currentFont = nullptr;
				
				
				error = ATSFontIteratorNext(toFont, &currentFont);
				if (noErr == error)
				{
					CFStringRef		nameCFString = nullptr;
					
					
					error = ATSFontGetName(currentFont, kATSOptionFlagsDefault, &nameCFString);
					if (noErr == error)
					{
						//Console_WriteValueCFString("iterated font", nameCFString);
						(OSStatus)AppendMenuItemTextWithCFString(inoutFontMenu, nameCFString, kMenuItemAttrIgnoreMeta,
																	0L/* command ID */, nullptr/* new item */);
						CFRelease(nameCFString), nameCFString = nullptr;
					}
				}
				else if (kATSIterationScopeModified == error)
				{
					error = ATSFontIteratorReset(kMyFontIterationContext, kMyFontIterationFilter,
													kMyFontIterationRefCon, kMyFontIterationOptions,
													&toFont);
					clearMenu(inoutFontMenu);
				}
			}
			
			(OSStatus)ATSFontIteratorRelease(&toFont);
		}
	}
#else
	FMFontFamilyIterator	fontFamilyIterator;
	FMFontFamily			fontFamily;
	Str255					fontName;
	TextEncoding			fontNameTextEncoding = kTextEncodingMacRoman;
	
	
	clearMenu(inoutFontMenu);
	
	// "kFMDefaultOptions" for fonts accessible to MacTelnet,
	// "kFMUseGlobalScopeOption" for all fonts available on the system
	FMCreateFontFamilyIterator(nullptr/* filter information */, nullptr/* user data */,
								kFMDefaultOptions, &fontFamilyIterator);
	while (FMGetNextFontFamily(&fontFamilyIterator, &fontFamily) == noErr)
	{
		// Insert code that does something with the font family information.
		// For example, you can get the string associated with 
		// the font family by using the function
		FMGetFontFamilyName(fontFamily, fontName);
		AppendMenu(inoutFontMenu, "\pfont"); // avoid Menu Manager interpolation
		SetMenuItemText(inoutFontMenu, CountMenuItems(inoutFontMenu), fontName);
		FMGetFontFamilyTextEncoding(fontFamily, &fontNameTextEncoding);
		SetMenuItemTextEncoding(inoutFontMenu, CountMenuItems(inoutFontMenu), fontNameTextEncoding);
	}
	FMDisposeFontFamilyIterator(&fontFamilyIterator);
#endif
}// constructFontMenu


/*!
Determines if a font is monospaced.

(2.6)
*/
static Boolean
isMonospacedFont	(Str255		inFontName)
{
	Boolean		result = false;
	Boolean		doRomanTest = false;
	SInt32		numberOfScriptsEnabled = GetScriptManagerVariable(smEnabled);
	
	
	if (numberOfScriptsEnabled > 1)
	{
		ScriptCode		scriptNumber = smRoman;
		FMFontFamily	fontID = FMGetFontFamilyFromName(inFontName);
		
		
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
		TextFontByName(inFontName);
		result = (CharWidth('W') == CharWidth('.'));
	}
	
	return result;
}// isMonospacedFont

// BELOW IS REQUIRED NEWLINE TO END FILE
