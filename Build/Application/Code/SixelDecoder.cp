/*!	\file SixelDecoder.cp
	\brief Implementation of decoder for Sixel graphics commands.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#include "SixelDecoder.h"
#include <UniversalDefines.h>

// standard-C includes
#include <climits>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <ParameterDecoder.h>

// application includes
#include "DebugInterface.h"



#pragma mark Constants
namespace {

SInt16 const	kMy_IntegerAccumulatorOverflowValue = SHRT_MIN;

} // anonymous namespace



#pragma mark Public Methods

/*!
Constructor.

(2017.11)
*/
SixelDecoder_StateMachine::
SixelDecoder_StateMachine ()
:
parameterDecoder(),
paramDecoderPendingState(ParameterDecoder_StateMachine::kStateInitial),
haveSetRasterAttributes(false),
byteRegister('\0'),
repetitionCharacter('?'),
repetitionCount(0),
integerAccumulator(0),
graphicsCursorX(0),
graphicsCursorY(0),
graphicsCursorMaxX(0),
graphicsCursorMaxY(0),
aspectRatioH(0),
aspectRatioV(0),
suggestedImageWidth(0),
suggestedImageHeight(0),
colorCreator(nullptr),
colorChooser(nullptr),
sixelHandler(nullptr),
currentState(kStateInitial)
{
}// SixelDecoder_StateMachine default constructor


/*!
Destructor.

(2017.12)
*/
SixelDecoder_StateMachine::
~SixelDecoder_StateMachine ()
{
	if (nullptr != colorCreator)
	{
		Block_release(colorCreator);
	}
	if (nullptr != colorChooser)
	{
		Block_release(colorChooser);
	}
	if (nullptr != sixelHandler)
	{
		Block_release(sixelHandler);
	}
}// SixelDecoder_StateMachine destructor


/*!
Returns the values of the (up to 6) pixels indicated by the given
raw Sixel data.  The exact meaning of the pixels, such as shape,
must be determined by looking at the rest of the Sixel data stream.

(2017.12)
*/
void
SixelDecoder_StateMachine::
getSixelBits	(UInt8				inByte,
				 std::bitset<6>&	outTopToBottomPixelSetFlags)
{
	// read 6 bits
	UInt8 const		kBitsValue = (inByte - 0x3F);
	
	
	outTopToBottomPixelSetFlags.set(0, (0 != (kBitsValue & 0x01)));
	outTopToBottomPixelSetFlags.set(1, (0 != (kBitsValue & 0x02)));
	outTopToBottomPixelSetFlags.set(2, (0 != (kBitsValue & 0x04)));
	outTopToBottomPixelSetFlags.set(3, (0 != (kBitsValue & 0x08)));
	outTopToBottomPixelSetFlags.set(4, (0 != (kBitsValue & 0x10)));
	outTopToBottomPixelSetFlags.set(5, (0 != (kBitsValue & 0x20)));
}// getSixelBits


/*!
Returns number of dots vertically (for each of the 6 bits) and
horizontally that a “sixel” occupies, for the stored aspect ratio.

See the 4-argument version of this method for details.

(2017.11)
*/
void
SixelDecoder_StateMachine::
getSixelSize	(UInt16&	outSixelHeight,
				 UInt16&	outSixelWidth)
const
{
	SixelDecoder_StateMachine::getSixelSizeFromPanPad(this->aspectRatioV, this->aspectRatioH, outSixelHeight, outSixelWidth);
}// getSixelSize


