/*!	\file UTF8Decoder.h
	\brief Implementation of decoder for Unicode byte sequences.
*/
/*###############################################################

	Data Access Library
	© 1998-2018 by Kevin Grant
	
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
This value is returned by certain routines to indicate that a
valid Unicode value was not found.
*/
UnicodeScalarValue const	kUTF8Decoder_InvalidUnicodeCodePoint = 0xFFFF;

#pragma mark Types

typedef std::basic_string<UInt8>	UTF8Decoder_ByteString;

/*!
Represents the state of a UTF-8 code point that is in the
process of being decoded from a series of bytes.
*/
struct UTF8Decoder_StateMachine
{
	enum State
	{
		kStateInitial				= 'init',	//!< the very first state, no bytes have yet been seen
		kStateUTF8IllegalSequence	= 'U8XX',	//!< single illegal byte was seen (0xC0, 0xC1, or a value of 0xF5 or greater), or illegal sequence;
												//!  in this case, the "multiByteAccumulator" contains a valid sequence for an error character
		kStateUTF8ValidSequence		= 'U8OK',	//!< the "multiByteAccumulator" contains a valid sequence of 0-6 bytes in UTF-8 encoding
		kStateUTF8ExpectingTwo		= 'U82B',	//!< byte with high bits of "110" received; one more continuation byte (only) should follow
		kStateUTF8ExpectingThree	= 'U83B',	//!< byte with high bits of "1110" received; two more continuation bytes (only) should follow
		kStateUTF8ExpectingFour		= 'U84B',	//!< byte with high bits of "11110" received; three more continuation bytes (only) should follow
		kStateUTF8ExpectingFive		= 'U85B',	//!< byte with high bits of "111110" received; four more continuation bytes (only) should follow
		kStateUTF8ExpectingSix		= 'U86B',	//!< byte with high bits of "1111110" received; five more continuation bytes (only) should follow
	};
	
	UTF8Decoder_ByteString		multiByteAccumulator;		//!< shows all bytes that comprise the most-recently-started UTF-8 code point
	
	UTF8Decoder_StateMachine ();
	
	//! Returns true if the current sequence is incomplete.
	Boolean
	incompleteSequence ();
	
	//! Transitions to a new state based on the current state and the given byte.
	void
	nextState	(UInt8, UInt32&);
	
	//! Returns the state machine to its initial state and clears the accumulator.
	void
	reset ()
	{
		currentState = kStateInitial;
		multiByteAccumulator.clear();
	}
	
	//! Returns the state the machine is in.
	State
	returnState ()
	{
		return currentState;
	}
	
	template < typename push_backable_container >
	static void
	appendErrorCharacter	(push_backable_container&);		//!< insert one or more bytes in UTF-8 encoding to represent an error character
	
	template < typename indexable_container >
	static UnicodeScalarValue
	byteSequenceTotalValue		(indexable_container const&, size_t,	//!< find Unicode value by extracting bits from a sequence of bytes
								 size_t, size_t* = nullptr);
	
	static Boolean	isContinuationByte	(UInt8);	//!< true only for a valid byte that cannot be the first in its sequence
	static Boolean	isFirstOfTwo		(UInt8);	//!< true only for a byte that must be followed by exactly one continuation byte to complete its sequence
	static Boolean	isFirstOfThree		(UInt8);	//!< true only for a byte that must be followed by exactly 2 continuation bytes to complete its sequence
	static Boolean	isFirstOfFour		(UInt8);	//!< true only for a byte that must be followed by exactly 3 continuation bytes to complete its sequence
	static Boolean	isFirstOfFive		(UInt8);	//!< true only for a byte that must be followed by exactly 4 continuation bytes to complete its sequence
	static Boolean	isFirstOfSix		(UInt8);	//!< true only for a byte that must be followed by exactly 5 continuation bytes to complete its sequence
	static Boolean	isIllegalByte		(UInt8);	//!< true for byte values that will never be valid in any situation in UTF-8
	static Boolean	isSingleByteGlyph	(UInt8);	//!< true only for a byte that is itself a complete sequence (normal ASCII)
	static Boolean	isStartingByte		(UInt8);	//!< if false, implies that this is a continuation byte

protected:
	Boolean
	isOverLong ();

private:
	State		currentState;		//!< determines which additional bytes are valid
};


