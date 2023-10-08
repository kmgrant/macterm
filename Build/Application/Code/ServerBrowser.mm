/*!	\file ServerBrowser.mm
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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
#import <objc/objc-runtime.h>
@import Cocoa;

// Unix includes
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// library includes
#import <BoundName.objc++.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <MemoryBlocks.h>
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>
#import <SoundSystem.h>

// application includes
#import "AlertMessages.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DNR.h"
#import "Session.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>



#pragma mark Types

/*!
Manages the Server Browser user interface.
*/
@interface ServerBrowser_Object : NSObject< NSWindowDelegate, PopoverManager_Delegate, ServerBrowser_VCDelegate > //{

// initializers
	- (instancetype)
	initWithPosition:(CGPoint)_
	relativeToParentWindow:(NSWindow*)_
	dataObserver:(id< ServerBrowser_DataChangeObserver >)_;

// new methods
	- (void)
	configureWithProtocol:(Session_Protocol)_
	hostName:(NSString*)_
	portNumber:(unsigned int)_
	userID:(NSString*)_;
	- (void)
	display;
	- (void)
	remove;

// accessors
	//! Holds the main view.
	@property (strong) Popover_Window*
	containerWindow;
	//! Object that is notified about key property changes.
	@property (assign) id< ServerBrowser_DataChangeObserver >
	dataObserver;
	//! Used to initialize the view when it loads.
	@property (strong) NSString*
	initialHostName;
	//! Used to initialize the view when it loads.
	@property (assign) unsigned int
	initialPortNumber;
	//! Used to initialize the view when it loads.
	@property (assign) Session_Protocol
	initialProtocol;
	//! Used to initialize the view when it loads.
	@property (strong) NSString*
	initialUserID;
	//! The view that implements the majority of the interface.
	@property (strong) NSView*
	managedView;
	//! Manages common aspects of popover window behavior.
	@property (strong) PopoverManager_Ref
	popoverMgr;
	//! The point relative to the parent window where the popover arrow appears.
	@property (assign) CGPoint
	parentRelativeArrowTip;
	//! The window that the point is relative to.
	@property (strong) NSWindow*
	parentWindow;
	//! Loads the server browser interface.
	@property (strong) ServerBrowser_VC*
	viewMgr;

@end //}


/*!
Implements an object wrapper for NSNetService instances returned
by Bonjour, that allows them to be easily inserted into user
interface elements without losing less user-friendly information
about each service.
*/
@interface ServerBrowser_NetService : NSObject< NSNetServiceDelegate > //{

// initializersbestResolvedPort
	- (instancetype)
	initWithViewModel:(UIServerBrowser_Model*)_
	netService:(NSNetService*)_
	addressFamily:(unsigned char)_;

// accessors; see "Discovered Hosts" array controller in the NIB, for key names
	//! Specifies type of address to prefer in queries.
	@property (assign) unsigned char
	addressFamily; // AF_INET or AF_INET6
	//! From a list of returned servers, the address picked for the host field.
	@property (strong) NSString*
	bestResolvedAddress;
	//! From a list of returned servers, the port number picked for the port field.
	@property (assign) unsigned short
	bestResolvedPort;
	//! Equivalent to "netService.name".
	- (NSString*)
	description;
	//! System object to resolve host names in the background.
	@property (strong, readonly) NSNetService*
	netService;
	//! Used to update the UI when a host is finally resolved.
	@property (weak) UIServerBrowser_Model*
	viewModel;

@end //}


/*!
Implements an object wrapper for protocol definitions, that
allows them to be easily inserted into user interface elements
without losing less user-friendly information about each
protocol.
*/
@interface ServerBrowser_Protocol : BoundName_Object //{

// accessors; see "Protocol Definitions" array controller in the NIB, for key names
	//! The port number typically used with the protocol, e.g. 22 for SSH.
	@property (assign) unsigned short
	defaultPort;
	//! The type of service this refers to (e.g. kSession_ProtocolSSH2).
	@property (assign) Session_Protocol
	protocolID;
	//! RFC 2782 / Bonjour type, e.g. "_xyz._tcp.".
	@property (copy) NSString*
	serviceType;

