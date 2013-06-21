/*!	\file ServerBrowser.mm
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTerm
		© 1998-2013 by Kevin Grant.
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
#import <BoundName.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>
#import <SoundSystem.h>

// application includes
#import "AlertMessages.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DNR.h"
#import "NetEvents.h"
#import "Session.h"



#pragma mark Types

@interface ServerBrowser_Handler : NSObject< NSWindowDelegate, PopoverManager_Delegate, ServerBrowser_ViewManagerChannel >
{
	ServerBrowser_Ref			selfRef;				// identical to address of structure, but typed as ref
	ServerBrowser_ViewManager*	viewMgr;				// loads the server browser interface
	Popover_Window*				containerWindow;		// holds the main view
	NSView*						managedView;			// the view that implements the majority of the interface
	HIWindowRef					parentWindow;			// the Carbon window that the point is relative to
	CGPoint						parentRelativeArrowTip;	// the point relative to the parent window where the popover arrow appears
	EventTargetRef				eventTarget;			// temporary; allows external Carbon interfaces to respond to events
	Session_Protocol			initialProtocol;		// used to initialize the view when it loads
	NSString*					initialHostName;		// used to initialize the view when it loads
	unsigned int				initialPortNumber;		// used to initialize the view when it loads
	NSString*					initialUserID;			// used to initialize the view when it loads
	PopoverManager_Ref			popoverMgr;				// manages common aspects of popover window behavior
}

+ (ServerBrowser_Handler*)
viewHandlerFromRef:(ServerBrowser_Ref)_;

- (id)
initWithPosition:(CGPoint)_
relativeToParentWindow:(HIWindowRef)_
eventTarget:(EventTargetRef)_;

- (void)
configureWithProtocol:(Session_Protocol)_
hostName:(NSString*)_
portNumber:(unsigned int)_
userID:(NSString*)_;

- (void)
display;

- (void)
remove;

- (NSWindow*)
parentCocoaWindow;

// PopoverManager_Delegate

- (NSPoint)
idealAnchorPointForParentWindowFrame:(NSRect)_;

- (Popover_Properties)
idealArrowPositionForParentWindowFrame:(NSRect)_;

- (NSSize)
idealSize;

// ServerBrowser_ViewManagerChannel

- (void)
serverBrowser:(ServerBrowser_ViewManager*)_
didLoadManagedView:(NSView*)_;

- (void)
serverBrowser:(ServerBrowser_ViewManager*)_
didFinishUsingManagedView:(NSView*)_;

- (void)
serverBrowser:(ServerBrowser_ViewManager*)_
setManagedView:(NSView*)_
toScreenFrame:(NSRect)_;

@end // ServerBrowser_Handler


/*!
Implements an object wrapper for NSNetService instances returned
by Bonjour, that allows them to be easily inserted into user
interface elements without losing less user-friendly information
about each service.
*/
@interface ServerBrowser_NetService : NSObject< NSNetServiceDelegate >
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

@end // ServerBrowser_NetService


/*!
Implements an object wrapper for protocol definitions, that
allows them to be easily inserted into user interface elements
without losing less user-friendly information about each
protocol.
*/
@interface ServerBrowser_Protocol : BoundName_Object
{
@private
	Session_Protocol	protocolID;
	NSString*			serviceType; // RFC 2782 / Bonjour, e.g. "_xyz._tcp."
	unsigned short		defaultPort;
}

// accessors; see "Protocol Definitions" array controller in the NIB, for key names

- (unsigned short)
defaultPort;
- (void)
setDefaultPort:(unsigned short)_;

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

@end // ServerBrowser_Protocol

