/*!	\file StringUtilities.mm
	\brief General-purpose routines for dealing with text.
*/
/*###############################################################
	
	Data Access Library
	© 1998-2021 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#import <StringUtilities.h>
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <string>

// Mac includes
@import CoreServices;

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <UTF8Decoder.h>



#pragma mark Internal Method Prototypes
namespace {

Boolean			unitTest_ReturnUnicodeSymbol_000				();
Boolean			unitTest_StudyInRange_000						();

} // anonymous namespace



#pragma mark Public Methods

/*!
A unit test for this module.  This should always
be run before a release, after any substantial
changes are made, or if you suspect bugs!  It
should also be EXPANDED as new functionality is
proposed (ideally, a test is written before the
functionality is added).

(2017.01)
*/
void
StringUtilities_RunTests ()
{
	UInt16		totalTests = 0;
	UInt16		failedTests = 0;
	
	
	++totalTests; if (false == unitTest_ReturnUnicodeSymbol_000()) ++failedTests;
	++totalTests; if (false == unitTest_StudyInRange_000()) ++failedTests;
	
	Console_WriteUnitTestReport("String Utilities", failedTests, totalTests);
}// RunTests


/*!
Creates a new array containing as many strings as necessary
to hold all of the lines from the original string.  You must
release the array yourself with CFRelease().

The strings are determined using CFStringGetLineBounds(), so
any sequence that means “new line” could cause the split.
None of the new-line sequences are included.

Although no problems are expected when searching for lines,
since an iterative search for lines is performed there is a
guard against infinite looping.  The search is arbitrarily
terminated after SHRT_MAX attempts to find lines have been
made.

(2.6)
*/
CFArrayRef
StringUtilities_CFNewStringsWithLines	(CFStringRef	inString)
{
	CFIndex const		kLength = CFStringGetLength(inString);
	SInt16				loopGuard = 0;
	CFRange				startCharacter = CFRangeMake(0, 1/* count */);
	CFIndex				lineEndIndex = 0;
	CFIndex				lineEndWithTerminator = 0;
	CFMutableArrayRef	mutableResult = CFArrayCreateMutable(kCFAllocatorDefault, 0/* capacity, or zero for no limit */,
																&kCFTypeArrayCallBacks);
	CFArrayRef			result = mutableResult;
	
	
	while ((lineEndIndex < kLength) && (++loopGuard < SHRT_MAX/* arbitrary */))
	{
		CFRetainRelease		lineCFString;
		
		
		CFStringGetLineBounds(inString, startCharacter, &startCharacter.location, &lineEndWithTerminator, &lineEndIndex);
		lineCFString.setWithNoRetain(CFStringCreateWithSubstring(kCFAllocatorDefault, inString,
																		CFRangeMake(startCharacter.location,
																					(lineEndIndex - startCharacter.location))));
		CFArrayAppendValue(mutableResult, lineCFString.returnCFStringRef());
		startCharacter.location = lineEndWithTerminator;
	}
	return result;
}// CFNewStringsWithLines


/*!
Converts a Core Foundation string into a C++ standard string
in UTF-8 encoding.

WARNING:	The result will be an empty string on failure.

(1.4)
*/
void
StringUtilities_CFToUTF8	(CFStringRef	inCFString,
							 std::string&	outBuffer)
{
	outBuffer.clear();
	if (nullptr != inCFString)
	{
		CFStringEncoding const		kDesiredEncoding = kCFStringEncodingUTF8;
		char const* const			kCharPtr = CFStringGetCStringPtr(inCFString, kDesiredEncoding);
		
		
		if (nullptr != kCharPtr)
		{
			outBuffer = std::string(kCharPtr);
		}
		else
		{
			// direct access failed; try copying
			size_t const	kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
												(CFStringGetLength(inCFString), kDesiredEncoding);
			char*			buffer = new char[kBufferSize];
			
			
			CFStringGetCString(inCFString, buffer, kBufferSize, kDesiredEncoding);
			outBuffer = buffer;
			delete [] buffer;
		}
	}
}// CFToUTF8


