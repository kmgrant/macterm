/*!	\file StringUtilities.h
	\brief General-purpose routines for dealing with text.
*/
/*###############################################################
	
	Data Access Library
	© 1998-2017 by Kevin Grant
	
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



#pragma mark Types

/*!
Used to iterate over composed character sequences.
*/
typedef void (^StringUtilities_ComposedCharacterBlock) (CFStringRef, CFRange, Boolean&);

/*!
Caches information about a string; for details, see the
StringUtilities_StudyRange() function.

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
	StringUtilities_ForEachComposedCharacterSequence	(CFStringRef,
														 StringUtilities_ComposedCharacterBlock);

void
	StringUtilities_ForEachComposedCharacterSequenceInRange	(CFStringRef,
														 CFRange,
														 StringUtilities_ComposedCharacterBlock);

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
