/*!	\file ServerBrowser.h
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTelnet
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

#include "UniversalDefines.h"

#ifndef __SERVERBROWSER__
#define __SERVERBROWSER__

// MacTelnet includes
#include "Session.h"



#pragma mark Types

#ifdef __OBJC__

@class ServerBrowser_NetService;
@class ServerBrowser_Protocol;

/*!
Implements the server browser panel.  See "ServerBrowserCocoa.xib".

This class is KVO-compliant for the following keys:
	hostName
	portNumber
	protocolIndexes
	userID

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface ServerBrowser_PanelController : NSWindowController // NSKeyValueObservingCustomization, NSNetServiceBrowserDelegateMethods, NSWindowNotifications
{
	NSMutableArray*			discoveredHosts; // binding
	NSMutableArray*			recentHosts; // binding
	NSArray*				protocolDefinitions; // binding
	IBOutlet NSTableView*	discoveredHostsTableView;
@private
	NSNetServiceBrowser*	browser;
	NSIndexSet*				discoveredHostIndexes;
	BOOL					hidesDiscoveredHosts;
	BOOL					hidesErrorMessage;
	BOOL					hidesPortNumberError;
	BOOL					hidesProgress;
	BOOL					hidesUserIDError;
	NSIndexSet*				protocolIndexes;
	NSString*				hostName;
	NSString*				portNumber;
	id						target;
	NSString*				userID;
	NSString*				errorMessage;
}

+ (id)
sharedServerBrowserPanelController;

// new methods

- (void)
didDoubleClickDiscoveredHostWithSelection:(NSArray*)_;

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
setTarget:(id)_
protocol:(Session_Protocol)_
hostName:(NSString*)_
portNumber:(unsigned int)_
userID:(NSString*)_;

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

// internal methods

- (ServerBrowser_NetService*)
discoveredHost;

- (void)
notifyOfChangeInValueReturnedBy:(SEL)_;

- (ServerBrowser_Protocol*)
protocol;

@end

#endif



#pragma mark Public Methods

Boolean
	ServerBrowser_IsVisible				();

void
	ServerBrowser_RemoveEventTarget		();

void
	ServerBrowser_SetEventTarget		(EventTargetRef		inTarget,
										 Session_Protocol	inProtocol,
										 CFStringRef		inHostName,
										 UInt16				inPortNumber,
										 CFStringRef		inUserID);

void
	ServerBrowser_SetVisible			(Boolean			inIsVisible);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
