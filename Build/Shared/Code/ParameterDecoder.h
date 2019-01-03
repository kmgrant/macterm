/*!	\file ParameterDecoder.h
	\brief Implementation of decoder for terminal-style parameters.
*/
/*###############################################################

	Data Access Library
	© 1998-2019 by Kevin Grant
	
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
#include <vector>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
A special parameter value that indicates the parameter is
not defined (e.g. if two delimiters in a row were parsed).
Parameters can also be undefined if the total size of the
parameter list is less than the expected count.
*/
extern SInt16 const		kParameterDecoder_Undefined;

#pragma mark Types

/*!
Valid parameters are nonnegative integers.  Undefined parameters
have value "kParameterDecoder_ParameterUndefined" (also, if
the vector is shorter than expected, the end values are undefined).
*/
typedef std::vector< SInt16 >	ParameterDecoder_IntegerVector;

/*!
Manages the state of decoding a stream of terminal parameters,
following the very common pattern of integers (any number of
digits 0-9) separated by a delimiter such as a semicolon.  The
state machine terminates as soon as any other character is
seen, since terminator characters in terminals are quite varied.
*/
struct ParameterDecoder_StateMachine
{
	enum State
	{
		kStateInitial			= 'init',	//!< the very first state, no bytes have yet been seen
		kStateSeenDigit			= 'xdgt',	//!< new digit defining an integer parameter
		kStateResetParameter	= 'rprm',	//!< a non-digit has been seen
		kStateTerminated		= 'term',	//!< a non-digit, non-delimiter has been seen
	};
	
	ParameterDecoder_IntegerVector	parameterValues;		//!< ordered list of parameter values parsed
	UInt8							delimiterCharacter;		//!< character that identifies a new parameter
	UInt8							byteRegister;			//!< for temporarily holding byte needed between stateDeterminant() and stateTransition()
	
	//! Constructs state machine with optional override for parameter delimiter.
	ParameterDecoder_StateMachine	(UInt8		inDelimiter = ';');
	
	//! Short-cut for combining stateTransition() and stateDeterminant().
	void
	goNextState		(UInt8		inByte,
					 Boolean&	outByteNotUsed)
	{
		this->stateTransition(stateDeterminant(inByte, outByteNotUsed));
	}
	
	//! Returns the state machine to its initial state and clears stored values.
	void
	reset ();
	
	//! Determines a new state based on the current state and the given byte.
	State
	stateDeterminant	(UInt8, Boolean&);
	
	//! Transitions to specified state, taking current state into account.
	void
	stateTransition		(State);
	
	//! Returns the state the machine is in.
	State
	returnState ()
	{
		return currentState;
	}

protected:

private:
	State		currentState;		//!< determines which additional bytes are valid
};


#pragma mark Public Methods

//!\name Module Tests
//@{

void
	ParameterDecoder_RunTests	();

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