/*!
Calls StringUtilities_ForEachComposedCellClusterInRange()
for the entire length of the string, starting at 0.

(2021.04)
*/
void
StringUtilities_ForEachComposedCellCluster	(CFStringRef					inStringData,
											 StringUtilities_CellBlock		inBlock)
{
	if (nullptr != inStringData)
	{
		StringUtilities_ForEachComposedCellClusterInRange(inStringData, CFRangeMake(0, CFStringGetLength(inStringData)), inBlock);
	}
}// ForEachComposedCellCluster


/*!
Invokes a block on each composed character sequence in the given
range of the given string, with a focus on rendering: additional
details are provided on the number of cells (terminal columns)
that are likely to be needed and the scaling that will be
necessary in Core Text to occupy that integral cell count.

This does not only use Core Text; it also adds in heuristics to
better match intent.  For instance, Unicode specifies (and legacy
applications expect) that all box-drawing characters in the range
of 0x2500 to 0x259F are a single column, even though the Mac will
typically try to make them double-width when rendered by fonts.
This function will force such corrections and ignore the fonts.

This uses StringUtilities_ForEachComposedCharacterSequenceInRange()
so it has that function’s documented advantages over an ordinary
NSString call: typically covering all composed character sequences
(even invalid ones that have replacement characters).

(2021.04)
*/
void
StringUtilities_ForEachComposedCellClusterInRange	(CFStringRef				inStringData,
													 CFRange					inRange,
													 StringUtilities_CellBlock	inBlock)
{
	// iterate over the composed character sequences and call the
	// given block on each one
	StringUtilities_ForEachComposedCharacterSequenceInRange
	(inStringData, inRange,
	^(CFStringRef	inSubstring,
	  CFRange		inSubstringRange,
	  Boolean&		outStop)
	{
		// TEMPORARY; this is a partial port of work done elsewhere
		// that is excluding attributes used to compute different
		// values depending on the contents of the string; for now,
		// to ease porting, these parameters are fixed
		// (INCOMPLETE)
		UInt16 const	kCellCount = 1;
		CGFloat const	kWidthScaleFactor = 1.0;
		
		
		inBlock(inSubstring, StringUtilities_Cell(kCellCount), inSubstringRange,
				kWidthScaleFactor, outStop);
	});
}// ForEachComposedCellClusterInRange



/*!
Calls StringUtilities_ForEachComposedCharacterSequenceInRange()
for the entire length of the string, starting at 0.

(2017.01)
*/
void
StringUtilities_ForEachComposedCharacterSequence	(CFStringRef								inString,
													 StringUtilities_ComposedCharacterBlock		inBlock)
{
	if (nullptr != inString)
	{
		StringUtilities_ForEachComposedCharacterSequenceInRange(inString, CFRangeMake(0, CFStringGetLength(inString)), inBlock);
	}
}// ForEachComposedCharacterSequence