/*!
Appends a valid sequence of bytes to the specified string, that
represent the “invalid character” code point.

(4.0)
*/
template < typename push_backable_container >
inline void
UTF8Decoder_StateMachine::
appendErrorCharacter	(push_backable_container&	inoutContainer)
{
#if 0
	// insert an error character, which arbitrarily will match the one used
	// by VT terminals, a checkered box (Unicode 0x2593 but encoded as UTF-8)
	inoutContainer.push_back(0xE2);
	inoutContainer.push_back(0x96);
	inoutContainer.push_back(0x93);
#else
	// another option is the replacement character (Unicode 0xFFFD but encoded
	// as UTF-8)
	inoutContainer.push_back(0xEF);
	inoutContainer.push_back(0xBF);
	inoutContainer.push_back(0xBD);
#endif
}// UTF8Decoder_StateMachine::appendErrorCharacter


/*!
Returns the complete value represented by the specified range
of bytes in a UTF-8-encoded container.

The count is relative to the offset, and should be at least as
large as the number of continuation bytes implied by the byte
at the offset point.  Any unused bytes at the end are ignored,
as the value will represent bytes referenced by the encoding;
but if you set "outBytesUsedOrNull" to a non-nullptr value, you
can find out how many bytes were required to determine the
value.  (This is useful if you want to pass in an entire buffer
of a set size, and just want to pull the first complete value
off the front.)

By default, "kUTF8Decoder_InvalidUnicodeCodePoint" is returned
(e.g. if the specified byte sequence does not actually decode
properly into the implied number of bytes).

(4.0)
*/
template < typename indexable_container >
inline UnicodeScalarValue
UTF8Decoder_StateMachine::
byteSequenceTotalValue	(indexable_container const&		inBytes,
						 size_t							inOffset,
						 size_t							inByteCount,
						 size_t*						outBytesUsedOrNull)
{
	UnicodeScalarValue		result = kUTF8Decoder_InvalidUnicodeCodePoint;
	
	
	if (nullptr != outBytesUsedOrNull)
	{
		*outBytesUsedOrNull = 0;
	}
	
	if ((inByteCount >= 1) && isSingleByteGlyph(inBytes[inOffset]))
	{
		result = inBytes[inOffset];
		if (nullptr != outBytesUsedOrNull)
		{
			*outBytesUsedOrNull = 1;
		}
	}
	else if ((inByteCount >= 2) && isFirstOfTwo(inBytes[inOffset]) &&
				isContinuationByte(inBytes[inOffset + 1]))
	{
		result = (((inBytes[inOffset] & 0x1F) << 6) +
					((inBytes[inOffset + 1] & 0x3F) << 0));
		if (nullptr != outBytesUsedOrNull)
		{
			*outBytesUsedOrNull = 2;
		}
	}
	else if ((inByteCount >= 3) && isFirstOfThree(inBytes[inOffset]) &&
				isContinuationByte(inBytes[inOffset + 1]) &&
				isContinuationByte(inBytes[inOffset + 2]))
	{
		result = (((inBytes[inOffset] & 0x0F) << 12) +
					((inBytes[inOffset + 1] & 0x3F) << 6) +
					((inBytes[inOffset + 2] & 0x3F) << 0));
		if (nullptr != outBytesUsedOrNull)
		{
			*outBytesUsedOrNull = 3;
		}
	}
	else if ((inByteCount >= 4) && isFirstOfFour(inBytes[inOffset]) &&
				isContinuationByte(inBytes[inOffset + 1]) &&
				isContinuationByte(inBytes[inOffset + 2]) &&
				isContinuationByte(inBytes[inOffset + 3]))
	{
		result = (((inBytes[inOffset] & 0x07) << 18) +
					((inBytes[inOffset + 1] & 0x3F) << 12) +
					((inBytes[inOffset + 2] & 0x3F) << 6) +
					((inBytes[inOffset + 3] & 0x3F) << 0));
		if (nullptr != outBytesUsedOrNull)
		{
			*outBytesUsedOrNull = 4;
		}
	}
	else if ((inByteCount >= 5) && isFirstOfFive(inBytes[inOffset]) &&
				isContinuationByte(inBytes[inOffset + 1]) &&
				isContinuationByte(inBytes[inOffset + 2]) &&
				isContinuationByte(inBytes[inOffset + 3]) &&
				isContinuationByte(inBytes[inOffset + 4]))
	{
		result = (((inBytes[inOffset] & 0x03) << 24) +
					((inBytes[inOffset + 1] & 0x3F) << 18) +
					((inBytes[inOffset + 2] & 0x3F) << 12) +
					((inBytes[inOffset + 3] & 0x3F) << 6) +
					((inBytes[inOffset + 4] & 0x3F) << 0));
		if (nullptr != outBytesUsedOrNull)
		{
			*outBytesUsedOrNull = 5;
		}
	}
	else if ((inByteCount >= 6) && isFirstOfSix(inBytes[inOffset]) &&
				isContinuationByte(inBytes[inOffset + 1]) &&
				isContinuationByte(inBytes[inOffset + 2]) &&
				isContinuationByte(inBytes[inOffset + 3]) &&
				isContinuationByte(inBytes[inOffset + 4]) &&
				isContinuationByte(inBytes[inOffset + 5]))
	{
		result = (((inBytes[inOffset] & 0x01) << 30) +
					((inBytes[inOffset + 1] & 0x3F) << 24) +
					((inBytes[inOffset + 2] & 0x3F) << 18) +
					((inBytes[inOffset + 3] & 0x3F) << 12) +
					((inBytes[inOffset + 4] & 0x3F) << 6) +
					((inBytes[inOffset + 5] & 0x3F) << 0));
		if (nullptr != outBytesUsedOrNull)
		{
			*outBytesUsedOrNull = 6;
		}
	}
	return result;
}// UTF8Decoder_StateMachine::byteSequenceTotalValue