/*!
Returns number of dots vertically (for each of the 6 bits) and
horizontally that a “sixel” occupies, given an aspect ratio.

The aspect ratio values can correspond directly to the integers
parsed from either the terminal parameters that introduce Sixel
data, or raster attributes in the Sixel data itself.  The ratio
is defined as “pan / pad” but the VT300 series will also round
to the nearest integer; this means that at least one of the two
output height and width will be set to 1, and the other will be
set to a rounded integer based on whichever fraction is bigger
(“pan / pad” or “pad / pan”).

(2017.11)
*/
void
SixelDecoder_StateMachine::
getSixelSizeFromPanPad		(UInt16		inPan,
							 UInt16		inPad,
							 UInt16&	outSixelHeight,
							 UInt16&	outSixelWidth)
{
	// prevent fractions from rounding down to zero: interpret as
	// “round whichever ratio is bigger and set the other side to 1”
	if (inPan > inPad)
	{
		outSixelWidth = 1;
		outSixelHeight = STATIC_CAST(roundf(STATIC_CAST(inPan, Float32) / STATIC_CAST(inPad, Float32)), UInt16);
	}
	else
	{
		outSixelWidth = STATIC_CAST(roundf(STATIC_CAST(inPad, Float32) / STATIC_CAST(inPan, Float32)), UInt16);
		outSixelHeight = 1;
	}
}// getSixelSizeFromPanPad


/*!
Invoked continuously as state transitions occur in the decoder
and commands are found (either individually or as part of a
repetition sequence), this responds by calling any "sixelHandler"
that has been defined.

The raw command character is suitable for a call to getSixelBits()
and the repeat count is at least 1.

(2017.12)
*/
void
SixelDecoder_StateMachine::
handleCommandCharacter	(UInt8		inRawCommandCharacter,
						 UInt16		inRepeatCount)
{
	if (DebugInterface_LogsSixelDecoderState())
	{
		Console_WriteValueCharacter("handle sixel with value", inRawCommandCharacter);
		if (inRepeatCount > 1)
		{
			Console_WriteValue("repeat count", inRepeatCount);
		}
	}
	
	if (nullptr != this->sixelHandler)
	{
		this->sixelHandler(inRawCommandCharacter, inRepeatCount);
	}
	
	this->graphicsCursorX += inRepeatCount;
	if (this->graphicsCursorMaxX < this->graphicsCursorX)
	{
		this->graphicsCursorMaxX = this->graphicsCursorX;
	}
}// handleCommandCharacter


/*!
Returns the state machine to its initial state and clears stored
values, except for blocks installed by setColorChooser(), etc.

(2017.11)
*/
void
SixelDecoder_StateMachine::
reset ()
{
	// these steps should probably agree with the constructor...
	parameterDecoder.reset();
	paramDecoderPendingState = ParameterDecoder_StateMachine::kStateInitial;
	haveSetRasterAttributes = false;
	byteRegister = '\0';
	repetitionCharacter = '?';
	repetitionCount = 0;
	integerAccumulator = 0;
	graphicsCursorX = 0;
	graphicsCursorY = 0;
	graphicsCursorMaxX = 0;
	graphicsCursorMaxY = 0;
	aspectRatioH = 0;
	aspectRatioV = 0;
	suggestedImageWidth = 0;
	suggestedImageHeight = 0;
	// (do not reset any of the blocks here)
	currentState = kStateInitial;
}// SixelDecoder_StateMachine::reset


/*!
Installs a block that will be invoked continuously during parsing
whenever a color is requested.

If this is called more than once, any previous block is released
and no longer invoked by the decoder.

(2017.12)
*/
void
SixelDecoder_StateMachine::
setColorChooser		(SixelDecoder_ColorChooser		inHandler)
{
	if (nullptr != this->colorChooser)
	{
		Block_release(this->colorChooser);
	}
	this->colorChooser = Block_copy(inHandler);
}// setColorChooser


/*!
Installs a block that will be invoked continuously during parsing
whenever a new color is defined.

If this is called more than once, any previous block is released
and no longer invoked by the decoder.

(2017.12)
*/
void
SixelDecoder_StateMachine::
setColorCreator		(SixelDecoder_ColorCreator	inHandler)
{
	if (nullptr != this->colorCreator)
	{
		Block_release(this->colorCreator);
	}
	this->colorCreator = Block_copy(inHandler);
}// setColorCreator


/*!
Installs a block that will be invoked continuously during parsing
whenever a sixel is defined (either as a single value or a
repetition sequence).

If this is called more than once, any previous block is released
and no longer invoked by the decoder.

(2017.12)
*/
void
SixelDecoder_StateMachine::
setSixelHandler		(SixelDecoder_SixelHandler		inHandler)
{
	if (nullptr != this->sixelHandler)
	{
		Block_release(this->sixelHandler);
	}
	this->sixelHandler = Block_copy(inHandler);
}// setSixelHandler