/*!
Similar to "enumerateSubstringsInRange:options:usingBlock:"
from NSString but crucially iterates even over ranges that
are skipped (for an unknown reason) by Apple’s method, such
as those having the “replacement character” sequence.  This
is therefore expected to actually iterate over sequences in
any string, not just those selected by Apple.

(2017.01)
*/
void
StringUtilities_ForEachComposedCharacterSequenceInRange		(CFStringRef								inString,
															 CFRange									inRange,
															 StringUtilities_ComposedCharacterBlock		inBlock)
{
	if ((nullptr != inString) && (inRange.length > 0))
	{
		NSString*			asNSString = BRIDGE_CAST(inString, NSString*);
		__block Boolean		anyIterations = false;
		
		
		// perform an iteration over individual character sequences that are
		// displayed as one symbol (for example, sometimes two 16-bit values
		// correspond to one logical displayed symbol)
		[asNSString
		enumerateSubstringsInRange:NSMakeRange(inRange.location, inRange.length)
		options:(NSStringEnumerationByComposedCharacterSequences)
		usingBlock:^(NSString*	inSubstring,
					 NSRange	inSubstringRange,
					 NSRange	UNUSED_ARGUMENT(inEnclosingRange), // not significant for this iteration type
					 BOOL*		outStopPtr)
		{
			Boolean		stopIter = false;
			
			
			anyIterations = true;
			inBlock(BRIDGE_CAST(inSubstring, CFStringRef), CFRangeMake(inSubstringRange.location, inSubstringRange.length),
					stopIter);
			*outStopPtr = ((stopIter) ? YES : NO);
		}];
		
		// NOTE: the "enumerateSubstringsInRange:options:usingBlock:" method
		// will NOT iterate over ranges in a string that contain sequences
		// such as the replacement-character (and perhaps other Unicode values
		// that are arguably invalid); while this might make sense to Apple,
		// it means the loop above is not truly generic and a work-around
		// must be forced to ensure that all possible symbols can be displayed
		if ((false == anyIterations) && (inRange.length > 0))
		{
			// string has data but no iteration occurred; force it now
			for (CFIndex i = inRange.location; i < (inRange.location + inRange.length); ++i)
			{
				CFRange				subRange = CFRangeMake(i, 1);
				CFRetainRelease		subString(CFStringCreateWithSubstring
												(kCFAllocatorDefault, inString, subRange),
												CFRetainRelease::kAlreadyRetained);
				Boolean				stopIter = false;
				
				
				inBlock(subString.returnCFStringRef(), subRange, stopIter);
				if (stopIter)
				{
					break;
				}
			}
		}
	}
}// ForEachComposedCharacterSequenceInRange


/*!
Returns a string consisting of the specified number of
blank characters.

This reuses substrings of an internal buffer to try to
minimize actual string allocation but allocation could
still occur if the given length is greater than the
cache.

(2021.04)
*/
CFStringRef
StringUtilities_ReturnBlankStringCopy	(CFIndex	inLength)
{
	static dispatch_once_t		onceToken;
	static CFStringRef			gBlankString = nullptr;
	static CFIndex				gBlankLength = 0;
	CFStringRef					result = nullptr;
	
	
	dispatch_once(&onceToken,
	^{
		gBlankString = CFSTR("                                                  "
							 "                                                  "
							 "                                                  "
							 "                                                  ");
		gBlankLength = CFStringGetLength(gBlankString);
	});
	
	if (inLength > gBlankLength)
	{
		CFMutableStringRef		mutableString = CFStringCreateMutable(kCFAllocatorDefault, inLength);
		
		
		CFStringPad(mutableString, CFSTR(" "), inLength, 0/* padding offset */);
		result = mutableString;
	}
	else
	{
		result = CFStringCreateWithSubstring(kCFAllocatorDefault, gBlankString, CFRangeMake(0, inLength));
	}
	
	return result;
}// ReturnBlankStringCopy


/*!
Returns the number of “characters” from the start of the
given string that are needed to cover the specified number
of cells.  (This can be used to construct substrings, for
example.)

Note that the accuracy of this call, and even its speed,
depends on how the cell spans of glyphs are determined.
See the "StringUtilities_Cell" documentation for cell
information, and see StringUtilities_StudyInRange() to see
characteristics of strings that may be tracked when finding
cells.

The last symbol that intersects the given range of cells
may be wider than the end of the range; if so, the behavior
depends on "inPartialSymbolRule".  When the rule is to bind
to the next symbol, the effective cell index is greater
than the requested cell; when the rule is to bind to the
previous symbol, the effective cell index is smaller.

The returned character index refers to the 16-bit elements
of a Core Foundation String object and not the number of
Unicode values formed after composing sequences, etc.

This is useful for anything that depends on the visual
presentation of a string, such as a terminal view.  It
allows you to start from a visual description, like the
range of text selected by the user, and convert into the
part of the string representing that region of cells.

(2021.04)
*/
CFIndex
StringUtilities_ReturnCharacterIndexForCell		(CFStringRef							UNUSED_ARGUMENT(inStringData),
												 StringUtilities_Cell					inCellIndex,
												 StringUtilities_PartialSymbolRule		UNUSED_ARGUMENT(inPartialSymbolRule))
{
	// TEMPORARY; this is simplified for porting purposes
	// and assumes the index matches the cell for now
	// (an alternate version of this has been developed
	// that determines the exact range of each displayed
	// glyph in the given string and it will be able to
	// return correct ranges in complex cases)
	CFIndex		result = inCellIndex.columns_;
	
	
	return result;
}// ReturnCharacterIndexForCell