#pragma mark Internal Method Prototypes
namespace {

void		notifyOfClosedPopover		(EventTargetRef);
OSStatus	receiveLookupComplete		(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

@interface ServerBrowser_ViewManager (ServerBrowser_ViewManagerInternal)

- (void)
didDoubleClickDiscoveredHostWithSelection:(NSArray*)_;

- (ServerBrowser_NetService*)
discoveredHost;

- (void)
notifyOfChangeInValueReturnedBy:(SEL)_;

- (ServerBrowser_Protocol*)
protocol;

- (void)
serverBrowserWindowWillClose:(NSNotification*)_;

@end // ServerBrowser_ViewManager (ServerBrowser_ViewManagerInternal)



#pragma mark Public Methods

/*!
Constructs a server browser as a popover that points to the given
location in the parent window.  Use ServerBrowser_Display() to
show the popover.  Use ServerBrowser_Configure() beforehand to set
the various fields in the interface.

Note that the initial position is expressed in window coordinates
(top zero, left zero), not Cartesian (Cocoa) coordinates.

For now, a Carbon Events target is used to communicate a couple
of key events: changes to the contents of the panel (e.g. user
modifies the host name field), and the closing of the server
browser itself.  In the future this will probably be replaced by
more direct invocation of Cocoa methods on a given object, but
not until dependent interfaces have been ported to Cocoa.

(4.0)
*/
ServerBrowser_Ref
ServerBrowser_New	(HIWindowRef		inParentWindow,
					 CGPoint			inParentRelativePoint,
					 EventTargetRef		inResponder)
{
	ServerBrowser_Ref	result = nullptr;
	
	
	// WARNING: this interpretation should match "viewHandlerFromRef:"
	result = (ServerBrowser_Ref)[[ServerBrowser_Handler alloc] initWithPosition:inParentRelativePoint
																				relativeToParentWindow:inParentWindow
																				eventTarget:inResponder];
	
	return result;
}// New


/*!
Releases the server browser and sets your copy of
the reference to nullptr.

(4.0)
*/
void
ServerBrowser_Dispose	(ServerBrowser_Ref*		inoutDialogPtr)
{
	ServerBrowser_Handler*	ptr = [ServerBrowser_Handler viewHandlerFromRef:*inoutDialogPtr];
	
	
	[ptr release];
	*inoutDialogPtr = nullptr;
}// Dispose


/*!
Fills in the fields of the interface with the given values.
This should be done before calling ServerBrowser_Display().
It allows you to reuse a browser for multiple data sources.

(4.0)
*/
void
ServerBrowser_Configure		(ServerBrowser_Ref		inDialog,
							 Session_Protocol		inProtocol,
							 CFStringRef			inHostName,
							 UInt16					inPortNumber,
							 CFStringRef			inUserID)
{
	ServerBrowser_Handler*		ptr = [ServerBrowser_Handler viewHandlerFromRef:inDialog];
	
	
	[ptr configureWithProtocol:inProtocol hostName:(NSString*)inHostName portNumber:inPortNumber
								userID:(NSString*)inUserID];
}// Configure


/*!
Shows the server browser, pointing at the target view
that was given at construction time.

(4.0)
*/
void
ServerBrowser_Display	(ServerBrowser_Ref		inDialog)
{
	ServerBrowser_Handler*		ptr = [ServerBrowser_Handler viewHandlerFromRef:inDialog];
	
	
	if (nullptr == ptr)
	{
		Alert_ReportOSStatus(paramErr);
	}
	else
	{
		// load the view asynchronously and eventually display it in a window
		[ptr display];
	}
}// Display


/*!
Hides the server browser.  It can be redisplayed at any
time by calling ServerBrowser_Display() again.

(4.0)
*/
void
ServerBrowser_Remove	(ServerBrowser_Ref		inDialog)
{
	ServerBrowser_Handler*		ptr = [ServerBrowser_Handler viewHandlerFromRef:inDialog];
	
	
	[ptr remove];
}// Remove


#pragma mark Internal Methods
namespace {

/*!
For Carbon compatibility, sends an event to the previous
Carbon event target of the panel, notifying it that a
new target is taking over.

(4.0)
*/
void
notifyOfClosedPopover	(EventTargetRef		inOldTarget)
{
	EventRef	panelClosedEvent = nullptr;
	OSStatus	error = noErr;
	
	
	// create a Carbon Event
	error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_ServerBrowser,
						kEventNetEvents_ServerBrowserClosed, GetCurrentEventTime(),
						kEventAttributeNone, &panelClosedEvent);
	if (noErr != error)
	{
		panelClosedEvent = nullptr;
	}
	else
	{
		error = SendEventToEventTargetWithOptions(panelClosedEvent, inOldTarget, kEventTargetDontPropagate);
	}
	
	// dispose of event
	if (nullptr != panelClosedEvent)
	{
		ReleaseEvent(panelClosedEvent), panelClosedEvent = nullptr;
	}
}// notifyOfClosedPopover


/*!
Handles "kEventNetEvents_HostLookupComplete" of
"kEventClassNetEvents_DNS" by updating the text
field containing the remote host name.

(4.0)
*/
OSStatus
receiveLookupComplete	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inViewManager)
{
	AutoPool					_;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	ServerBrowser_ViewManager*	serverBrowser = REINTERPRET_CAST(inViewManager, ServerBrowser_ViewManager*);
	OSStatus					result = eventNotHandledErr;
	
	
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
					[serverBrowser setHostName:(NSString*)addressCFString];
					CFRelease(addressCFString), addressCFString = nullptr;
					result = noErr;
				}
			}
			DNR_Dispose(&lookupDataPtr);
		}
	}
	
	// hide progress indicator
	[serverBrowser setHidesProgress:YES];
	
	return result;
}// receiveLookupComplete

} // anonymous namespace


