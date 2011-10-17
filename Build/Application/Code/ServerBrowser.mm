/*!	\file ServerBrowser.mm
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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

#import "ServerBrowser.h"

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// Unix includes
extern "C"
{
#	include <netdb.h>
#	include <arpa/inet.h>
#	include <netinet/in.h>
}

// library includes
#import <AutoPool.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DNR.h"
#import "NetEvents.h"
#import "Session.h"



#pragma mark Types

/*!
Implements an object wrapper for NSNetService instances returned
by Bonjour, that allows them to be easily inserted into user
interface elements without losing less user-friendly information
about each service.
*/
@interface ServerBrowser_NetService : NSObject // implements NSNetServiceDelegateMethods
{
@private
	NSNetService*		netService;
	unsigned char		addressFamily; // AF_INET or AF_INET6
	NSString*			bestResolvedAddress;
	unsigned short		bestResolvedPort;
}

- (id)
initWithNetService:(NSNetService*)_
addressFamily:(unsigned char)_;

// accessors; see "Discovered Hosts" array controller in the NIB, for key names

- (NSString*)
bestResolvedAddress;

- (unsigned short)
bestResolvedPort;

- (NSString*)
description;

- (NSNetService*)
netService;

- (void)
setBestResolvedAddress:(NSString*)_;

- (void)
setBestResolvedPort:(unsigned short)_;

@end


/*!
Implements an object wrapper for protocol definitions, that
allows them to be easily inserted into user interface elements
without losing less user-friendly information about each
protocol.
*/
@interface ServerBrowser_Protocol : NSObject
{
@private
	Session_Protocol	protocolID;
	NSString*			description;
	NSString*			serviceType; // RFC 2782 / Bonjour, e.g. "_xyz._tcp."
	unsigned short		defaultPort;
}

// accessors; see "Protocol Definitions" array controller in the NIB, for key names

- (NSString*)
boundName;
- (void)
setBoundName:(NSString*)_;

- (unsigned short)
defaultPort;
- (void)
setDefaultPort:(unsigned short)_;

- (NSString*)
description;
- (void)
setDescription:(NSString*)_;

- (Session_Protocol)
protocolID;
- (void)
setProtocolID:(Session_Protocol)_;

- (NSString*)
serviceType;
- (void)
setServiceType:(NSString*)_;

// initializers

- (id)
initWithID:(Session_Protocol)_
description:(NSString*)_
serviceType:(NSString*)_
defaultPort:(unsigned short)_;

@end

#pragma mark Internal Method Prototypes
namespace {

id			flagNo						();
void		initializePanel				(ServerBrowser_PanelController*, Session_Protocol, CFStringRef, UInt16,
										 CFStringRef);
void		notifyOldEventTarget		(EventTargetRef);
OSStatus	receiveLookupComplete		(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

CarbonEventHandlerWrap&			gBrowserLookupResponder ()
								{
									static CarbonEventHandlerWrap		x(GetApplicationEventTarget(), receiveLookupComplete,
																			CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_DNS),
																									kEventNetEvents_HostLookupComplete),
																			nullptr/* user data */);
									return x;
								}
EventTargetRef					gPanelEventTarget = nullptr;	//!< temporary, for Carbon interaction
ServerBrowser_PanelController*	gServerBrowser_PanelController = nil;

}// anonymous namespace



#pragma mark Public Methods

/*!
Returns true only if the panel is showing.

(4.0)
*/
Boolean
ServerBrowser_IsVisible ()
{
	AutoPool	_;
	Boolean		result = false;
	
	
	result = (YES == [[[ServerBrowser_PanelController sharedServerBrowserPanelController] window] isVisible]);
	
	return result;
}// IsVisible


/*!
Removes any current event target, resetting the panel to
arbitrary values and notifying the previous target of a change.

This is automatically done when the window is closed.

(4.0)
*/
void
ServerBrowser_RemoveEventTarget ()
{
	// stop associating the panel with any event target, and notify the previous target
	ServerBrowser_SetEventTarget(nullptr/* event target */, kSession_ProtocolSSH1, CFSTR("")/* host */,
									22/* port - arbitrary */, CFSTR("")/* user ID */);
}// RemoveEventTarget


/*!
Sets the current event target.  This is for Carbon interfaces,
and is deprecated; Cocoa interfaces are better off using the
"setTarget:protocol:hostName:portNumber:userID:" method of
ServerBrowser_PanelController.

The target is sent an event whenever anything in the panel is
changed by the user, which includes the given initial values.
(Initialization is mandatory, because the panel could already be
open and in use by some other target; to not be misleading, it
should be refreshed to reflect the new target.)

You can pass "nullptr" as the target to set no target, but it is
easier to just call ServerBrowser_RemoveEventTarget().

Before the window is shown, a valid target should be set;
otherwise, the user could change something that goes unnoticed.

IMPORTANT:	This is for Carbon compatibility and is not a
			long-term solution.  In Cocoa, notifications are
			sent along the responder chain, using the methods
			declared in "ServerBrowser_PanelChanges".

(4.0)
*/
void
ServerBrowser_SetEventTarget	(EventTargetRef		inTargetOrNull,
								 Session_Protocol	inProtocol,
								 CFStringRef		inHostName,
								 UInt16				inPortNumber,
								 CFStringRef		inUserID)
{
	AutoPool						_;
	ServerBrowser_PanelController*	panelController = [ServerBrowser_PanelController sharedServerBrowserPanelController];
	
	
	// if a target already exists, send it one final event to tell it
	// that a new target will be chosen
	[panelController setTarget:nil protocol:kSession_ProtocolSSH1 hostName:@"" portNumber:22 userID:@""];
	if ((nullptr != gPanelEventTarget) && (inTargetOrNull != gPanelEventTarget))
	{
		notifyOldEventTarget(gPanelEventTarget);
	}
	
	gPanelEventTarget = inTargetOrNull;
	
	// now update the panel
	initializePanel(panelController, inProtocol, inHostName, inPortNumber, inUserID);
}// SetEventTarget


