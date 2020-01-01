/*!	\file ParameterDecoder.cp
	\brief Implementation of decoder for terminal-style parameters.
*/
/*###############################################################

	Data Access Library
	© 1998-2020 by Kevin Grant
	
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

#include "ParameterDecoder.h"
#include <UniversalDefines.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>



#pragma mark Constants

SInt16 const		kParameterDecoder_Undefined = -1; // see header file

#pragma mark Internal Method Prototypes
namespace {

Boolean		unitTest_StateMachine_000		();
Boolean		unitTest_StateMachine_001		();
Boolean		unitTest_StateMachine_002		();
Boolean		unitTest_StateMachine_003		();
Boolean		unitTest_StateMachine_004		();

} // anonymous namespace



#pragma mark Public Methods

/*!
A unit test for this module.  This should always
be run before a release, after any substantial
changes are made, or if you suspect bugs!  It
should also be EXPANDED as new functionality is
proposed (ideally, a test is written before the
functionality is added).

(2017.11)
*/
void
ParameterDecoder_RunTests ()
{
	UInt16		totalTests = 0;
	UInt16		failedTests = 0;
	
	
	++totalTests; if (false == unitTest_StateMachine_000()) ++failedTests;
	++totalTests; if (false == unitTest_StateMachine_001()) ++failedTests;
	++totalTests; if (false == unitTest_StateMachine_002()) ++failedTests;
	++totalTests; if (false == unitTest_StateMachine_003()) ++failedTests;
	++totalTests; if (false == unitTest_StateMachine_004()) ++failedTests;
	
	Console_WriteUnitTestReport("Terminal Parameter Decoder", failedTests, totalTests);
}// RunTests


/*!
Constructor.

(2017.11)
*/
ParameterDecoder_StateMachine::
ParameterDecoder_StateMachine	(UInt8		inDelimiter)
:
parameterValues(),
delimiterCharacter(inDelimiter),
byteRegister('\0'),
currentState(kStateInitial)
{
	this->reset();
}// ParameterDecoder_StateMachine default constructor


/*!
Returns the state machine to its initial state and clears
stored values.

(2017.11)
*/
void
ParameterDecoder_StateMachine::
reset ()
{
	// these steps should probably agree with the constructor...
	//Console_WriteLine("terminal parameter parser RESET"); // debug
	parameterValues.clear();
	byteRegister = '\0';
	currentState = kStateInitial;
}// SixelDecoder_StateMachine::reset


/*!
Uses the current state and the given byte to determine
the next decoder state.

(2017.11)
*/
ParameterDecoder_StateMachine::State
ParameterDecoder_StateMachine::
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
	//Console_WriteValueFourChars("terminal parameter parser original state", this->currentState);
	//Console_WriteValueCharacter("terminal parameter parser byte", inNextByte);
	
	switch (this->currentState)
	{
	case kStateTerminated:
		outByteNotUsed = true;
		break;
	
	case kStateInitial:
	case kStateSeenDigit:
	case kStateResetParameter:
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
			result = kStateSeenDigit;
			break;
		
		default:
			// delimiters cause current parameter to be stored,
			// non-delimiters cause the parser to terminate
			if (inNextByte == this->delimiterCharacter)
			{
				result = kStateResetParameter;
			}
			else
			{
				result = kStateTerminated;
				outByteNotUsed = true;
			}
			break;
		}
	}
	
	// for debugging
	//Console_WriteValueFourChars("                      terminal parameter parser next state", result);
	//Console_WriteValue("                      terminal parameter byte unused", outByteNotUsed);
	
	return result;
}// ParameterDecoder_StateMachine::stateDeterminant