@implementation ServerBrowser_Handler


/*!
Converts from the opaque reference type to the internal type.

(4.0)
*/
+ (ServerBrowser_Handler*)
viewHandlerFromRef:(ServerBrowser_Ref)		aRef
{
	return (ServerBrowser_Handler*)aRef;
}// viewHandlerFromRef


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithPosition:(CGPoint)				aPoint
relativeToParentWindow:(HIWindowRef)	aWindow
eventTarget:(EventTargetRef)			aTarget
{
	self = [super init];
	if (nil != self)
	{
		self->selfRef = (ServerBrowser_Ref)self;
		self->viewMgr = nil;
		self->containerWindow = nil;
		self->managedView = nil;
		self->parentWindow = aWindow;
		self->parentRelativeArrowTip = aPoint;
		self->eventTarget = aTarget;
		self->initialProtocol = kSession_ProtocolSSH1;
		self->initialHostName = [@"" retain];
		self->initialPortNumber = 22;
		self->initialUserID = [@"" retain];
		self->popoverMgr = nullptr;
	}
	return self;
}// initWithPosition:relativeToParentWindow:eventTarget:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[managedView release];
	[viewMgr release];
	[initialHostName release];
	[initialUserID release];
	if (nullptr != popoverMgr)
	{
		PopoverManager_Dispose(&popoverMgr);
	}
	[super dealloc];
}// dealloc


/*!
Specifies the initial values for the user interface.  This is
typically done just before calling "display".

(4.0)
*/
- (void)
configureWithProtocol:(Session_Protocol)	aProtocol
hostName:(NSString*)						aHostName
portNumber:(unsigned int)					aPortNumber
userID:(NSString*)							aUserID
{
	self->initialProtocol = aProtocol;
	self->initialHostName = [aHostName retain];
	self->initialPortNumber = aPortNumber;
	self->initialUserID = [aUserID retain];
}// configureWithProtocol:hostName:portNumber:userID:


/*!
Creates the server browser view asynchronously; when the view
is ready, it calls "serverBrowser:didLoadManagedView:".

(4.0)
*/
- (void)
display
{
	if (nil == self->viewMgr)
	{
		// no focus is done the first time because this is
		// eventually done in "serverBrowser:didLoadManagedView:"
		self->viewMgr = [[ServerBrowser_ViewManager alloc]
							initWithResponder:self eventTarget:self->eventTarget];
	}
	else
	{
		// window is already loaded, just activate it (but also initialize
		// again, to mimic initialization performed in the “create new” case)
		[self->viewMgr setProtocolIndexByProtocol:self->initialProtocol];
		[self->viewMgr setHostName:self->initialHostName];
		[self->viewMgr setPortNumber:[NSString stringWithFormat:@"%d", self->initialPortNumber]];
		[self->viewMgr setUserID:self->initialUserID];
		PopoverManager_DisplayPopover(self->popoverMgr);
	}
}// display


/*!
Hides the popover.  It can be shown again at any time
using the "display" method.

(4.0)
*/
- (void)
remove
{
	if (nil != self->popoverMgr)
	{
		PopoverManager_RemovePopover(self->popoverMgr);
	}
}// remove


/*!
Returns the parent window of the target view.

(4.0)
*/
- (HIWindowRef)
parentCarbonWindow
{
	HIWindowRef		result = self->parentWindow;
	
	
	return result;
}// parentCarbonWindow


/*!
Returns the Cocoa window that represents the parent
of the target view, even if that is a Carbon window.

(4.0)
*/
- (NSWindow*)
parentCocoaWindow
{
	NSWindow*	result = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow([self parentCarbonWindow]);
	
	
	return result;
}// parentCocoaWindow