/*!
Shows or hides the panel.

(4.0)
*/
void
ServerBrowser_SetVisible	(Boolean	inIsVisible)
{
	AutoPool		_;
	
	
	if (inIsVisible)
	{
		[[ServerBrowser_PanelController sharedServerBrowserPanelController] showWindow:NSApp];
	}
	else
	{
		[[ServerBrowser_PanelController sharedServerBrowserPanelController] close];
	}
}// SetVisible


#pragma mark Internal Methods
namespace {

/*!
Returns an object representing a false value, useful in many
simple flag methods.  The given item is not used.

(4.0)
*/
id
flagNo ()
{
	BOOL	result = NO;
	
	
	return [NSNumber numberWithBool:result];
}// flagNo


/*!
Fills in all of the main views of the Servers panel
with the given default values.

(4.0)
*/
void
initializePanel		(ServerBrowser_PanelController*		inController,
					 Session_Protocol					inProtocol,
					 CFStringRef						inHostName,
					 UInt16								inPortNumber,
					 CFStringRef						inUserID)
{
	if (nil != inController)
	{
		[inController setProtocolIndexByProtocol:inProtocol];
		[inController setHostName:(NSString*)inHostName];
		[inController setPortNumber:[[NSNumber numberWithUnsignedShort:inPortNumber] stringValue]];
		[inController setUserID:(NSString*)inUserID];
	}
}// initializePanel


/*!
For Carbon compatibility, sends an event to the previous
Carbon event target of the panel, notifying it that a
new target is taking over.

(4.0)
*/
void
notifyOldEventTarget	(EventTargetRef		inOldTarget)
{
	EventRef	panelHasNewTargetEvent = nullptr;
	OSStatus	error = noErr;
	
	
	// create a Carbon Event
	error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_ServerBrowser,
						kEventNetEvents_ServerBrowserNewEventTarget, GetCurrentEventTime(),
						kEventAttributeNone, &panelHasNewTargetEvent);
	
	// attach required parameters to event, then dispatch it
	if (noErr != error) panelHasNewTargetEvent = nullptr;
	else
	{
		Boolean		doPost = true;
		
		
		if (doPost)
		{
			// finally, send the message to the target
			error = SendEventToEventTargetWithOptions(panelHasNewTargetEvent, inOldTarget,
														kEventTargetDontPropagate);
		}
	}
	
	// dispose of event
	if (nullptr != panelHasNewTargetEvent) ReleaseEvent(panelHasNewTargetEvent), panelHasNewTargetEvent = nullptr;
}// notifyOldEventTarget


/*!
Handles "kEventNetEvents_HostLookupComplete" of
"kEventClassNetEvents_DNS" by updating the text
field containing the remote host name.

(4.0)
*/
OSStatus
receiveLookupComplete	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	AutoPool						_;
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	ServerBrowser_PanelController*	panelController = [ServerBrowser_PanelController sharedServerBrowserPanelController];
	OSStatus						result = eventNotHandledErr;
	
	
	assert(kEventClass == kEventClassNetEvents_DNS);
	assert(kEventKind == kEventNetEvents_HostLookupComplete);
	{
		struct hostent*		lookupDataPtr = nullptr;
		
		
		// find the lookup results
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_DirectHostEnt,
														typeNetEvents_StructHostEntPtr, lookupDataPtr);
		if (noErr == result)
		{
			// NOTE: The lookup data could be a linked list of many matches.
			// The first is used arbitrarily.
			if ((nullptr != lookupDataPtr->h_addr_list) && (nullptr != lookupDataPtr->h_addr_list[0]))
			{
				CFStringRef		addressCFString = DNR_CopyResolvedHostAsCFString(lookupDataPtr, 0/* which address */);
				
				
				if (nullptr != addressCFString)
				{
					[panelController setHostName:(NSString*)addressCFString];
					CFRelease(addressCFString), addressCFString = nullptr;
					result = noErr;
				}
			}
			DNR_Dispose(&lookupDataPtr);
		}
	}
	
	// hide progress indicator
	[panelController setHidesProgress:YES];
	
	return result;
}// receiveLookupComplete

} // anonymous namespace