/*!
This is equivalent to but potentially faster than calling
StringUtilities_ReturnCharacterIndexForCell() for both ends
of a range.

(2021.04)
*/
CFRange
StringUtilities_ReturnSubstringRangeForCellRange	(CFStringRef							UNUSED_ARGUMENT(inStringData),
													 StringUtilities_Cell					inCellStartIndex,
													 StringUtilities_Cell					inCellCount,
													 StringUtilities_PartialSymbolRule		UNUSED_ARGUMENT(inPartialSymbolRule1),
													 StringUtilities_PartialSymbolRule		UNUSED_ARGUMENT(inPartialSymbolRule2))
{
	// TEMPORARY; this is simplified for porting purposes
	// and assumes the indexes matches the cells for now
	// (an alternate version of this has been developed
	// that can traverse the string and determine the
	// exact ranges occupied by glyphs in each cell)
	CFRange		result = CFRangeMake(inCellStartIndex.columns_, inCellCount.columns_);
	
	
	return result;
}// ReturnSubstringRangeForCellRange


/*!
Given a substring representing a single composed character
sequence (such as that returned by the iteration method
"enumerateSubstringsInRange:options:usingBlock:"), returns
the precise Unicode value that is represented.

This is a short-cut for extracting a UTF-8 string and
using the UTF8Decoder module.

Returns "kUTF8Decoder_InvalidUnicodeCodePoint" if the given
string is not valid.

(2017.01)
*/
UnicodeScalarValue
StringUtilities_ReturnUnicodeSymbol		(CFStringRef		inSingleComposedCharacterSubstring)
{
	UnicodeScalarValue	result = kUTF8Decoder_InvalidUnicodeCodePoint;
	NSString*			asNSString = BRIDGE_CAST(inSingleComposedCharacterSubstring, NSString*);
	
	
	if (nil != asNSString)
	{
		char const*		asUTF8 = [asNSString UTF8String];
		size_t			byteCountUTF8 = CPP_STD::strlen(asUTF8);
		
		
		result = UTF8Decoder_StateMachine::byteSequenceTotalValue
					(asUTF8, 0/* offset */, byteCountUTF8,
						nullptr/* bytes used */);
	}
	
	return result;
}// ReturnUnicodeSymbol


/*!
Calls StringUtilities_StudyInRange() for the entire length of the
string, starting at 0.

(2017.04)
*/
void
StringUtilities_Study	(CFStringRef						inString,
						 StringUtilities_DataFromStudy&		outData)
{
	if (nullptr != inString)
	{
		StringUtilities_StudyInRange(inString, CFRangeMake(0, CFStringGetLength(inString)), outData);
	}
}// Study


