/*!	\file SixelDecoder.cp
	\brief Implementation of decoder for Sixel graphics commands.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <ParameterDecoder.h>



#pragma mark Public Methods

/*!
Constructor.

(2017.11)
*/
SixelDecoder_StateMachine::
SixelDecoder_StateMachine ()
:
commandVector(),
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
currentState(kStateInitial)
{
}// SixelDecoder_StateMachine default constructor


/*!
Returns the values of the (up to 6) pixels indicated by
the given raw Sixel data.  The exact meaning of the
pixels, such as shape, must be determined by looking at
the rest of the Sixel data stream.

(2017.11)
*/
void
SixelDecoder_StateMachine::
getSixelBits	(UInt8		inByte,
				 Boolean&	outIsTopPixelSet,
				 Boolean&	outIsPixel2Set,
				 Boolean&	outIsPixel3Set,
				 Boolean&	outIsPixel4Set,
				 Boolean&	outIsPixel5Set,
				 Boolean&	outIsPixel6Set)
{
	// read 6 bits
	UInt8 const		kBitsValue = (inByte - 0x3F);
	
	
	outIsTopPixelSet = (0 != (kBitsValue & 0x01));
	outIsPixel2Set = (0 != (kBitsValue & 0x02));
	outIsPixel3Set = (0 != (kBitsValue & 0x04));
	outIsPixel4Set = (0 != (kBitsValue & 0x08));
	outIsPixel5Set = (0 != (kBitsValue & 0x10));
	outIsPixel6Set = (0 != (kBitsValue & 0x20));
}// getSixelBits


/*!
Returns number of dots vertically and horizontally that a
“sixel” occupies, for the stored aspect ratio.

See the 4-argument version of this method for details.

(2017.11)
*/
void
SixelDecoder_StateMachine::
getSixelSize	(UInt16&	outSixelHeight,
				 UInt16&	outSixelWidth)
{
	SixelDecoder_StateMachine::getSixelSizeFromPanPad(this->aspectRatioV, this->aspectRatioH, outSixelHeight, outSixelWidth);
}// getSixelSize


/*!
Returns number of dots vertically and horizontally that a
“sixel” occupies, given an aspect ratio.

The aspect ratio values can correspond directly to the
integers parsed from either the terminal parameters that
introduce Sixel data, or raster attributes in the Sixel
data itself.  The ratio is defined as “pan / pad” but the
VT300 series will also round to the nearest integer; this
means that at least one of the two output height and width
will be set to 1, and the other will be set to a rounded
integer based on whichever fraction is bigger (“pan / pad”
or “pad / pan”).

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
Returns the state machine to its initial state and clears
stored values.

(2017.11)
*/
void
SixelDecoder_StateMachine::
reset ()
{
	// these steps should probably agree with the constructor...
	commandVector.clear();
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
	currentState = kStateInitial;
}// SixelDecoder_StateMachine::reset


