/*!	\file StringUtilities.h
	\brief General-purpose routines for dealing with text.
*/
/*###############################################################
	
	Data Access Library
	© 1998-2023 by Kevin Grant
	
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

#include <UniversalDefines.h>

#pragma once

// standard-C++ includes
#include <string>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
This determines how to treat symbols that cover more
than one cell (column) when an intersecting range does
not cover the entire region.  For example, when there
is a two-column-span symbol and only one of its columns
is in the target range, this rule decides whether the
symbol is discarded or preserved.
*/
enum StringUtilities_PartialSymbolRule
{
	kStringUtilities_PartialSymbolRulePrevious	= 0,	//!< pretend target cell is earlier (back to end of previous full symbol)
	kStringUtilities_PartialSymbolRuleNext		= 1		//!< pretend target cell is later (ahead to start of next full symbol)
};

#pragma mark Types

/*!
Wrapper for integer values meant to represent columns
(as opposed to array indices or something else).

A common source of bugs would be to treat a character
index the same as a column, and they may be different.
The explicit type makes it easy to ensure correct use.

A “column” or “cell” is the amount of space that would
normally be consumed by a Latin-alphabet letter such as
the letter “A”.  There are composed character sequences
that consume more than one column (like most elements
of Asian languages, among others), and even sequences
that use less than one column (like zero-width space).

Note that although a column count is represented as an
integer, a font rendering could occupy a fractional
number of columns.  Symbols are prevented from bleeding
into neighboring cells by applying scaling factors that
produce integral cell widths.  The scaling factors are
found by “studying” (StringUtilities_StudyInRange()).

See the utility functions below.
*/
struct StringUtilities_Cell
{
	UInt16		columns_;
	
	explicit StringUtilities_Cell	(UInt16		inValue)
	:
	columns_(inValue)
	{
	}
	
	StringUtilities_Cell
	operator + (StringUtilities_Cell const&		other)
	{
		return StringUtilities_Cell(columns_ + other.columns_);
	}
	
	StringUtilities_Cell
	operator - (StringUtilities_Cell const&		other)
	{
		return StringUtilities_Cell(columns_ - other.columns_);
	}
};

/*!
Caches information about a string; for details, see the
StringUtilities_StudyInRange() function.

All of the data is for PRIVATE use by string functions
to gain efficiency.
*/
struct StringUtilities_DataFromStudy
{
	CFHashCode	stringHashValue_;					//!< sanity check to avoid using stale data
	CFIndex		composedCharacterSequenceCount_;	//!< maximum possible array length
	CFIndex		firstNonTrivialCCSIndex_;			//!< first CCS that could be multi-cell
	
	StringUtilities_DataFromStudy ()
	:
	stringHashValue_(0),
	composedCharacterSequenceCount_(0),
	firstNonTrivialCCSIndex_(0)
	{
	}
};

/*!
Used to iterate over composed character sequences whose
widths have been measured for rendering purposes.  For
example, a wide symbol that should be rendered across two
adjacent columns of a terminal has a cell count of 2, and
an Emoji that is slightly wider than 2 cells might have a
scaling factor like 0.9 to indicate that it must be shrunk
in order to fit the specified number of cells (columns).

The arguments are: the composed character sequence string,
its rounded integer cell (terminal column) count, the
substring range of the sequence in the original string,
the scaling factor that should be applied when rendering
to fit the precise-integer cell count, and an output flag
to terminate iteration early if necessary.
*/
typedef void (^StringUtilities_CellBlock) (CFStringRef, StringUtilities_Cell, CFRange, CGFloat, Boolean&);

/*!
Used to iterate over composed character sequences.
*/
typedef void (^StringUtilities_ComposedCharacterBlock) (CFStringRef, CFRange, Boolean&);


#pragma mark Public Methods

//!\name Module Tests
//@{

void
	StringUtilities_RunTests					();

//@}

//!\name String Tokenizing
//@{

CFArrayRef
	StringUtilities_CFNewStringsWithLines		(CFStringRef);

//@}

//!\name String Conversion Utilities
//@{

void
	StringUtilities_CFToUTF8					(CFStringRef,
												 std::string&);

//@}

//!\name Unicode Utilities
//@{

void
	StringUtilities_ForEachComposedCellCluster			(CFStringRef,
														 StringUtilities_CellBlock);

void
	StringUtilities_ForEachComposedCellClusterInRange	(CFStringRef,
														 CFRange,
														 StringUtilities_CellBlock);

void
	StringUtilities_ForEachComposedCharacterSequence	(CFStringRef,
														 StringUtilities_ComposedCharacterBlock);

void
	StringUtilities_ForEachComposedCharacterSequenceInRange	(CFStringRef,
														 CFRange,
														 StringUtilities_ComposedCharacterBlock);

CFStringRef
	StringUtilities_ReturnBlankStringCopy				(CFIndex);

CFIndex
	StringUtilities_ReturnCharacterIndexForCell			(CFStringRef,
														 StringUtilities_Cell,
														 StringUtilities_PartialSymbolRule);

CFRange
	StringUtilities_ReturnSubstringRangeForCellRange	(CFStringRef,
														 StringUtilities_Cell,
														 StringUtilities_Cell,
														 StringUtilities_PartialSymbolRule = kStringUtilities_PartialSymbolRulePrevious,
														 StringUtilities_PartialSymbolRule = kStringUtilities_PartialSymbolRuleNext);

UnicodeScalarValue
	StringUtilities_ReturnUnicodeSymbol					(CFStringRef);

void
	StringUtilities_Study								(CFStringRef,
														 StringUtilities_DataFromStudy&);

void
	StringUtilities_StudyInRange						(CFStringRef,
														 CFRange,
														 StringUtilities_DataFromStudy&);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