/*!
The StringUtilities_StudyInRange() function can be used to capture
information about a string that may be useful in other operations
on EXACTLY the same string.  If the string is changed, any data
you have captured should be discarded and the function should be
called again to ensure accuracy.  To help prevent errors, the
hash value of the given string is also stored and functions that
read the study data can easily tell if their source string does
not have the same hash value as the string that was studied.

Functions that use data from a study will always accept the data
optionally, regenerating it from scratch when not provided.

This should be used as an optimization, albeit an important one.

(2017.04)
*/
void
StringUtilities_StudyInRange	(CFStringRef						inString,
								 CFRange							inRange,
								 StringUtilities_DataFromStudy&		outData)
{
	NSString*	asNSString = [BRIDGE_CAST(inString, NSString*)
								substringWithRange:NSMakeRange(inRange.location, inRange.length)];
	NSString*	baseString = asNSString; // may change to skip trivial range
	
	
	outData.stringHashValue_ = CFHash(inString);
	outData.composedCharacterSequenceCount_ = 0; // initialize (incremented below)
	outData.firstNonTrivialCCSIndex_ = 0; // initialize (incremented below)
	if (asNSString.length > 0)
	{
		__block BOOL		sawNonTrivialChar = NO;
		__block CFIndex		baseStringOffset = 0;
		
		
		StringUtilities_ForEachComposedCharacterSequenceInRange
		(inString, inRange,
		^(CFStringRef	inSubstring,
		  CFRange		inSubstringRange,
		  Boolean&		UNUSED_ARGUMENT(outStop))
		{
			if ((NO == sawNonTrivialChar) &&
				((inSubstringRange.length > 1) &&
					(StringUtilities_ReturnUnicodeSymbol(inSubstring) > 0x7F)))
			{
				// arbitrary optimization: keep track of the longest range
				// of “trivial” characters from the beginning that will
				// definitely have a cell (column) count of exactly 1.00,
				// avoiding expensive layout calculations in many cases
				sawNonTrivialChar = YES;
			}
			++(outData.composedCharacterSequenceCount_);
			if (NO == sawNonTrivialChar)
			{
				++(outData.firstNonTrivialCCSIndex_);
				baseStringOffset += inSubstringRange.length;
			}
		});
		baseString = [asNSString substringWithRange:NSMakeRange(baseStringOffset, asNSString.length - baseStringOffset)];
	}
}// StudyInRange