@implementation ServerBrowser_NetService


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithNetService:(NSNetService*)	aNetService
addressFamily:(unsigned char)		aSocketAddrFamily
{
	self = [super init];
	if (nil != self)
	{
		addressFamily = aSocketAddrFamily;
		bestResolvedAddress = [[NSString string] retain];
		bestResolvedPort = 0;
		netService = [aNetService retain];
		[netService setDelegate:self];
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		[netService resolveWithTimeout:5.0];
	#else
		[netService resolve];
	#endif
	}
	return self;
}// initWithNetService:addressFamily:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[netService release];
	[bestResolvedAddress release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (NSString*)
bestResolvedAddress
{
	return [[bestResolvedAddress retain] autorelease];
}
- (void)
setBestResolvedAddress:(NSString*)		aString
{
	if (aString != bestResolvedAddress)
	{
		[bestResolvedAddress release];
		bestResolvedAddress = [aString retain];
	}
}// setBestResolvedAddress:


/*!
Accessor.

(4.0)
*/
- (unsigned short)
bestResolvedPort
{
	return bestResolvedPort;
}
- (void)
setBestResolvedPort:(unsigned short)	aNumber
{
	bestResolvedPort = aNumber;
}// setBestResolvedPort:


/*!
Accessor.

(4.0)
*/
- (NSString*)
description
{
	return [[self netService] name];
}


/*!
Accessor.

(4.0)
*/
- (NSNetService*)
netService
{
	return netService;
}


#pragma mark NSNetServiceDelegateMethods


/*!
Called when a discovered host name could not be mapped to
an IP address.

(4.0)
*/
- (void)
netService:(NSNetService*)		aService
didNotResolve:(NSDictionary*)	errorDict
{
	id		errorCode = [errorDict objectForKey:NSNetServicesErrorCode];
	
	
	// TEMPORARY - should a more specific error be displayed somewhere?
	NSLog(@"service %@.%@.%@ could not resolve, error code = %@",
			[aService name], [aService type], [aService domain], errorCode);
}// netService:didNotResolve:


/*!
Called when a discovered host name has been resolved to
one or more IP addresses.

(4.0)
*/
- (void)
netServiceDidResolveAddress:(NSNetService*)		resolvingService
{
	NSEnumerator*	toAddressData = [[resolvingService addresses] objectEnumerator];
	NSString*		resolvedHost = nil;
	unsigned int	resolvedPort = 0;
	
	
	//Console_WriteLine("service did resolve"); // debug
	while (NSData* addressData = [toAddressData nextObject])
	{
		struct sockaddr_in*		dataPtr = (struct sockaddr_in*)[addressData bytes];
		
		
		//Console_WriteValue("found address of family", dataPtr->sin_family); // debug
		if (addressFamily == dataPtr->sin_family)
		{
			switch (addressFamily)
			{
			case AF_INET:
			case AF_INET6:
				{
					struct sockaddr_in*		inetDataPtr = REINTERPRET_CAST(dataPtr, struct sockaddr_in*);
					char					buffer[512];
					
					
					if (inet_ntop(addressFamily, &inetDataPtr->sin_addr, buffer, sizeof(buffer)))
					{
						buffer[sizeof(buffer) - 1] = '\0'; // ensure termination in case of overrun
						resolvedHost = [NSString stringWithCString:buffer];
						resolvedPort = ntohs(inetDataPtr->sin_port);
					}
					else
					{
						Console_Warning(Console_WriteLine, "unable to resolve address data that was apparently the right type");
					}
				}
				break;
			
			default:
				// ???
				Console_Warning(Console_WriteLine, "cannot resolve address because preferred address family is unsupported");
				break;
			}
			
			// found desired type of address, so stop resolving
			[resolvingService stop];
			break;
		}
	}
	if (nil != resolvedHost) [self setBestResolvedAddress:resolvedHost];
	if (0 != resolvedPort) [self setBestResolvedPort:resolvedPort];
}// netServiceDidResolveAddress:


@end


@implementation ServerBrowser_Protocol


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithID:(Session_Protocol)	anID
description:(NSString*)			aString
serviceType:(NSString*)			anRFC2782Name
defaultPort:(unsigned short)	aNumber
{
	self = [super init];
	if (nil != self)
	{
		[self setProtocolID:anID];
		[self setDescription:aString];
		[self setServiceType:anRFC2782Name];
		[self setDefaultPort:aNumber];
	}
	return self;
}// initWithID:description:serviceType:defaultPort:


#pragma mark Accessors


/*!
Accessor.

IMPORTANT:	The "boundName" key is ONLY required because older
			versions of Mac OS X do not seem to work properly
			when bound to the "description" accessor.  (Namely,
			the OS seems to stubbornly use its own "description"
			instead of invoking the right one.)  In the future
			this might be removed and rebound to "description".

(4.0)
*/
- (NSString*)
boundName
{
	return [[description retain] autorelease];
}
- (void)
setBoundName:(NSString*)	aString
{
	if (description != aString)
	{
		[description release];
		description = [aString copy];
	}
}// setBoundName:


/*!
Accessor.

(4.0)
*/
- (unsigned short)
defaultPort
{
	return defaultPort;
}
- (void)
setDefaultPort:(unsigned short)		aNumber
{
	defaultPort = aNumber;
}// setDefaultPort:


/*!
Accessor.

(4.0)
*/
- (NSString*)
description
{
	return [[description retain] autorelease];
}
- (void)
setDescription:(NSString*)		aString
{
	if (description != aString)
	{
		[description release];
		description = [aString copy];
	}
}// setDescription:


/*!
Accessor.

(4.0)
*/
- (Session_Protocol)
protocolID
{
	return protocolID;
}
- (void)
setProtocolID:(Session_Protocol)	anID
{
	protocolID = anID;
}// setProtocolID:


/*!
Accessor.

(4.0)
*/
- (NSString*)
serviceType
{
	return [[serviceType retain] autorelease];
}
- (void)
setServiceType:(NSString*)		aString
{
	if (serviceType != aString)
	{
		[serviceType release];
		serviceType = [aString copy];
	}
}// setServiceType:


@end


@implementation ServerBrowser_PanelController


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedServerBrowserPanelController
{
	if (nil == gServerBrowser_PanelController)
	{
		gServerBrowser_PanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gServerBrowser_PanelController;
}// sharedServerBrowserPanelController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	// The correct place for pre-NIB initialization is "windowWillLoad".
	// The correct place for post-NIB initialization is "windowDidLoad".
	self = [super initWithWindowNibName:@"ServerBrowserCocoa"];
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	// See "init", "windowWillLoad", and "windowDidLoad" for initializations to clean up here.
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
	[discoveredHosts release];
	[recentHosts release];
	[protocolDefinitions release];
	[browser release];
	[discoveredHostIndexes release];
	[protocolIndexes release];
	[errorMessage release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Responds to a double-click of a discovered host by
automatically closing the drawer.

Note that there is already a single-click action (handled
via selection bindings) for actually using the selected
service’s host and port, so double-clicks do not need to
do further processing.

(4.0)
*/
- (void)
didDoubleClickDiscoveredHostWithSelection:(NSArray*)	objects
{
#pragma unused(objects)
	[self setHidesDiscoveredHosts:YES];
}// didDoubleClickDiscoveredHostWithSelection:


/*!
Looks up the host name currently displayed in the host name
field, and replaces it with an IP address.

(4.0)
*/
- (void)
lookUpHostName:(id)		sender
{
#pragma unused(sender)
	Boolean		lookupStartedOK = false;
	
	
	if ([[self hostName] length] <= 0)
	{
		// there has to be some text entered there; let the user
		// know that a blank is unacceptable
		Sound_StandardAlert();
	}
	else
	{
		char	hostNameBuffer[256];
		
		
		gBrowserLookupResponder(); // install lookup handler if none exists
		[self setHidesProgress:NO];
		if (CFStringGetCString((CFStringRef)[self hostName], hostNameBuffer, sizeof(hostNameBuffer), kCFStringEncodingASCII))
		{
			DNR_Result		lookupAttemptResult = kDNR_ResultOK;
			
			
			// the global handler installed as gBrowserLookupResponder() will receive a Carbon Event from this eventually
			lookupAttemptResult = DNR_New(hostNameBuffer, false/* use IP version 4 addresses (defaults to IPv6) */);
			if (false == lookupAttemptResult.ok())
			{
				// could not even initiate, so restore UI
				[self setHidesProgress:YES];
			}
			else
			{
				// okay so far...
				lookupStartedOK = true;
			}
		}
	}
}// lookUpHostName:


/*!
Initiates a search for nearby services (via Bonjour) that
match the currently selected protocol’s service type.

(4.0)
*/
- (void)
rediscoverServices
{
	ServerBrowser_Protocol*		theProtocol = [self protocol];
	
	
	// first destroy the old list
	int		loopGuard = 0;
	while (([discoveredHosts count] > 0) && (loopGuard < 50/* arbitrary */))
	{
		[self removeObjectFromDiscoveredHostsAtIndex:0];
		++loopGuard;
	}
	
	// now search for new services, which will eventually repopulate the list;
	// only do this when the drawer is visible, though
	if (NO == [self hidesDiscoveredHosts])
	{
		if (nil == theProtocol)
		{
			Console_Warning(Console_WriteLine, "cannot rediscover services because no protocol is yet defined");
		}
		else
		{
			//Console_WriteValueCFString("initiated search for services of type", (CFStringRef)[theProtocol serviceType]); // debug
			[browser stop];
			// TEMPORARY - determine if one needs to wait for the browser to stop, before starting a new search...
			[browser searchForServicesOfType:[theProtocol serviceType] inDomain:@""/* empty string implies local search */];
		}
	}
}// rediscoverServices


#pragma mark Accessors

/*!
Accessor.

(4.0)
*/
- (Session_Protocol)
currentProtocolID
{
	ServerBrowser_Protocol*		protocolObject = [self protocol];
	assert(nil != protocolObject);
	Session_Protocol			result = [protocolObject protocolID];
	
	
	return result;
}// currentProtocolID


/*!
Accessor.

(4.0)
*/
- (void)
insertObject:(ServerBrowser_NetService*)	service
inDiscoveredHostsAtIndex:(unsigned long)	index
{
	[discoveredHosts insertObject:service atIndex:index];
}
- (void)
removeObjectFromDiscoveredHostsAtIndex:(unsigned long)		index
{
	[discoveredHosts removeObjectAtIndex:index];
}// removeObjectFromDiscoveredHostsAtIndex:


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
discoveredHostIndexes
{
	return [[discoveredHostIndexes retain] autorelease];
}
- (void)
setDiscoveredHostIndexes:(NSIndexSet*)		indexes
{
	ServerBrowser_NetService*		theDiscoveredHost = nil;
	
	
	[discoveredHostIndexes release];
	discoveredHostIndexes = [indexes retain];
	
	theDiscoveredHost = [self discoveredHost];
	if (nil != theDiscoveredHost)
	{
		// auto-set the host and port to match this service
		[self setHostName:[theDiscoveredHost bestResolvedAddress]];
		[self setPortNumber:[[NSNumber numberWithUnsignedShort:[theDiscoveredHost bestResolvedPort]] stringValue]];
	}
}// setDiscoveredHostIndexes:


/*!
Accessor.

(4.0)
*/
- (NSString*)
errorMessage
{
	return errorMessage;
}
- (void)
setErrorMessage:(NSString*)		aString
{
	if (aString != errorMessage)
	{
		[errorMessage release];
		errorMessage = [aString retain];
	}
}// setErrorMessage:


/*!
Accessor.

(4.0)
*/
- (BOOL)
hidesDiscoveredHosts
{
	return hidesDiscoveredHosts;
}
- (void)
setHidesDiscoveredHosts:(BOOL)		flag
{
	hidesDiscoveredHosts = flag;
	if (flag)
	{
		[browser stop];
	}
	else
	{
		[self rediscoverServices];
	}
}// setHidesDiscoveredHosts:


/*!
Accessor.

(4.0)
*/
- (BOOL)
hidesErrorMessage
{
	return hidesErrorMessage;
}
- (void)
setHidesErrorMessage:(BOOL)		flag
{
	// note, it is better to call a more specific routine,
	// such as setHidesPortNumberError:
	hidesErrorMessage = flag;
}// setHidesErrorMessage:


/*!
Accessor.

(4.0)
*/
- (BOOL)
hidesPortNumberError
{
	return hidesPortNumberError;
}
- (void)
setHidesPortNumberError:(BOOL)		flag
{
	hidesPortNumberError = flag;
	[self setHidesErrorMessage:flag];
}// setHidesPortNumberError:


/*!
Accessor.

(4.0)
*/
- (BOOL)
hidesProgress
{
	return hidesProgress;
}
- (void)
setHidesProgress:(BOOL)		flag
{
	hidesProgress = flag;
}// setHidesProgress:


/*!
Accessor.

(4.0)
*/
- (BOOL)
hidesUserIDError
{
	return hidesUserIDError;
}
- (void)
setHidesUserIDError:(BOOL)		flag
{
	hidesUserIDError = flag;
	[self setHidesErrorMessage:flag];
}// setHidesUserIDError:


/*!
Accessor.

(4.0)
*/
- (NSString*)
hostName
{
	return [[hostName copy] autorelease];
}
+ (id)
autoNotifyOnChangeToHostName
{
	return flagNo();
}
- (void)
setHostName:(NSString*)		aString
{
	if (aString != hostName)
	{
		[self willChangeValueForKey:@"hostName"];
		
		if (nil == aString)
		{
			hostName = [@"" retain];
		}
		else
		{
			[hostName autorelease];
			hostName = [aString copy];
		}
		
		[self didChangeValueForKey:@"hostName"];
		// TEMPORARY; while Carbon is supported, send a Carbon event as well
		[self notifyOfChangeInValueReturnedBy:@selector(hostName)];
	}
}// setHostName:


/*!
Accessor.

(4.0)
*/
- (void)
insertObject:(NSString*)				name
inRecentHostsAtIndex:(unsigned long)	index
{
	[recentHosts insertObject:name atIndex:index];
}
- (void)
removeObjectFromRecentHostsAtIndex:(unsigned long)		index
{
	[recentHosts removeObjectAtIndex:index];
}// removeObjectFromRecentHostsAtIndex:


/*!
Accessor.

(4.0)
*/
- (NSString*)
portNumber
{
	return [[portNumber copy] autorelease];
}
+ (id)
autoNotifyOnChangeToPortNumber
{
	return flagNo();
}
- (void)
setPortNumber:(NSString*)	aString
{
	if (aString != portNumber)
	{
		[self willChangeValueForKey:@"portNumber"];
		
		if (nil == aString)
		{
			portNumber = [@"" retain];
		}
		else
		{
			[portNumber autorelease];
			portNumber = [aString copy];
		}
		[self setHidesPortNumberError:YES];
		
		[self didChangeValueForKey:@"portNumber"];
		// TEMPORARY; while Carbon is supported, send a Carbon event as well
		[self notifyOfChangeInValueReturnedBy:@selector(portNumber)];
	}
}// setPortNumber:


/*!
Accessor.

(4.0)
*/
- (NSArray*)
protocolDefinitions
{
	return [[protocolDefinitions retain] autorelease];
}


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
protocolIndexes
{
	return [[protocolIndexes retain] autorelease];
}
- (void)
setProtocolIndexByProtocol:(Session_Protocol)	aProtocol
{
	NSEnumerator*	toProtocolDesc = [[self protocolDefinitions] objectEnumerator];
	unsigned int	i = 0;
	
	
	while (ServerBrowser_Protocol* thisProtocol = [toProtocolDesc nextObject])
	{
		if (aProtocol == [thisProtocol protocolID])
		{
			[self setProtocolIndexes:[NSIndexSet indexSetWithIndex:i]];
			break;
		}
		++i;
	}
}
+ (id)
autoNotifyOnChangeToProtocolIndexes
{
	return flagNo();
}
- (void)
setProtocolIndexes:(NSIndexSet*)	indexes
{
	if (indexes != protocolIndexes)
	{
		[self willChangeValueForKey:@"protocolIndexes"];
		
		[protocolIndexes release];
		protocolIndexes = [indexes retain];
		
		[self didChangeValueForKey:@"protocolIndexes"];
		// TEMPORARY; while Carbon is supported, send a Carbon event as well
		[self notifyOfChangeInValueReturnedBy:@selector(protocolIndexes)];
		
		ServerBrowser_Protocol*		theProtocol = [self protocol];
		if (nil != theProtocol)
		{
			// auto-set the port number to match the default for this protocol
			[self setPortNumber:[[NSNumber numberWithUnsignedShort:[theProtocol defaultPort]] stringValue]];
			// rediscover services appropriate for this selection
			[self rediscoverServices];
		}
	}
}// setProtocolIndexes:


/*!
Accessor.

(4.0)
*/
- (id)
target
{
	return target;
}
+ (id)
autoNotifyOnChangeToTarget
{
	return flagNo();
}
- (void)
setTarget:(id)				anObject
protocol:(Session_Protocol)	aProtocol
hostName:(NSString*)		aString
portNumber:(unsigned int)	aNumber
userID:(NSString*)			anID
{
	if (anObject != target)
	{
		[self willChangeValueForKey:@"target"];
		
		[target release];
		target = [anObject retain];
		
		[self didChangeValueForKey:@"target"];
		
		initializePanel(self, aProtocol, (CFStringRef)aString, aNumber, (CFStringRef)anID);
	}
}// setTarget:protocol:hostName:portNumber:userID:


/*!
Accessor.

(4.0)
*/
- (NSString*)
userID
{
	return [[userID copy] autorelease];
}
+ (id)
autoNotifyOnChangeToUserID
{
	return flagNo();
}
- (void)
setUserID:(NSString*)	aString
{
	if (aString != userID)
	{
		[self willChangeValueForKey:@"userID"];
		
		if (nil == aString)
		{
			userID = [@"" retain];
		}
		else
		{
			[userID autorelease];
			userID = [aString copy];
		}
		[self setHidesUserIDError:YES];
		
		[self didChangeValueForKey:@"userID"];
		// TEMPORARY; while Carbon is supported, send a Carbon event as well
		[self notifyOfChangeInValueReturnedBy:@selector(userID)];
	}
}// setUserID:


#pragma mark Validators


/*!
Validates a port number entered by the user, returning an
appropriate error (and a NO result) if the number is incorrect.

(4.0)
*/
- (BOOL)
validatePortNumber:(id*/* NSString* */)	ioValue
error:(NSError**)						outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		// first strip whitespace
		*ioValue = [[*ioValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] retain];
		
		// while an NSNumberFormatter is more typical for validation,
		// the requirements for port numbers are quite simple
		NSScanner*	scanner = [NSScanner scannerWithString:*ioValue];
		int			value = 0;
		
		
		if ([scanner scanInt:&value] && [scanner isAtEnd] && (value >= 0) && (value <= 65535/* given in TCP/IP spec. */))
		{
			result = YES;
		}
		else
		{
			if (nil != outError) result = NO;
			else result = YES; // cannot return NO when the error instance is undefined
		}
		
		if (NO == result)
		{
			*outError = [NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
							code:kConstantsRegistry_NSErrorBadUserID
							userInfo:[[[NSDictionary alloc] initWithObjectsAndKeys:
										NSLocalizedStringFromTable
										(@"The port must be a number from 0 to 65535.", @"ServerBrowser"/* table */,
											@"message displayed for bad port numbers"), NSLocalizedDescriptionKey,
										nil] autorelease]];
			[self setErrorMessage:[[*outError userInfo] objectForKey:NSLocalizedDescriptionKey]];
			[self setHidesPortNumberError:NO];
		}
	}
	return result;
}// validatePortNumber:error:


/*!
Validates a user ID entered by the user, returning an
appropriate error (and a NO result) if the ID is incorrect.

(4.0)
*/
- (BOOL)
validateUserID:(id*/* NSString* */)	ioValue
error:(NSError**)					outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		// first strip whitespace
		*ioValue = [[*ioValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] retain];
		
		NSScanner*				scanner = [NSScanner scannerWithString:*ioValue];
		NSMutableCharacterSet*	validCharacters = [[[NSCharacterSet alphanumericCharacterSet] mutableCopy] autorelease];
		NSString*				value = nil;
		
		
		// periods, underscores and hyphens are also valid in Unix user names
		[validCharacters addCharactersInString:@".-_"];
		
		if ([scanner scanCharactersFromSet:validCharacters intoString:&value] && [scanner isAtEnd])
		{
			result = YES;
		}
		else
		{
			if (nil != outError) result = NO;
			else result = YES; // cannot return NO when the error instance is undefined
		}
		
		if (NO == result)
		{
			*outError = [NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
							code:kConstantsRegistry_NSErrorBadPortNumber
							userInfo:[[[NSDictionary alloc] initWithObjectsAndKeys:
										NSLocalizedStringFromTable
										(@"The user ID must only use letters, numbers, dashes, underscores, and periods.",
											@"ServerBrowser"/* table */, @"message displayed for bad user IDs"),
										NSLocalizedDescriptionKey,
										nil] autorelease]];
			[self setErrorMessage:[[*outError userInfo] objectForKey:NSLocalizedDescriptionKey]];
			[self setHidesUserIDError:NO];
		}
	}
	return result;
}// validateUserID:error:


