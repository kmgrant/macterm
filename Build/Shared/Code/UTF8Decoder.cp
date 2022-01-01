/*!	\file UTF8Decoder.cp
	\brief Implementation of decoder for Unicode byte sequences.
*/
/*###############################################################

	Data Access Library
	© 1998-2022 by Kevin Grant
	
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

#include "UTF8Decoder.h"
#include <UniversalDefines.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>



#pragma mark Public Methods

/*!
Constructor.

(4.0)
*/
UTF8Decoder_StateMachine::
UTF8Decoder_StateMachine ()
:
multiByteAccumulator(),
currentState(kStateInitial)
{
	this->reset();
}// UTF8Decoder_StateMachine default constructor


/*!
Returns true only if the current sequence of bytes is
incomplete.

(4.0)
*/
Boolean
UTF8Decoder_StateMachine::
incompleteSequence ()
{
	Boolean		result = ((false == this->multiByteAccumulator.empty()) &&
							(kStateInitial != this->currentState) &&
							(kStateUTF8ValidSequence != this->currentState));
	
	
	return result;
}// UTF8Decoder_StateMachine::incompleteSequence


/*!
Returns true if the currently-formed sequence of bytes in
"multiByteAccumulator" is over-long; that is, there is a way
to encode the same point with fewer bytes than were actually
used.  These kinds of sequences are illegal in UTF-8.

(4.0)
*/
Boolean
UTF8Decoder_StateMachine::
isOverLong ()
{
	UTF8Decoder_ByteString const&	kBuffer = this->multiByteAccumulator;
	size_t const					kPastEnd = this->multiByteAccumulator.size();
	size_t const					kSequenceLength = this->multiByteAccumulator.size();
	Boolean							result = false;
	
	
	if (kSequenceLength <= kPastEnd)
	{
		size_t const	kStartIndex = kPastEnd - kSequenceLength;
		
		
		if ((0xC0 == kBuffer[kStartIndex]) ||
			(0xC1 == kBuffer[kStartIndex]))
		{
			// always means the sequence is over-long
			result = true;
		}
		else if (kSequenceLength > 5)
		{
			if ((0xFC == kBuffer[kStartIndex]) &&
				(byteSequenceTotalValue(kBuffer, kStartIndex, 6) <= 0x03FFFFFF))
			{
				// a sequence that starts with 0xFC cannot decode to a value
				// in the 5-byte range (0x03FFFFFF) unless it is over-long
				result = true;
			}
		}
		else if (kSequenceLength > 4)
		{
			if ((0xF8 == kBuffer[kStartIndex]) &&
				(byteSequenceTotalValue(kBuffer, kStartIndex, 5) <= 0x001FFFFF))
			{
				// a sequence that starts with 0xF8 cannot decode to a value
				// in the 4-byte range (0x001FFFFF) unless it is over-long
				result = true;
			}
		}
		else if (kSequenceLength > 3)
		{
			if ((0xF0 == kBuffer[kStartIndex]) &&
				(byteSequenceTotalValue(kBuffer, kStartIndex, 4) <= 0xFFFF))
			{
				// a sequence that starts with 0xF0 cannot decode to a value
				// in the 3-byte range (0xFFFF) unless it is over-long
				result = true;
			}
		}
		else if (kSequenceLength > 2)
		{
			if ((0xE0 == kBuffer[kStartIndex]) &&
				(byteSequenceTotalValue(kBuffer, kStartIndex, 3) <= 0x07FF))
			{
				// a sequence that starts with 0xE0 cannot decode to a value
				// in the 2-byte range (0x07FF) unless it is over-long
				result = true;
			}
		}
	}
	return result;
}// UTF8Decoder_StateMachine::isOverLong