/*!
Transitions to the specified state from the current state,
performing all appropriate actions.

(2017.11)
*/
void
ParameterDecoder_StateMachine::
stateTransition		(State		inNextState)
{
	// for debugging
	//Console_WriteValueFourChars("terminal parameter parser original state", this->currentState);
	//Console_WriteValueFourChars("terminal parameter parser transition to state", inNextState);
	
	this->currentState = inNextState;
	
	switch (inNextState)
	{
	case kStateSeenDigit:
		{
			// update parameter value
			if (this->parameterValues.empty())
			{
				this->parameterValues.push_back(kParameterDecoder_Undefined);
			}
			if (kParameterDecoder_Undefined == this->parameterValues.back())
			{
				this->parameterValues.back() = 0;
			}
			this->parameterValues.back() *= 10;
			this->parameterValues.back() += (this->byteRegister - '0');
		}
		break;
	
	case kStateResetParameter:
		// the next character is a delimiter; define a new parameter
		//Console_WriteValue("define parameter with value", this->parameterValues.back()); // debug
		parameterValues.push_back(kParameterDecoder_Undefined);
		break;
	
	case kStateTerminated:
		// the next character is not recognized as parameter syntax
		//Console_WriteValue("define parameter with value", this->parameterValues.back()); // debug
		break;
	
	default:
		// ???
		break;
	}
}// ParameterDecoder_StateMachine::stateTransition


#pragma mark Internal Methods: Unit Tests
namespace {

/*!
Tests ParameterDecoder_StateMachine with “garbage”.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.11)
*/
Boolean
unitTest_StateMachine_000 ()
{
	ParameterDecoder_StateMachine			decoderObject;
	ParameterDecoder_StateMachine::State	proposedState = ParameterDecoder_StateMachine::kStateInitial;
	Boolean									byteNotUsed = false;
	Boolean									result = true;
	
	
	// default object should always have initial state
	{
		auto const		testValue = decoderObject.returnState();
		Console_TestAssertUpdate(result, ParameterDecoder_StateMachine::kStateInitial == testValue,
									Console_WriteValueFourChars, "garbage input test: expected init-state, actual state", testValue);
	}
	
	// for non-parameter data, state transition should not be allowed
	byteNotUsed = false;
	proposedState = decoderObject.stateDeterminant('X', byteNotUsed);
	{
		Console_TestAssertUpdate(result, true == byteNotUsed,
									Console_WriteLine, "garbage input test: expected unused byte 'X'");
	}
	
	return result;
}// unitTest_StateMachine_000


/*!
Tests ParameterDecoder_StateMachine with a single
digit (trivial test).

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.11)
*/
Boolean
unitTest_StateMachine_001 ()
{
	ParameterDecoder_StateMachine			decoderObject;
	ParameterDecoder_StateMachine::State	proposedState = ParameterDecoder_StateMachine::kStateInitial;
	Boolean									byteNotUsed = false;
	Boolean									result = true;
	
	
	// digits are used to define numerical parameters; the
	// parameter values are not “complete” however until
	// the decoder enters a non-digit state
	byteNotUsed = false;
	proposedState = decoderObject.stateDeterminant('2', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "single number test: expected use of byte '2'");
	}
	{
		auto const		testValue = proposedState;
		Console_TestAssertUpdate(result, ParameterDecoder_StateMachine::kStateSeenDigit == testValue,
									Console_WriteValueFourChars, "single number test: '2': expected seen-digit-state, actual state", testValue);
	}
	decoderObject.stateTransition(proposedState);
	{
		auto const		testValue = decoderObject.parameterValues.size();
		Console_TestAssertUpdate(result, 1 == testValue,
									Console_WriteValue, "single number test: '2': expected one parameter, actual count", testValue);
	}
	
	// transition to a finishing state using a byte that
	// cannot represent a parameter or delimiter
	byteNotUsed = false;
	proposedState = decoderObject.stateDeterminant('X', byteNotUsed);
	{
		Console_TestAssertUpdate(result, true == byteNotUsed,
									Console_WriteLine, "single number test: expected unused byte 'X'");
	}
	decoderObject.stateTransition(proposedState);
	{
		auto const		testValue = decoderObject.parameterValues.size();
		Console_TestAssertUpdate(result, 1 == testValue,
									Console_WriteValue, "single number test: actual parameter count", testValue);
		if (result)
		{
			auto const		testValue2 = decoderObject.parameterValues[0];
			Console_TestAssertUpdate(result, 2 == testValue2,
										Console_WriteValue, "single number test: actual parameter value", testValue2);
		}
	}
	
	return result;
}// unitTest_StateMachine_001