#pragma mark NSWindowDelegate


/*!
Returns the appropriate animation rectangle for the
given sheet.

(4.0)
*/
- (NSRect)
window:(NSWindow*)				window
willPositionSheet:(NSWindow*)	sheet
usingRect:(NSRect)				rect
{
#pragma unused(window, sheet)
	NSRect	result = rect;
	NSRect	someFrame = [self->containerWindow frameRectForViewRect:NSZeroRect];
	
	
	// move an arbitrary distance away from the top edge
	result.origin.y -= 1;
	
	// also offset further using the view position as a clue (namely, if the arrow is on top the sheet moves further)
	result.origin.y -= (someFrame.origin.y + someFrame.size.height);
	
	// the meaning of the height is undefined, so make it zero
	result.size.height = 0;
	
	// force the fold-out animation, which seems to look better with popovers
	result.size.width -= 200;
	result.origin.x += 100;
	
	return result;
}// window:willPositionSheet:usingRect:


#pragma mark PopoverManager_Delegate


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForParentWindowFrame:".

(4.0)
*/
- (NSPoint)
idealAnchorPointForParentWindowFrame:(NSRect)	parentFrame
{
#pragma unused(parentFrame)
	NSPoint		result = NSMakePoint(parentRelativeArrowTip.x, parentFrame.size.height - parentRelativeArrowTip.y);
	
	
	return result;
}// idealAnchorPointForParentWindowFrame:


/*!
Returns arrow placement information for the popover.

(4.0)
*/
- (Popover_Properties)
idealArrowPositionForParentWindowFrame:(NSRect)		parentFrame
{
#pragma unused(parentFrame)
	Popover_Properties	result = kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow;
	
	
	return result;
}// idealArrowPositionForParentWindowFrame:


/*!
Returns the initial size for the popover.

(4.0)
*/
- (NSSize)
idealSize
{
	NSRect		frameRect = [self->containerWindow frameRectForViewRect:[self->managedView frame]];
	NSSize		result = frameRect.size;
	
	
	return result;
}// idealSize


#pragma mark ServerBrowser_ViewManagerChannel


/*!
Called when a ServerBrowser_ViewManager has finished
loading and initializing its view; responds by displaying
the view in a window and giving it keyboard focus.

Since this may be invoked multiple times, the window is
only created during the first invocation.

(4.0)
*/
- (void)
serverBrowser:(ServerBrowser_ViewManager*)	aBrowser
didLoadManagedView:(NSView*)				aManagedView
{
	self->managedView = aManagedView;
	[self->managedView retain];
	
	[aBrowser setProtocolIndexByProtocol:self->initialProtocol];
	[aBrowser setHostName:self->initialHostName];
	[aBrowser setPortNumber:[NSString stringWithFormat:@"%d", self->initialPortNumber]];
	[aBrowser setUserID:self->initialUserID];
	
	if (nil == self->containerWindow)
	{
		self->containerWindow = [[Popover_Window alloc] initWithView:aManagedView
																		attachedToPoint:NSZeroPoint/* see delegate */
																		inWindow:[self parentCocoaWindow]];
		[self->containerWindow setDelegate:self];
		[self->containerWindow setReleasedWhenClosed:NO];
		CocoaBasic_ApplyStandardStyleToPopover(self->containerWindow, true/* has arrow */);
		self->popoverMgr = PopoverManager_New(self->containerWindow, [aBrowser logicalFirstResponder],
												self/* delegate */, kPopoverManager_AnimationTypeMinimal,
												[self parentCarbonWindow]);
		PopoverManager_DisplayPopover(self->popoverMgr);
	}
}// serverBrowser:didLoadManagedView:


/*!
Responds to a close of the interface by updating
any associated interface elements (such as a button
that opened the popover in the first place).

(4.0)
*/
- (void)
serverBrowser:(ServerBrowser_ViewManager*)	aBrowser
didFinishUsingManagedView:(NSView*)			aManagedView
{
#pragma unused(aBrowser, aManagedView)
	notifyOfClosedPopover(self->eventTarget);
}// serverBrowser:didFinishUsingManagedView:


