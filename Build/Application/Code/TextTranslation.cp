/*###############################################################

	TextTranslation.cp
	
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

#include "UniversalDefines.h"

// standard-C includes
#include <cstring>

// standard-C++ includes
#include <algorithm>
#include <vector>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <StringUtilities.h>

// resource includes
#include "GeneralResources.h"
#include "MenuResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "TextTranslation.h"



#pragma mark Types

struct MyTextEncodingInfo
{
	CFRetainRelease		name;
	CFStringEncoding	textEncoding;
	CFStringEncoding	menuItemEncoding;
	TextEncodingBase	base;
};
typedef MyTextEncodingInfo*		MyTextEncodingInfoPtr;

typedef std::vector< MyTextEncodingInfoPtr >	MyTextEncodingInfoList;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	TextEncodingBase			gPreferredEncodingBase = kCFStringEncodingMacRoman;
	MyTextEncodingInfoList&		gTextEncodingInfoList ()	{ static MyTextEncodingInfoList x; return x; }
}

#pragma mark Internal Method Prototypes

static void			clearCharacterSetList		();
static void			fillInCharacterSetList		();
static bool			textEncodingInfoComparer	(MyTextEncodingInfoPtr, MyTextEncodingInfoPtr);



#pragma mark Public Methods

/*!
Reads all available translation tables and appends
their names to the given menu.  You must invoke
this routine before any other routines from this
module.

(2.6)
*/
void
TextTranslation_Init ()
{
	fillInCharacterSetList();
}// Init


/*!
Call this method after you are finished with this
module, to free resources.
*/
void
TextTranslation_Done ()
{
	clearCharacterSetList();
}// Done


/*!
Inserts items at the end of the specified menu
for each available character set.

Specify 0 to not indent the items, or specify
a number to indicate how many indentation levels
(multipled by some pixel value) to inset items.

(3.0)
*/
void
TextTranslation_AppendCharacterSetsToMenu	(MenuRef	inToWhichMenu,
											 UInt16		inIndentationLevel)
{
	MyTextEncodingInfoPtr					dataPtr = nullptr;
	MyTextEncodingInfoList::const_iterator	textEncodingInfoIterator;
	MenuItemIndex							currentMenuItemIndex = CountMenuItems(inToWhichMenu);
	
	
	for (textEncodingInfoIterator = gTextEncodingInfoList().begin();
			textEncodingInfoIterator != gTextEncodingInfoList().end(); ++textEncodingInfoIterator)
	{
		dataPtr = *textEncodingInfoIterator;
		if (dataPtr != nullptr)
		{
			AppendMenuItemTextWithCFString(inToWhichMenu, dataPtr->name.returnCFStringRef(),
											kMenuItemAttrIgnoreMeta/* attributes */, 0L/* command ID */,
											&currentMenuItemIndex);
			if (inIndentationLevel) SetMenuItemIndent(inToWhichMenu, currentMenuItemIndex, inIndentationLevel);
			// perhaps the encoding step is not necessary now that CFStrings are used? (verify) INCOMPLETE
			//???SetMenuItemTextEncoding(inToWhichMenu, currentMenuItemIndex, dataPtr->menuItemEncoding);
		}
	}
}// AppendCharacterSetsToMenu