// initializers
	- (instancetype)
	initWithID:(Session_Protocol)_
	description:(NSString*)_
	serviceType:(NSString*)_
	defaultPort:(unsigned short)_;

@end //}


/*!
Implements the server browser.
*/
@interface ServerBrowser_VC : NSViewController< NSNetServiceBrowserDelegate,
												UIServerBrowser_ActionHandling > //{

// initializers
	- (instancetype)
	initWithResponder:(id< ServerBrowser_VCDelegate >)_
	dataObserver:(id< ServerBrowser_DataChangeObserver >)_;

// new methods
	- (void)
	serverBrowserWindowWillClose:(NSNotification*)_;

// accessors
	//! System object to keep track of a background search for services.
	@property (strong) NSNetServiceBrowser*
	browser;
	//! Object that is notified about key property changes (e.g. to update the UI).
	@property (weak) id< ServerBrowser_DataChangeObserver >
	dataObserver;
	//! Control construction of constraints.
	@property (assign) BOOL
	didSetConstraints;
	//! Details on discovered services; for populating a list in the UI.
	@property (strong) NSMutableArray< ServerBrowser_NetService* >*
	discoveredHosts;
	//! Structures collecting all properties of supported protocol options.
	@property (strong, nonnull, readonly) NSArray< ServerBrowser_Protocol* >*
	protocolDefinitions;
	//! An object notified of basic changes to the browser, such as showing/hiding/resizing.
	@property (weak) id< ServerBrowser_VCDelegate >
	responder;
	//! The data that is bound to the panel’s user interface.
	@property (strong) UIServerBrowser_Model*
	viewModel;

@end //}



#pragma mark Public Methods

/*!
Constructs a server browser as a popover that points to the given
location in the parent window.  Use ServerBrowser_Display() to
show the popover.  Use ServerBrowser_Configure() beforehand to set
the various fields in the interface.

Note that the initial position is expressed in window coordinates
(top zero, left zero), not Cartesian (Cocoa) coordinates.

(4.1)
*/
ServerBrowser_Ref
ServerBrowser_New	(NSWindow*									inParentWindow,
					 CGPoint									inParentRelativePoint,
					 id< ServerBrowser_DataChangeObserver >		inDataObserver)
{
	ServerBrowser_Ref	result = nullptr;
	
	
	result = [[ServerBrowser_Object alloc] initWithPosition:inParentRelativePoint
															relativeToParentWindow:inParentWindow
															dataObserver:inDataObserver];
	
	return result;
}// New


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
	[inDialog configureWithProtocol:inProtocol
									hostName:BRIDGE_CAST(inHostName, NSString*)
									portNumber:inPortNumber
									userID:BRIDGE_CAST(inUserID, NSString*)];
}// Configure


/*!
Shows the server browser, pointing at the target view
that was given at construction time.

(4.0)
*/
void
ServerBrowser_Display	(ServerBrowser_Ref		inDialog)
{
	if (nullptr == inDialog)
	{
		Sound_StandardAlert(); // TEMPORARY (display alert message?)
	}
	else
	{
		// load the view asynchronously and eventually display it in a window
		[inDialog display];
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
	[inDialog remove];
}// Remove


#pragma mark -
@implementation ServerBrowser_Object //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPosition:(CGPoint)								aPoint
relativeToParentWindow:(NSWindow*)						aWindow
dataObserver:(id< ServerBrowser_DataChangeObserver >)	anObserver
{
	self = [super init];
	if (nil != self)
	{
		_viewMgr = nil;
		_containerWindow = nil;
		_managedView = nil;
		_parentWindow = aWindow;
		_parentRelativeArrowTip = aPoint;
		_dataObserver = anObserver;
		_initialProtocol = kSession_ProtocolSSH2;
		_initialHostName = @"";
		_initialPortNumber = 22;
		_initialUserID = @"";
		_popoverMgr = nullptr;
	}
	return self;
}// initWithPosition:relativeToParentWindow:dataObserver:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	Memory_EraseWeakReferences(BRIDGE_CAST(self, void*));
}// dealloc


