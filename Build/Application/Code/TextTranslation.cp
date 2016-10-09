/*!	\file TextTranslation.cp
	\brief Lists methods for character translation (i.e. between
	what the user sees and what is transmitted on the network).
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#include "TextTranslation.h"
#include <UniversalDefines.h>

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



#pragma mark Types
namespace {

struct My_TextEncodingInfo
{
	CFRetainRelease		name;
	CFStringEncoding	textEncoding;
	CFStringEncoding	menuItemEncoding;
	TextEncodingBase	base;
};
typedef My_TextEncodingInfo*	My_TextEncodingInfoPtr;

typedef std::vector< My_TextEncodingInfoPtr >	My_TextEncodingInfoList;

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean						gInitialized = false;
TextEncodingBase			gPreferredEncodingBase = kCFStringEncodingMacRoman;
My_TextEncodingInfoList&	gTextEncodingInfoList ()	{ static My_TextEncodingInfoList x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void	fillInCharacterSetList		(Boolean = false);
bool	textEncodingInfoComparer	(My_TextEncodingInfoPtr, My_TextEncodingInfoPtr);

} // anonymous namespace



#pragma mark Public Methods

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
	MenuItemIndex		currentMenuItemIndex = CountMenuItems(inToWhichMenu);
	
	
	// initialize the module if necessary
	fillInCharacterSetList();
	
	for (auto dataPtr : gTextEncodingInfoList())
	{
		if (nullptr != dataPtr)
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
Returns the text encoding preference stored in the given
context, preferring kPreferences_TagTextEncodingIANAName
but using kPreferences_TagTextEncodingID if necessary.

If neither of these is available, the default value is
returned.  You can set it to "kCFStringEncodingInvalidId"
if you want to detect this failure.

See also TextTranslation_ContextSetEncoding().

(4.0)
*/
CFStringEncoding
TextTranslation_ContextReturnEncoding	(Preferences_ContextRef		inContext,
										 CFStringEncoding			inEncodingDefault)
{
	CFStringEncoding	result = kCFStringEncodingInvalidId;
	CFStringRef			selectedEncodingIANAName = nullptr;
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	
	
	// initialize the module if necessary
	fillInCharacterSetList();
	
	// first search for a name; prefer this as a way to express the
	// desired character set, but fall back on a simple encoding ID
	prefsResult = Preferences_ContextGetData(inContext, kPreferences_TagTextEncodingIANAName, sizeof(selectedEncodingIANAName),
												&selectedEncodingIANAName, true/* search defaults too */);
	if (kPreferences_ResultOK == prefsResult)
	{
		result = CFStringConvertIANACharSetNameToEncoding(selectedEncodingIANAName);
		CFRelease(selectedEncodingIANAName), selectedEncodingIANAName = nullptr;
	}
	
	if (kCFStringEncodingInvalidId == result)
	{
		// the name was not found, or could not be resolved; look for an ID
		prefsResult = Preferences_ContextGetData(inContext, kPreferences_TagTextEncodingID, sizeof(result),
													&result, true/* search defaults too */);
		if (kPreferences_ResultOK != prefsResult)
		{
			// nothing found - guess!!!
			result = inEncodingDefault;
		}
	}
	
	return result;
}// ContextReturnEncoding


