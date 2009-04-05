/*!	\file BasicTypesAE.h
	\brief Lists convenient functions for creating Apple Event
	descriptor “wrappers” of commonly-used types.
	
	Indispensable for recordable applications.
*/
/*###############################################################

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

#ifndef __BASICTYPESAE__
#define __BASICTYPESAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

OSStatus
	BasicTypesAE_CreateAbsoluteOrdinalDesc	(DescType			inOrdinalValue,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateBooleanDesc			(Boolean			inBooleanValue,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateClassIDDesc			(FourCharCode		inClassID,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateEnumerationDesc		(FourCharCode		inEnumeration,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateFileOrFolderDesc		(FSSpec const*		inFSSpecPtr,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateFileOrFolderDesc		(FSRef const&		inFSRef,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreatePairDesc				(AEDesc const*		inDesc1Ptr,
											 AEDesc const*		inDesc2Ptr,
											 AEDescList*		outDescListPtr);

OSStatus
	BasicTypesAE_CreatePointDesc			(Point				inPoint,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreatePropertyIDDesc		(FourCharCode		inPropertyID,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreatePStringDesc			(Str255				inPascalStringData,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateRectDesc				(Rect const*		inRectPtr,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateRGBColorDesc			(RGBColor const*	inColor,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateSInt16Desc			(SInt16				inInteger,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateSInt32Desc			(SInt32				inInteger,
											 AEDesc*			outDescPtr);

OSStatus
	BasicTypesAE_CreateUInt32Desc			(UInt32				inInteger,
											 AEDesc*			outDescPtr);

// ALTHOUGH YOU COULD USE BasicTypesAE_CreateUInt32Desc() FOR “GET DATA SIZE”, PLEASE USE THIS INSTEAD
#define BasicTypesAE_CreateSizeDesc			BasicTypesAE_CreateUInt32Desc

OSStatus
	BasicTypesAE_CreateUInt32PairDesc		(UInt32				inInteger1,
											 UInt32				inInteger2,
											 AEDescList*		outDescListPtr);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
