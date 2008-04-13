/*###############################################################

	CoercionsAE.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// standard-C includes
#include <cctype>
#include <cstring>

// library includes
#include <Console.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "CoercionsAE.h"
#include "ObjectClassesAE.h"
#include "Terminology.h"



#pragma mark Public Methods

/*!
Handles coercions between objects and other types
using standard means.  This allows objects to be
transformed into any MacTelnet-defined class or
property.

(3.0)
*/
pascal OSErr
CoercionsAE_CoerceObjectToAnotherType	(AEDesc const*	inOriginalDescPtr,
										 DescType		inDesiredType,
										 SInt32			UNUSED_ARGUMENT(inRefCon),
										 AEDesc*		outResultDescPtr)
{
	OSErr		result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("coercing object to token");
	if (inOriginalDescPtr->descriptorType != typeObjectSpecifier) result = errAEWrongDataType;
	else
	{
		AEDesc		token;
		
		
		result = AppleEventUtilities_InitAEDesc(&token);
		assert_noerr(result);
		result = AEResolve(inOriginalDescPtr, kAEIDoMinimum, &token); // *might* do the trick...
		if (result == noErr) result = AECoerceDesc(&token, inDesiredType, outResultDescPtr);
		if (token.dataHandle != nullptr) AEDisposeToken(&token);
	}
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// CoerceObjectToAnotherType

// BELOW IS REQUIRED NEWLINE TO END FILE
