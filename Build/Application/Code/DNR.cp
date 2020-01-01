/*!	\file DNR.cp
	\brief Domain name resolver library.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#include "DNR.h"
#include <UniversalDefines.h>

// Unix includes
extern "C"
{
#	include <netdb.h>
#	include <sys/socket.h>
#	include <sys/types.h>
}

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>



#pragma mark Public Methods

/*!
Initiates an asynchronous lookup of a host name, which may be
an IPv4 or IPv6 numerical or named address.  Returns
"kDNR_ResultOK" if this succeeded; also, upon success, spawns
a thread that will wait for the lookup to complete.

The thread sleeps until the resolver returns, at which time it
invokes the given block with the "struct hostent*" result of
the completed call.  If this data is "nullptr", the lookup has
failed; otherwise, read the lookup data and finally call
DNR_Dispose() to free it.

If "inRestrictIPv4" is true, then the result will be in IPv4
(traditional) format; otherwise, it will use the IPv6 (longer)
format.

(4.1)
*/
DNR_Result
DNR_New		(char const*	inHostNameCString,
			 Boolean		inRestrictIPv4,
			 void			(^inResponseBlock)(struct hostent*))
{
	DNR_Result		result = kDNR_ResultOK;
	
	
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0/* flags */),
	^{
		int					posixError = 0;
		struct hostent*		hostData = getipnodebyname(inHostNameCString,
														inRestrictIPv4 ? AF_INET : AF_INET6,
														AI_DEFAULT, &posixError);
		
		if (nullptr == hostData)
		{
			if (HOST_NOT_FOUND == posixError)
			{
				Console_WriteLine("lookup failed; host not found");
			}
			else if (TRY_AGAIN == posixError)
			{
				Console_WriteLine("lookup failed (suggest retrying)");
			}
			else
			{
				Console_WriteValue("lookup failed; error", posixError);
			}
		}
		
		dispatch_async(dispatch_get_main_queue(),
		^{
			inResponseBlock(hostData);
		});
	});
	
	return result;
}// New


/*!
Disposes of memory allocated by a DNR lookup.  You
will receive this pointer as a parameter to your
Carbon Event handler (see DNR_New()).  On return,
your copy of the pointer is set to nullptr.

(3.1)
*/
void
DNR_Dispose		(struct hostent**	inoutHostEntryPtr)
{
	freehostent(*inoutHostEntryPtr);
	*inoutHostEntryPtr = nullptr;
}// Dispose


/*!
Creates a string representation of the specified
host address in the structure.  The index is
usually 0, but may be greater if you know that
more than one matching address was found in your
resolver record.

The string representation is going to be an IP
address in version 4 (dotted decimal) or version 6
(colon-delimited hex) form.

A Core Foundation string is returned, since it is
likely you will want to use this for UI purposes
anyway (e.g. display in a text box) and a CFString
is easily converted to a C string, etc. if needed.
You must CFRelease() the string when finished with
it.

Returns nullptr if there is a problem allocating the
new string.

(3.1)
*/
CFStringRef
DNR_CopyResolvedHostAsCFString	(struct hostent const*	inDNR,
								 int					inWhichIndex)
{
	CFStringRef		result = nullptr;
	
	
	switch (inDNR->h_addrtype)
	{
	case AF_INET:
		// NOTE: I am implementing this myself because there does not
		// seem to be a good, simple, consistent API I can count on
		// across all versions of Mac OS X.
		{
			int const						kLength = inDNR->h_length;
			assert(4 == kLength);
			// despite what "struct hostent" uses, these are unsigned characters, dammit!
			unsigned char const* const*		kAddressList = REINTERPRET_CAST(inDNR->h_addr_list,
																			unsigned char const* const*);
			unsigned char const*			kAddressDecimals = kAddressList[inWhichIndex];
			
			
			// a "struct hostent" already has numbers in network order,
			// which is the order they should appear in when printed
			result = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format option dictionary */,
												CFSTR("%u.%u.%u.%u"),
												kAddressDecimals[0], kAddressDecimals[1],
												kAddressDecimals[2], kAddressDecimals[3]);
		}
		break;
	
	case AF_INET6:
		// NOTE: I am implementing this myself because there does not
		// seem to be a good, simple, consistent API I can count on
		// across all versions of Mac OS X.
		{
			int const						kLength = inDNR->h_length;
			assert(16 == kLength);
			// despite what "struct hostent" uses, these are unsigned characters, dammit!
			unsigned char const* const*		kAddressList = REINTERPRET_CAST(inDNR->h_addr_list,
																			unsigned char const* const*);
			unsigned char const*			kAddressOctets = kAddressList[inWhichIndex];
			
			
			// a "struct hostent" already has numbers in network order,
			// which is the order they should appear in when printed
			result = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format option dictionary */,
												CFSTR("%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:"
														"%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx"),
												kAddressOctets[0], kAddressOctets[1],
												kAddressOctets[2], kAddressOctets[3],
												kAddressOctets[4], kAddressOctets[5],
												kAddressOctets[6], kAddressOctets[7],
												kAddressOctets[8], kAddressOctets[9],
												kAddressOctets[10], kAddressOctets[11],
												kAddressOctets[12], kAddressOctets[13],
												kAddressOctets[14], kAddressOctets[15]);
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// CopyResolvedHostAsText

// BELOW IS REQUIRED NEWLINE TO END FILE
