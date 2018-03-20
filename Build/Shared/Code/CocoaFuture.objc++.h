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
	© 2008-2018 by Kevin Grant
	
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

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1060 /* MAC_OS_X_VERSION_10_6 */
#	define NSAppKitVersionNumber10_6 1038
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1070 /* MAC_OS_X_VERSION_10_7 */
#	define NSAppKitVersionNumber10_7 1138
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1080 /* MAC_OS_X_VERSION_10_8 */
#	define NSAppKitVersionNumber10_8 1187
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1090 /* MAC_OS_X_VERSION_10_9 */
#	define NSAppKitVersionNumber10_9 1265
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 101000 /* MAC_OS_X_VERSION_10_10 */
#	define NSAppKitVersionNumber10_10 1343
#	ifndef NS_DESIGNATED_INITIALIZER
//		NS_DESIGNATED_INITIALIZER was only added in the 10.10 SDK
//		but it is useful as a marker in earlier versions of code
#		define NS_DESIGNATED_INITIALIZER
#	endif
#endif

// this should be used in the same place that you might otherwise
// find NS_DESIGNATED_INITIALIZER, to show initializers that are
// NOT supported by the subclass (e.g. runtime error); if a class
// does this, it should define some new designated initializer
#define DISABLED_SUPERCLASS_DESIGNATED_INITIALIZER

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 101100 /* MAC_OS_X_VERSION_10_11 */
#	define NSAppKitVersionNumber10_11 1404
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 101200 /* MAC_OS_X_VERSION_10_12 */
#	define NSAppKitVersionNumber10_12 1504
#endif



#pragma mark Types

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070 /* MAC_OS_X_VERSION_10_7 */

// Things that are implemented ONLY on OS 10.7 and beyond.
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


#if MAC_OS_X_VERSION_MAX_ALLOWED < 1080 /* MAC_OS_X_VERSION_10_8 */

// Things that are implemented ONLY on OS 10.8 and beyond.
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

#endif


#if MAC_OS_X_VERSION_MAX_ALLOWED < 101000 /* MAC_OS_X_VERSION_10_10 */

enum
{
	NSWindowTitleVisible	= 0,
	NSWindowTitleHidden		= 1
};
typedef int/*NSInteger*/	NSWindowTitleVisibility;

// Things that are implemented ONLY on OS 10.10 and beyond.
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

@interface NSWindow (NSWindowExtensionsFromYosemite) //{

// new methods
	- (void)
	setTitleVisibility:(NSWindowTitleVisibility)_;

@end //}


#endif // MAC_OS_X_VERSION_10_10


#if MAC_OS_X_VERSION_MAX_ALLOWED < 101200 /* MAC_OS_X_VERSION_10_12 */

// Things that are implemented ONLY on OS 10.12 and beyond.
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

@interface NSWindow (NSWindowExtensionsFromSierra) //{

// new methods
	- (void)
	addTabbedWindow:(NSWindow*)_
	ordered:(NSWindowOrderingMode)_;
	- (void)
	setAllowsAutomaticWindowTabbing:(BOOL)_;
	- (NSArray*/* of NSWindow* */)
	tabbedWindows;

@end //}


#endif // MAC_OS_X_VERSION_10_12


#if MAC_OS_X_VERSION_MAX_ALLOWED < 101201 /* MAC_OS_X_VERSION_10_12_1 */

// Things that are implemented ONLY on OS 10.12.1 and beyond.
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

@class NSColorPickerTouchBarItem;
@class NSCustomTouchBarItem;
@class NSTouchBar;
@class NSTouchBarItem;

typedef NSString* NSTouchBarCustomizationIdentifier;
typedef NSString* NSTouchBarItemIdentifier;


@interface NSApplication (NSApplicationExtensionsFromSierra) //{

// new methods
	- (IBAction)
	toggleTouchBarCustomizationPalette:(id)_;

@end //}


@interface NSResponder (NSResponderExtensionsFromSierra) //{

// new methods
	- (void)
	setTouchBar:(NSTouchBar*)_;
	- (NSTouchBar*)
	touchBar;

@end //}


#endif // MAC_OS_X_VERSION_10_12_1



#pragma mark Public Methods

id// NSUserNotification*
	CocoaFuture_AllocInitUserNotification				();

id// NSVisualEffectView*
	CocoaFuture_AllocInitVisualEffectViewWithFrame		(CGRect);

id// NSXPCConnection*
	CocoaFuture_AllocInitXPCConnectionWithServiceName	(NSString*);

id// NSUserNotificationCenter*
	CocoaFuture_DefaultUserNotificationCenter			();

void
	CocoaFuture_TouchBarSetCustomizationIdentifier		(NSTouchBar*,
														 NSTouchBarCustomizationIdentifier);

void
	CocoaFuture_TouchBarSetCustomizationAllowedItemIdentifiers	(NSTouchBar*,
														 NSArray*);

id
	CocoaFuture_XPCConnectionRemoteObjectProxy			(NSXPCConnection*,
														 void (^)(NSError*));

void
	CocoaFuture_XPCConnectionResume						(NSXPCConnection*);

void
	CocoaFuture_XPCConnectionSetInterruptionHandler		(NSXPCConnection*,
														 void (^)());

void
	CocoaFuture_XPCConnectionSetInvalidationHandler		(NSXPCConnection*,
														 void (^)());

void
	CocoaFuture_XPCConnectionSetRemoteObjectInterface	(NSXPCConnection*,
														 NSXPCInterface*);

id// NSXPCInterface*
	CocoaFuture_XPCInterfaceWithProtocol				(Protocol*);

// BELOW IS REQUIRED NEWLINE TO END FILE