/*!
Uses the current state and the given byte to determine
the next decoder state.

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
			if (ParameterDecoder_StateMachine::kStateResetParameter == this->paramDecoderPendingState)
			{
				if (byteNotUsed)
				{
					// end of parameters
					result = kStateRasterAttrsApplyParams;
					outByteNotUsed = true;
				}
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
			if (ParameterDecoder_StateMachine::kStateResetParameter == this->paramDecoderPendingState)
			{
				if (byteNotUsed)
				{
					// end of parameters
					result = kStateSetColorApplyParams;
					outByteNotUsed = true;
				}
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
					Console_Warning(Console_WriteValueCharacter, "current state (“repeat begin”) saw unexpected input", inNextByte);
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
			//Console_Warning(Console_WriteValue, "ignoring part of Sixel data, character", this->byteRegister);
			break;
		
		case '"':
			{
				// raster attributes (must be first)
				if (this->haveSetRasterAttributes)
				{
					Console_Warning(Console_WriteLine, "Sixel raster attributes encountered more than once; ignoring");
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
					Console_Warning(Console_WriteValueCharacter, "ignoring invalid Sixel data, character", inNextByte);
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
	//if (outByteNotUsed)
	//{
	//	Console_WriteLine("                      Sixel byte not used");
	//}
	//Console_WriteValueFourChars("                      Sixel next state", result);
	
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
	//Console_WriteValueFourChars("Sixel original state", kPreviousState);
	//Console_WriteValueFourChars("Sixel transition to state", inNextState);
	
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
				Console_Warning(Console_WriteLine, "Sixel raster attributes encountered more than once; ignoring");
			}
			else
			{
				//Console_WriteLine("Sixel raster attributes");
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
				// TEMPORARY; just log for now
				Console_WriteValue("found raster attributes parameter", paramValue);
				switch (i)
				{
				case 0:
					// pan (pixel aspect ratio, numerator)
					Console_WriteValue("found pan parameter (pixel aspect ratio, numerator)", paramValue);
					this->aspectRatioV = paramValue;
					break;
				
				case 1:
					// pad (pixel aspect ratio, denominator)
					Console_WriteValue("found pad parameter (pixel aspect ratio, denominator)", paramValue);
					this->aspectRatioH = paramValue;
					break;
				
				case 2:
					// suggested image width (used for background fill)
					Console_WriteValue("found suggested image width", paramValue);
					this->suggestedImageWidth = paramValue;
					break;
				
				case 3:
					// suggested image height (used for background fill)
					Console_WriteValue("found suggested image height", paramValue);
					this->suggestedImageHeight = paramValue;
					break;
				
				default:
					// ???
					Console_WriteValue("ignoring unexpected raster attributes parameter", paramValue);
					break;
				}
				++i;
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
			{
				// modify repeat count
				if (this->integerAccumulator < 0)
				{
					this->integerAccumulator = 0;
				}
				this->integerAccumulator *= 10;
				this->integerAccumulator += (this->byteRegister - '0');
				this->repetitionCount = this->integerAccumulator; 
				//Console_WriteValue("updated repetition count", this->repetitionCount);
			}
			break;
		
		default:
			// ???
			Console_Warning(Console_WriteValue, "assertion failure, repeat-count expected digit but byte ASCII", STATIC_CAST(this->byteRegister, SInt16));
			break;
		}
		break;
	
	case kStateRepeatExpectCharacter:
		{
			// set the character to be repeated
			if ((this->byteRegister < 0x3F) || (this->byteRegister > 0x7E))
			{
				Console_Warning(Console_WriteValue, "ignoring request for Sixel repetition; out of range, character", this->byteRegister);
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
			Console_WriteValue("repetition count", this->repetitionCount);
			Console_WriteValueCharacter("repetition character", this->repetitionCharacter);
			commandVector.reserve(commandVector.size() + this->repetitionCount);
			for (UInt16 i = 0; i < this->repetitionCount; ++i)
			{
				commandVector.push_back(this->repetitionCharacter);
			}
		}
		break;
	
	case kStateSetColorInitParams:
		{
			//Console_WriteLine("Sixel set color");
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
			// store all accumulated data
			UInt16		i = 0;
			
			
			this->parameterDecoder.stateTransition(this->paramDecoderPendingState);
			if (this->parameterDecoder.parameterValues.size() > 0)
			{
				UInt16 const	kColorNumber = ((kParameterDecoder_Undefined == this->parameterDecoder.parameterValues[0])
												? 0
												: this->parameterDecoder.parameterValues[0]);
				
				
				Console_WriteValue("set color number", kColorNumber);
			}
			for (SInt16 paramValue : this->parameterDecoder.parameterValues)
			{
				// TEMPORARY; just log for now
				if (0 != i)
				{
					switch (i)
					{
					case 1:
						switch (paramValue)
						{
						case 1:
							// hue, lightness and saturation (HLS), a.k.a. hue, saturation and brightness (HSB)
							if ((i + 3) >= this->parameterDecoder.parameterValues.size())
							{
								Console_WriteLine("color format: error, insufficient parameters for hue, lightness and saturation (HLS)");
							}
							else
							{
								Console_WriteValue("HLS color, hue component (°)", this->parameterDecoder.parameterValues[i + 1]);
								Console_WriteValue("HLS color, lightness component (%)", this->parameterDecoder.parameterValues[i + 2]);
								Console_WriteValue("HLS color, saturation component (%)", this->parameterDecoder.parameterValues[i + 3]);
							}
							break;
						
						case 2:
							// red, green, blue (RGB)
							if ((i + 3) >= this->parameterDecoder.parameterValues.size())
							{
								Console_WriteLine("color format: error, insufficient parameters for red, green, blue (RGB)");
							}
							else
							{
								Console_WriteValue("RGB color, red component (%)", this->parameterDecoder.parameterValues[i + 1]);
								Console_WriteValue("RGB color, green component (%)", this->parameterDecoder.parameterValues[i + 2]);
								Console_WriteValue("RGB color, blue component (%)", this->parameterDecoder.parameterValues[i + 3]);
							}
							break;
						
						default:
							// ???
							Console_Warning(Console_WriteValue, "unrecognized color type", paramValue);
							break;
						}
						break;
					
					default:
						Console_WriteValue("found color setting parameter", paramValue);
						break;
					}
				}
				++i;
			}
		}
		break;
	
	case kStateSetPixels:
		{
			if ((this->byteRegister < 0x3F) || (this->byteRegister > 0x7E))
			{
				// this should only be encountered while in the set-pixels command state
				Console_Warning(Console_WriteValueCharacter, "ignoring invalid data for set-pixels, character", this->byteRegister);
				//Console_Warning(Console_WriteValueFourChars, "ignoring invalid data while in state", this->currentState);
				//Console_Warning(Console_WriteValueFourChars, "ignoring invalid data when asked to go to state", inNextState);
			}
			else
			{
				commandVector.push_back(this->byteRegister);
				++(this->graphicsCursorX);
				if (this->graphicsCursorMaxX < this->graphicsCursorX)
				{
					this->graphicsCursorMaxX = this->graphicsCursorX;
				}
			}
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueCharacter, "ignoring invalid data for set-pixels, character", this->byteRegister);
		break;
	}
}// SixelDecoder_StateMachine::stateTransition

// BELOW IS REQUIRED NEWLINE TO END FILE