/*!
Uses the current multi-byte decoder state and the given byte
to transition to the next decoder state.  For example, if the
decoder is currently expecting 3 continuation bytes and has
received 2 already, a continuation byte in "inNextByte" will
complete the sequence.

If the given byte indicates an error condition, "outErrorCount"
is greater than 0.  Note that this does NOT mean the given byte
is itself bad; errors may occur for instance if the PREVIOUS
sequence was not completed because the given byte is the start
of a new sequence (possibly indicating that the previous one
did not encounter enough bytes to be completed).  The caller
should arrange to send an error character to the user once for
EACH counted error, and prior to doing anything else as a
result of the current byte.

(4.0)
*/
void
UTF8Decoder_StateMachine::
nextState	(UInt8		inNextByte,
			 UInt32&	outErrorCount)
{
	// for debugging
	//Console_WriteValue("UTF-8 original state", this->currentState);
	//Console_WriteValue("UTF-8 byte", inNextByte);
	
	outErrorCount = 0; // initially...
	
	// automatically handle the starting bytes and continuation bytes of
	// multi-byte sequences; the current decoder state is used as a guide
	if (isSingleByteGlyph(inNextByte))
	{
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		this->multiByteAccumulator.push_back(inNextByte); // should not strictly be necessary to copy this
		this->currentState = kStateUTF8ValidSequence;
	}
	else if (isIllegalByte(inNextByte))
	{
		// a byte value that is never allowed in a valid UTF-8 sequence;
		// note that this must be checked relatively early because other
		// checks that follow examine just a few bits that could well be
		// set for byte values in the illegal range
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		++outErrorCount; // yes, in this case 2 errors in a row are possible
		this->reset();
		this->currentState = kStateUTF8IllegalSequence;
	}
	else if (isFirstOfTwo(inNextByte))
	{
		// first byte in a sequence of 2
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		this->multiByteAccumulator.push_back(inNextByte);
		this->currentState = kStateUTF8ExpectingTwo;
	}
	else if (isFirstOfThree(inNextByte))
	{
		// first byte in a sequence of 3
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		this->multiByteAccumulator.push_back(inNextByte);
		this->currentState = kStateUTF8ExpectingThree;
	}
	else if (isFirstOfFour(inNextByte))
	{
		// first byte in a sequence of 4
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		this->multiByteAccumulator.push_back(inNextByte);
		this->currentState = kStateUTF8ExpectingFour;
	}
	else if (isFirstOfFive(inNextByte))
	{
		// first byte in a sequence of 5
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		this->multiByteAccumulator.push_back(inNextByte);
		this->currentState = kStateUTF8ExpectingFive;
	}
	else if (isFirstOfSix(inNextByte))
	{
		// first byte in a sequence of 6
		if (this->incompleteSequence())
		{
			++outErrorCount;
			this->reset();
		}
		this->multiByteAccumulator.push_back(inNextByte);
		this->currentState = kStateUTF8ExpectingSix;
	}
	else if (isContinuationByte(inNextByte))
	{
		// continuation byte, possibly the final byte; the next state
		// becomes the illegal state if there are now too many bytes,
		// otherwise it flags a complete code point
		Boolean		isIllegal = false;
		size_t		endSize = 0;
		
		
		this->multiByteAccumulator.push_back(inNextByte);
		
		switch (this->currentState)
		{
		case kStateUTF8ExpectingTwo:
			endSize = 2;
			break;
		
		case kStateUTF8ExpectingThree:
			endSize = 3;
			break;
		
		case kStateUTF8ExpectingFour:
			endSize = 4;
			break;
		
		case kStateUTF8ExpectingFive:
			endSize = 5;
			break;
		
		case kStateUTF8ExpectingSix:
			endSize = 6;
			break;
		
		default:
			// ???
			isIllegal = true;
			break;
		}
		
		// a multi-byte sequence is illegal if it has too many continuation
		// bytes or it is “over-long” (e.g. by starting with 0xC0, or in
		// some cases by starting with 0xE0 or 0xF0)
		if ((false == isIllegal) && (0 != endSize))
		{
			isIllegal = ((this->multiByteAccumulator.size() > endSize) || this->isOverLong());
		}
		
		if (isIllegal)
		{
			// previous sequence is now over-long
			//Console_WriteValue("sequence is now over-long, byte count", this->multiByteAccumulator.size());
			++outErrorCount;
			this->reset();
		}
		else if (endSize == this->multiByteAccumulator.size())
		{
			// this is the normal case, the sequence is complete...but even here, if
			// the composed code point is invalid (such as part of a surrogate pair
			// belonging to a different Unicode encoding) it is an error anyway!
			UnicodeScalarValue const	kCodePoint = byteSequenceTotalValue(this->multiByteAccumulator, 0/* start index */,
																			this->multiByteAccumulator.size());
			
			
			if ((kCodePoint >= 0x10FFFF)/* high range of illegal UTF-8 values */ ||
				((kCodePoint >= 0xD800) && (kCodePoint <= 0xDFFF))/* surrogate halves used by UTF-16 */)
			{
				// a technically valid sequence of UTF-8 bytes, but it resolves to an illegal code point
				this->currentState = kStateUTF8IllegalSequence;
				++outErrorCount;
				this->reset();
			}
			else
			{
				// the common case, hopefully...this multi-byte sequence is completely valid!
				//Console_WriteValue("complete sequence, bytes", this->multiByteAccumulator.size());
				this->currentState = kStateUTF8ValidSequence;
			}
		}
		else
		{
			// no error yet; the sequence is not finished yet
			//Console_WriteValue("incomplete sequence, bytes so far", this->multiByteAccumulator.size());
		}
	}
	else
	{
		// ???
	}
	
	// for debugging
	//Console_WriteValue("                      UTF-8 next state", this->currentState);
}// UTF8Decoder_StateMachine::nextState

// BELOW IS REQUIRED NEWLINE TO END FILE
