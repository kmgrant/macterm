/*!	\file Releases.h
	\brief Routines for decoding the return values from the
	Gestalt() system call.
	
	There are also routines for compactly storing version data
	for application shared libraries, which you might use to
	implement a versioning function for a library.
*/
/*###############################################################

	Data Access Library 2.6
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

#include <UniversalDefines.h>

#ifndef __RELEASES__
#define __RELEASES__



#pragma mark Constants

typedef UInt32 Releases_ApplicationSharedLibraryVersion;

enum
{
	kReleases_MajorVersionNumberMask		= 0xFF000000,	// high 8 bits
	kReleases_MinorVersionNumberMask		= 0x00FFE000,	// next 11 bits
	kReleases_SuperminorVersionNumberMask	= 0x00001FFC,	// next 11 bits
	kReleases_KindMask						= 0x00000003	// low 2 bits
};

enum
{
	// use these to define the lower two bits (kind) of a library version
	kReleases_KindFinal			= ((0 << 1) | (0 << 0)),
	kReleases_KindPrealpha		= ((0 << 1) | (1 << 0)),
	kReleases_KindDevelopment	= kReleases_KindPrealpha,
	kReleases_KindAlpha			= ((1 << 1) | (0 << 0)),
	kReleases_KindBeta			= ((1 << 1) | (1 << 0))
};



#pragma mark Public Methods

/*###############################################################
	EXTRACTING APPLICATION OR LIBRARY VERSION INFORMATION
###############################################################*/

#define Releases_ReturnMajorVersionNumber(v)		((v & kReleases_MajorVersionNumberMask) >> 24)
#define Releases_ReturnMinorVersionNumber(v)		((v & kReleases_MinorVersionNumberMask) >> 13)
#define Releases_ReturnSuperminorVersionNumber(v)	((v & kReleases_SuperminorVersionNumberMask) >> 2)
#define Releases_ReturnKind(v)						((v & kReleases_KindMask) >> 0)
#define Releases_Version(maj, min, supm, kind)	(	\
													((maj << 24) & kReleases_MajorVersionNumberMask) & \
													((min << 13) & kReleases_MinorVersionNumberMask) & \
													((supm << 2) & kReleases_SuperminorVersionNumberMask) & \
													((kind << 0) & kReleases_KindMask) \
												)

/*###############################################################
	EXTRACTING MAC OS VERSION INFORMATION FROM LONG INTEGERS
###############################################################*/

UInt8
	Releases_ReturnMajorRevisionForVersion			(long const		inVersion);

UInt8
	Releases_ReturnMinorRevisionForVersion			(long const		inVersion);

UInt8
	Releases_ReturnSuperminorRevisionForVersion 	(long const		inVersion);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
