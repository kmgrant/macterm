/*!	\file BasicTypesAE.cp
	\brief Lists convenient functions for creating Apple Event
	descriptor “wrappers” of commonly-used types.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#include "BasicTypesAE.h"
#include <UniversalDefines.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>

// application includes
#include "AppleEventUtilities.h"
#include "Terminology.h"



#pragma mark Public Methods

/*!
Creates an Apple Event descriptor “wrapper”
for an absolute ordinal (“first”, “all”, etc.).

(3.0)
*/
OSStatus
BasicTypesAE_CreateAbsoluteOrdinalDesc	(DescType	inOrdinalValue,
										 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeAbsoluteOrdinal, &inOrdinalValue, sizeof(inOrdinalValue), outDescPtr);
	return result;
}// CreateAbsoluteOrdinalDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a "true" or "false" value.

(3.0)
*/
OSStatus
BasicTypesAE_CreateBooleanDesc	(Boolean	inBooleanValue,
								 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeBoolean, &inBooleanValue, sizeof(inBooleanValue), outDescPtr);
	return result;
}// CreateBooleanDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a class ID.

(3.0)
*/
OSStatus
BasicTypesAE_CreateClassIDDesc	(FourCharCode	inClassID,
								 AEDesc*		outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeType, &inClassID, sizeof(inClassID), outDescPtr);
	return result;
}// CreateClassIDDesc


/*!
Creates an Apple Event descriptor “wrapper”
for an enumerated type.

(3.0)
*/
OSStatus
BasicTypesAE_CreateEnumerationDesc	(FourCharCode	inEnumeration,
									 AEDesc*		outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeEnumerated, &inEnumeration, sizeof(inEnumeration), outDescPtr);
	return result;
}// CreateEnumerationDesc


/*!
Creates an Apple Event descriptor “wrapper” for a
new-style file system specification record.

(3.1)
*/
OSStatus
BasicTypesAE_CreateFileOrFolderDesc		(FSRef const&	inFSRef,
										 AEDesc*		outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeFSRef, &inFSRef, sizeof(inFSRef), outDescPtr);
	return result;
}// CreateFileOrFolderDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a list containing two object references.
You might use this to create more interesting
assignment statements, e.g. { obj1, obj2 } =
{ value1, value2 }.

(3.0)
*/
OSStatus
BasicTypesAE_CreatePairDesc		(AEDesc const*		inDesc1Ptr,
								 AEDesc const*		inDesc2Ptr,
								 AEDescList*		outDescListPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateList(nullptr/* factoring */, 0/* size */, false/* is a record */, outDescListPtr);
	if (noErr == result)
	{
		result = AEPutDesc(outDescListPtr, 1/* index */, inDesc1Ptr);
		if (noErr == result)
		{
			result = AEPutDesc(outDescListPtr, 2/* index */, inDesc2Ptr);
		}
	}
	
	return result;
}// CreatePairDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a QuickDraw Point.

(3.0)
*/
OSStatus
BasicTypesAE_CreatePointDesc	(Point		inPoint,
								 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(cQDPoint, &inPoint, sizeof(inPoint), outDescPtr);
	return result;
}// CreatePointDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a property ID.

(3.0)
*/
OSStatus
BasicTypesAE_CreatePropertyIDDesc	(FourCharCode	inPropertyID,
									 AEDesc*		outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeType, &inPropertyID, sizeof(inPropertyID), outDescPtr);
	return result;
}// CreatePropertyIDDesc


/*!
Creates an Apple Event descriptor “wrapper”
for text, using Pascal string data.

(3.0)
*/
OSStatus
BasicTypesAE_CreatePStringDesc	(Str255		inPascalStringData,
								 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(cString, inPascalStringData + 1,
							PLstrlen(inPascalStringData) * sizeof(UInt8), outDescPtr);
	return result;
}// CreatePStringDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a QuickDraw rectangle.

(3.0)
*/
OSStatus
BasicTypesAE_CreateRectDesc		(Rect const*	inRectPtr,
								 AEDesc*		outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(cQDRectangle, inRectPtr, sizeof(Rect), outDescPtr);
	return result;
}// CreateRectDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a QuickDraw RGB color.

(3.0)
*/
OSStatus
BasicTypesAE_CreateRGBColorDesc		(RGBColor const*	inColor,
									 AEDesc*			outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(cRGBColor, inColor, sizeof(RGBColor), outDescPtr);
	return result;
}// CreateRGBColorDesc


/*!
Creates an Apple Event descriptor “wrapper”
for a signed, short integer.

(3.0)
*/
OSStatus
BasicTypesAE_CreateSInt16Desc	(SInt16		inInteger,
								 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeSInt16, &inInteger, sizeof(inInteger), outDescPtr);
	return result;
}// CreateSInt16Desc


/*!
Creates an Apple Event descriptor “wrapper”
for a signed, long integer.

(3.0)
*/
OSStatus
BasicTypesAE_CreateSInt32Desc	(SInt32		inInteger,
								 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeSInt32, &inInteger, sizeof(inInteger), outDescPtr);
	return result;
}// CreateSInt32Desc


/*!
Creates an Apple Event descriptor “wrapper”
for an unsigned, long integer.

(3.0)
*/
OSStatus
BasicTypesAE_CreateUInt32Desc	(UInt32		inInteger,
								 AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateDesc(typeUInt32, &inInteger, sizeof(inInteger), outDescPtr);
	return result;
}// CreateUInt32Desc


/*!
Creates an Apple Event descriptor “wrapper”
for a list containing two unsigned, long
integers.

(3.0)
*/
OSStatus
BasicTypesAE_CreateUInt32PairDesc	(UInt32		inInteger1,
									 UInt32		inInteger2,
									 AEDesc*	outDescListPtr)
{
	OSStatus	result = noErr;
	AEDesc		integerDesc;
	
	
	result = AppleEventUtilities_InitAEDesc(&integerDesc);
	assert_noerr(result);
	result = AECreateList(nullptr/* factoring */, 0/* size */, false/* is a record */, outDescListPtr);
	if (noErr == result)
	{
		result = AECreateDesc(typeUInt32, &inInteger1, sizeof(inInteger1), &integerDesc);
		if (noErr == result)
		{
			result = AEPutDesc(outDescListPtr, 1/* index */, &integerDesc);
			(OSStatus)AEDisposeDesc(&integerDesc);
			if (noErr == result)
			{
				result = AECreateDesc(typeUInt32, &inInteger2, sizeof(inInteger2), &integerDesc);
				if (noErr == result)
				{
					result = AEPutDesc(outDescListPtr, 2/* index */, &integerDesc);
					(OSStatus)AEDisposeDesc(&integerDesc);
				}
			}
		}
	}
	
	return result;
}// CreateUInt32PairDesc

// BELOW IS REQUIRED NEWLINE TO END FILE