#pragma mark Initializers Disabled From Superclass


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
init
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithPosition:CGPointZero relativeToParentWindow:nil dataObserver:nil];
}// init


#pragma mark New Methods


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
	self.initialProtocol = aProtocol;
	self.initialHostName = aHostName;
	self.initialPortNumber = aPortNumber;
	self.initialUserID = aUserID;
}// configureWithProtocol:hostName:portNumber:userID:


/*!
Creates the server browser view asynchronously; when the view
is ready, it calls "serverBrowser:didLoadManagedView:".

(4.0)
*/
- (void)
display
{
	if (nil == _viewMgr)
	{
		// no focus is done the first time because this is
		// eventually done in "serverBrowser:didLoadManagedView:"
		self.viewMgr = [[ServerBrowser_VC alloc] initWithResponder:self dataObserver:self.dataObserver];
	}
	else
	{
		// window is already loaded, just activate it (but also initialize
		// again, to mimic initialization performed in the “create new” case)
		self.viewMgr.viewModel.connectionProtocol = self.initialProtocol;
		self.viewMgr.viewModel.hostName = self.initialHostName;
		self.viewMgr.viewModel.portNumber = [NSString stringWithFormat:@"%d", self.initialPortNumber];
		self.viewMgr.viewModel.userID = self.initialUserID;
		PopoverManager_DisplayPopover(_popoverMgr);
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
	if (nil != self.popoverMgr)
	{
		PopoverManager_RemovePopover(self.popoverMgr, true/* is confirming */);
	}
}// remove


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
	NSRect		result = rect;
	NSRect		someFrame = [self.containerWindow frameRectForViewSize:NSZeroSize];
	
	
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
Assists the dynamic resize of a popover window by indicating
whether or not there are per-axis constraints on resizing.

(2017.05)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getHorizontalResizeAllowed:(BOOL*)		outHorizontalFlagPtr
getVerticalResizeAllowed:(BOOL*)		outVerticalFlagPtr
{
#pragma unused(aPopoverManager)
	*outHorizontalFlagPtr = YES;
	//*outVerticalFlagPtr = NO;
	*outVerticalFlagPtr = YES;
}// popoverManager:getHorizontalResizeAllowed:getVerticalResizeAllowed:


/*!
Returns the initial view size for the popover.

(2017.05)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getIdealSize:(NSSize*)					outSizePtr
{
#pragma unused(aPopoverManager)
	// see also: "dataUpdatedServicesListVisible:" below
	*outSizePtr = CGSizeMake(660, 230); // arbitrary; see SwiftUI code/playground
}// popoverManager:getIdealSize:


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForFrame:parentWindow:".

(4.0)
*/
- (NSPoint)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealAnchorPointForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentFrame, parentWindow)
	NSPoint		result = NSMakePoint(_parentRelativeArrowTip.x, self.parentRelativeArrowTip.y);
	
	
	return result;
}// popoverManager:idealAnchorPointForFrame:parentWindow:


/*!
Returns arrow placement information for the popover.

(4.0)
*/
- (Popover_Properties)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentFrame, parentWindow)
	Popover_Properties	result = kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow;
	
	
	return result;
}// popoverManager:idealArrowPositionForFrame:parentWindow:


#pragma mark ServerBrowser_VCDelegate


