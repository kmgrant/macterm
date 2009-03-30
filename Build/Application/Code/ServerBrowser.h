/*!	\file ServerBrowser.h
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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
Implements the server browser panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface ServerBrowser_PanelController : NSWindowController
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
	NSString*				userID;
	NSString*				errorMessage;
}
// the following MUST match what is in "ServerBrowserCocoa.nib"
+ (id)				sharedServerBrowserPanelController;
// new methods
- (void)			didDoubleClickDiscoveredHostWithSelection:(NSArray*)objects;
- (IBAction)		lookUpHostName:(id)sender;
- (void)			rediscoverServices;
// accessors
- (NSIndexSet*)		discoveredHostIndexes;
- (NSString*)		errorMessage;
- (BOOL)			hidesDiscoveredHosts;
- (BOOL)			hidesErrorMessage;
- (BOOL)			hidesPortNumberError;
- (BOOL)			hidesProgress;
- (BOOL)			hidesUserIDError;
- (NSString*)		hostName;
- (NSString*)		portNumber;
- (NSArray*)		protocolDefinitions;
- (NSIndexSet*)		protocolIndexes;
- (NSString*)		userID;
- (void)			insertObject:(NSString*)name
						inDiscoveredHostsAtIndex:(unsigned long)index;
- (void)			removeObjectFromDiscoveredHostsAtIndex:(unsigned long)index;
- (void)			insertObject:(NSString*)name
						inRecentHostsAtIndex:(unsigned long)index;
- (void)			removeObjectFromRecentHostsAtIndex:(unsigned long)index;
- (void)			setDiscoveredHostIndexes:(NSIndexSet*)indexes; // binding
- (void)			setErrorMessage:(NSString*)aString;
- (void)			setHidesDiscoveredHosts:(BOOL)flag;
- (void)			setHidesErrorMessage:(BOOL)flag;
- (void)			setHidesPortNumberError:(BOOL)flag; // binding
- (void)			setHidesProgress:(BOOL)flag; // binding
- (void)			setHidesUserIDError:(BOOL)flag; // binding
- (void)			setHostName:(NSString*)aString; // binding
- (void)			setPortNumber:(NSString*)aString; // binding
- (void)			setProtocolIndexByProtocol:(Session_Protocol)aProtocol;
- (void)			setProtocolIndexes:(NSIndexSet*)indexes; // binding
- (void)			setUserID:(NSString*)aString; // binding
// validators
- (BOOL)			validatePortNumber:(id*)ioValue
						error:(NSError**)outError;
- (BOOL)			validateUserID:(id*)ioValue
						error:(NSError**)outError;
// NSNetServiceBrowserDelegateMethods
- (void)			netServiceBrowser:(NSNetServiceBrowser*)aNetServiceBrowser
						didFindService:(NSNetService*)aNetService
						moreComing:(BOOL)moreComing;
- (void)			netServiceBrowser:(NSNetServiceBrowser*)aNetServiceBrowser
						didNotSearch:(NSDictionary*)errorInfo;
- (void)			netServiceBrowserDidStopSearch:(NSNetServiceBrowser*)aNetServiceBrowser;
- (void)			netServiceBrowserWillSearch:(NSNetServiceBrowser*)aNetServiceBrowser;
// NSWindowController
- (void)			windowDidLoad;
// NSWindowNotifications
- (void)			windowWillClose:(NSNotification*)notification;
// internal methods
- (void)			notifyOfChangeInValueReturnedBy:(SEL)valueGetter;
- (ServerBrowser_NetService*)	discoveredHost;
- (ServerBrowser_Protocol*)		protocol;
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
