/*!	\file SixelDecoder.h
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

#include <UniversalDefines.h>

#pragma once

// standard-C++ includes
#include <bitset>
#include <string>
#include <vector>

// library includes
#include <ParameterDecoder.h>

// Mac includes
#include <CoreServices/CoreServices.h>


#pragma mark Types

typedef std::vector< UInt8 >	SixelDecoder_CommandVector;

/*!
Manages the state of decoding a stream of Sixel data.
*/
struct SixelDecoder_StateMachine
{
	enum State
	{
		kStateInitial					= 'init',	//!< the very first state, no bytes have yet been seen
		kStateExpectCommand				= 'root',	//!< default non-initial state, awaiting a valid sequence
		kStateRasterAttrsInitParams		= 'anew',	//!< should begin parsing parameters for raster attributes
		kStateRasterAttrsDecodeParams	= 'aprm',	//!< currently parsing parameters for raster attributes
		kStateRasterAttrsApplyParams	= 'asav',	//!< finished parsing parameters for raster attributes
		kStateSetPixels					= 'spix',	//!< set pixels using 6-bit value
		kStateCarriageReturn			= 'crtn',	//!< move cursor to position 0
		kStateCarriageReturnLineFeed	= 'crlf',	//!< move cursor to position 0 and move cursor down by one
		kStateLineFeed					= 'newl',	//!< move cursor downward to next vertical position
		kStateRepeatBegin				= 'rbgn',	//!< '!' seen; now should see zero or more digits to set a count
		kStateRepeatReadCount			= 'rcnt',	//!< currently parsing digits for count value
		kStateRepeatExpectCharacter		= 'rxch',	//!< '![0-9]+' seen; now should see single command byte to repeat
		kStateRepeatApply				= 'rsav',	//!< apply the repetition values that were parsed
		kStateSetColorInitParams		= 'cnew',	//!< should begin parsing parameters for color setting
		kStateSetColorDecodeParams		= 'cprm',	//!< currently parsing parameters for color setting
		kStateSetColorApplyParams		= 'csav',	//!< finished parsing parameters for color setting
	};
	
	SixelDecoder_CommandVector				commandVector;				//!< ordered list of actions to perform to generate image
	ParameterDecoder_StateMachine			parameterDecoder;			//!< used to parse parameters while in states that recognize parameters
	ParameterDecoder_StateMachine::State	paramDecoderPendingState;	//!< track pending state to prepare for transition step
	Boolean									haveSetRasterAttributes;	//!< tracks whether or not a Raster Attributes request has been seen (")
	UInt8									byteRegister;				//!< for temporarily holding byte needed between stateDeterminant() and stateTransition()
	UInt8									repetitionCharacter;		//!< during repetition parsing, the command character to be repeated "repetitionCount" times
	UInt16									repetitionCount;			//!< during repetition parsing, the number of repetitions encountered (otherwise unused)
	SInt16									integerAccumulator;			//!< reset to 0 but grows as digit characters are encountered
	UInt16									graphicsCursorX;			//!< horizontal position relative to start of image
	UInt16									graphicsCursorY;			//!< vertical position relative to start of image
	UInt16									graphicsCursorMaxX;			//!< largest value ever seen for "graphicsCursorX"
	UInt16									graphicsCursorMaxY;			//!< largest value ever seen for "graphicsCursorY"
	UInt16									aspectRatioH;				//!< a “pad” value (can initialize but may be overridden by parsing raster attributes)
	UInt16									aspectRatioV;				//!< a “pan” value (can initialize but may be overridden by parsing raster attributes)
	UInt16									suggestedImageWidth;		//!< auto-filled background area, width, in “sixels”
	UInt16									suggestedImageHeight;		//!< auto-filled background area, height, in “sixels”
	
	//! Constructs state machine.
	SixelDecoder_StateMachine ();
	
	//! Returns values of the (up to 6) pixels indicated by a raw Sixel data value.
	static void
	getSixelBits	(UInt8, Boolean&, Boolean&, Boolean&, Boolean&, Boolean&, Boolean&);
	
	//! Returns number of dots vertically and horizontally that a “sixel” occupies, at stored aspect ratio.
	void
	getSixelSize	(UInt16&, UInt16&);
	
	//! Returns number of dots vertically and horizontally that a “sixel” occupies, given an aspect ratio.
	static void
	getSixelSizeFromPanPad	(UInt16, UInt16, UInt16&, UInt16&);
	
	//! Short-cut for combining stateTransition() and stateDeterminant().
	void
	goNextState		(UInt8		inByte,
					 Boolean&	outByteNotUsed)
	{
		this->stateTransition(stateDeterminant(inByte, outByteNotUsed));
	}
	
	//! Returns the state machine to its initial state and clears accumulated values.
	void
	reset ();
	
	//! Returns the state the machine is in.
	State
	returnState ()
	{
		return currentState;
	}
	
	//! Determines a new state based on the current state and the given byte.
	State
	stateDeterminant	(UInt8, Boolean&);
	
	//! Transitions to specified state, taking current state into account.
	void
	stateTransition		(State);

protected:

private:
	State		currentState;		//!< determines which additional bytes are valid
};

// BELOW IS REQUIRED NEWLINE TO END FILE