#pragma mark NSKeyValueObservingCustomization


/*!
Returns true for keys that manually notify observers
(through "willChangeValueForKey:", etc.).

(4.0)
*/
+ (BOOL)
automaticallyNotifiesObserversForKey:(NSString*)	theKey
{
	BOOL	result = YES;
	SEL		flagSource = NSSelectorFromString([[self class] selectorNameForKeyChangeAutoNotifyFlag:theKey]);
	
	
	if (NULL != class_getClassMethod([self class], flagSource))
	{
		// See selectorToReturnKeyChangeAutoNotifyFlag: for more information on the form of the selector.
		result = [[self performSelector:flagSource] boolValue];
	}
	else
	{
		result = [super automaticallyNotifiesObserversForKey:theKey];
	}
	return result;
}// automaticallyNotifiesObserversForKey:


#pragma mark NSNetServiceBrowserDelegateMethods


/*!
Called as new services are discovered.

(4.0)
*/
- (void)
netServiceBrowser:(NSNetServiceBrowser*)	aNetServiceBrowser
didFindService:(NSNetService*)				aNetService
moreComing:(BOOL)							moreComing
{
#pragma unused(aNetServiceBrowser)
#pragma unused(moreComing)
	[self insertObject:[[[ServerBrowser_NetService alloc] initWithNetService:aNetService addressFamily:AF_INET] autorelease]
			inDiscoveredHostsAtIndex:[discoveredHosts count]];
	//NSLog(@"%@", [self mutableArrayValueForKey:@"discoveredHosts"]); // debug
}// netServiceBrowser:didFindService:moreComing:


