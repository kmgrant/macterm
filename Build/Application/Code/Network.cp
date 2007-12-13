/*###############################################################

	Network.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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

/*!
NOTE:	The copyIPAddressListSCF() routine is the equivalent
		to allCurrentIPAddressesAndAliases(), except is based
		on System Configuration Framework.
		
		SCF seems to return IPv4 addresses okay, but does NOT
		seem to return IPv6 yet.  Even if the code is in error
		somehow, it still seems ridiculously complex compared
		to the POSIX equivalent (and requires an extra framework
		to be linked in), so the POSIX code is being preferred.
*/
#define USE_SYSTEM_CONFIGURATION_FRAMEWORK	0

// standard-C includes
#include <cstdlib>
#include <cstring>

// standard-C++ includes
#include <string>

// UNIX includes
extern "C"
{
#	include <netdb.h>
#	include <unistd.h>
#	include <arpa/inet.h>
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <sys/types.h>
}

// Mac includes
#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>

// library includes
#include <AlertMessages.h>
#include <CFUtilities.h>
#include <Console.h>
#include <SoundSystem.h>

// resource includes
#include "StringResources.h"

// MacTelnet includes
#include "DialogUtilities.h"
#include "DNR.h"
#include "ErrorAlerts.h"
#include "Network.h"
#include "UIStrings.h"



#pragma mark Internal Method Prototypes

static bool			allCurrentIPAddressesAndAliases		(struct hostent*&);
#if USE_SYSTEM_CONFIGURATION_FRAMEWORK
static bool			copyIPAddressListSCF				(CFArrayRef&);
static void			getIPAddressListFromValue			(void const*, void const*, void*);
#endif



#pragma mark Public Methods


/*!
Finds the IP address(es) of the computer running
this program.  Returns true only if any are found.

If there is a problem finding a particular address,
an (empty) entry is still added to the list.  So
the length of the resulting list should always be
an accurate count of machine addresses, even if
they are not all returned.

(3.1)
*/
Boolean
Network_CopyIPAddresses		(std::vector< CFRetainRelease >&	inoutAddresses)
{
	Boolean				result = false;
	struct hostent*		currentHost = nullptr;
	
	
	result = allCurrentIPAddressesAndAliases(currentHost);
	if (result)
	{
		char**				hostList = currentHost->h_addr_list;
		assert(nullptr != hostList);
		register SInt16		hostIndex = 0;
		CFStringRef			addressCFString = nullptr;
		
		
		for (; nullptr != *hostList; ++hostList, ++hostIndex)
		{
			addressCFString = DNR_CopyResolvedHostAsCFString(currentHost, hostIndex/* index in array to use */);
			inoutAddresses.push_back(addressCFString);
			if (nullptr != addressCFString)
			{
				result = true;
				CFRelease(addressCFString), addressCFString = nullptr;
			}
		}
	}
	
	return result;
}// CopyIPAddresses


/*!
Obtains the first available IP address of the computer running
this program, as a C++ std::string.

The type of address is returned as well.  This will be either
AF_INET or AF_INET6.

If the IP address converts successfully, "true" is returned;
otherwise, "false" is returned.

(3.0)
*/
bool
Network_CurrentIPAddressToString	(std::string&	outString,
									 int&			outAddressType)
{
	bool				result = false;
	struct hostent*		currentHost;
	
	
	result = allCurrentIPAddressesAndAliases(currentHost);
	if (result)
	{
		outAddressType = currentHost->h_addrtype;
		
	#if 0
		// this works, but not (yet) for IPv6, and is kind of a hack anyway...
		outString = inet_ntoa(*(struct in_addr*)(currentHost->h_addr_list[0]));
		result = true;
	#else
		{
			CFStringRef		addressCFString = nullptr;
			
			
			addressCFString = DNR_CopyResolvedHostAsCFString(currentHost, 0/* index in array to use */);
			if (nullptr != addressCFString)
			{
				CFStringEncoding const	kEncoding = kCFStringEncodingUTF8;
				size_t const			kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
															(CFStringGetLength(addressCFString), kEncoding);
				
				
				try
				{
					char*	addressBuffer = new char[kBufferSize];
					
					
					if (CFStringGetCString(addressCFString, addressBuffer, kBufferSize, kEncoding))
					{
						outString = addressBuffer;
						result = true;
					}
					delete [] addressBuffer;
				}
				catch (std::bad_alloc)
				{
					// ignore
				}
				CFRelease(addressCFString), addressCFString = nullptr;
			}
		}
	#endif
	}
	return result;
}// CurrentIPAddressToString


#pragma mark Internal Methods

/*!
Finds all available information about the current host -
all of the IP addresses it has.  Returns "true" only if
successful, and ONLY in this case will "outHost" be defined.

If possible, IPv6 addresses are returned.

(3.1)
*/
bool
allCurrentIPAddressesAndAliases		(struct hostent*&	outHost)
{
	bool	result = false;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	char	hostName[_POSIX_HOST_NAME_MAX];
#else
	char	hostName[255];
#endif
	int		getHostResult = gethostname(hostName, sizeof(hostName) - 1);
	
	
	// null-terminate regardless
	hostName[sizeof(hostName) / sizeof(char) - 1] = '\0';
	
	if (0 == getHostResult)
	{
		if (FlagManager_Test(kFlagOS10_3API))
		{
			outHost = gethostbyname2(hostName, AF_INET6);
		}
		else
		{
			outHost = gethostbyname(hostName);
		}
		
		if (nullptr != outHost)
		{
			result = true;
		}
	}
	return result;
}// allCurrentIPAddressesAndAliases