/*!
Sets text encoding preferences in the given context: both
kPreferences_TagTextEncodingIANAName and
kPreferences_TagTextEncodingID.

If "inViaCopy" is true, then the tags are not written
individually; rather, they are written in “batch mode”,
by Preferences_ContextCopy().  This is required in certain
cases where only batch-mode changes to a context are being
monitored.

Returns true only if successful.

See also TextTranslation_ContextReturnEncoding().

(4.0)
*/
Boolean
TextTranslation_ContextSetEncoding	(Preferences_ContextRef		inContext,
									 CFStringEncoding			inEncodingToSet,
									 Boolean					inViaCopy)
{
	Boolean					result = false;
	CFStringRef				selectedEncodingIANAName = CFStringConvertEncodingToIANACharSetName(inEncodingToSet);
	Preferences_ContextRef	targetContext = (inViaCopy)
											? Preferences_NewContext(Quills::Prefs::TRANSLATION)
											: inContext;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	// initialize the module if necessary
	fillInCharacterSetList();
	
	// set name; this is preferred by readers since it is easier to
	// tell at a glance what the preference actually is
	if (nullptr != selectedEncodingIANAName)
	{
		prefsResult = Preferences_ContextSetData(targetContext, kPreferences_TagTextEncodingIANAName,
													sizeof(selectedEncodingIANAName), &selectedEncodingIANAName);
		if (kPreferences_ResultOK == prefsResult)
		{
			result = true;
		}
	}
	
	// set the “machine readable” encoding number; this is very
	// convenient for programmers, but in practice is only read
	// if a name preference does not exist
	{
		prefsResult = Preferences_ContextSetData(targetContext, kPreferences_TagTextEncodingID,
													sizeof(inEncodingToSet), &inEncodingToSet);
		if (kPreferences_ResultOK == prefsResult)
		{
			result = true;
		}
	}
	
	// if copy-mode is requested, the original context may only be
	// changed by copying the specified tags into it
	if (inViaCopy)
	{
		std::vector< Preferences_Tag >		tagList;
		Preferences_TagSetRef				tagSet = nullptr;
		
		
		result = false; // initially...
		tagList.push_back(kPreferences_TagTextEncodingIANAName);
		tagList.push_back(kPreferences_TagTextEncodingID);
		tagSet = Preferences_NewTagSet(tagList);
		if (nullptr != tagSet)
		{
			prefsResult = Preferences_ContextCopy(targetContext, inContext, tagSet);
			if (kPreferences_ResultOK == prefsResult)
			{
				result = true;
			}
			Preferences_ReleaseTagSet(&tagSet);
		}
		
		// delete the temporary context that was allocated in this mode
		Preferences_ReleaseContext(&targetContext);
	}
	
	return result;
}// ContextSetEncoding


/*!
This is like CFStringCreateWithBytes(), except on failure it
will loop up to "inByteMaxBacktrack" times; each time the tail
of the buffer is reduced by half (at least one byte), in an
attempt to find a proper translation.  (Setting the backtrack
to 0 is equivalent to calling CFStringCreateWithBytes()
directly.)

The number of bytes successfully used is returned in
"outBytesUsed", though this only has meaning if the string
result is defined.

This is very useful when a byte stream is a “slice” of a larger
or infinite stream, since a slice could end with only part of
an encoded character (e.g. the final byte turns out to be the
first byte of a multi-byte character, the rest of which is
missing, which would normally cause the entire translation to
fail).

IMPORTANT:	It is almost always better to use this routine in
			place of a call to CFStringCreateWithBytes().

(4.0)
*/
CFStringRef
TextTranslation_PersistentCFStringCreate	(CFAllocatorRef			inAllocator,
											 UInt8 const*			inBytes,
											 CFIndex				inByteCount,
											 CFStringEncoding		inEncoding,
											 Boolean				inIsExternalRepresentation,
											 CFIndex&				outBytesUsed,
											 CFIndex				inMaxBacktrack)
{
	Boolean const	kDebug = false;
	CFStringRef		result = nullptr;
	
	
	// keep trying with smaller buffers until translation succeeds
	// or too many iterations have occurred
	outBytesUsed = inByteCount;
	if (kDebug)
	{
		Console_WriteValue("attempt to translate persistently to encoding", inEncoding);
	}
	for (CFIndex i = 0; ((nullptr == result) && (outBytesUsed > 0) && (i < (1 + inMaxBacktrack))); ++i)
	{
		result = CFStringCreateWithBytes(inAllocator, inBytes, outBytesUsed, inEncoding, inIsExternalRepresentation);
		if (kDebug)
		{
			Console_WriteValue("against byte array of size", outBytesUsed);
			Console_WriteValueCFString("result", result);
		}
		
		// ignore empty strings
		if ((nullptr != result) && (0 == CFStringGetLength(result)))
		{
			CFRelease(result), result = nullptr;
		}
		
		// if translation fails, start over with fewer bytes; to have
		// reasonable runtime with huge buffers, reduce by an amount
		// proportionate to the original size, but at least one byte
		if ((nullptr == result) && (outBytesUsed > 0))
		{
			CFIndex		reductionAmount = STATIC_CAST(STATIC_CAST(outBytesUsed, Float32) / 2.0f, CFIndex);
			
			
			if (reductionAmount < 1)
			{
				reductionAmount = 1;
			}
			outBytesUsed -= reductionAmount;
			
			if (kDebug)
			{
				Console_WriteValue("translation failed; count of bytes to be ignored next time", reductionAmount);
			}
		}
	}
	
	return result;
}// PersistentCFStringCreate