/*!
Called when a search fails.

(4.0)
*/
- (void)
netServiceBrowser:(NSNetServiceBrowser*)	aNetServiceBrowser
didNotSearch:(NSDictionary*)				errorInfo
{
#pragma unused(aNetServiceBrowser)
	Console_Warning(Console_WriteValue, "search for services failed with error",
					[[errorInfo objectForKey:NSNetServicesErrorCode] intValue]);
}// netServiceBrowser:didNotSearch:


/*!
Called when a search has stopped.

(4.0)
*/
- (void)
netServiceBrowserDidStopSearch:(NSNetServiceBrowser*)	aNetServiceBrowser
{
#pragma unused(aNetServiceBrowser)
	//Console_WriteLine("search for services has stopped"); // debug
}// netServiceBrowserDidStopSearch:


/*!
Called when a search is about to begin.

(4.0)
*/
- (void)
netServiceBrowserWillSearch:(NSNetServiceBrowser*)	aNetServiceBrowser
{
#pragma unused(aNetServiceBrowser)
	//Console_WriteLine("search for services has begun"); // debug
}// netServiceBrowserWillSearch:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	
	// find out when the window will close, so that the event target can change
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:)
											name:NSWindowWillCloseNotification object:[self window]];
	
	// since double-click bindings require 10.4 or later, do this manually now
	[discoveredHostsTableView setDoubleAction:@selector(didDoubleClickDiscoveredHostWithSelection:)];
}// windowDidLoad