/*!
Persistently translates a buffer using the given
text encoding converter (i.e. if the converter
cannot translate all the text at once, this
method keeps trying until it does).  Since the
encoded text might be longer than the original
(e.g. a single character requiring multiple bytes
to represent using a different encoding), it
cannot be translated in place - therefore, a new
handle is returned containing the translated text.
If there are memory problems, "memFullErr" is the
result, and the output handle is invalid.

(3.0)
*/
OSStatus
TextTranslation_ConvertBufferToNewHandle	(UInt8*			inText,
											 ByteCount		inLength,
											 TECObjectRef	inConverter,
											 Handle*		outNewHandleWithTranslatedText)
{
	OSStatus	result = noErr;
	
	
	if ((inText != nullptr) && (outNewHandleWithTranslatedText != nullptr))
	{
		enum
		{
			kOutputBufferSize = 4096	// arbitrary
		};
		Ptr				outputBuffer = nullptr;
		UInt8*			inputPtr = nullptr;
		ByteCount		inputLength = 0,
						actualInputLength = 0,
						actualOutputLength = 0;
		
		
		// create a handle that will ultimate contain all of the translated text
		*outNewHandleWithTranslatedText = Memory_NewHandleInProperZone(kOutputBufferSize, kMemoryBlockLifetimeLong);
		if (*outNewHandleWithTranslatedText == nullptr) result = memFullErr;
		else
		{
			inputLength = inLength;
			inputPtr = inText;
			outputBuffer = Memory_NewPtr(kOutputBufferSize);
			if (outputBuffer == nullptr) result = memFullErr;
			else
			{
				Ptr			translatedBufferPtr = nullptr;
				Boolean		stillConverting = true;
				
				
				HLock(*outNewHandleWithTranslatedText);
				translatedBufferPtr = *(*outNewHandleWithTranslatedText);
				
				// convert portions of the text until it is all converted
				do
				{
					if (stillConverting)
					{
						// convert a chunk of text
						result = TECConvertText(inConverter, inputPtr, inputLength, &actualInputLength,
												(UInt8*)outputBuffer, kOutputBufferSize, &actualOutputLength);
						if (result != kTECUsedFallbacksStatus) stillConverting = false;
					}
					else
					{
						// flush remaining text from converterÕs internal buffers
						result = TECFlushText(inConverter, (UInt8*)outputBuffer,
												kOutputBufferSize, &actualOutputLength);
					}
					
					if (result == noErr)
					{
						// append the newly-translated text to the final buffer; but first, make sure
						// there is enough memory to resize the output buffer to accommodate the text
						result = Memory_SetHandleSize(*outNewHandleWithTranslatedText,
														GetHandleSize(*outNewHandleWithTranslatedText) +
															actualOutputLength);
						if (result == noErr)
						{
							BlockMoveData(outputBuffer, translatedBufferPtr, actualOutputLength);
							translatedBufferPtr += actualOutputLength;
						}
					}
					inputPtr += actualInputLength;
					inputLength -= actualInputLength;
				} while ((result == kTECUsedFallbacksStatus) || (result == kTECNeedFlushStatus));
				
				Memory_DisposePtr(&outputBuffer);
				HUnlock(*outNewHandleWithTranslatedText);
			}
		}
	}
	
	return result;
}// ConvertBufferToNewHandle


/*!
Returns the number of character sets that
TextTranslation_AppendCharacterSetsToMenu()
would add.

(3.0)
*/
inline UInt16
TextTranslation_GetCharacterSetCount ()
{
	return gTextEncodingInfoList().size();
}// GetCharacterSetCount


/*!
Determines the index in the list of available
encodings of the specified encoding.  The
list used by this routine is guaranteed to be
consistent with the order and size of the list
used by TextTranslation_AppendConvertersToMenu(),
so you can use this routine for initializing
menus that have had encodings appended to them.

If the index canÕt be found, 0 is returned.

(3.0)
*/
UInt16
TextTranslation_GetCharacterSetIndex	(CFStringEncoding	inTextEncoding)
{
	UInt16									result = 0;
	SInt16									i = 1;
	MyTextEncodingInfoPtr					dataPtr = nullptr;
	MyTextEncodingInfoList::const_iterator	textEncodingInfoIterator;
	
	
	// look for matching encoding information
	for (i = 1, textEncodingInfoIterator = gTextEncodingInfoList().begin();
			textEncodingInfoIterator != gTextEncodingInfoList().end(); ++textEncodingInfoIterator, ++i)
	{
		dataPtr = *textEncodingInfoIterator;
		if ((dataPtr != nullptr) && (dataPtr->textEncoding == inTextEncoding)) result = i;
	}
	return result;
}// GetCharacterSetIndex


/*!
Determines the text encoding at the specified
index in the list of available encodings.  The
list used by this routine is guaranteed to be
consistent with the order and size of the list
used by TextTranslation_AppendConvertersToMenu(),
so you can use this routine for handling menu
item selections from a menu that had encodings
appended to it.

If the encoding canÕt be found, one is assumed.

(3.0)
*/
CFStringEncoding
TextTranslation_GetIndexedCharacterSet		(UInt16		inOneBasedIndex)
{
	CFStringEncoding		result = kCFStringEncodingMacRoman;
	MyTextEncodingInfoPtr	dataPtr = nullptr;
	
	
	// look for matching encoding information
	dataPtr = gTextEncodingInfoList()[inOneBasedIndex - 1];
	if (dataPtr != nullptr) result = dataPtr->textEncoding;
	
	return result;
}// GetIndexedCharacterSet


#pragma mark Internal Methods

/*!
Deletes all of the character set information
structures in the global list.  Useful prior to
rebuilding the list, or deleting it permanently.

(3.0)
*/
static void
clearCharacterSetList ()
{
	MyTextEncodingInfoPtr				dataPtr = nullptr;
	MyTextEncodingInfoList::iterator	textEncodingInfoIterator;
	
	
	// destroy all encoding information structures in the global list
	for (textEncodingInfoIterator = gTextEncodingInfoList().begin();
			textEncodingInfoIterator != gTextEncodingInfoList().end(); ++textEncodingInfoIterator)
	{
		dataPtr = *textEncodingInfoIterator;
		if (nullptr != dataPtr) delete dataPtr;
	}
}// clearCharacterSetList