/*!
Called when a ServerBrowser_VC has finished loading and
initializing its view; responds by displaying the view
in a window and giving it keyboard focus.

Since this may be invoked multiple times, the window is
only created during the first invocation.

(4.0)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)	aBrowser
didLoadManagedView:(NSView*)		aManagedView
{
	self.managedView = aManagedView;
	
	aBrowser.viewModel.connectionProtocol = self.initialProtocol;
	aBrowser.viewModel.hostName = self.initialHostName;
	aBrowser.viewModel.portNumber = [NSString stringWithFormat:@"%d", self.initialPortNumber];
	aBrowser.viewModel.userID = self.initialUserID;
	
	if (nil == self.containerWindow)
	{
		self.containerWindow = [[Popover_Window alloc] initWithView:aManagedView
																	windowStyle:kPopover_WindowStyleNormal
																	arrowStyle:kPopover_ArrowStyleDefaultRegularSize
																	attachedToPoint:NSZeroPoint/* see delegate */
																	inWindow:self.parentWindow];
		self.containerWindow.delegate = self;
		self.containerWindow.releasedWhenClosed = NO;
		self.popoverMgr = PopoverManager_New(self.containerWindow, aBrowser.view/* first responder; not sure how to handle this with SwiftUI! */,
												self/* delegate */, kPopoverManager_AnimationTypeMinimal,
												kPopoverManager_BehaviorTypeStandard,
												self.parentWindow.contentView);
		PopoverManager_DisplayPopover(_popoverMgr);
	}
}// serverBrowser:didLoadManagedView:


/*!
Responds to a close of the interface by updating
any associated interface elements (such as a button
that opened the popover in the first place).

(4.0)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)		aBrowser
didFinishUsingManagedView:(NSView*)		aManagedView
{
#pragma unused(aBrowser, aManagedView)
	[self.dataObserver serverBrowserDidClose:aBrowser];
}// serverBrowser:didFinishUsingManagedView:


/*!
Requests that the containing window be resized to allow the given
view size.  This is only for odd cases like a user interface
element causing part of the display to appear or disappear;
normally the window can just be manipulated directly.

(4.0)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)	aBrowser
setManagedView:(NSView*)			aManagedView
toScreenFrame:(NSRect)				aRect
{
#pragma unused(aBrowser, aManagedView)
	NSRect		windowFrame = [self.containerWindow frameRectForViewSize:aRect.size];
	
	
	[self.containerWindow setFrame:windowFrame display:YES animate:YES];
	PopoverManager_UseIdealLocationAfterDelay(_popoverMgr, 0.1f/* arbitrary delay */);
}// serverBrowser:setManagedView:toScreenFrame:


@end //} ServerBrowser_Object


#pragma mark -
@implementation ServerBrowser_NetService //{


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithViewModel:(UIServerBrowser_Model*)	aViewModel
netService:(NSNetService*)					aNetService
addressFamily:(unsigned char)				aSocketAddrFamily
{
	self = [super init];
	if (nil != self)
	{
		_addressFamily = aSocketAddrFamily;
		_bestResolvedAddress = @"";
		_bestResolvedPort = 0;
		_netService = aNetService;
		_viewModel = aViewModel;
		self.netService.delegate = self;
		[self.netService resolveWithTimeout:5.0];
	}
	return self;
}// initWithViewModel:netService:addressFamily:


#pragma mark Accessors

