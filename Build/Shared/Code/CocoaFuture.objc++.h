/*!	\file CocoaFuture.objc++.h
	\brief Declarations for certain constants and methods
	available only on newer OSes, to aid compilations for
	older OSes.
	
	It is very important that these be based on the exact
	definitions in later SDKs.  Symbols should be added
	only if they are already set, e.g. a constant whose
	value is now established in old OS versions or a
	selector that is already implemented for a class.
	
	You MUST use "respondsToSelector:" or an equivalent
	mechanism to guard against use of these methods on
	older OSes.  The advantage of importing this file is
	that you can directly invoke the target method (in
	if-statements, say) without seeing compiler warnings.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2015 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Constants

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
#define NSAppKitVersionNumber10_4 824
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
#define NSAppKitVersionNumber10_5 949
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED <= 1060 /* MAC_OS_X_VERSION_10_6 */
#define NSAppKitVersionNumber10_6 1038
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED <= 1070 /* MAC_OS_X_VERSION_10_7 */
#define NSAppKitVersionNumber10_7 1138
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED <= 1080 /* MAC_OS_X_VERSION_10_8 */
#define NSAppKitVersionNumber10_8 1187
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED <= 1090 /* MAC_OS_X_VERSION_10_9 */

#define NSAppKitVersionNumber10_9 1265

// NS_DESIGNATED_INITIALIZER was only added in the 10.10 SDK
// but it is useful as a marker in earlier versions of code
#ifndef NS_DESIGNATED_INITIALIZER
#	define NS_DESIGNATED_INITIALIZER
#endif

#endif



#pragma mark Types

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070 /* MAC_OS_X_VERSION_10_7 */

// Things that are implemented ONLY on Mac OS X 10.7 and beyond.
// These declarations should match the latest SDK.
//
// WARNING:	You MUST use "respondsToSelector:" or an equivalent
//			mechanism to guard against use of these methods on
//			older OSes.  The advantage of importing this file
//			is that you can directly invoke the target method
//			(in an if-statement, say) without seeing compiler
//			warnings.  Note that "performSelector:" is also an
//			option, but that is much more cumbersome for APIs
//			that take or return non-objects.

enum
{
	NSScrollerStyleLegacy	= 0,
	NSScrollerStyleOverlay	= 1
};
typedef int/*NSInteger*/	NSScrollerStyle;


@interface NSObject (NSObjectExtensionsFromLion) //{

// new methods
	- (void)
	removeObserver:(NSObject*)_
	forKeyPath:(NSString*)_
	context:(void*)_;

@end //}


@interface NSResponder (NSResponderExtensionsFromLion) //{

	- (void)
	invalidateRestorableState;

@end //}


@interface NSScroller (NSScrollerExtensionsFromLion) //{

// class methods
	+ (NSScrollerStyle)
	preferredScrollerStyle;

// accessors
	- (NSScrollerStyle)
	scrollerStyle;

@end //}


@interface NSWindow (NSWindowExtensionsFromLion) //{

// new methods
	- (void)
	setAnimationBehavior:(int/*NSInteger*//*NSWindowAnimationBehavior*/)_;
	- (void)
	toggleFullScreen:(id)_;

@end //}


#endif // MAC_OS_X_VERSION_10_7


#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */

// Things that are implemented ONLY on Mac OS X 10.8 and beyond.
// These declarations should match the latest SDK.
//
// WARNING:	You MUST use "respondsToSelector:" or an equivalent
//			mechanism to guard against use of these methods on
//			older OSes.  The advantage of importing this file
//			is that you can directly invoke the target method
//			(in an if-statement, say) without seeing compiler
//			warnings.  Note that "performSelector:" is also an
//			option, but that is much more cumbersome for APIs
//			that take or return non-objects.

@class NSUserNotification;
@class NSUserNotificationCenter;
@class NSXPCConnection;
@class NSXPCInterface;

@protocol NSUserNotificationCenterDelegate < NSObject > //{

// NOTE: Technically all methods are optional but older versions of
// the Objective-C language do not have this concept.  To ensure
// that the system sees an object “conforming” to the protocol it
// is unfortunately necessary to implement ALL methods regardless.

	- (void)
	userNotificationCenter:(NSUserNotificationCenter*)_
	didDeliverNotification:(NSUserNotification*)_;

	- (void)
	userNotificationCenter:(NSUserNotificationCenter*)_
	didActivateNotification:(NSUserNotification*)_;

	- (BOOL)
	userNotificationCenter:(NSUserNotificationCenter*)_
	shouldPresentNotification:(NSUserNotification*)_;

@end //}


id// NSUserNotification*
	CocoaFuture_AllocInitUserNotification				();

id// NSXPCConnection*
	CocoaFuture_AllocInitXPCConnectionWithServiceName	(NSString*			inName);

id// NSUserNotificationCenter*
	CocoaFuture_DefaultUserNotificationCenter			();

id
	CocoaFuture_XPCConnectionRemoteObjectProxy			(NSXPCConnection*	inConnection,
														 void (^inHandler)(NSError*));

void
	CocoaFuture_XPCConnectionResume						(NSXPCConnection*	inConnection);

void
	CocoaFuture_XPCConnectionSetInterruptionHandler		(NSXPCConnection*	inConnection,
														 void				(^inBlock)());

void
	CocoaFuture_XPCConnectionSetInvalidationHandler		(NSXPCConnection*	inConnection,
														 void				(^inBlock)());

void
	CocoaFuture_XPCConnectionSetRemoteObjectInterface	(NSXPCConnection*	inConnection,
														 NSXPCInterface*	inInterface);

id// NSXPCInterface*
	CocoaFuture_XPCInterfaceWithProtocol				(Protocol*			inProtocol);

#else

// this functionality is available if compiling against a later OS version
// but the routines are implemented anyway to ease porting efforts (this
// also serves as documentation for those using the alternates above, as
// it shows what the emulated versions are trying to achieve)

id// NSUserNotification*
	CocoaFuture_AllocInitUserNotification		()	{ return [[NSUserNotification alloc] init]; }

id// NSXPCConnection*
	CocoaFuture_AllocInitXPCConnectionWithServiceName	(NSString*		inName)
	{
		return [[NSXPCConnection alloc] initWithServiceName:inName];
	}

id// NSUserNotificationCenter*
	CocoaFuture_DefaultUserNotificationCenter	()	{ return [NSUserNotificationCenter defaultUserNotificationCenter]; }

void
	CocoaFuture_XPCConnectionResume				(NSXPCConnection*		inConnection)
	{
		[inConnection resume];
	}

void
	CocoaFuture_XPCConnectionSetRemoteObjectInterface	(NSXPCConnection*	inConnection,
														 NSXPCInterface*	inInterface)
	{
		inConnection.remoteObjectInterface = inInterface;
	}

void
	CocoaFuture_XPCConnectionSetInterruptionHandler		(NSXPCConnection*	inConnection,
														 void				(^inBlock)())
	{
		inConnection.interruptionHandler = inBlock;
	}

void
	CocoaFuture_XPCConnectionSetInvalidationHandler		(NSXPCConnection*	inConnection,
														 void				(^inBlock)())
	{
		inConnection.invalidationHandler = inBlock;
	}

id// NSXPCInterface*
	CocoaFuture_XPCInterfaceWithProtocol		(Protocol*			inProtocol)
	{
		return [NSXPCInterface interfaceWithProtocol:inProtocol];
	}

#endif // MAC_OS_X_VERSION_10_8

// BELOW IS REQUIRED NEWLINE TO END FILE