/*!
Uses the current state and the given byte to determine the next
decoder state.

(2017.11)
*/
SixelDecoder_StateMachine::State
SixelDecoder_StateMachine::
stateDeterminant	(UInt8		inNextByte,
					 Boolean&	outByteNotUsed)
{
	State		result = this->currentState;
	Boolean		stateAcceptsAnyInput = false; // initially...
	
	
	outByteNotUsed = false; // initially...
	
	// for certain states, it helps to remember the byte that was read
	// (used in stateTransition())
	this->byteRegister = inNextByte;
	
	// for debugging
	//Console_WriteValueFourChars("Sixel original state", this->currentState);
	//Console_WriteValueCharacter("Sixel byte", inNextByte);
	
	switch (this->currentState)
	{
	case kStateRasterAttrsApplyParams:
	case kStateCarriageReturn:
	case kStateCarriageReturnLineFeed:
	case kStateLineFeed:
	case kStateRepeatApply:
	case kStateSetColorApplyParams:
		{
			// terminating states that take actions return to the default state
			// when finished to ensure that their actions can never be repeated
			// accidentally by remaining in the same state
			result = kStateExpectCommand;
			outByteNotUsed = true;
		}
		break;
	
	case kStateRasterAttrsInitParams:
	case kStateRasterAttrsDecodeParams:
		{
			// determine how next byte will affect parameters (by default, state does not change)
			Boolean		byteNotUsed = true;
			
			
			this->paramDecoderPendingState = this->parameterDecoder.stateDeterminant(inNextByte, byteNotUsed);
			result = kStateRasterAttrsDecodeParams; // initially...
			if (byteNotUsed)
			{
				// end of parameters
				result = kStateRasterAttrsApplyParams;
				outByteNotUsed = true;
			}
		}
		break;
	
	case kStateSetColorInitParams:
	case kStateSetColorDecodeParams:
		{
			// determine how next byte will affect parameters (by default, state does not change)
			Boolean		byteNotUsed = true;
			
			
			this->paramDecoderPendingState = this->parameterDecoder.stateDeterminant(inNextByte, byteNotUsed);
			result = kStateSetColorDecodeParams; // initially...
			if (byteNotUsed)
			{
				// end of parameters
				result = kStateSetColorApplyParams;
				outByteNotUsed = true;
			}
		}
		break;
	
	case kStateRepeatBegin:
		{
			switch (inNextByte)
			{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					result = kStateRepeatReadCount;
					break;
				
				default:
					// ???
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_Warning(Console_WriteValueCharacter, "current state (“repeat begin”) saw unexpected input", inNextByte);
					}
					stateAcceptsAnyInput = true;
					break;
				}
			}
			break;
	
	case kStateRepeatReadCount:
		{
			switch (inNextByte)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				// stay in state as long as there are digits
				result = kStateRepeatReadCount;
				break;
			
			default:
				// non-digit is going to be character to repeat
				result = kStateRepeatExpectCharacter;
				break;
			}
		}
		break;
	
	case kStateRepeatExpectCharacter:
		{
			result = kStateRepeatApply;
			outByteNotUsed = true;
		}
		break;
	
	case kStateInitial:
	case kStateExpectCommand:
	case kStateSetPixels:
	default:
		// for this state, the current state is not critical
		// (only the input)
		stateAcceptsAnyInput = true;
		break;
	}
	
	if (stateAcceptsAnyInput)
	{
		switch (inNextByte)
		{
		case 0x09: // horizontal tab
		case 0x0A: // line feed
		case 0x0D: // carriage return
		case 0x20: // space
			// silently ignore various whitespace arbitrarily since these
			// appear to be common in the output of Sixel data generators
			if (DebugInterface_LogsSixelDecoderState())
			{
				//Console_Warning(Console_WriteValue, "ignoring part of Sixel data, character", this->byteRegister);
			}
			break;
		
		case '"':
			{
				// raster attributes (must be first)
				if (this->haveSetRasterAttributes)
				{
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_Warning(Console_WriteLine, "Sixel raster attributes encountered more than once; ignoring");
					}
					result = kStateSetPixels;
				}
				else
				{
					result = kStateRasterAttrsInitParams;
				}
			}
			break;
		
		case '#':
			{
				// color specification
				result = kStateSetColorInitParams;
			}
			break;
		
		case '$':
			{
				// carriage return
				result = kStateCarriageReturn;
			}
			break;
		
		case '-':
			{
				// line feed
				result = kStateLineFeed;
			}
			break;
		
		case '/':
			{
				// (for printer only) carriage return and line feed
				result = kStateCarriageReturnLineFeed;
			}
			break;
		
		case '!':
			{
				// repetition
				result = kStateRepeatBegin;
			}
			break;
		
		default:
			{
				if ((inNextByte < 0x3F) || (inNextByte > 0x7E))
				{
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_Warning(Console_WriteValueCharacter, "ignoring invalid Sixel data, character", inNextByte);
					}
					outByteNotUsed = true;
				}
				else
				{
					result = kStateSetPixels;
				}
			}
			break;
		}
	}
	
	// for debugging
