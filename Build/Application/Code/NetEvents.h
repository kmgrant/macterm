/*!	\file NetEvents.h
	\brief Legacy; now used for a few Apple Event data types.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once



#pragma mark Constants

/*!
Custom Carbon Event data types.
*/
enum
{
	typeNetEvents_CFBooleanRef			= 'CFTF',	//!< "CFBooleanRef"; could use typeCFBooleanRef but that is not available in the Mac OS 10.1 SDK
	typeNetEvents_CFDataRef				= 'CFDa',	//!< "CFDataRef"
	typeNetEvents_CFNumberRef			= 'CFNm',	//!< "CFNumberRef"; could use typeCFNumberRef but that is not available in the Mac OS 10.1 SDK
	typeNetEvents_CGPoint				= 'CGPt',	//!< "CGPoint"; could use typeCGPoint but that is not available in the Mac OS SDK
};

// BELOW IS REQUIRED NEWLINE TO END FILE
