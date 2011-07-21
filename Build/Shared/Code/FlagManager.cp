/*!	\file FlagManager.cp
	\brief This module stores and retrieves true/false values
	globally, so that they are easily accessible.
*/
/*###############################################################

	Data Access Library 2.5
	© 1998-2011 by Kevin Grant
	
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

#include "UniversalDefines.h"

// standard-C++ includes
#include <stdexcept>

// Mac includes
#include <CoreFoundation/CoreFoundation.h>

// library includes
#include <FlagManager.h>



#pragma mark Variables
namespace {

UInt32&		gFlagSet1 () { static UInt32 x = 0L; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Sets or clears the specified global flag, which must be between
kFlagManager_FirstValidFlag and kFlagManager_LastValidFlag.

(1.0)
*/
void
FlagManager_Set		(FlagManager_Flag	inFlag,
					 Boolean			inIsSet)
{
	if (inFlag < (1 << 5))
	{
		if (inIsSet)
		{
			gFlagSet1() |= (1 << inFlag); // 32 flags are accessed quickly
		}
		else
		{
			gFlagSet1() &= (~(1 << inFlag));
		}
	}
	else
	{
		throw std::runtime_error("FlagManager_Set() cannot set more than 32 bits");
	}
}// Set


/*!
Determines the on/off state of the specified global flag,
which must be between kFlagManager_FirstValidFlag and
kFlagManager_LastValidFlag.

(1.0)
*/
Boolean
FlagManager_Test	(FlagManager_Flag	inFlag)
{
	Boolean		result = false;
	
	
	if (inFlag < (1 << 5))
	{
		result = ((gFlagSet1() & (1 << inFlag)) != 0);
	}
	else
	{
		throw std::runtime_error("FlagManager_Test() cannot test more than 32 bits");
	}
	return result;
}// Test

// BELOW IS REQUIRED NEWLINE TO END FILE