#if 0
	if (outByteNotUsed)
	{
		Console_WriteLine("                      Sixel byte not used");
	}
	Console_WriteValueFourChars("                      Sixel next state", result);
#endif
	
#if 0
	if (result == this->currentState)
	{
		// not always a problem, just suspicious...
		Console_Warning(Console_WriteValueFourChars, "will still be in same state", result);
		Console_Warning(Console_WriteValueCharacter, "same state with input byte", inNextByte);
	}
#endif
	
	return result;
}// SixelDecoder_StateMachine::stateDeterminant


/*!
Transitions to the specified state from the current state,
performing all appropriate actions.

(2017.11)
*/
void
SixelDecoder_StateMachine::
stateTransition		(State		inNextState)
{
	State const		kPreviousState = this->currentState;
	
	
	// for debugging
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars("Sixel decoder original state", kPreviousState);
		Console_WriteValueFourChars("Sixel decoder transition to state", inNextState);
	}
	
	this->currentState = inNextState;
	
	switch (inNextState)
	{
	case kStateInitial:
		// nothing to do
		break;
	
	case kStateExpectCommand:
		// nothing to do
		break;
	
	case kStateRasterAttrsInitParams:
		{
			if (this->haveSetRasterAttributes)
			{
				if (DebugInterface_LogsSixelDecoderErrors())
				{
					Console_Warning(Console_WriteLine, "Sixel raster attributes encountered more than once; ignoring");
				}
			}
			else
			{
				if (DebugInterface_LogsSixelDecoderState())
				{
					//Console_WriteLine("Sixel raster attributes");
				}
				this->haveSetRasterAttributes = true;
				this->parameterDecoder.reset();
			}
		}
		break;
	
	case kStateRasterAttrsDecodeParams:
		{
			this->parameterDecoder.stateTransition(this->paramDecoderPendingState);
		}
		break;
	
	case kStateRasterAttrsApplyParams:
		if (kPreviousState != inNextState)
		{
			// store all accumulated data
			UInt16		i = 0;
			
			
			this->parameterDecoder.stateTransition(this->paramDecoderPendingState);
			for (SInt16 paramValue : this->parameterDecoder.parameterValues)
			{
				UInt16 const	paramIndex = i;
				
				
				++i;
				
				if (false == this->parameterDecoder.isValidValue(paramValue))
				{
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_WriteLine("found raster attributes parameter with invalid or overflowed value");
					}
					continue;
				}
				
				if (DebugInterface_LogsSixelDecoderState())
				{
					Console_WriteValue("found raster attributes parameter", paramValue);
				}
				
				switch (paramIndex)
				{
				case 0:
					// pan (pixel aspect ratio, numerator)
					if (DebugInterface_LogsSixelDecoderState())
					{
						Console_WriteValue("found pan parameter (pixel aspect ratio, numerator)", paramValue);
					}
					this->aspectRatioV = paramValue;
					break;
				
				case 1:
					// pad (pixel aspect ratio, denominator)
					if (DebugInterface_LogsSixelDecoderState())
					{
						Console_WriteValue("found pad parameter (pixel aspect ratio, denominator)", paramValue);
					}
					this->aspectRatioH = paramValue;
					break;
				
				case 2:
					// suggested image width (used for background fill)
					if (DebugInterface_LogsSixelDecoderState())
					{
						Console_WriteValue("found suggested image width", paramValue);
					}
					this->suggestedImageWidth = paramValue;
					break;
				
				case 3:
					// suggested image height (used for background fill)
					if (DebugInterface_LogsSixelDecoderState())
					{
						Console_WriteValue("found suggested image height", paramValue);
					}
					this->suggestedImageHeight = paramValue;
					break;
				
				default:
					// ???
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_WriteValue("ignoring unexpected raster attributes parameter", paramValue);
					}
					break;
				}
			}
		}
		break;
	
	case kStateCarriageReturn:
		{
			this->graphicsCursorX = 0;
		}
		break;
	
	case kStateCarriageReturnLineFeed:
		{
			this->graphicsCursorX = 0;
			++(this->graphicsCursorY);
			if (this->graphicsCursorMaxY < this->graphicsCursorY)
			{
				this->graphicsCursorMaxY = this->graphicsCursorY;
			}
		}
		break;
	
	case kStateLineFeed:
		{
			// according to the VT330/VT340 manual, a Graphics New Line also
			// causes the cursor to return to the left margin (next line)
			this->graphicsCursorX = 0;
			++(this->graphicsCursorY);
			if (this->graphicsCursorMaxY < this->graphicsCursorY)
			{
				this->graphicsCursorMaxY = this->graphicsCursorY;
			}
		}
		break;
	
	case kStateRepeatBegin:
		this->integerAccumulator = 0;
		break;
	
	case kStateRepeatReadCount:
		switch (this->byteRegister)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			// modify repeat count
			{
				SInt16&			valueRef = this->integerAccumulator;
				SInt16 const	digitValue = (this->byteRegister - '0');
				char const*		overflowType = nullptr;
				
				
				// remove any undefined-value placeholder (-1)
				if (valueRef == kParameterDecoder_ValueUndefined)
				{
					valueRef = 0;
				}
				
				// compute new value that is safe to store; reject overly-large final values
				// (can test this with a very long sequence of digits in a repeat count);
				//
				// this risk is documented in more detail at these links:
				// - https://nvd.nist.gov/vuln/detail/CVE-2022-24130
				// - https://www.openwall.com/lists/oss-security/2022/01/30/3
				SInt16		newValue = valueRef;
				if (__builtin_mul_overflow(newValue, 10, &newValue))
				{
					overflowType = "multiplication";
				}
				else
				{
					if (__builtin_add_overflow(newValue, digitValue, &newValue))
					{
						overflowType = "add";
					}
					else
					{
						if (newValue > kSixelDecoder_RepeatCountMaximum)
						{
							overflowType = "maximum value";
						}
						else
						{
							this->integerAccumulator = newValue;
							this->repetitionCount = this->integerAccumulator;
							if (DebugInterface_LogsSixelDecoderState())
							{
								//Console_WriteValue("updated repetition count", this->repetitionCount);
							}
						}
					}
				}
				
				if (nullptr != overflowType)
				{
					// for debug, report at most one overflow per value (may trigger
					// multiple times through state machine but this resets when
					// the accumulator does)
					if (kMy_IntegerAccumulatorOverflowValue != this->integerAccumulator)
					{
						if (DebugInterface_LogsSixelDecoderErrors())
						{
							Console_WriteValueCString("repetition count too large; overflow type", overflowType);
						}
					}
					else
					{
						this->integerAccumulator = kMy_IntegerAccumulatorOverflowValue;
					}
				}
			}
			break;
		
		default:
			// ???
			if (DebugInterface_LogsSixelDecoderErrors())
			{
				Console_Warning(Console_WriteValue, "assertion failure, repeat-count expected digit but byte ASCII", STATIC_CAST(this->byteRegister, SInt16));
			}
			break;
		}
		break;
	
	case kStateRepeatExpectCharacter:
		{
			// set the character to be repeated
			if ((this->byteRegister < 0x3F) || (this->byteRegister > 0x7E))
			{
				if (DebugInterface_LogsSixelDecoderErrors())
				{
					Console_Warning(Console_WriteValue, "ignoring request for Sixel repetition; out of range, character", this->byteRegister);
				}
				this->repetitionCharacter = '?';
			}
			else
			{
				this->repetitionCharacter = this->byteRegister;
			}
		}
		break;
	
	case kStateRepeatApply:
		if (kPreviousState != inNextState)
		{
			if (DebugInterface_LogsSixelDecoderState())
			{
				Console_WriteValue("repetition count", this->repetitionCount);
				Console_WriteValueCharacter("repetition character", this->repetitionCharacter);
			}
			handleCommandCharacter(this->repetitionCharacter, this->repetitionCount);
			this->repetitionCharacter = '?';
			this->repetitionCount = 0;
		}
		break;
	
	case kStateSetColorInitParams:
		{
			if (DebugInterface_LogsSixelDecoderState())
			{
				//Console_WriteLine("Sixel set color");
			}
			this->parameterDecoder.reset();
		}
		break;
	
	case kStateSetColorDecodeParams:
		{
			this->parameterDecoder.stateTransition(this->paramDecoderPendingState);
		}
		break;
	
	case kStateSetColorApplyParams:
		if (kPreviousState != inNextState)
		{
			// process all accumulated data
			SInt16		colorNumber = 0;
			
			
			this->parameterDecoder.stateTransition(this->paramDecoderPendingState);
			
			if (this->parameterDecoder.parameterValues.size() > 0)
			{
				if (false == this->parameterDecoder.getParameterOrDefault(0, 0/* default value */, colorNumber))
				{
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_WriteLine("failed to read valid color number; ignoring value and resetting to 0");
						colorNumber = 0;
					}
				}
				
				if (DebugInterface_LogsSixelDecoderState())
				{
					Console_WriteValue("color number", colorNumber);
				}
				
				if (1 == this->parameterDecoder.parameterValues.size())
				{
					// select color (as opposed to define-color)
					if (nullptr != this->colorChooser)
					{
						this->colorChooser(colorNumber);
					}
				}
				else
				{
					UInt16		currentIndex = 0;
					
					
					while (currentIndex < this->parameterDecoder.parameterValues.size())
					{
						UInt16 const	startIndex = currentIndex;
						SInt16 const	paramValue = this->parameterDecoder.parameterValues[currentIndex];
						
						
						++currentIndex;
						
						if (false == this->parameterDecoder.isValidValue(paramValue))
						{
							if (DebugInterface_LogsSixelDecoderErrors())
							{
								Console_WriteLine("found set-color parameter with invalid or overflowed value");
							}
							continue;
						}
						
						switch (startIndex)
						{
						case 0:
							// ignore (color number was already processed above)
							break;
						
						case 1: // may process up to 4 total values
							switch (paramValue)
							{
							case 1:
								// hue, lightness and saturation (HLS), a.k.a. hue, saturation and brightness (HSB)
								{
									SInt16		component1Value = kParameterDecoder_ValueUndefined;
									SInt16		component2Value = kParameterDecoder_ValueUndefined;
									SInt16		component3Value = kParameterDecoder_ValueUndefined;
									
									
									currentIndex += 3;
									if ((false == this->parameterDecoder.getParameter(startIndex + 1, component1Value)) ||
										(false == this->parameterDecoder.getParameter(startIndex + 2, component2Value)) ||
										(false == this->parameterDecoder.getParameter(startIndex + 3, component3Value)))
									{
										if (DebugInterface_LogsSixelDecoderErrors())
										{
											Console_WriteLine("color format: error, invalid or overflowed value in one or more parameters for hue, lightness and saturation (HLS)");
										}
										
										if (currentIndex >= this->parameterDecoder.parameterValues.size())
										{
											currentIndex = this->parameterDecoder.parameterValues.size();
											if (DebugInterface_LogsSixelDecoderErrors())
											{
												Console_WriteLine("color format: error, insufficient parameters for hue, lightness and saturation (HLS)");
											}
										}
									}
									else
									{
										if (DebugInterface_LogsSixelDecoderState())
										{
											Console_WriteValue("HLS color, hue component (°)", component1Value);
											Console_WriteValue("HLS color, lightness component (%)", component2Value);
											Console_WriteValue("HLS color, saturation component (%)", component3Value);
										}
										
										if (nullptr != this->colorCreator)
										{
											this->colorCreator(colorNumber, kSixelDecoder_ColorTypeHLS, component1Value, component2Value, component3Value);
										}
									}
								}
								break;
							
							case 2:
								// red, green, blue (RGB)
								{
									SInt16		component1Value = kParameterDecoder_ValueUndefined;
									SInt16		component2Value = kParameterDecoder_ValueUndefined;
									SInt16		component3Value = kParameterDecoder_ValueUndefined;
									
									
									currentIndex += 3;
									if ((false == this->parameterDecoder.getParameter(startIndex + 1, component1Value)) ||
										(false == this->parameterDecoder.getParameter(startIndex + 2, component2Value)) ||
										(false == this->parameterDecoder.getParameter(startIndex + 3, component3Value)))
									{
										if (DebugInterface_LogsSixelDecoderErrors())
										{
											Console_WriteLine("color format: error, invalid or overflowed value in one or more parameters for red, green, blue (RGB)");
										}
										
										if (currentIndex >= this->parameterDecoder.parameterValues.size())
										{
											currentIndex = this->parameterDecoder.parameterValues.size();
											if (DebugInterface_LogsSixelDecoderErrors())
											{
												Console_WriteLine("color format: error, insufficient parameters for red, green, blue (RGB)");
											}
										}
									}
									else
									{
										if (DebugInterface_LogsSixelDecoderState())
										{
											Console_WriteValue("RGB color, red component (%)", component1Value);
											Console_WriteValue("RGB color, green component (%)", component2Value);
											Console_WriteValue("RGB color, blue component (%)", component3Value);
										}
										
										if (nullptr != this->colorCreator)
										{
											this->colorCreator(colorNumber, kSixelDecoder_ColorTypeRGB, component1Value, component2Value, component3Value);
										}
									}
								}
								break;
							
							default:
								// ???
								if (DebugInterface_LogsSixelDecoderErrors())
								{
									Console_Warning(Console_WriteValue, "unrecognized color type", paramValue);
								}
								break;
							}
							break;
						
						default:
							if (DebugInterface_LogsSixelDecoderState())
							{
								Console_WriteValue("found color setting parameter", paramValue);
							}
							break;
						}
					}
				}
			}
		}
		break;
	
	case kStateSetPixels:
		{
			if ((this->byteRegister < 0x3F) || (this->byteRegister > 0x7E))
			{
				// this should only be encountered while in the set-pixels command state
				if (DebugInterface_LogsSixelDecoderErrors())
				{
					Console_Warning(Console_WriteValueCharacter, "ignoring invalid data for set-pixels, character", this->byteRegister);
					//Console_Warning(Console_WriteValueFourChars, "ignoring invalid data while in state", this->currentState);
					//Console_Warning(Console_WriteValueFourChars, "ignoring invalid data when asked to go to state", inNextState);
				}
			}
			else
			{
				handleCommandCharacter(this->byteRegister, 1/* count */);
			}
		}
		break;
	
	default:
		// ???
		if (DebugInterface_LogsSixelDecoderErrors())
		{
			Console_Warning(Console_WriteValueCharacter, "ignoring invalid data for set-pixels, character", this->byteRegister);
		}
		break;
	}
}// SixelDecoder_StateMachine::stateTransition


// BELOW IS REQUIRED NEWLINE TO END FILE
