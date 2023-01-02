/*!	\file ParameterDecoder.h
	\brief Implementation of decoder for terminal-style parameters.
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
#include <vector>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
A special parameter value that indicates the parameter has
exceeded the maximum storage space for parameter values.

All special values are negative so you can see if any of
them applies by checking that a value is >= 0.
*/
extern SInt16 const		kParameterDecoder_ValueOverflow;

/*!
A special parameter value that indicates the parameter is
not defined (e.g. if two delimiters in a row were parsed).
Parameters can also be undefined if the total size of the
parameter list is less than the expected count.

All special values are negative so you can see if any of
them applies by checking that a value is >= 0.
*/
extern SInt16 const		kParameterDecoder_ValueUndefined;

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

Only nonnegative parameter values are considered valid.  If you
need more information on why a parameter is not valid, compare
it to one of the special values defined above, e.g. to detect
undefined (empty) values or integer overflow.  The helper
method getParameter() can be useful to distinguish these.
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
	
	//! Helper method for extracting a parameter, with error-checking.
	//! If the result is true, "outValue" is nonnegative (valid);
	//! otherwise, the parameter is invalid but "outValue" is still
	//! set in case you need to know why, e.g. to distinguish the
	//! states of invalid-parameter and overflow.
	bool
	getParameter	(UInt16		inIndex,
					 SInt16&	outValue)
	{
		if (inIndex < parameterValues.size())
		{
			outValue = parameterValues[inIndex];
			if (false == isValidValue(outValue))
			{
				// parameter is stored but is invalid, e.g. undefined or overflow
				return false;
			}
			// parameter is valid (nonnegative)
			return true;
		}
		// out of range
		outValue = kParameterDecoder_ValueUndefined;
		return false;
	}
	
	//! Helper method similar to getParameter() except that the
	//! value "kParameterDecoder_ValueUndefined" is no longer
	//! considered an error; instead, true is returned for that
	//! case and "outValue" is set to "inDefaultValue".  Other
	//! negative cases like "kParameterDecoder_ValueOverflow"
	//! are still considered errors that return false.
	bool
	getParameterOrDefault	(UInt16		inIndex,
							 UInt16		inDefaultValue,
							 SInt16&	outValue)
	{
		bool	result = getParameter(inIndex, outValue);
		
		
		if ((false == result) && (kParameterDecoder_ValueUndefined == outValue))
		{
			outValue = inDefaultValue;
			result = true;
		}
		
		return result;
	}
	
	//! Short-cut for combining stateTransition() and stateDeterminant().
	void
	goNextState		(UInt8		inByte,
					 Boolean&	outByteNotUsed)
	{
		this->stateTransition(stateDeterminant(inByte, outByteNotUsed));
	}
	
	//! Helper method to determine if a value is valid.
	static bool
	isValidValue	(UInt16		inValue)
	{
		return (inValue >= 0);
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