/*!
Requests that the containing window be resized to allow the given
view size.  This is only for odd cases like a user interface
element causing part of the display to appear or disappear;
normally the window can just be manipulated directly.

(4.0)
*/
- (void)
serverBrowser:(ServerBrowser_ViewManager*)	aBrowser
setManagedView:(NSView*)					aManagedView
toScreenFrame:(NSRect)						aRect
{
#pragma unused(aBrowser, aManagedView)
	NSRect	windowFrame = [self->containerWindow frameRectForViewRect:aRect];
	
	
	[self->containerWindow setFrame:windowFrame display:YES animate:YES];
	PopoverManager_UseIdealLocationAfterDelay(self->popoverMgr, 0.1/* arbitrary delay */);
}// serverBrowser:setManagedView:toScreenFrame:


@end // ServerBrowser_Handler


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


@end // ServerBrowser_NetService


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
	self = [super initWithBoundName:aString];
	if (nil != self)
	{
		[self setProtocolID:anID];
		[self setServiceType:anRFC2782Name];
		[self setDefaultPort:aNumber];
	}
	return self;
}// initWithID:description:serviceType:defaultPort:


#pragma mark Accessors


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


@end // ServerBrowser_Protocol


@implementation ServerBrowser_ViewManager


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithResponder:(id< ServerBrowser_ViewManagerChannel >)	aResponder
eventTarget:(EventTargetRef)								aTarget
{
	self = [super init];
	if (nil != self)
	{
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
		discoveredHostsContainer = nil;
		discoveredHostsTableView = nil;
		managedView = nil;
		nextResponderWhenHidingDiscoveredHosts = nil;
		
		responder = aResponder;
		eventTarget = aTarget;
		lookupHandlerPtr = nullptr;
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
		
		// it is necessary to capture and release all top-level objects here
		// so that "self" can actually be deallocated; otherwise, the implicit
		// retain-count of 1 on each top-level object prevents deallocation
		{
			NSArray*	objects = nil;
			NSNib*		loader = [[NSNib alloc] initWithNibNamed:@"ServerBrowserCocoa" bundle:nil];
			
			
			if (NO == [loader instantiateNibWithOwner:self topLevelObjects:&objects])
			{
				[self dealloc], self = nil;
			}
			[loader release];
			[objects makeObjectsPerformSelector:@selector(release)];
		}
	}
	return self;
}// initWithResponder:eventTarget:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	// See the initializer and "awakeFromNib" for initializations to clean up here.
	delete lookupHandlerPtr, lookupHandlerPtr = nullptr;
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
Returns the view that a window ought to focus first
using NSWindow’s "makeFirstResponder:".

(4.0)
*/
- (NSView*)
logicalFirstResponder
{
	return self->logicalFirstResponder;
}// logicalFirstResponder


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
		
		
		// install lookup handler if none exists
		if (nullptr == self->lookupHandlerPtr)
		{
			self->lookupHandlerPtr = new CarbonEventHandlerWrap(GetApplicationEventTarget(), receiveLookupComplete,
																CarbonEventSetInClass
																(CarbonEventClass(kEventClassNetEvents_DNS),
																	kEventNetEvents_HostLookupComplete),
																self/* user data */);
		}
		
		// begin lookup of the domain name
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
	NSRect const	kOldFrame = [self->managedView frame];
	NSRect			newFrame = [self->managedView frame];
	NSRect			convertedFrame = [self->managedView frame];
	
	
	hidesDiscoveredHosts = flag;
	if (flag)
	{
		Float32 const	kPersistentHeight = 200; // IMPORTANT: must agree with NIB layout!!!
		Float32			deltaHeight = (kPersistentHeight - kOldFrame.size.height);
		
		
		[browser stop];
		
		newFrame.size.height += deltaHeight;
		convertedFrame.size.height += deltaHeight;
		convertedFrame.origin.y -= deltaHeight;
		
		// fix the current keyboard focus, if necessary
		{
			NSWindow*		popoverWindow = [self->managedView window];
			NSResponder*	firstResponder = [popoverWindow firstResponder];
			
			
			if ((nil != firstResponder) && [firstResponder isKindOfClass:[NSView class]])
			{
				NSView*		asView = (NSView*)firstResponder;
				NSRect		windowRelativeFrame = [[asView superview] convertRect:[asView frame]
																					toView:[popoverWindow contentView]];
				
				
				// NOTE: this calculation assumes the persistent part is always at the top window edge
				if (windowRelativeFrame.origin.y < (kOldFrame.size.height - kPersistentHeight))
				{
					// current keyboard focus is in the region that is being hidden;
					// force the keyboard focus to change to something that is visible
					[[self->managedView window] makeFirstResponder:self->nextResponderWhenHidingDiscoveredHosts];
				}
			}
		}
	}
	else
	{
		Float32		deltaHeight = (450/* IMPORTANT: must agree with NIB layout!!! */ - kOldFrame.size.height);
		
		
		newFrame.size.height += deltaHeight;
		convertedFrame.size.height += deltaHeight;
		convertedFrame.origin.y -= deltaHeight;
		[self rediscoverServices];
	}
	convertedFrame.origin = [[self->managedView window] convertBaseToScreen:convertedFrame.origin];
	
	if (flag)
	{
		[self->responder serverBrowser:self setManagedView:self->managedView toScreenFrame:convertedFrame];
		[self->managedView setFrame:newFrame];
		[self->discoveredHostsContainer setHidden:flag];
	}
	else
	{
		[self->discoveredHostsContainer setHidden:flag];
		[self->responder serverBrowser:self setManagedView:self->managedView toScreenFrame:convertedFrame];
		[self->managedView setFrame:newFrame];
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
	return [NSNumber numberWithBool:NO];
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
	return [NSNumber numberWithBool:NO];
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
	return [NSNumber numberWithBool:NO];
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
	return [NSNumber numberWithBool:NO];
}
- (void)
setTarget:(id)		anObject
{
	if (anObject != target)
	{
		[self willChangeValueForKey:@"target"];
		
		[target release];
		target = [anObject retain];
		
		[self didChangeValueForKey:@"target"];
	}
}// setTarget:


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
	return [NSNumber numberWithBool:NO];
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