/*!
Returns the number of character sets that
TextTranslation_AppendCharacterSetsToMenu()
would add.

(3.0)
*/
UInt16
TextTranslation_ReturnCharacterSetCount ()
{
	// initialize the module if necessary
	fillInCharacterSetList();
	
	return STATIC_CAST(gTextEncodingInfoList().size(), UInt16);
}// ReturnCharacterSetCount


/*!
Determines the index in the list of available
encodings of the specified encoding.  The
list used by this routine is guaranteed to be
consistent with the order and size of the list
used by TextTranslation_AppendConvertersToMenu(),
so you can use this routine for initializing
menus that have had encodings appended to them.

If the index can’t be found, 0 is returned.

(3.0)
*/
UInt16
TextTranslation_ReturnCharacterSetIndex		(CFStringEncoding	inTextEncoding)
{
	UInt16		result = 0;
	SInt16		i = 1;
	
	
	// initialize the module if necessary
	fillInCharacterSetList();
	
	// look for matching encoding information
	// TEMPORARY: O(n), may want to change implementation
	for (auto dataPtr : gTextEncodingInfoList())
	{
		if ((nullptr != dataPtr) && (dataPtr->textEncoding == inTextEncoding))
		{
			result = i;
			break;
		}
		++i;
	}
	return result;
}// ReturnCharacterSetIndex


/*!
Determines the text encoding at the specified
index in the list of available encodings.  The
list used by this routine is guaranteed to be
consistent with the order and size of the list
used by TextTranslation_AppendConvertersToMenu(),
so you can use this routine for handling menu
item selections from a menu that had encodings
appended to it.

If the encoding can’t be found, one is assumed.

(3.0)
*/
CFStringEncoding
TextTranslation_ReturnIndexedCharacterSet	(UInt16		inOneBasedIndex)
{
	CFStringEncoding		result = kCFStringEncodingMacRoman;
	My_TextEncodingInfoPtr	dataPtr = nullptr;
	
	
	// initialize the module if necessary
	fillInCharacterSetList();
	
	// look for matching encoding information
	dataPtr = gTextEncodingInfoList()[inOneBasedIndex - 1];
	if (dataPtr != nullptr) result = dataPtr->textEncoding;
	
	return result;
}// ReturnIndexedCharacterSet


#pragma mark Internal Methods
namespace {

/*!
Constructs the internal menu of available text
encodings, and the sorted list of character set
info.

NOTE:	MacTerm should re-invoke this method
		whenever the user changes the current
		key script; it currently DOES NOT.

(3.0)
*/
void
fillInCharacterSetList	(Boolean	inForce)
{
	if ((false == gInitialized) || (inForce))
	{
		CFStringEncoding			currentEncoding = GetApplicationTextEncoding();
		TextEncodingBase			currentBase = kTextEncodingMacRoman;
		My_TextEncodingInfoPtr		dataPtr = nullptr;
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
			else
			{
				// add data to list
				try
				{
					dataPtr = new My_TextEncodingInfo;
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
					dataPtr->name.setWithRetain(encodingName);
					//Console_WriteValueCFString("adding", encodingName);
					{
						My_TextEncodingInfoList::size_type const	kLength = gTextEncodingInfoList().size();
						
						
						gTextEncodingInfoList().push_back(dataPtr);
						assert(gTextEncodingInfoList().size() == (1 + kLength));
					}
				}
			}
			
			if (loopTerminator > 500/* arbitrary */)
			{
				Console_WriteLine("WARNING, forcing text encoding loop to terminate after too many iterations");
				break;
			}
		}
		
		// sort items by name and by encoding base
		std::sort(gTextEncodingInfoList().begin(), gTextEncodingInfoList().end(), textEncodingInfoComparer);
		
		gInitialized = true;
	}
}// fillInCharacterSetList


/*!
A standard comparison function that expects
both of its operands to be of type
"My_TextEncodingInfoPtr".  If the first
name belongs before the second one in an
ascending order, then -1 is returned.  If the
reverse is true, 1 is returned.  Otherwise,
the names seem to be equal, so 0 is returned.

(3.0)
*/
bool
textEncodingInfoComparer	(My_TextEncodingInfoPtr		inMyTextEncodingInfoPtr1,
							 My_TextEncodingInfoPtr		inMyTextEncodingInfoPtr2)
{
	My_TextEncodingInfoPtr	dataPtr1 = inMyTextEncodingInfoPtr1;
	My_TextEncodingInfoPtr	dataPtr2 = inMyTextEncodingInfoPtr2;
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
