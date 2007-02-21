/*###############################################################

	Releases.h
	
	This package includes utility routines for decoding Gestalt
	version numbers into more useful forms; specifically, the
	three important version numbers.  You can use this routine,
	for example, to decode the return value Gestalt gives you
	when you ask it for the current Mac OS version.
	
	There are also routines for compactly storing version data
	for application shared libraries, which you might use to
	implement a versioning function for a library.
	
	Data Access Library 1.3
	© 1998-2004 by Kevin Grant
	
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

#ifndef __RELEASES__
#define __RELEASES__



#pragma mark Constants

typedef UInt32 ApplicationSharedLibraryVersion;

enum
{
	kReleaseMajorVersionNumberMask		= 0xFF000000,	// high 8 bits
	kReleaseMinorVersionNumberMask		= 0x00FFE000,	// next 11 bits
	kReleaseSuperminorVersionNumberMask	= 0x00001FFC,	// next 11 bits
	kReleaseKindMask					= 0x00000003	// low 2 bits
};

enum
{
	// use these to define the lower two bits (kind) of a library version
	kReleaseKindFinal		= ((0 << 1) | (0 << 0)),
	kReleaseKindPrealpha	= ((0 << 1) | (1 << 0)),
	kReleaseKindDevelopment	= kReleaseKindPrealpha,
	kReleaseKindAlpha		= ((1 << 1) | (0 << 0)),
	kReleaseKindBeta		= ((1 << 1) | (1 << 0))
};



#pragma mark Public Methods

/*###############################################################
	EXTRACTING APPLICATION OR LIBRARY VERSION INFORMATION
###############################################################*/

#define Releases_GetMajorVersionNumber(v)		((v & kReleaseMajorVersionNumberMask) >> 24)
#define Releases_GetMinorVersionNumber(v)		((v & kReleaseMinorVersionNumberMask) >> 13)
#define Releases_GetSuperminorVersionNumber(v)	((v & kReleaseSuperminorVersionNumberMask) >> 2)
#define Releases_GetKind(v)						((v & kReleaseKindMask) >> 0)
#define Releases_Version(maj, min, supm, kind)	(	\
													((maj << 24) & kReleaseMajorVersionNumberMask) & \
													((min << 13) & kReleaseMinorVersionNumberMask) & \
													((supm << 2) & kReleaseSuperminorVersionNumberMask) & \
													((kind << 0) & kReleaseKindMask) \
												)

/*###############################################################
	EXTRACTING MAC OS VERSION INFORMATION FROM LONG INTEGERS
###############################################################*/

UInt8
	Releases_GetMajorRevisionForVersion 		(long const		inVersion);

UInt8
	Releases_GetMinorRevisionForVersion 		(long const		inVersion);

UInt8
	Releases_GetSuperminorRevisionForVersion 	(long const		inVersion);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
