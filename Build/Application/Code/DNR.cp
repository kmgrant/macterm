/*!	\file DNR.cp
	\brief Domain name resolver library.
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

#include "DNR.h"
#include <UniversalDefines.h>

// Unix includes
extern "C"
{
#	include <netdb.h>
#	include <pthread.h>
#	include <sys/socket.h>
#	include <sys/types.h>
}

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlocks.h>

// application includes
#include "NetEvents.h"



#pragma mark Types
namespace {

/*!
Thread context passed to threadForDNS().
*/
struct DNSThreadContext
{
	EventQueueRef	eventQueue;
	char*			hostNameCString;
	Boolean			restrictIPv4;
};
typedef DNSThreadContext*			DNSThreadContextPtr;
typedef DNSThreadContext const*		DNSThreadContextConstPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void*	threadForDNS	(void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Initiates an asynchronous lookup of a host name, which may be
an IPv4 or IPv6 numerical or named address.  Returns
"kDNR_ResultOK" if this succeeded; also, upon success, spawns
a thread that will wait for the lookup to complete.

The thread sleeps until the resolver returns, at which time it
fires a Carbon Event of class "kMyCarbonEventClassDNS" and kind
"kMyCarbonEventKindHostLookupComplete".  The event should have
a parameter "kMyCarbonEventParamDirectHostEnt" whose data is of
"typeMyStructHostEntPtr" (containing the "struct hostent*"
of the completed call).  At this point, your event handler can
read the lookup data and finally call DNR_Dispose() to free it.

If "inRestrictIPv4" is true, then the result will be in IPv4
(traditional) format; otherwise, it will use the IPv6 (longer)
format.

The domain name resolver has been re-written in version 3.1
to use BSD calls (sigh, to replace the one rewritten in 3.0 to
use Open Transport natively...when will the next “wave of the
future” hit?).

(3.1)
*/
DNR_Result
DNR_New		(char const*	inHostNameCString,
			 Boolean		inRestrictIPv4)
{
	DNR_Result			result = kDNR_ResultOK;
	pthread_attr_t		attr;
	int					error = 0;
	
	
	// start a thread for DNS lookup so that the main event loop can still run
	error = pthread_attr_init(&attr);
	if (0 != error) result = kDNR_ResultThreadError;
	else
	{
		DNSThreadContextPtr		threadContextPtr = nullptr;
		pthread_t				thread;
		
		
		threadContextPtr = REINTERPRET_CAST(Memory_NewPtrInterruptSafe(sizeof(DNSThreadContext)),
											DNSThreadContextPtr);
		if (nullptr == threadContextPtr) result = kDNR_ResultThreadError;
		else
		{
			size_t const	kInputHostStringLength = std::strlen(inHostNameCString);
			size_t const	kHostBufferLength = 1 + kInputHostStringLength;
			
			
			// set up context
			threadContextPtr->hostNameCString = REINTERPRET_CAST
												(Memory_NewPtrInterruptSafe(kHostBufferLength),
													char*);
			if (nullptr == threadContextPtr) result = kDNR_ResultThreadError;
			else
			{
				std::strncpy(threadContextPtr->hostNameCString, inHostNameCString, kInputHostStringLength);
				threadContextPtr->hostNameCString[kHostBufferLength - 1] = '\0';
				threadContextPtr->restrictIPv4 = inRestrictIPv4;
				
				// create thread
				error = pthread_create(&thread, &attr, threadForDNS, threadContextPtr);
				if (0 != error) result = kDNR_ResultThreadError;
			}
		}
	}
	
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


#pragma mark Internal Methods
namespace {

/*!
A POSIX thread (which can be preempted) that handles
otherwise-synchronous DNS lookups.  Since preemptive
threads are used, the application will not “block”
waiting for data and will not halt important things
like the main event loop!

WARNING:	As this is a preemptable thread, you MUST
			NOT use thread-unsafe system calls here.
			On the other hand, you can arrange for
			events to be handled (e.g. with Carbon
			Events).

(3.1)
*/
void*
threadForDNS	(void*		inDNSThreadContextPtr)
{
	assert(GetCurrentEventQueue() != GetMainEventQueue());
	
	DNSThreadContextPtr		contextPtr = REINTERPRET_CAST(inDNSThreadContextPtr, DNSThreadContextPtr);
	OSStatus				error = noErr;
	int						posixError = 0;
	struct hostent*			hostData = getipnodebyname(contextPtr->hostNameCString,
														contextPtr->restrictIPv4 ? AF_INET : AF_INET6,
														AI_DEFAULT, &posixError);
	
	
	// return the data indirectly by posting a new event to the main queue
	{
		EventRef	lookupCompleteEvent = nullptr;
		
		
		// create a Carbon Event
		error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_DNS,
							kEventNetEvents_HostLookupComplete, GetCurrentEventTime(),
							kEventAttributeNone, &lookupCompleteEvent);
		
		// attach required parameters to event, then dispatch it
		if (noErr != error) lookupCompleteEvent = nullptr;
		else
		{
			Boolean		doPost = true;
			
			
			// if the lookup failed, it will have returned nothing;
			// a lack of host info is the handler’s hint that there
			// was a problem with the DNS lookup
			if (nullptr != hostData)
			{
				// include the "struct hostent" that the system returned
				error = SetEventParameter(lookupCompleteEvent, kEventParamNetEvents_DirectHostEnt,
											typeNetEvents_StructHostEntPtr, sizeof(hostData), &hostData);
				doPost = (noErr == error);
			}
			
			if (doPost)
			{
				// finally, send the message to the main event loop
				error = PostEventToQueue(GetMainEventQueue(), lookupCompleteEvent, kEventPriorityStandard);
			}
		}
		
		// dispose of event
		if (nullptr != lookupCompleteEvent) ReleaseEvent(lookupCompleteEvent), lookupCompleteEvent = nullptr;
	}
	
	// since the thread is finished, the context should be disposed of
	Memory_DisposePtrInterruptSafe(REINTERPRET_CAST(&contextPtr->hostNameCString, void**));
	Memory_DisposePtrInterruptSafe(REINTERPRET_CAST(&contextPtr, void**));
	
	return nullptr;
}// threadForLocalProcessDataLoop

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