#pragma mark NSNibAwaking


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
awakeFromNib
{
	// NOTE: superclass does not implement "awakeFromNib", otherwise it should be called here
	assert(nil != discoveredHostsContainer);
	assert(nil != discoveredHostsTableView);
	assert(nil != managedView);
	assert(nil != nextResponderWhenHidingDiscoveredHosts);
	
	[self setHidesDiscoveredHosts:YES];
	[self->responder serverBrowser:self didLoadManagedView:self->managedView];
	
	// find out when the window will close, so that the button that opened the window can return to normal
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(serverBrowserWindowWillClose:)
											name:NSWindowWillCloseNotification object:[self->managedView window]];
	
	// since double-click bindings require 10.4 or later, do this manually now
	[discoveredHostsTableView setIgnoresMultiClick:NO];
	[discoveredHostsTableView setTarget:self];
	[discoveredHostsTableView setDoubleAction:@selector(didDoubleClickDiscoveredHostWithSelection:)];
}// awakeFromNib


@end // ServerBrowser_ViewManager


@implementation ServerBrowser_ViewManager (ServerBrowser_ViewManagerInternal)


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
ServerBrowser_New()), sends an event to the target to notify the
target of changes to the panel.

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
	if (nullptr != self->eventTarget)
	{
		EventRef	panelChangedEvent = nullptr;
		OSStatus	error = noErr;
		
		
		// create a Carbon Event
		error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_ServerBrowser,
							kEventNetEvents_ServerBrowserNewData, GetCurrentEventTime(),
							kEventAttributeNone, &panelChangedEvent);
		
		// attach required parameters to event, then dispatch it
		if (noErr != error)
		{
			panelChangedEvent = nullptr;
		}
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
				CFStringRef		hostNameForEvent = BRIDGE_CAST([self hostName], CFStringRef);
				
				
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
				CFStringRef		userIDForEvent = BRIDGE_CAST([self userID], CFStringRef);
				
				
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
				error = SendEventToEventTargetWithOptions(panelChangedEvent, self->eventTarget,
															kEventTargetDontPropagate);
			}
		}
		
		// dispose of event
		if (nullptr != panelChangedEvent)
		{
			ReleaseEvent(panelChangedEvent), panelChangedEvent = nullptr;
		}
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
serverBrowserWindowWillClose:(NSNotification*)	notification
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
	
	// notify the handler
	[self->responder serverBrowser:self didFinishUsingManagedView:self->managedView];
}// serverBrowserWindowWillClose:


@end

// BELOW IS REQUIRED NEWLINE TO END FILE
