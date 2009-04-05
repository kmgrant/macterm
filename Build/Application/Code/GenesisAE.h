/*!	\file GenesisAE.h
	\brief Allows tokens to “evolve” into more refined types.
	
	This allows you to use a basic object’s data to construct
	the most refined possible version of it (assuming there is
	any object type that inherits from the base type of your
	object).  For instance, if "window 1" turns out to really
	be a "terminal window" (or even a "session window"), that
	realization can happen automatically using this API.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __GENESISAE__
#define __GENESISAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "ObjectClassesAE.h"



#pragma mark Public Methods

OSStatus
	GenesisAE_CreateTokenFromObjectData			(DescType					inClassTypeFromTerminology,
												 ObjectClassesAE_TokenPtr   inoutClassStructurePtr,
												 AEDesc*					inoutNewClassAEDescPtr,
												 DescType*					outFinalClassTypeFromTerminologyOrNull);

OSStatus
	GenesisAE_CreateTokenFromObjectSpecifier	(AEDesc const*				inFromWhat,
												 ObjectClassesAE_TokenPtr   outTokenDataPtr);

// USE THIS TO DETERMINE IF AN UNKNOWN CLASS TYPE “CAN” BE PROMOTED TO A SIMPLER (PARENT) CLASS TYPE
Boolean
	GenesisAE_FirstClassIs						(DescType					inUnknownClassTypeFromTerminology,
												 DescType					inMinimumClassTypeFromTerminology);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