/*!
Tests ParameterDecoder_StateMachine with a leading
zero.

NOTE: This test does not also test states because
other unit tests are meant to do that.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.11)
*/
Boolean
unitTest_StateMachine_002 ()
{
	ParameterDecoder_StateMachine			decoderObject;
	ParameterDecoder_StateMachine::State	proposedState = ParameterDecoder_StateMachine::kStateInitial;
	Boolean									byteNotUsed = false;
	Boolean									result = true;
	
	
	// a leading zero should not affect the value
	proposedState = decoderObject.stateDeterminant('0', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "leading-zero test: expected use of byte '0'");
	}
	decoderObject.stateTransition(proposedState);
	proposedState = decoderObject.stateDeterminant('0', byteNotUsed);
	decoderObject.stateTransition(proposedState);
	proposedState = decoderObject.stateDeterminant('3', byteNotUsed);
	decoderObject.stateTransition(proposedState);
	decoderObject.goNextState('0', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "leading-zero test: expected use of byte '0'");
	}
	decoderObject.goNextState('5', byteNotUsed);
	decoderObject.goNextState('0', byteNotUsed);
	proposedState = decoderObject.stateDeterminant(';', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "leading-zero test: expected use of byte ';'");
	}
	decoderObject.stateTransition(proposedState);
	{
		auto const		testValue = proposedState;
		Console_TestAssertUpdate(result, ParameterDecoder_StateMachine::kStateResetParameter == testValue,
									Console_WriteValueFourChars, "leading-zero test: actual state", testValue);
	}
	{
		auto const		testValue = decoderObject.parameterValues.size();
		Console_TestAssertUpdate(result, 2 == testValue,
									Console_WriteValue, "leading-zero test: actual parameter count", testValue);
		if (result)
		{
			auto const		testValue2 = decoderObject.parameterValues[0];
			Console_TestAssertUpdate(result, 3050 == testValue2,
										Console_WriteValue, "leading-zero test: actual 1st parameter", testValue2);
		}
		if (result)
		{
			auto const		testValue3 = decoderObject.parameterValues[1];
			Console_TestAssertUpdate(result, kParameterDecoder_Undefined == testValue3,
										Console_WriteValue, "leading-zero test: actual 2nd parameter", testValue3);
		}
	}
	
	return result;
}// unitTest_StateMachine_002


/*!
Tests ParameterDecoder_StateMachine with more than
one parameter, including empty parameters.

NOTE: This test does not also test states because
other unit tests are meant to do that.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.11)
*/
Boolean
unitTest_StateMachine_003 ()
{
	ParameterDecoder_StateMachine			decoderObject;
	ParameterDecoder_StateMachine::State	proposedState = ParameterDecoder_StateMachine::kStateInitial;
	Boolean									byteNotUsed = false;
	Boolean									result = true;
	
	
	// define multiple integer parameters, with some undefined
	proposedState = decoderObject.stateDeterminant('2', byteNotUsed);
	decoderObject.stateTransition(proposedState);
	proposedState = decoderObject.stateDeterminant(';', byteNotUsed);
	decoderObject.stateTransition(proposedState);
	proposedState = decoderObject.stateDeterminant('3', byteNotUsed);
	decoderObject.stateTransition(proposedState);
	decoderObject.goNextState('0', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "multiple parameters test: '30': expected use of byte '0'");
	}
	decoderObject.goNextState(';', byteNotUsed);
	decoderObject.goNextState(';', byteNotUsed);
	decoderObject.goNextState('5', byteNotUsed);
	decoderObject.goNextState('9', byteNotUsed);
	proposedState = decoderObject.stateDeterminant('1', byteNotUsed);
	decoderObject.stateTransition(proposedState);
	proposedState = decoderObject.stateDeterminant('X', byteNotUsed);
	{
		Console_TestAssertUpdate(result, true == byteNotUsed,
									Console_WriteLine, "multiple parameters test: expected unused byte 'X'");
	}
	decoderObject.stateTransition(proposedState);
	{
		auto const		testValue = decoderObject.parameterValues.size();
		Console_TestAssertUpdate(result, 4 == testValue,
									Console_WriteValue, "multiple parameters test: actual parameter count", testValue);
		if (result)
		{
			auto const		testValue2 = decoderObject.parameterValues[0];
			auto const		testValue3 = decoderObject.parameterValues[1];
			auto const		testValue4 = decoderObject.parameterValues[2];
			auto const		testValue5 = decoderObject.parameterValues[3];
			Console_TestAssertUpdate(result, 2 == testValue2,
										Console_WriteValue, "multiple parameters test: actual parameter value", testValue2);
			Console_TestAssertUpdate(result, 30 == testValue3,
										Console_WriteValue, "multiple parameters test: actual parameter value", testValue3);
			Console_TestAssertUpdate(result, kParameterDecoder_Undefined == testValue4,
										Console_WriteValue, "multiple parameters test: actual parameter value", testValue4);
			Console_TestAssertUpdate(result, 591 == testValue5,
										Console_WriteValue, "multiple parameters test: actual parameter value", testValue5);
		}
	}
	
	return result;
}// unitTest_StateMachine_003


