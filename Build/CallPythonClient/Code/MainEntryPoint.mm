/*!	\file MainEntryPoint.mm
	\brief Front end to the Call Python Client application.
*/
/*###############################################################

	Call Python Client
		© 2015-2021 by Kevin Grant.
	
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

#import "UniversalDefines.h"

// standard-C++ includes
#import <iostream>

// Mac includes
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>

// library includes
#import "XPCCallPythonClient.objc++.h"



#pragma mark Internal Method Prototypes

/*!
NSApplication subclass.  (It appears the delegate methods are
never called unless NSApplication is subclassed.)
*/
@interface CallPythonClientApp : NSApplication //{

// initializers
	- (instancetype)
	init;

@end //}


/*!
The delegate for NSApplication.
*/
@interface CallPythonClientAppDelegate : NSObject <NSApplicationDelegate> //{

// NSApplicationDelegate
	- (void)
	applicationDidFinishLaunching:(NSNotification*)_;

@end //}


/*!
The delegate for the XPC service.
*/
@interface XPCServiceDelegate : NSObject <NSXPCListenerDelegate> //{

// NSXPCListenerDelegate
	// (undeclared)

@end //}


/*!
The implementation of the service protocol; remote connections
reach the service through these methods.
*/
@interface XPCServiceImplementation : NSObject <XPCCallPythonClient_RemoteObjectInterface> //{

// XPCCallPythonClient_RemoteObjectInterface
	// (undeclared)

@end //}



#pragma mark Public Methods

/*!
Main entry point.
*/
int
main	(int			argc,
		 const char*	argv[])
{
	NSLog(@"service started");
	
	return NSApplicationMain(argc, argv);
}


#pragma mark Internal Methods

@implementation CallPythonClientApp //{


#pragma mark Initializers


- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
	}
	return self;
}


@end //}


@implementation CallPythonClientAppDelegate //{


#pragma mark NSApplicationDelegate

- (void)
applicationDidFinishLaunching:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	
	XPCServiceDelegate*		delegate = [[XPCServiceDelegate alloc] init];
	NSXPCListener*			listener = [NSXPCListener serviceListener];
	
	
	// bring this process to the front
	[NSApp activateIgnoringOtherApps:YES];
	
	listener.delegate = delegate;
	NSLog(@"call-Python client active using listener %@ and delegate %@", listener, delegate);
	[listener resume]; // DOES NOT RETURN
}// applicationDidFinishLaunching:


#pragma mark NSNibLoading


- (void)
awakeFromNib
{
	[super awakeFromNib];
	
	NSLog(@"loaded application delegate object");
}


@end //} CallPythonClientAppDelegate


@implementation XPCServiceDelegate //{


#pragma mark NSXPCListenerDelegate

/*!
Implements the protocol by setting the exported interface and object.

Returns YES if the connection should be accepted, otherwise the
connection is invalidated.

(4.1)
*/
- (BOOL)
listener:(NSXPCListener*)						listener
shouldAcceptNewConnection:(NSXPCConnection*)	newConnection
{
#pragma unused(listener)
	
	newConnection.exportedInterface = [NSXPCInterface interfaceWithProtocol:@protocol(XPCCallPythonClient_RemoteObjectInterface)];
	
	// note: connection retains the exported object
	newConnection.exportedObject = [[XPCServiceImplementation alloc] init];
	
	NSLog(@"accepted connection, created handler object %@ with interface %@",
			newConnection.exportedObject, newConnection.exportedInterface);
	
	// listen for other connections
	[newConnection resume];
	
	// note: if NO is returned, call "invalidate" on the connection first
	return YES;
}


@end //}


@implementation XPCServiceImplementation //{


#pragma mark XPCCallPythonClient_RemoteObjectInterface


- (void)
xpcServiceSendMessage:(NSString*)			aMessage
withReply:(XPCCallPythonClient_ReplyBlock)	aReplyBlock
{
	NSLog(@"service received message: '%@'", aMessage);
	aReplyBlock([NSString stringWithFormat:@"yes, I received your “%@”", aMessage]);
}


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