/*!
Constructs the internal menu of available text
encodings, and the sorted list of character set
info.

NOTE:	MacTelnet should re-invoke this method
		whenever the user changes the current
		key script; it currently DOES NOT.

(3.0)
*/
static void
fillInCharacterSetList ()
{
	CFStringEncoding			currentEncoding = GetApplicationTextEncoding();
	TextEncodingBase			oldBase = kTextEncodingMacUnicode; // arbitrary; must differ from "currentBase" for initial loop
	TextEncodingBase			currentBase = kTextEncodingMacRoman;
	MyTextEncodingInfoPtr		dataPtr = nullptr;
	CFStringEncoding const*		availableEncodings = nullptr;
	CFStringRef					encodingName = nullptr;
	UInt32						loopTerminator = 0L;
	
	
	gPreferredEncodingBase = GetTextEncodingBase(currentEncoding);
	
	// create one structure for each unique Text Encoding that is available
	for (availableEncodings = CFStringGetListOfAvailableEncodings();
			(nullptr != availableEncodings) && (kCFStringEncodingInvalidId != *availableEncodings);
			++availableEncodings, ++loopTerminator)
	{
		// use the text encoding base to determine which menu items will have identical names
		currentBase = GetTextEncodingBase(*availableEncodings);
		
		// find an appropriate name for the menu item
		encodingName = CFStringGetNameOfEncoding(*availableEncodings);
		if (nullptr == encodingName) Console_WriteValue("could not get the name of encoding", *availableEncodings);
		else if (currentBase != oldBase) // do not show duplicate items in the menu
		{
			// add data to list
			try
			{
				dataPtr = new MyTextEncodingInfo;
			}
			catch (std::bad_alloc)
			{
				dataPtr = nullptr;
			}
			
			if (nullptr == dataPtr) Console_WriteLine("not enough memory to add encoding info!");
			else
			{
				dataPtr->base = currentBase;
				dataPtr->menuItemEncoding = *availableEncodings;
				dataPtr->textEncoding = *availableEncodings;
				dataPtr->name.setCFTypeRef(encodingName);
				//Console_WriteValueCFString("adding", encodingName);
				{
					MyTextEncodingInfoList::size_type	length = gTextEncodingInfoList().size();
					
					
					gTextEncodingInfoList().push_back(dataPtr);
					assert(gTextEncodingInfoList().size() == (1 + length));
				}
			}
		}
		oldBase = currentBase;
		
		if (loopTerminator > 500/* arbitrary */)
		{
			Console_WriteLine("WARNING, forcing text encoding loop to terminate after too many iterations");
			break;
		}
	}
	
	// sort items by name and by encoding base
	std::sort(gTextEncodingInfoList().begin(), gTextEncodingInfoList().end(), textEncodingInfoComparer);
}// fillInCharacterSetList


/*!
A standard comparison function that expects
both of its operands to be of type
"MyTextEncodingInfoPtr".  If the first
name belongs before the second one in an
ascending order, then -1 is returned.  If the
reverse is true, 1 is returned.  Otherwise,
the names seem to be equal, so 0 is returned.

(3.0)
*/
static bool
textEncodingInfoComparer	(MyTextEncodingInfoPtr		inMyTextEncodingInfoPtr1,
							 MyTextEncodingInfoPtr		inMyTextEncodingInfoPtr2)
{
	MyTextEncodingInfoPtr	dataPtr1 = inMyTextEncodingInfoPtr1;
	MyTextEncodingInfoPtr	dataPtr2 = inMyTextEncodingInfoPtr2;
	bool					result = 0;
	
	
	if ((dataPtr1 != nullptr) && (dataPtr2 != nullptr))
	{
		if ((dataPtr1->base == gPreferredEncodingBase) && (dataPtr2->base != gPreferredEncodingBase))
		{
			result = true; // any encodings matching the host Mac OS version should come first in the list
		}
		else if ((dataPtr1->base != gPreferredEncodingBase) && (dataPtr2->base == gPreferredEncodingBase))
		{
			result = false; // any encodings matching the host Mac OS version should come first in the list
		}
		else
		{
			// if the bases match or otherwise are not the preferred base, then
			// their names will determine which goes first
			result = (kCFCompareLessThan == CFStringCompare
											(dataPtr1->name.returnCFStringRef(), dataPtr2->name.returnCFStringRef(),
												kNilOptions));
		}
	}
	return result;
}// textEncodingInfoComparer

// BELOW IS REQUIRED NEWLINE TO END FILE