#if USE_SYSTEM_CONFIGURATION_FRAMEWORK
/*!
Of standard "CFDictionaryApplierFunction" form, this
routine is used to pull information out of a dictionary
provided by the System Configuration Framework.

Based almost exclusively on Apple Tech Note 1145.

(3.1)
*/
static void
getIPAddressListFromValue	(void const*	UNUSED_ARGUMENT(inKey),
							 void const*	inValue,
							 void*			inContextPtr)
{
	//assert(nullptr != inKey);
	//CFStringRef const		kKeyCFString = CFUtilities_StringCast(inKey);
	assert(nullptr != inValue);
	CFDictionaryRef const	kValueCFDictionary = CFUtilities_DictionaryCast(inValue);
	assert(nullptr != inContextPtr);
	CFMutableArrayRef const	kResultsCFArray = CFUtilities_MutableArrayCast(inContextPtr);
	CFArrayRef				addressCFArray = nullptr;
	
	
	addressCFArray = CFUtilities_ArrayCast(CFDictionaryGetValue(kValueCFDictionary,
																kSCPropNetIPv6Addresses));
	if (nullptr != addressCFArray)
	{
		CFArrayAppendArray(kResultsCFArray, addressCFArray, CFRangeMake(0, CFArrayGetCount(addressCFArray)));
	}
	addressCFArray = CFUtilities_ArrayCast(CFDictionaryGetValue(kValueCFDictionary,
																kSCPropNetIPv4Addresses));
	if (nullptr != addressCFArray)
	{
		CFArrayAppendArray(kResultsCFArray, addressCFArray, CFRangeMake(0, CFArrayGetCount(addressCFArray)));
	}
}// getIPAddressListFromValue
#endif


#if USE_SYSTEM_CONFIGURATION_FRAMEWORK
/*!
Returns a new array of addresses (in IPv4 and/or IPv6
format).  You must call CFRelease() on the array when
finished.

Returns "true" only if successful, and ONLY in this
case will "outAddressesCFArray" be defined.

Based almost exclusively on Apple Tech Note 1145.

(3.1)
*/
static bool
copyIPAddressListSCF	(CFArrayRef&	outAddressesCFArray)
{
	SCDynamicStoreRef	dynamicStore = nullptr;
	int					configError = kSCStatusOK;
	bool				result = false;
	
	
	outAddressesCFArray = nullptr; // initially...
	
	// Create a connection to the dynamic store, then create
	// a search pattern that finds all IPv4 and IPv6 entities.
	dynamicStore = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("copyIPAddressListSCF"),
										nullptr/* callback */, nullptr/* context for callback */);
	configError = SCError();
	if ((nullptr != dynamicStore) && (kSCStatusOK == configError))
	{
		CFStringRef		patternIPv4CFString = nullptr;
		
		
		patternIPv4CFString = SCDynamicStoreKeyCreateNetworkServiceEntity
								(kCFAllocatorDefault, kSCDynamicStoreDomainState/* domain */,
									kSCCompAnyRegex/* service ID or match pattern */,
									kSCEntNetIPv4/* entity */);
		configError = SCError();
		if ((nullptr != patternIPv4CFString) && (kSCStatusOK == configError))
		{
			CFStringRef		patternIPv6CFString = nullptr;
			
			
			patternIPv6CFString = SCDynamicStoreKeyCreateNetworkServiceEntity
									(kCFAllocatorDefault, kSCDynamicStoreDomainState/* domain */,
										kSCCompAnyRegex/* service ID or match pattern */,
										kSCEntNetIPv6/* entity */);
			configError = SCError();
			if ((nullptr != patternIPv6CFString) && (kSCStatusOK == configError))
			{
				CFArrayRef		patternsCFArray = nullptr;
				void const*		listValues[] = { patternIPv6CFString, patternIPv4CFString };
				
				
				patternsCFArray = CFArrayCreate(kCFAllocatorDefault, listValues,
												sizeof(listValues) / sizeof(void*),
												&kCFTypeArrayCallBacks);
				if (nullptr != patternsCFArray)
				{
					CFDictionaryRef		addressesCFDictionary = nullptr;
					
					
					addressesCFDictionary = SCDynamicStoreCopyMultiple(dynamicStore, nullptr/* specific keys */,
																		patternsCFArray/* keys to match */);
					configError = SCError();
					if ((nullptr != addressesCFDictionary) && (kSCStatusOK == configError))
					{
						CFMutableArrayRef	addressesCFArray = nullptr;
						
						
						// For each IPv4 or IPv6 entity that was found, extract the
						// list of addresses and append it to the results array.
						addressesCFArray = CFArrayCreateMutable(kCFAllocatorDefault,
																0/* capacity, or 0 for unlimited */,
																&kCFTypeArrayCallBacks);
						if (nullptr != addressesCFArray)
						{
							// Iterate over the values, extracting the IP address
							// arrays and appending them to the result.
							CFDictionaryApplyFunction(addressesCFDictionary, getIPAddressListFromValue,
														addressesCFArray);
							outAddressesCFArray = addressesCFArray;
							
							// success!
							result = true;
						}
					}
					CFRelease(patternsCFArray), patternsCFArray = nullptr;
				}
				CFRelease(patternIPv6CFString), patternIPv6CFString = nullptr;
			}
			CFRelease(patternIPv4CFString), patternIPv4CFString = nullptr;
		}
		CFRelease(dynamicStore), dynamicStore = nullptr;
	}
	
	return result;
}// copyIPAddressListSCF
#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
