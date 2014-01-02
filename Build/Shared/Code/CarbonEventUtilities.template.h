/*!	\file CarbonEventUtilities.template.h
	\brief A collection of template functions that implement
	highly repetitive tasks related to Carbon Events.
	
	Carbon Events is powerful, but tends to lead to large
	amounts of similar but not identical code for such things
	as handling event parameters.  The routines in this header
	give you C++ templates that generate appropriate code but
	without the hassle of copy-and-paste or the danger of doing
	things incorrectly in some places.
*/
/*###############################################################

	Interface Library
	© 1998-2014 by Kevin Grant
	
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

#ifndef __CARBONEVENTUTILITIES__
#define __CARBONEVENTUTILITIES__

// Mac includes
#include <Carbon/Carbon.h>



#pragma mark Public Methods

/*!
Retrieves an event parameter and copies it into
the space provided.  Automatically asserts that
the type and size are what was expected.

IMPORTANT: This will not work for variable-sized
           data such as "typeChar" arrays; for
           that, use the variable-sized version
           below.

(3.0)
*/
template < typename parameter_ctype >
OSStatus
CarbonEventUtilities_GetEventParameter	(EventRef			inEvent,
										 EventParamName		inParameterName,
										 EventParamType		inParameterType,
										 parameter_ctype&	inoutParameterValue)
{
	OSStatus		result = noErr;
	UInt32 const	kExpectedSize = sizeof(inoutParameterValue);
	UInt32			actualSize = 0L;
	EventParamType	actualType = typeNull;
	
	
	result = GetEventParameter(inEvent, inParameterName, inParameterType, &actualType,
								kExpectedSize, &actualSize, &inoutParameterValue);
	if (noErr == result)
	{
		assert(inParameterType == actualType);
		assert(kExpectedSize == actualSize);
	}
	return result;
}// GetEventParameter


/*!
Retrieves an event parameter that might have an
unknown size, and copies it into the array provided
if possible.  Automatically asserts that the type
is what was expected.

(3.0)
*/
template < typename parameter_carray >
OSStatus
CarbonEventUtilities_GetEventParameterVariableSize	(EventRef			inEvent,
													 EventParamName		inParameterName,
													 EventParamType		inParameterType,
													 parameter_carray&	inoutParameterValue,
													 UInt32&			outActualSize)
{
	OSStatus		result = noErr;
	UInt32 const	kExpectedSize = sizeof(inoutParameterValue);
	EventParamType	actualType = typeNull;
	
	
	result = GetEventParameter(inEvent, inParameterName, inParameterType, &actualType,
								kExpectedSize, &outActualSize, &inoutParameterValue);
	if (noErr == result)
	{
		assert(inParameterType == actualType);
	}
	return result;
}// GetEventParameterVariableSize

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
