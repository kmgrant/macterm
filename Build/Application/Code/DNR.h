/*!	\file DNR.h
	\brief Domain name resolver library.
	
	Completely rewritten in 3.1 to use IPv6 and BSD
	routines (even though this was rewritten in 3.0 to
	use Open Transport!!!).
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#ifndef __DNR__
#define __DNR__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include "ResultCode.template.h"



#pragma mark Constants

typedef ResultCode< UInt16 >	DNR_Result;
DNR_Result const	kDNR_ResultOK(0);			//!< no error
DNR_Result const	kDNR_ResultThreadError(1);	//!< lookup failed because of error setting up thread

#pragma mark Types

/*!
The host entry structure that contains information
on a resolved address.  See /usr/include/netdb.h.
*/
struct hostent;



#pragma mark Public Methods

DNR_Result
	DNR_New							(char const*			inHostNameCString,
									 Boolean				inRestrictIPv4,
									 void					(^inResponseBlock)(struct hostent*));

void
	DNR_Dispose						(struct hostent**		inoutDataPtr);

CFStringRef
	DNR_CopyResolvedHostAsCFString	(struct hostent const*	inDNR,
									 int					inWhichIndex);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
