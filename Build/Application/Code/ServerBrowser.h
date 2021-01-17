/*!	\file ServerBrowser.h
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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
#else
class NSWindow;
#endif

// library includes
#ifdef __OBJC__
#	import <PopoverManager.objc++.h>
#endif

// application includes
#include "Session.h"



#pragma mark Types

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
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetProtocol:(Session_Protocol)_;

	// the user has entered a different server host name
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetHostName:(NSString* _Nonnull)_;

	// the user has entered a different server port number
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetPortNumber:(NSUInteger)_;

	// the user has entered a different server log-in ID
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetUserID:(NSString* _Nonnull)_;

@optional

	// the browser has been removed
	- (void)
	serverBrowserDidClose:(ServerBrowser_VC* _Nonnull)_;

@end //}


/*!
Classes that are delegates of ServerBrowser_VC
must conform to this protocol.
*/
@protocol ServerBrowser_VCDelegate //{

	// use this opportunity to create and display a window to wrap the view
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didLoadManagedView:(NSView* _Nonnull)_;

	// when the view is going away, perform any final updates
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didFinishUsingManagedView:(NSView* _Nonnull)_;

	// user interface has hidden or displayed something that requires the view size to change
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	setManagedView:(NSView* _Nonnull)_
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

// initializers
	- (instancetype _Nullable)
	initWithResponder:(id< ServerBrowser_VCDelegate > _Nullable)_
	dataObserver:(id< ServerBrowser_DataChangeObserver > _Nullable)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (IBAction)
	lookUpHostName:(id _Nullable)_;
	- (void)
	rediscoverServices;

// accessors: outlets
	@property (strong, nonnull) IBOutlet NSView*
	discoveredHostsContainer;
	@property (strong, nonnull) IBOutlet NSTableView*
	discoveredHostsTableView;
	//! The view that a window ought to focus first using
	//! NSWindow’s "makeFirstResponder:".
	@property (strong, nonnull) IBOutlet NSView*
	logicalFirstResponder;
	@property (strong, nonnull) IBOutlet NSResponder*
	nextResponderWhenHidingDiscoveredHosts;

// accessors: array values
	- (void)
	insertObject:(ServerBrowser_NetService* _Nonnull)_
	inDiscoveredHostsAtIndex:(unsigned long)_;
	- (void)
	removeObjectFromDiscoveredHostsAtIndex:(unsigned long)_;
	@property (strong, nonnull) NSIndexSet*
	discoveredHostIndexes; // binding
	@property (strong, nonnull, readonly) NSArray*
	protocolDefinitions;
	@property (strong, nonnull) NSIndexSet*
	protocolIndexes; // binding
	- (void)
	setProtocolIndexByProtocol:(Session_Protocol)_;
	- (void)
	insertObject:(NSString* _Nonnull)_
	inRecentHostsAtIndex:(unsigned long)_;
	- (void)
	removeObjectFromRecentHostsAtIndex:(unsigned long)_;

// accessors: general
	- (Session_Protocol)
	currentProtocolID;
	@property (strong, nonnull) NSString*
	errorMessage;
	@property (strong, nonnull) NSString*
	hostName; // binding
	@property (strong, nonnull) NSString*
	portNumber; // binding
	@property (strong, nonnull) id
	target;
	@property (strong, nonnull) NSString*
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
	validatePortNumber:(id _Nonnull* _Nonnull)_
	error:(NSError* _Nonnull* _Nonnull)_;
	- (BOOL)
	validateUserID:(id _Nonnull* _Nonnull)_
	error:(NSError* _Nonnull* _Nonnull)_;

// NSViewController
	- (void)
	loadView;

@end //}


/*!
Manages the Server Browser user interface.
*/
@interface ServerBrowser_Object : NSObject< NSWindowDelegate, PopoverManager_Delegate, ServerBrowser_VCDelegate > @end

#else

@class ServerBrowser_Object;

#endif // __OBJC__

// This is defined as an Objective-C object so it is compatible
// with ARC rules (e.g. strong references).
typedef ServerBrowser_Object*	ServerBrowser_Ref;



#pragma mark Public Methods

#ifdef __OBJC__
ServerBrowser_Ref _Nullable
	ServerBrowser_New			(NSWindow* _Nonnull				inParentWindow,
								 CGPoint						inParentRelativePoint,
								 id< ServerBrowser_DataChangeObserver > _Nullable	inDataObserver);
#endif

void
	ServerBrowser_Configure		(ServerBrowser_Ref _Nonnull		inDialog,
								 Session_Protocol				inProtocol,
								 CFStringRef _Nullable			inHostName,
								 UInt16							inPortNumber,
								 CFStringRef _Nullable			inUserID);

void
	ServerBrowser_Display		(ServerBrowser_Ref _Nonnull		inDialog);

void
	ServerBrowser_Remove		(ServerBrowser_Ref _Nonnull		inDialog);

// BELOW IS REQUIRED NEWLINE TO END FILE