/*!
Handles initialization that should definitely occur before
the window NIB is loaded.  (Everything else is just done in
"init".)

(4.0)
*/
- (void)
windowWillLoad
{
	[super windowWillLoad];
	
	discoveredHosts = [[NSMutableArray alloc] init];
	recentHosts = [[NSMutableArray alloc] init];
	// TEMPORARY - it should be possible to externally define these (probably via Python)
	protocolDefinitions = [[[NSArray alloc] initWithObjects:
							[[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolSSH1
								description:NSLocalizedStringFromTable(@"SSH Version 1", @"ServerBrowser"/* table */, @"ssh-1")
								serviceType:@"_ssh._tcp."
								defaultPort:22] autorelease],
							[[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolSSH2
								description:NSLocalizedStringFromTable(@"SSH Version 2", @"ServerBrowser"/* table */, @"ssh-2")
								serviceType:@"_ssh._tcp."
								defaultPort:22] autorelease],
							[[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolTelnet
								description:NSLocalizedStringFromTable(@"TELNET", @"ServerBrowser"/* table */, @"telnet")
								serviceType:@"_telnet._tcp."
								defaultPort:23] autorelease],
							[[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolFTP
								description:NSLocalizedStringFromTable(@"FTP", @"ServerBrowser"/* table */, @"ftp")
								serviceType:@"_ftp._tcp."
								defaultPort:21] autorelease],
							[[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolSFTP
								description:NSLocalizedStringFromTable(@"SFTP", @"ServerBrowser"/* table */, @"sftp")
								serviceType:@"_ssh._tcp."
								defaultPort:22] autorelease],
							nil] autorelease];
	browser = [[NSNetServiceBrowser alloc] init];
	[browser setDelegate:self];
	discoveredHostIndexes = [[NSIndexSet alloc] init];
	hidesDiscoveredHosts = YES;
	hidesErrorMessage = YES;
	hidesPortNumberError = YES;
	hidesProgress = YES;
	hidesUserIDError = YES;
	protocolIndexes = [[NSIndexSet alloc] init];
	hostName = [[[NSString alloc] initWithString:@""] autorelease];
	portNumber = [[[NSString alloc] initWithString:@""] autorelease];
	userID = [[[NSString alloc] initWithString:@""] autorelease];
	errorMessage = [[NSString string] retain];
}// windowWillLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"ServerBrowser";
}// windowFrameAutosaveName


