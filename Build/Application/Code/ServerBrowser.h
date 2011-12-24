/*!	\file ServerBrowser.h
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

#include <UniversalDefines.h>

#ifndef __SERVERBROWSER__
#define __SERVERBROWSER__

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif

// library includes
#ifdef __OBJC__
@class CarbonEventHandlerWrap;
@class NSWindow;
#else
class CarbonEventHandlerWrap;
class NSWindow;
#endif

// application includes
#include "Session.h"



#pragma mark Types

typedef struct ServerBrowser_OpaqueStruct*		ServerBrowser_Ref;

#ifdef __OBJC__

@class ServerBrowser_ViewManager;

@protocol ServerBrowser_ViewManagerChannel

// use this opportunity to create and display a window to wrap the view
- (void)
serverBrowser:(ServerBrowser_ViewManager*)_
didLoadManagedView:(NSView*)_;

// when the view is going away, perform any final updates
- (void)
serverBrowser:(ServerBrowser_ViewManager*)_
didFinishUsingManagedView:(NSView*)_;

// user interface has hidden or displayed something that requires the view size to change
- (void)
serverBrowser:(ServerBrowser_ViewManager*)_
setManagedView:(NSView*)_
toScreenFrame:(NSRect)_;

@end // ServerBrowser_ViewManagerChannel


@class ServerBrowser_NetService;
@class ServerBrowser_Protocol;

/*!
Implements the server browser.  See "ServerBrowserCocoa.xib".

This class is KVO-compliant for the following keys:
	hostName
	portNumber
	protocolIndexes
	userID

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface ServerBrowser_ViewManager : NSObject
{
	NSMutableArray*			discoveredHosts; // binding
	NSMutableArray*			recentHosts; // binding
	NSArray*				protocolDefinitions; // binding
	IBOutlet NSView*		discoveredHostsContainer;
	IBOutlet NSTableView*	discoveredHostsTableView;
	IBOutlet NSView*		managedView;
	IBOutlet NSView*		logicalFirstResponder;
	IBOutlet NSResponder*	nextResponderWhenHidingDiscoveredHosts;
@private
	id< ServerBrowser_ViewManagerChannel >	responder;
	EventTargetRef							eventTarget;
	CarbonEventHandlerWrap*					lookupHandlerPtr;
	NSNetServiceBrowser*					browser;
	NSIndexSet*								discoveredHostIndexes;
	BOOL									hidesDiscoveredHosts;
	BOOL									hidesErrorMessage;
	BOOL									hidesPortNumberError;
	BOOL									hidesProgress;
	BOOL									hidesUserIDError;
	NSIndexSet*								protocolIndexes;
	NSString*								hostName;
	NSString*								portNumber;
	id										target;
	NSString*								userID;
	NSString*								errorMessage;
}

// initializers

- (id)
initWithResponder:(id< ServerBrowser_ViewManagerChannel >)_
eventTarget:(EventTargetRef)_;

// new methods

- (NSView*)
logicalFirstResponder;

- (IBAction)
lookUpHostName:(id)_;

- (void)
rediscoverServices;

// accessors

- (Session_Protocol)
currentProtocolID;

- (void)
insertObject:(ServerBrowser_NetService*)_
inDiscoveredHostsAtIndex:(unsigned long)_;
- (void)
removeObjectFromDiscoveredHostsAtIndex:(unsigned long)_;

- (NSIndexSet*)
discoveredHostIndexes;
- (void)
setDiscoveredHostIndexes:(NSIndexSet*)_; // binding

- (NSString*)
errorMessage;
- (void)
setErrorMessage:(NSString*)_;

- (BOOL)
hidesDiscoveredHosts;
- (void)
setHidesDiscoveredHosts:(BOOL)_;

- (BOOL)
hidesErrorMessage;
- (void)
setHidesErrorMessage:(BOOL)_;

- (BOOL)
hidesPortNumberError;
- (void)
setHidesPortNumberError:(BOOL)_; // binding

- (BOOL)
hidesProgress;
- (void)
setHidesProgress:(BOOL)_; // binding

- (BOOL)
hidesUserIDError;
- (void)
setHidesUserIDError:(BOOL)_; // binding

- (NSString*)
hostName;
- (void)
setHostName:(NSString*)_; // binding

- (NSString*)
portNumber;
- (void)
setPortNumber:(NSString*)_; // binding

- (NSArray*)
protocolDefinitions;

- (NSIndexSet*)
protocolIndexes;
- (void)
setProtocolIndexByProtocol:(Session_Protocol)_;
- (void)
setProtocolIndexes:(NSIndexSet*)_; // binding

- (void)
insertObject:(NSString*)_
inRecentHostsAtIndex:(unsigned long)_;
- (void)
removeObjectFromRecentHostsAtIndex:(unsigned long)_;

- (id)
target;
- (void)
setTarget:(id)_;

- (NSString*)
userID;
- (void)
setUserID:(NSString*)_; // binding

// validators

- (BOOL)
validatePortNumber:(id*)_
error:(NSError**)_;

- (BOOL)
validateUserID:(id*)_
error:(NSError**)_;

@end

#endif



#pragma mark Public Methods

ServerBrowser_Ref
	ServerBrowser_New			(HIWindowRef			inParentWindow,
								 CGPoint				inParentRelativePoint,
								 EventTargetRef			inResponder);

void
	ServerBrowser_Dispose		(ServerBrowser_Ref*		inoutDialogPtr);

void
	ServerBrowser_Configure		(ServerBrowser_Ref		inDialog,
								 Session_Protocol		inProtocol,
								 CFStringRef			inHostName,
								 UInt16					inPortNumber,
								 CFStringRef			inUserID);

void
	ServerBrowser_Display		(ServerBrowser_Ref		inDialog);

void
	ServerBrowser_Remove		(ServerBrowser_Ref		inDialog);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