/*!
Accessor.

(4.0)
*/
- (NSString*)
description
{
	return self.netService.name;
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
	NSString*		resolvedHost = nil;
	unsigned int	resolvedPort = 0;
	
	
	//Console_WriteLine("service did resolve"); // debug
	for (NSData* addressData in [resolvingService addresses])
	{
		struct sockaddr_in*		dataPtr = (struct sockaddr_in*)[addressData bytes];
		
		
		//Console_WriteValue("found address of family", dataPtr->sin_family); // debug
		if (_addressFamily == dataPtr->sin_family)
		{
			switch (_addressFamily)
			{
			case AF_INET:
			case AF_INET6:
				{
					struct sockaddr_in*		inetDataPtr = REINTERPRET_CAST(dataPtr, struct sockaddr_in*);
					char					buffer[512];
					
					
					if (inet_ntop(_addressFamily, &inetDataPtr->sin_addr, buffer, sizeof(buffer)))
					{
						buffer[sizeof(buffer) - 1] = '\0'; // ensure termination in case of overrun
						resolvedHost = [NSString stringWithCString:buffer encoding:NSASCIIStringEncoding];
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
	if (nil != resolvedHost)
	{
		self.bestResolvedAddress = resolvedHost;
	}
	if (0 != resolvedPort)
	{
		self.bestResolvedPort = STATIC_CAST(resolvedPort, UInt16);
	}
	
	// update the UI now that the service is fully described
	auto	newServiceItem = [[UIServerBrowser_ServiceItemModel alloc]
								init:self.description
										hostName:self.bestResolvedAddress
										portNumber:[[NSNumber numberWithInteger:self.bestResolvedPort] stringValue]];
	self.viewModel.serverArray = [[NSArray arrayWithObject:newServiceItem] arrayByAddingObjectsFromArray:self.viewModel.serverArray];
}// netServiceDidResolveAddress:


@end //} ServerBrowser_NetService


#pragma mark -
@implementation ServerBrowser_Protocol //{


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithID:(Session_Protocol)	anID
description:(NSString*)			aString
serviceType:(NSString*)			anRFC2782Name
defaultPort:(unsigned short)	aNumber
{
	self = [super initWithBoundName:aString];
	if (nil != self)
	{
		_protocolID = anID;
		_serviceType = [anRFC2782Name copy];
		_defaultPort = aNumber;
	}
	return self;
}// initWithID:description:serviceType:defaultPort:


@end //} ServerBrowser_Protocol


#pragma mark -
@implementation ServerBrowser_VC //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithResponder:(id< ServerBrowser_VCDelegate >)		aResponder
dataObserver:(id< ServerBrowser_DataChangeObserver >)	aDataObserver
{
	self = [super initWithNibName:nil bundle:nil];
	if (nil != self)
	{
		_viewModel = [[UIServerBrowser_Model alloc] initWithRunner:self];
		_responder = aResponder;
		_dataObserver = aDataObserver;
		_browser = [[NSNetServiceBrowser alloc] init];
		self.browser.delegate = self;
		_didSetConstraints = NO;
		_discoveredHosts = [[NSMutableArray< ServerBrowser_NetService* > alloc] init];
		// TEMPORARY - it should be possible to externally define these (probably via Python)
		_protocolDefinitions = [[NSArray< ServerBrowser_Protocol* > alloc] initWithObjects:
							#if 0
								// recent versions of macOS no longer support SSH 1
								[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolSSH1
									description:NSLocalizedStringFromTable(@"SSH Version 1", @"ServerBrowser"/* table */, @"ssh-1")
									serviceType:@"_ssh._tcp."
									defaultPort:22],
							#endif
								[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolSSH2
									description:NSLocalizedStringFromTable(@"SSH Version 2", @"ServerBrowser"/* table */, @"ssh-2")
									serviceType:@"_ssh._tcp."
									defaultPort:22],
								[[ServerBrowser_Protocol alloc] initWithID:kSession_ProtocolSFTP
									description:NSLocalizedStringFromTable(@"SFTP", @"ServerBrowser"/* table */, @"sftp")
									serviceType:@"_ssh._tcp."
									defaultPort:22],
								nil];
		
		// the Popover module assumes the first subview of the
		// main view is the basis for the vibrancy effect (which
		// works for Panel instances); for correct behavior,
		// create an intermediate view to serve as parent
		NSView*		childView = [UIServerBrowser_ObjC makeView:self.viewModel];
		NSBox*		parentView = [[NSBox alloc] initWithFrame:NSZeroRect];
		parentView.borderWidth = 0.0;
		parentView.boxType = NSBoxCustom;
		parentView.contentView = childView;
		parentView.title = @"";
		parentView.translatesAutoresizingMaskIntoConstraints = NO;
		childView.translatesAutoresizingMaskIntoConstraints = NO; // constraints are set in "viewDidAppear"
		
		// note: without a XIB file, the view must be set directly
		// and the callback must be invoked here
		self.view = parentView;
		[self.responder serverBrowser:self didLoadManagedView:self.view];
	}
	return self;
}// initWithResponder:dataObserver:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark Initializers Disabled From Superclass


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
#pragma unused(aCoder)
	assert(false && "invalid way to initialize derived class");
	return [super initWithNibName:nil bundle:nil];
}// initWithCoder:


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithNibName:(NSString*)		aNibName
bundle:(NSBundle*)				aBundle
{
#pragma unused(aNibName, aBundle)
	assert(false && "invalid way to initialize derived class");
	return [super initWithNibName:nil bundle:nil];
}// initWithNibName:bundle:


#pragma mark New Methods


/*!
Initiates a search for nearby services (via Bonjour) that
match the currently selected protocol’s service type.

(4.0)
*/
- (void)
rediscoverServices
{
	ServerBrowser_Protocol*		theProtocol = nil;
	
	
	for (ServerBrowser_Protocol* aProtocol in self.protocolDefinitions)
	{
		if (self.viewModel.connectionProtocol == aProtocol.protocolID)
		{
			theProtocol = aProtocol;
			break;
		}
	}
	
	// first destroy the old list
	[self.discoveredHosts removeAllObjects];
	self.viewModel.serverArray = @[];
	
	// now search for new services, which will eventually repopulate the list;
	// only do this when the list is visible, though
	if (self.viewModel.isNearbyServicesListVisible)
	{
		if (nil == theProtocol)
		{
			Console_Warning(Console_WriteLine, "cannot rediscover services because no protocol is yet defined");
		}
		else
		{
			//Console_WriteValueCFString("initiated search for services of type", BRIDGE_CAST([theProtocol serviceType], CFStringRef)); // debug
			[self.browser stop];
			// TEMPORARY - determine if one needs to wait for the browser to stop, before starting a new search...
			[self.browser searchForServicesOfType:theProtocol.serviceType inDomain:@""/* empty string implies local search */];
		}
	}
}// rediscoverServices


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
	[self.browser stop];
	
	// remember the selected host as a recent item
	NSArray*	newArray = [NSArray arrayWithObject:self.viewModel.hostName];
	NSRange		copiedRange = NSMakeRange(0, std::min<NSInteger>(4/* arbitrary */, self.viewModel.recentHostsArray.count));
	self.viewModel.recentHostsArray = [newArray arrayByAddingObjectsFromArray:[self.viewModel.recentHostsArray subarrayWithRange:copiedRange]];
	
	// notify the handler
	[self.responder serverBrowser:self didFinishUsingManagedView:self.view];
}// serverBrowserWindowWillClose:


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
	[self.discoveredHosts addObject:[[ServerBrowser_NetService alloc] initWithViewModel:self.viewModel netService:aNetService addressFamily:AF_INET]];
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


#pragma mark NSViewController


/*!
Called when the view enters its window.  This responds
by observing the window.

(2021.05)
*/
- (void)
viewDidAppear
{
	[super viewDidAppear];
	
	// find out when the window will close, so that the button that opened the window can return to normal
	[self whenObject:self.view.window postsNote:NSWindowWillCloseNotification
						performSelector:@selector(serverBrowserWindowWillClose:)];
	
	if (NO == self.didSetConstraints)
	{
		NSView*		 	constrainedView = self.view;
		assert([self.view isKindOfClass:NSBox.class]);
		NSBox*		 	browserView = STATIC_CAST(self.view, NSBox*).contentView;
		CGFloat const	borderWidth = 5.2; // for constant offsets in constraints below; should match Popover module’s view margin and border
		
		
		assert(nil != constrainedView);
		
		// set constraints so that background effects and
		// show/hide resizing of window are correct
		[NSLayoutConstraint activateConstraints:@[
													[browserView.topAnchor constraintEqualToAnchor:browserView.superview.topAnchor],
													[browserView.heightAnchor constraintEqualToAnchor:browserView.superview.heightAnchor],
													[browserView.superview.bottomAnchor constraintEqualToAnchor:browserView.bottomAnchor],
													[browserView.leadingAnchor constraintEqualToAnchor:browserView.superview.leadingAnchor],
													[browserView.widthAnchor constraintEqualToAnchor:browserView.superview.widthAnchor],
													[browserView.superview.trailingAnchor constraintEqualToAnchor:browserView.trailingAnchor],
													[constrainedView.topAnchor constraintEqualToAnchor:constrainedView.superview.topAnchor constant:borderWidth],
													//[constrainedView.heightAnchor constraintEqualToAnchor:constrainedView.superview.heightAnchor],
													[constrainedView.superview.bottomAnchor constraintEqualToAnchor:constrainedView.bottomAnchor constant:borderWidth],
													[constrainedView.leadingAnchor constraintEqualToAnchor:constrainedView.superview.leadingAnchor constant:borderWidth],
													//[constrainedView.widthAnchor constraintEqualToAnchor:constrainedView.superview.widthAnchor],
													[constrainedView.superview.trailingAnchor constraintEqualToAnchor:constrainedView.trailingAnchor constant:borderWidth],
												]];
		self.didSetConstraints = YES;
	}
}// viewDidAppear


#pragma mark UIServerBrowser_ActionHandling


/*!
Responds to user changes in the Server Browser panel
by notifying the delegate.

(2021.05)
*/
- (void)
dataUpdatedHostName:(NSString*)		aString
{
	[self.dataObserver serverBrowser:self didSetHostName:aString];
} // dataUpdatedHostName:


/*!
Responds to user changes in the Server Browser panel
by notifying the delegate.

(2021.05)
*/
- (void)
dataUpdatedPortNumber:(NSString*)		aString
{
	[self.dataObserver serverBrowser:self didSetPortNumber:[aString integerValue]];
} // dataUpdatedPortNumber:


/*!
Responds to user changes in the Server Browser panel
by notifying the delegate.

(2021.05)
*/
- (void)
dataUpdatedProtocol:(Session_Protocol)		aProtocol
{
	[self.dataObserver serverBrowser:self didSetProtocol:aProtocol];
	// auto-set the port number to match the default for this protocol
	for (ServerBrowser_Protocol* aDefinition in self.protocolDefinitions)
	{
		if (aProtocol == aDefinition.protocolID)
		{
			self.viewModel.portNumber = [NSString stringWithFormat:@"%d", (int)aDefinition.defaultPort];
			self.viewModel.isErrorInPortNumber = NO;
			break;
		}
	}
	// rediscover services appropriate for this selection
	[self rediscoverServices];
} // dataUpdatedProtocol:


/*!
Responds to changes in the visibility of the list
of nearby services.

(2021.05)
*/
- (void)
dataUpdatedServicesListVisible:(BOOL)	aFlag
{
	// note: SwiftUI and constraints take care of the
	// resizing of the view/window so this just has
	// to handle starting/stopping background tasks 
	if (NO == aFlag)
	{
		[self.browser stop];
	}
	else
	{
		[self rediscoverServices];
	}
}// dataUpdatedServicesListVisible:


/*!
Responds to user changes in the Server Browser panel
by notifying the delegate.

(2021.05)
*/
- (void)
dataUpdatedUserID:(NSString*)		aString
{
	[self.dataObserver serverBrowser:self didSetUserID:aString];
} // dataUpdatedUserID:


/*!
Responds to user request to look up the host name.

(2021.05)
*/
- (void)
lookUpSelectedHostNameWithViewModel:(UIServerBrowser_Model*) 	viewModel
{
	if (viewModel.hostName.length <= 0)
	{
		// there has to be some text entered there; let the user
		// know that a blank is unacceptable
		Sound_StandardAlert();
	}
	else
	{
		char	hostNameBuffer[256];
		
		
		// begin lookup of the domain name
		viewModel.isLookupInProgress = YES;
		if (CFStringGetCString(BRIDGE_CAST(viewModel.hostName, CFStringRef), hostNameBuffer, sizeof(hostNameBuffer), kCFStringEncodingASCII))
		{
			DNR_Result		lookupAttemptResult = kDNR_ResultOK;
			
			
			lookupAttemptResult = DNR_New(hostNameBuffer, false/* use IP version 4 addresses (defaults to IPv6) */,
			^(struct hostent* inLookupDataPtr)
			{
				if (nullptr == inLookupDataPtr)
				{
					// lookup failed (TEMPORARY; add error message to user interface?)
					Sound_StandardAlert();
				}
				else
				{
					// NOTE: The lookup data could be a linked list of many matches.
					// The first is used arbitrarily.
					if ((nullptr != inLookupDataPtr->h_addr_list) && (nullptr != inLookupDataPtr->h_addr_list[0]))
					{
						CFStringRef		addressCFString = DNR_CopyResolvedHostAsCFString(inLookupDataPtr, 0/* which address */);
						
						
						if (nullptr != addressCFString)
						{
							viewModel.hostName = BRIDGE_CAST(addressCFString, NSString*);
							viewModel.isErrorInHostName = NO;
							CFRelease(addressCFString), addressCFString = nullptr;
						}
					}
					DNR_Dispose(&inLookupDataPtr);
				}
				
				// hide progress indicator
				viewModel.isLookupInProgress = NO;
			});
			
			if (false == lookupAttemptResult.ok())
			{
				// could not even initiate, so restore UI
				viewModel.isLookupInProgress = NO;
			}
		}
	}
} // lookUpSelectedHostNameWithViewModel:


/*!
Checks the host name value and updates the model’s
error flags if there are any issues.

(2021.05)
*/
- (void)
validateHostNameWithViewModel:(UIServerBrowser_Model*) 		viewModel
{
	// UNIMPLEMENTED; currently no scanning is done (hosts are
	// extremely flexible, e.g. allowing host names, IPv4 or
	// IPv6 addresses, etc. so validation would have to be
	// done carefully)
	viewModel.isErrorInHostName = NO;
} // validateHostNameWithViewModel:


/*!
Checks the port number value and updates the model’s
error flags if there are any issues.

(2021.05)
*/
- (void)
validatePortNumberWithViewModel:(UIServerBrowser_Model*) 	viewModel
{
	// first strip whitespace
	viewModel.portNumber = [viewModel.portNumber stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
	
	if (0 == viewModel.portNumber.length)
	{
		viewModel.isErrorInPortNumber = NO;
	}
	else
	{
		// while an NSNumberFormatter is more typical for validation,
		// the requirements for port numbers are quite simple
		NSScanner*	scanner = [NSScanner scannerWithString:viewModel.portNumber];
		int			value = 0;
		
		
		if ([scanner scanInt:&value] && [scanner isAtEnd] && (value >= 0) && (value <= 65535/* given in TCP/IP spec. */))
		{
			viewModel.isErrorInPortNumber = NO;
		}
		else
		{
			viewModel.isErrorInPortNumber = YES;
		}
	}
} // validatePortNumberWithViewModel:


/*!
Checks the user ID value and updates the model’s
error flags if there are any issues.

(2021.05)
*/
- (void)
validateUserIDWithViewModel:(UIServerBrowser_Model*) 	viewModel
{
	// first strip whitespace
	viewModel.userID = [viewModel.userID stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
	
	NSScanner*				scanner = [NSScanner scannerWithString:viewModel.userID];
	NSMutableCharacterSet*	validCharacters = [[NSCharacterSet alphanumericCharacterSet] mutableCopy];
	NSString*				value = nil;
	
	
	if (0 == viewModel.userID.length)
	{
		viewModel.isErrorInUserID = NO;
	}
	else
	{
		// periods, underscores and hyphens are also valid in Unix user names
		[validCharacters addCharactersInString:@".-_"];
		
		if ([scanner scanCharactersFromSet:validCharacters intoString:&value] && [scanner isAtEnd])
		{
			viewModel.isErrorInUserID = NO;
		}
		else
		{
			viewModel.isErrorInUserID = YES;
		}
	}
} // validateUserIDWithViewModel:


@end //} ServerBrowser_VC

// BELOW IS REQUIRED NEWLINE TO END FILE
