/*!	\file FlagManager.h
	\brief This module stores and retrieves true/false values
	globally, so that they are easily accessible.
*/
/*###############################################################

	Data Access Library
	© 1998-2016 by Kevin Grant
	
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

#ifndef __FLAGMANAGER__
#define __FLAGMANAGER__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef UInt16 FlagManager_Flag;
enum
{
	kFlagManager_FirstValidFlag		= 0,
	kFlagManager_LastValidFlag		= 31
};



#pragma mark Public Methods

//!\name Accessing True/False Values
//@{

void
	FlagManager_Set			(FlagManager_Flag		inFlag,
							 Boolean				inIsSet);

Boolean
	FlagManager_Test		(FlagManager_Flag		inFlag);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
