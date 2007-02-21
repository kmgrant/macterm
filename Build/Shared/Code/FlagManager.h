/*###############################################################

	FlagManager.h
	
	To organize and streamline storage of and access to those
	inevitable Boolean globals, use this module.  Every modern
	Mac OS application is storing the typical existence flags:
	Color QuickDraw, Apple Guide, Navigation Services, and even
	Contextual Menus.  Now, you can maximize efficiency in
	storing such flags by simply assigning each a unique ID
	number reflecting the flag, and using this module to set or
	test the value.  Assign your application-defined flags
	relative to "kFlagManager_FirstValidFlag".
	
	Data Access Library 1.3
	© 1998-2006 by Kevin Grant
	
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

#ifndef __FLAGMANAGER__
#define __FLAGMANAGER__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef UInt16 FlagManagerFlag;
enum
{
	kFlagManager_FirstValidFlag = 0
};



#pragma mark Public Methods

/*###############################################################
	INITIALIZING AND FINISHING WITH THE FLAG MANAGER
###############################################################*/

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER FLAG MANAGER ROUTINE
void
	FlagManager_Init		();

// CALL THIS ROUTINE AFTER YOU ARE PERMANENTLY DONE WITH THE FLAG MANAGER
void
	FlagManager_Done		();

/*###############################################################
	MANIPULATING FLAG MANAGER FLAGS IN THE GLOBAL SET
###############################################################*/

void
	FlagManager_Set			(FlagManagerFlag				inFlag,
							 Boolean						inIsSet);

Boolean
	FlagManager_Test		(FlagManagerFlag				inFlag);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
