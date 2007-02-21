/*!	\file InternetServices.h
	\brief This module can be used to manage TCP/IP service
	providers under Open Transport.
	
	When you call InternetServices_Init(), a synchronous
	Internet Services reference is created, which will allow you
	to make synchronous Open Transport calls.  In addition, you
	can use InternetServices_New() to create any number of
	asynchronous references in a standard way, which will allow
	you to make asynchronous Open Transport calls.
	
	Using configuration strings, it is possible to create many
	varieties of Internet Services providers.  However, while
	you can generate any configuration string you like, it may
	not be possible to create the configuration using this
	module because Open Transport may not support it.
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

#ifndef __INTERNETSERVICES__
#define __INTERNETSERVICES__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

#define kInternetServices_DefaultConfigurationString		kDNRName



#pragma mark Public Methods

void
	InternetServices_Init			();

void
	InternetServices_Done			();

OSStatus
	InternetServices_New			(OTNotifyUPP		inNotificationRoutine,
									 char const*		inConfigurationString,
									 void*				inUserDefinedContextPtr);

OSStatus
	InternetServices_NewTCPIP		(OTNotifyUPP		inNotificationRoutine,
									 void*				inUserDefinedContextPtr);

OSStatus
	InternetServices_NewWithOTFlags	(OTOpenFlags		inFlags,
									 OTNotifyUPP		inNotificationRoutine,
									 char const*		inConfigurationString,
									 void*				inUserDefinedContextPtr);

void
	InternetServices_Dispose		(InetSvcRef*		inoutRefPtr);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