#pragma mark NSWindowNotifications


/*!
Responds to the panel closing by removing any ties to an
event target, but notifying that target first.  This would
have the effect, for instance, of associated windows
removing highlighting from interface elements to show that
they are no longer using this panel.

Also interrupts any Bonjour scans that may be in progress.

(4.0)
*/
- (void)
windowWillClose:(NSNotification*)	notification
{
#pragma unused(notification)
	// interrupt any Bonjour scans in progress
	[browser stop];
	
	// remember the selected host as a recent item
	[self insertObject:[[hostName copy] autorelease] inRecentHostsAtIndex:0];
	if ([recentHosts count] > 4/* arbitrary */)
	{
		[self removeObjectFromRecentHostsAtIndex:([recentHosts count] - 1)];
	}
	
	// stop associating the panel with any event target, and notify the previous target
	ServerBrowser_RemoveEventTarget();
}// windowWillClose:


#pragma mark Internal Methods


/*!
Accessor.

(4.0)
*/
- (ServerBrowser_NetService*)
discoveredHost
{
	ServerBrowser_NetService*	result = nil;
	unsigned int				selectedIndex = [[self discoveredHostIndexes] firstIndex];
	
	
	if (NSNotFound != selectedIndex)
	{
		result = [discoveredHosts objectAtIndex:selectedIndex];
	}
	return result;
}// discoveredHost