/*!
Returns true only for bytes that are intended to follow a
“first byte” (satisfying isFirstOfTwo(), isFirstOfThree(),
etc.).

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isContinuationByte	(UInt8		inByte)
{
	return (0x80 == (inByte & 0xC0));
}// UTF8Decoder_StateMachine::isContinuationByte


/*!
Returns true only for bytes that indicate the start of a
sequence of exactly two bytes.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isFirstOfTwo	(UInt8		inByte)
{
	return (0xC0 == (inByte & 0xE0));
}// UTF8Decoder_StateMachine::isFirstOfTwo


/*!
Returns true only for bytes that indicate the start of a
sequence of exactly 3 bytes.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isFirstOfThree	(UInt8		inByte)
{
	return (0xE0 == (inByte & 0xF0));
}// UTF8Decoder_StateMachine::isFirstOfThree


/*!
Returns true only for bytes that indicate the start of a
sequence of exactly 4 bytes.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isFirstOfFour	(UInt8		inByte)
{
	return (0xF0 == (inByte & 0xF8));
}// UTF8Decoder_StateMachine::isFirstOfFour


/*!
Returns true only for bytes that indicate the start of a
sequence of exactly 5 bytes.

Not to be confused with Third of Five or Seven of Nine.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isFirstOfFive	(UInt8		inByte)
{
	return (0xF8 == (inByte & 0xFC));
}// UTF8Decoder_StateMachine::isFirstOfFive


/*!
Returns true only for bytes that indicate the start of a
sequence of exactly 6 bytes.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isFirstOfSix	(UInt8		inByte)
{
	return (0xFC == (inByte & 0xFE));
}// UTF8Decoder_StateMachine::isFirstOfSix


/*!
Returns true only for bytes that cannot ever be considered
valid UTF-8, no matter what the context.

Note that this does not reject bytes that could be used to
begin over-long encodings (such as 0xC0).  Those problems
are detected later so that they can be represented as a
single error character.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isIllegalByte	(UInt8		inByte)
{
	return ((0xFE == inByte) || (0xFF == inByte));
}// UTF8Decoder_StateMachine::isIllegalByte


/*!
Returns true only for bytes that are sufficient to describe
entire UTF-8 code points by themselves (i.e. normal ASCII).

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isSingleByteGlyph	(UInt8		inByte)
{
	return (inByte <= 0x7F);
}// UTF8Decoder_StateMachine::isSingleByteGlyph


/*!
Returns true only for bytes that must form the first (or
perhaps only) byte of a code point in the UTF-8 encoding.

(4.0)
*/
inline Boolean
UTF8Decoder_StateMachine::
isStartingByte	(UInt8		inByte)
{
#if 1
	// this should be logically equivalent to checking everything else
	return (false == isContinuationByte(inByte));
#else
	// this is the paranoid form that should never be necessary given
	// the actual definition of the bits in the encoding
	return (isSingleByteGlyph(inByte) || isFirstOfTwo(inByte) || isFirstOfThree(inByte) ||
			isFirstOfFour(inByte) || isFirstOfFive(inByte) || isFirstOfSix(inByte));
#endif
}// UTF8Decoder_StateMachine::isStartingByte

// BELOW IS REQUIRED NEWLINE TO END FILE
