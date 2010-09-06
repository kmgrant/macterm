/*###############################################################

	FlagManager.cp
	
	This module allows you to store and retrieve flags that
	represent anything, such as whether or not something exists.
	
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

// Mac includes
#include <CoreFoundation/CoreFoundation.h>

// library includes
#include <FlagManager.h>



#pragma mark Constants

enum
{
	kFlagCountPerSet = 16		// number of bits in "bits" field of the structure
};

#pragma mark Types

struct FlagSet
{
	UInt16				bits;	// the bit width of this field must be at least "kFlagCountPerSet"
	struct FlagSet*		next;
};
typedef struct FlagSet 	FlagSet;
typedef FlagSet*		FlagSetPtr;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	FlagSet*	gFlagsPtr = nullptr;
	UInt32		gRocketFlags = 0L;
}

#pragma mark Internal Method Prototypes

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	void		newFlagSet		(FlagSetPtr*);
}



#pragma mark Public Methods

/*!
Call this routine before using any other
routine from this module.

(1.0)
*/
void
FlagManager_Init ()
{
	gRocketFlags = 0L;
}// Init


/*!
Call this routine when you are completely
done with the Flag Manager, so it can
dispose of its internal storage.

(1.0)
*/
void
FlagManager_Done ()
{
	FlagSetPtr		setPtr = gFlagsPtr;
	
	
	gRocketFlags = 0L;
	if (setPtr != nullptr)
	{
		Ptr		memoryManagerPtr = nullptr;
		
		
		while (setPtr != nullptr)
		{
			memoryManagerPtr = (Ptr)setPtr;
			setPtr = setPtr->next;
			CFAllocatorDeallocate(kCFAllocatorDefault, memoryManagerPtr);
		}
	}
}// Done


/*!
Sets a new or existing flag.  Flags are stored in
groups: it is efficient to access up to 32 flags,
after which it is necessary to first find the
proper set in the linked list before accessing
the flag.

If the flag "inFlag" does not exist and is more
than 32, this routine will create as many sets as
needed to make that flag accessible, and then set
it as indicated.  For this reason, all of your
application’s flags should be consecutive and
relative to "kFlagManager_FirstValidFlag", and
the “most needed” flags should be the first 32.

(1.0)
*/
void
FlagManager_Set		(FlagManagerFlag	inFlag,
					 Boolean			inIsSet)
{
	if (inFlag < (1 << 5))
	{
		if (inIsSet) gRocketFlags |= (1 << inFlag); // 32 flags are accessed quickly
		else gRocketFlags &= (~(1 << inFlag));
	}
	else
	{
		// more than 32 flags are allowed, but there’s a performance penalty
		if (gFlagsPtr == nullptr) newFlagSet(&gFlagsPtr);
		if (gFlagsPtr != nullptr)
		{
			FlagSetPtr			setPtr = gFlagsPtr;
			register UInt16		flagCount = 0;
			
			
			// find the set that contains the flag
			while (flagCount < inFlag)
			{
				++flagCount;
				if (flagCount >= kFlagCountPerSet)
				{
					if (setPtr->next == nullptr) newFlagSet(&setPtr->next);
					setPtr = setPtr->next;
				}
			}
			
			// isolate the flag number in the set
			flagCount %= kFlagCountPerSet;
			
			// set or clear the flag
			if (inIsSet) setPtr->bits |= (1 << flagCount);
			else setPtr->bits &= (~(1 << flagCount));
		}
	}
}// Set


/*!
Determines the on/off state of an existing flag.  If
you inquire about an unknown flag, the result is
always false.  This routine is efficient only if the
desired flag is less than 32, so the flags you access
most frequently should be assigned positions from 0
to 31.

(1.0)
*/
Boolean
FlagManager_Test	(FlagManagerFlag	inFlag)
{
	Boolean		result = false;
	
	
	// the first 32 flags are accessed quickly (note that a Boolean isn’t 32 bits, so "!= 0" is required!)
	if (inFlag < (1 << 5)) result = ((gRocketFlags & (1 << inFlag)) != 0);
	else if (gFlagsPtr != nullptr)
	{
		FlagSetPtr			setPtr = gFlagsPtr;
		register UInt16		flagCount = 0;
		Boolean				badFlag = false;
		
		
		// find the set that contains the flag
		while (flagCount < inFlag)
		{
			++flagCount;
			if (flagCount >= kFlagCountPerSet)
			{
				if (setPtr->next == nullptr)
				{
					badFlag = true;
					break;
				}
				setPtr = setPtr->next;
			}
		}
		
		if (!badFlag)
		{
			// isolate the flag number in the set
			flagCount %= kFlagCountPerSet;
			
			// get the flag (note that a Boolean isn’t 32 bits, so "!= 0" is required!)
			result = ((setPtr->bits & (1 << flagCount)) != 0);
		}
	}
	return result;
}// Test


#pragma mark Internal Methods

namespace {


/*!
To create a new set of flags, use this
method.  The flags are set to all-zeroes,
and the "next" flag set is set to nullptr.

(1.0)
*/
void
newFlagSet	(FlagSetPtr*	outFlagsPtr)
{
	if (outFlagsPtr != nullptr)
	{
		*outFlagsPtr = REINTERPRET_CAST(CFAllocatorAllocate
										(kCFAllocatorDefault, sizeof(FlagSet), 0/* hint */), FlagSetPtr);
		if (*outFlagsPtr != nullptr)
		{
			(*outFlagsPtr)->bits = 0;
			(*outFlagsPtr)->next = nullptr;
		}
	}
}// newFlagSet


} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