/*!
If an event target has been set (and one should have been, with
ServerBrowser_SetEventTarget()), sends an event to the target
to notify the target of changes to the panel.

Call this whenever the user makes a change to a core setting in
the panel.

This currently is implemented using Carbon Events for
compatibility.  In order to translate accordingly, only specific
selectors are allowed:
	hostName
	portNumber
	protocolIndexes
	userID
These methods are called when given, and their current return
values are translated into new Carbon event parameters of the
appropriate type.

(4.0)
*/
- (void)
notifyOfChangeInValueReturnedBy:(SEL)	valueGetter
{
	if (nullptr != gPanelEventTarget)
	{
		EventRef	panelChangedEvent = nullptr;
		OSStatus	error = noErr;
		
		
		// create a Carbon Event
		error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_ServerBrowser,
							kEventNetEvents_ServerBrowserNewData, GetCurrentEventTime(),
							kEventAttributeNone, &panelChangedEvent);
		
		// attach required parameters to event, then dispatch it
		if (noErr != error) panelChangedEvent = nullptr;
		else
		{
			Boolean		doPost = true;
			
			
			if (valueGetter == @selector(protocolIndexes))
			{
				Session_Protocol	protocolForEvent = [self currentProtocolID];
				
				
				error = SetEventParameter(panelChangedEvent, kEventParamNetEvents_Protocol,
											typeNetEvents_SessionProtocol, sizeof(protocolForEvent), &protocolForEvent);
				assert_noerr(error);
			}
			else if (valueGetter == @selector(hostName))
			{
				CFStringRef		hostNameForEvent = (CFStringRef)[self hostName];
				
				
				error = SetEventParameter(panelChangedEvent, kEventParamNetEvents_HostName,
											typeCFStringRef, sizeof(hostNameForEvent), &hostNameForEvent);
				assert_noerr(error);
			}
			else if (valueGetter == @selector(portNumber))
			{
				NSString*		portNumberString = [self portNumber];
				UInt32			portNumberForEvent = STATIC_CAST([portNumberString intValue], UInt32);
				
				
				error = SetEventParameter(panelChangedEvent, kEventParamNetEvents_PortNumber,
											typeUInt32, sizeof(portNumberForEvent), &portNumberForEvent);
				assert_noerr(error);
			}
			else if (valueGetter == @selector(userID))
			{
				CFStringRef		userIDForEvent = (CFStringRef)[self userID];
				
				
				error = SetEventParameter(panelChangedEvent, kEventParamNetEvents_UserID,
											typeCFStringRef, sizeof(userIDForEvent), &userIDForEvent);
				assert_noerr(error);
			}
			else
			{
				Console_Warning(Console_WriteLine, "invalid selector passed to notifyOfChangeInValueReturnedBy:");
				doPost = false;
			}
			
			if (doPost)
			{
				// finally, send the message to the target
				error = SendEventToEventTargetWithOptions(panelChangedEvent, gPanelEventTarget,
															kEventTargetDontPropagate);
			}
		}
		
		// dispose of event
		if (nullptr != panelChangedEvent) ReleaseEvent(panelChangedEvent), panelChangedEvent = nullptr;
	}
}// notifyOfChangeInValueReturnedBy:


/*!
Accessor.

(4.0)
*/
- (ServerBrowser_Protocol*)
protocol
{
	ServerBrowser_Protocol*		result = nil;
	unsigned int				selectedIndex = [[self protocolIndexes] firstIndex];
	
	
	if (NSNotFound != selectedIndex)
	{
		result = [[self protocolDefinitions] objectAtIndex:selectedIndex];
	}
	return result;
}// protocol


@end

// BELOW IS REQUIRED NEWLINE TO END FILE