/*!
Tests ParameterDecoder_StateMachine with more than
one parameter and non-default delimiters.

NOTE: This test does not also test states because
other unit tests are meant to do that.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2017.11)
*/
Boolean
unitTest_StateMachine_004 ()
{
	ParameterDecoder_StateMachine	decoderObject(':');
	Boolean							byteNotUsed = false;
	Boolean							result = true;
	
	
	// define multiple integer parameters, with some undefined
	decoderObject.goNextState('2', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "alternate delimiter test: expected use of byte '2'");
	}
	decoderObject.goNextState(':', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "alternate delimiter test: expected use of byte ':'");
	}
	decoderObject.goNextState('1', byteNotUsed);
	decoderObject.goNextState('0', byteNotUsed);
	decoderObject.goNextState(':', byteNotUsed);
	decoderObject.goNextState(':', byteNotUsed);
	decoderObject.goNextState('1', byteNotUsed);
	decoderObject.goNextState('2', byteNotUsed);
	decoderObject.goNextState('3', byteNotUsed);
	{
		Console_TestAssertUpdate(result, false == byteNotUsed,
									Console_WriteLine, "alternate delimiter test: expected use of byte '3'");
	}
	{
		auto const		testValue = decoderObject.returnState();
		Console_TestAssertUpdate(result, ParameterDecoder_StateMachine::kStateSeenDigit == testValue,
									Console_WriteValueFourChars, "alternate delimiter test: '3': actual state", decoderObject.returnState());
	}
	decoderObject.goNextState('?', byteNotUsed);
	{
		Console_TestAssertUpdate(result, true == byteNotUsed,
									Console_WriteLine, "alternate delimiter test: expected unused byte '?'");
	}
	{
		auto const		testValue = decoderObject.returnState();
		Console_TestAssertUpdate(result, ParameterDecoder_StateMachine::kStateTerminated == testValue,
									Console_WriteValueFourChars, "alternate delimiter test: '?': actual state", decoderObject.returnState());
	}
	{
		Boolean			firstTestResult = true;
		auto const		testValue = decoderObject.parameterValues.size();
		Console_TestAssertUpdate(firstTestResult, 4 == testValue,
									Console_WriteValue, "alternate delimiter test: actual parameter count", testValue);
		if (false == firstTestResult)
		{
			result = false;
		}
		else
		{
			auto const		testValue2 = decoderObject.parameterValues[0];
			auto const		testValue3 = decoderObject.parameterValues[1];
			auto const		testValue4 = decoderObject.parameterValues[2];
			auto const		testValue5 = decoderObject.parameterValues[3];
			Console_TestAssertUpdate(result, 2 == testValue2,
										Console_WriteValue, "alternate delimiter test: actual parameter value", testValue2);
			Console_TestAssertUpdate(result, 10 == testValue3,
										Console_WriteValue, "alternate delimiter test: actual parameter value", testValue3);
			Console_TestAssertUpdate(result, kParameterDecoder_Undefined == testValue4,
										Console_WriteValue, "alternate delimiter test: actual parameter value", testValue4);
			Console_TestAssertUpdate(result, 123 == testValue5,
										Console_WriteValue, "alternate delimiter test: actual parameter value", testValue5);
		}
	}
	
	return result;
}// unitTest_StateMachine_004

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
