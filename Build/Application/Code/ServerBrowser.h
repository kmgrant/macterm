/*!	\file ServerBrowser.h
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif

// library includes
#ifdef __OBJC__
@class NSWindow;
#else
class NSWindow;
#endif

// application includes
#include "Session.h"



#pragma mark Types

typedef struct ServerBrowser_OpaqueStruct*		ServerBrowser_Ref;

#ifdef __OBJC__

@class ServerBrowser_VC;


/*!
The object that implements this protocol will be told about
changes made to the Server Browser panel’s key properties.
Currently MacTerm responds by composing an equivalent Unix
command line.

Yes, "observeValueForKeyPath:ofObject:change:context:" does
appear to provide the same functionality but that is not
nearly as convenient.
*/
@protocol ServerBrowser_DataChangeObserver //{

@required

	// the user has selected a different connection protocol type
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	didSetProtocol:(Session_Protocol)_;

	// the user has entered a different server host name
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	didSetHostName:(NSString*)_;

	// the user has entered a different server port number
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	didSetPortNumber:(NSUInteger)_;

	// the user has entered a different server log-in ID
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	didSetUserID:(NSString*)_;

@optional

	// the browser has been removed
	- (void)
	serverBrowserDidClose:(ServerBrowser_VC*)_;

@end //}


/*!
Classes that are delegates of ServerBrowser_VC
must conform to this protocol.
*/
@protocol ServerBrowser_VCDelegate //{

	// use this opportunity to create and display a window to wrap the view
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	didLoadManagedView:(NSView*)_;

	// when the view is going away, perform any final updates
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	didFinishUsingManagedView:(NSView*)_;

	// user interface has hidden or displayed something that requires the view size to change
	- (void)
	serverBrowser:(ServerBrowser_VC*)_
	setManagedView:(NSView*)_
	toScreenFrame:(NSRect)_;

@end //}


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
@interface ServerBrowser_VC : NSViewController< NSNetServiceBrowserDelegate > //{
{
	IBOutlet NSView*		discoveredHostsContainer;
	IBOutlet NSTableView*	discoveredHostsTableView;
	IBOutlet NSView*		logicalFirstResponder;
	IBOutlet NSResponder*	nextResponderWhenHidingDiscoveredHosts;
@private
	id< ServerBrowser_VCDelegate >			_responder;
	id< ServerBrowser_DataChangeObserver >	_dataObserver;
	NSNetServiceBrowser*					_browser;
	NSIndexSet*								_discoveredHostIndexes;
	NSIndexSet*								_protocolIndexes;
	NSMutableArray*							_discoveredHosts; // binding
	NSMutableArray*							_recentHosts; // binding
	NSArray*								_protocolDefinitions; // binding
	NSString*								_errorMessage;
	NSString*								_hostName;
	NSString*								_portNumber;
	NSString*								_userID;
	id										_target;
	BOOL									_hidesDiscoveredHosts;
	BOOL									_hidesErrorMessage;
	BOOL									_hidesPortNumberError;
	BOOL									_hidesProgress;
	BOOL									_hidesUserIDError;
}

// initializers
	- (instancetype)
	initWithCoder:(NSCoder*)_ DISABLED_SUPERCLASS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithNibName:(NSString*)_
	bundle:(NSBundle*)_ DISABLED_SUPERCLASS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithResponder:(id< ServerBrowser_VCDelegate >)_
	dataObserver:(id< ServerBrowser_DataChangeObserver >)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (NSView*)
	logicalFirstResponder;
	- (IBAction)
	lookUpHostName:(id)_;
	- (void)
	rediscoverServices;

// accessors: array values
	- (void)
	insertObject:(ServerBrowser_NetService*)_
	inDiscoveredHostsAtIndex:(unsigned long)_;
	- (void)
	removeObjectFromDiscoveredHostsAtIndex:(unsigned long)_;
	@property (retain) NSIndexSet*
	discoveredHostIndexes; // binding
	@property (retain, readonly) NSArray*
	protocolDefinitions;
	@property (retain) NSIndexSet*
	protocolIndexes; // binding
	- (void)
	setProtocolIndexByProtocol:(Session_Protocol)_;
	- (void)
	insertObject:(NSString*)_
	inRecentHostsAtIndex:(unsigned long)_;
	- (void)
	removeObjectFromRecentHostsAtIndex:(unsigned long)_;

// accessors: general
	- (Session_Protocol)
	currentProtocolID;
	@property (retain) NSString*
	errorMessage;
	@property (copy) NSString*
	hostName; // binding
	@property (copy) NSString*
	portNumber; // binding
	@property (retain) id
	target;
	@property (copy) NSString*
	userID; // binding

// accessors: low-level user interface state
	@property (assign) BOOL
	hidesDiscoveredHosts;
	@property (assign) BOOL
	hidesErrorMessage; // it is better to set a specific property such as "hidesPortNumberError"
	@property (assign) BOOL
	hidesPortNumberError; // binding
	@property (assign) BOOL
	hidesProgress; // binding
	@property (assign) BOOL
	hidesUserIDError; // binding

// validators
	- (BOOL)
	validatePortNumber:(id*)_
	error:(NSError**)_;
	- (BOOL)
	validateUserID:(id*)_
	error:(NSError**)_;

// NSViewController
	- (void)
	loadView;

@end //}

#endif



#pragma mark Public Methods

#ifdef __OBJC__
ServerBrowser_Ref
	ServerBrowser_New			(NSWindow*				inParentWindow,
								 CGPoint				inParentRelativePoint,
								 id< ServerBrowser_DataChangeObserver >		inDataObserver);
#endif

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

// BELOW IS REQUIRED NEWLINE TO END FILE