#pragma mark Internal Methods: Unit Tests
namespace {

/*!
Tests StringUtilities_ReturnUnicodeSymbol().

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.01)
*/
Boolean
unitTest_ReturnUnicodeSymbol_000 ()
{
	UInt8 const		kInvalidStr2UTF8[] = { 0x80, 0xBF, '\0' }; // a couple of continuation bytes (invalid UTF-8)
	char const*		kInvalidCStr2UTF8 = REINTERPRET_CAST(kInvalidStr2UTF8, char const*);
	CFStringRef		testString1 = BRIDGE_CAST(@"A", CFStringRef);
	CFStringRef		testString2 = BRIDGE_CAST(@"\U00000152", CFStringRef); // joined OE character
	CFStringRef		testString3 = BRIDGE_CAST(@"\U00002602", CFStringRef); // umbrella character
	CFStringRef		testString4 = BRIDGE_CAST(@"\U000011A8", CFStringRef); // a code that represents several visible glyphs
	CFStringRef		invalidTestString1 = nullptr;
	CFStringRef		invalidTestString2 = BRIDGE_CAST([NSString stringWithUTF8String:kInvalidCStr2UTF8], CFStringRef);
	Boolean			result = true;
	
	
	// simple one-byte case
	result &= Console_Assert("Unicode symbol, one-byte", 'A' == StringUtilities_ReturnUnicodeSymbol(testString1));
	
	// two-byte case
	result &= Console_Assert("Unicode symbol, two-byte", 0x00000152/* 32-bit code for joined OE character */ == StringUtilities_ReturnUnicodeSymbol(testString2));
	
	// three-byte case
	result &= Console_Assert("Unicode symbol, three-byte", 0x00002602/* 32-bit code for umbrella character */ == StringUtilities_ReturnUnicodeSymbol(testString3));
	
	// long-width case
	result &= Console_Assert("Unicode symbol, long-width", 0x000011A8/* 32-bit code for umbrella character */ == StringUtilities_ReturnUnicodeSymbol(testString4));
	
	// illegal cases (note that there are a huge number of
	// byte combinations that are not legal, and many tests
	// can be found online; it is recommended that other
	// corner cases be tried from the terminal itself)
	result &= Console_Assert("Unicode symbol, illegal case 1", kUTF8Decoder_InvalidUnicodeCodePoint == StringUtilities_ReturnUnicodeSymbol(invalidTestString1));
	result &= Console_Assert("Unicode symbol, illegal case 2", kUTF8Decoder_InvalidUnicodeCodePoint == StringUtilities_ReturnUnicodeSymbol(invalidTestString2));
	
	return result;
}// unitTest_ReturnUnicodeSymbol_000


/*!
Tests StringUtilities_StudyInRange().

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.04)
*/
Boolean
unitTest_StudyInRange_000 ()
{
	Boolean		result = true;
	
	
	// nullptr case
	{
		StringUtilities_DataFromStudy	studyData;
		
		
		StringUtilities_Study(nullptr, studyData);
		result &= Console_Assert("study, nullptr", 0 == studyData.stringHashValue_);
		result &= Console_Assert("study, nullptr", 0 == studyData.composedCharacterSequenceCount_);
		result &= Console_Assert("study, nullptr", 0 == studyData.firstNonTrivialCCSIndex_);
	}
	
	// empty case
	{
		StringUtilities_DataFromStudy	studyData;
		
		
		StringUtilities_Study(CFSTR(""), studyData);
		result &= Console_Assert("study, empty, hash", 0 == studyData.stringHashValue_);
		result &= Console_Assert("study, empty, #CCS", 0 == studyData.composedCharacterSequenceCount_);
		result &= Console_Assert("study, empty, 1st non-trivial index", 0 == studyData.firstNonTrivialCCSIndex_);
	}
	
	// ASCII case
	{
		StringUtilities_DataFromStudy	studyData;
		CFStringRef						testString = BRIDGE_CAST(@"Hello, world!", CFStringRef);
		
		
		StringUtilities_Study(testString, studyData);
		result &= Console_Assert("study, ASCII, hash", 0 != studyData.stringHashValue_);
		result &= Console_Assert("study, ASCII, #CCS", 13 == studyData.composedCharacterSequenceCount_);
		result &= Console_Assert("study, ASCII, 1st non-trivial index", 13 == studyData.firstNonTrivialCCSIndex_);
	}
	
	// mixed non-trivial case
	{
		StringUtilities_DataFromStudy	studyData;
		CFStringRef						testString = BRIDGE_CAST(@"eée😐cçz", CFStringRef); // some normal and accented characters with an Emoji in the middle
		
		
		StringUtilities_Study(testString, studyData);
		result &= Console_Assert("study, mixed, hash", 0 != studyData.stringHashValue_);
		result &= Console_Assert("study, mixed, #CCS", 7 == studyData.composedCharacterSequenceCount_);
		result &= Console_Assert("study, mixed, length", 8 == CFStringGetLength(testString));
		result &= Console_Assert("study, mixed, 1st non-trivial index", 3 == studyData.firstNonTrivialCCSIndex_);
	}
	
	// entirely non-trivial case
	{
		StringUtilities_DataFromStudy	studyData;
		CFStringRef						testString = BRIDGE_CAST(@"😐😐😐😐😐", CFStringRef); // only Emoji
		
		
		StringUtilities_Study(testString, studyData);
		result &= Console_Assert("study, Emoji, hash", 0 != studyData.stringHashValue_);
		result &= Console_Assert("study, Emoji, #CCS", 5 == studyData.composedCharacterSequenceCount_);
		result &= Console_Assert("study, Emoji, length", 10 == CFStringGetLength(testString));
		result &= Console_Assert("study, Emoji, 1st non-trivial index", 0 == studyData.firstNonTrivialCCSIndex_);
	}
	
	return result;
}// unitTest_StudyInRange_000

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
