/*!	\file CocoaFuture.mm
	\brief Methods available only in newer OS versions.
	
	Most symbols are only declared, to ease compilation.
	But this file exists in case it is necessary to offer
	implementations (using different names) of certain
	functionality on older OSes.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2017 by Kevin Grant
	
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

#import <CocoaFuture.objc++.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <Console.h>



#pragma mark Types

/*!
This protocol exists in order to suppress warnings about
undeclared selectors used in "@selector()" calls below.
There is code to dynamically check for selectors that are
only available in the 10.8 SDK for XPC.
*/
@protocol CocoaFuture_LikeMountainLionXPC //{

	// 10.8: selector to return autoreleased NSUserNotificationCenter* from NSUserNotificationCenter class
	+ (NSUserNotificationCenter*)
	defaultUserNotificationCenter;

	// 10.8: selector to initialize an NSXPCConnection
	- (id)
	initWithServiceName:(NSString*)_;

	// 10.8: selector to return autoreleased NSXPCInterface* from NSXPCInterface class
	+ (NSXPCInterface*)
	interfaceWithProtocol:(Protocol*)_;

	// 10.8: selector to return a proxy for an NSXPCConnection
	- (id)
	remoteObjectProxyWithErrorHandler:(void (^)(NSError*))_;

	// 10.8: selector to set invalidation handler for an NSXPCConnection
	- (id)
	setInvalidationHandler:(void (^)())_;

	// 10.8: selector to set interruption handler for an NSXPCConnection
	- (id)
	setInterruptionHandler:(void (^)())_;

	// 10.8: selector to return remote object interface for an NSXPCConnection
	- (void)
	setRemoteObjectInterface:(NSXPCInterface*)_;

@end //}


/*!
This protocol exists in order to suppress warnings about
undeclared selectors used in "@selector()" calls below.
There is code to dynamically check for selectors that are
only available in the 10.12.1 SDK for NSTouchBar.
*/
@protocol CocoaFuture_LikeSierraTouchBar //{

	// 10.12.1: selector to set array of allowed item identifiers for customization of an NSTouchBar
	- (void)
	setCustomizationAllowedItemIdentifiers:(NSArray*)_;

	// 10.12.1: selector to set array of default item identifiers for customization of an NSTouchBar
	- (void)
	setCustomizationDefaultItemIdentifiers:(NSArray*)_;

	// 10.12.1: selector to set customization identifier for an NSTouchBar
	- (void)
	setCustomizationIdentifier:(NSTouchBarCustomizationIdentifier)_;

	// 10.12.1: selector to set customization label for an NSCustomTouchBarItem
	- (void)
	setCustomizationLabel:(NSString*)_;

	// 10.12.1: selector to set array of default item identifiers for an NSTouchBar
	- (void)
	setDefaultItemIdentifiers:(NSArray*)_;

	// 10.12.1: selector to set view controller for an NSCustomTouchBarItem
	- (void)
	setViewController:(NSViewController*)_;

@end //}


#pragma mark Public Methods


/*!
On OS 10.8 and later, "[[NSUserNotification alloc] init]"; on
earlier OS versions, "nil".

This work-around is necessary because older code bases will not
be able to directly refer to the class (it doesn’t exist).

(1.10)
*/
id
CocoaFuture_AllocInitUserNotification ()
{
	id		result = nil;
	id		targetClass = objc_getClass("NSUserNotification");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSUserNotification class");
	}
	else
	{
		SEL		allocSelector = @selector(alloc);
		SEL		initSelector = @selector(init);
		
		
		if ([targetClass respondsToSelector:allocSelector] &&
			[targetClass instancesRespondToSelector:initSelector])
		{
			id		allocatedInstance = [targetClass performSelector:allocSelector];
			
			
			if (nil != allocatedInstance)
			{
				result = [allocatedInstance performSelector:initSelector];
			}
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSUserNotification found but it does not implement 'alloc' and/or 'init'");
		}
	}
	
	return result;
}// AllocInitUserNotification


/*!
On OS 10.10 and later, "[[NSVisualEffectView alloc] initWithFrame:]";
on earlier OS versions, "nil".

This work-around is necessary because older code bases will not be
able to directly refer to the class (it doesn’t exist).

(1.10)
*/
id
CocoaFuture_AllocInitVisualEffectViewWithFrame		(CGRect		inFrame)
{
	id		result = nil;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSVisualEffectView");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSVisualEffectView class");
	}
	else
	{
		SEL		allocSelector = @selector(alloc);
		SEL		initSelector = @selector(initWithFrame:);
		
		
		if ([targetClass respondsToSelector:allocSelector] &&
			[targetClass instancesRespondToSelector:initSelector])
		{
			id		allocatedInstance = [targetClass performSelector:allocSelector];
			
			
			if (nil != allocatedInstance)
			{
				// NOTE: must use obc_msgSend() and not "performSelector:"
				// because the parameter is a C structure type
				result = objc_msgSend(allocatedInstance, initSelector, NSRectFromCGRect(inFrame));
			}
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSVisualEffectView found but it does not implement 'alloc' and/or 'initWithFrame:'");
		}
	}
#else
	result = [[NSUserNotification alloc] init];
#endif
	
	return result;
}// AllocInitVisualEffectViewWithFrame


/*!
On OS 10.8 and later, "[[NSXPCConnection alloc] initWithServiceName:aName]";
on earlier OS versions, "nil".

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
id
CocoaFuture_AllocInitXPCConnectionWithServiceName	(NSString*		inName)
{
	id		result = nil;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCConnection");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCConnection class");
	}
	else
	{
		SEL		allocSelector = @selector(alloc);
		SEL		initSelector = @selector(initWithServiceName:);
		
		
		if ([targetClass respondsToSelector:allocSelector] &&
			[targetClass instancesRespondToSelector:initSelector])
		{
			id		allocatedInstance = [targetClass performSelector:allocSelector];
			
			
			if (nil != allocatedInstance)
			{
				result = [allocatedInstance performSelector:initSelector withObject:inName];
			}
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCConnection found but it does not implement 'alloc' and/or 'initWithServiceName'");
		}
	}
#else
	result = [[NSXPCConnection alloc] initWithServiceName:inName];
#endif
	
	return result;
}// AllocInitXPCConnectionWithServiceName


/*!
On OS 10.8 and later, asks the Objective-C runtime for a
handle to the NSUserNotificationCenter class and returns the
result of the "defaultUserNotificationCenter" class method.
Earlier OS versions will return "nil".

This work-around is necessary because older code bases will
not be able to directly refer to the class (as it doesn’t
exist).

(1.10)
*/
id
CocoaFuture_DefaultUserNotificationCenter ()
{
	id		result = nil;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSUserNotificationCenter");
	
	
	if (nil == targetClass)
	{
		//Console_Warning(Console_WriteLine, "cannot find NSUserNotificationCenter class");
	}
	else
	{
		SEL		defaultUNCSelector = @selector(defaultUserNotificationCenter);
		
		
		if ([targetClass respondsToSelector:defaultUNCSelector])
		{
			result = [targetClass performSelector:defaultUNCSelector];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSUserNotificationCenter found but it does not implement 'defaultUserNotificationCenter'");
		}
	}
#else
	result = [NSUserNotificationCenter defaultUserNotificationCenter];
#endif
	
	return result;
}// DefaultUserNotificationCenter


/*!
On OS 10.12.1 and later, "inTouchBar.customizationAllowedItemIdentifiers = inIdentifierArray";
on earlier OS versions, no effect.

This work-around is necessary because older code bases will not be able
to directly refer to the class (it doesn’t exist).

(2016.11)
*/
void
CocoaFuture_TouchBarSetCustomizationAllowedItemIdentifiers	(NSTouchBar*	inTouchBar,
															 NSArray*		inIdentifierArray)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101201 /* MAC_OS_X_VERSION_10_12_1 */
	id		targetClass = objc_getClass("NSTouchBar");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSTouchBar class");
	}
	else
	{
		SEL		setSelector = @selector(setCustomizationAllowedItemIdentifiers:);
		
		
		if ([targetClass instancesRespondToSelector:setSelector])
		{
			[inTouchBar performSelector:setSelector withObject:inIdentifierArray];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSTouchBar found but it does not implement 'setCustomizationAllowedItemIdentifiers:'");
		}
	}
#else
	inTouchBar.customizationAllowedItemIdentifiers = inIdentifier;
#endif
}// TouchBarSetCustomizationAllowedItemIdentifiers


/*!
On OS 10.12.1 and later, "inTouchBar.customizationIdentifier = inIdentifier";
on earlier OS versions, no effect.

This work-around is necessary because older code bases will not be able
to directly refer to the class (it doesn’t exist).

(2016.11)
*/
void
CocoaFuture_TouchBarSetCustomizationIdentifier	(NSTouchBar*						inTouchBar,
												 NSTouchBarCustomizationIdentifier	inIdentifier)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101201 /* MAC_OS_X_VERSION_10_12_1 */
	id		targetClass = objc_getClass("NSTouchBar");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSTouchBar class");
	}
	else
	{
		SEL		setSelector = @selector(setCustomizationIdentifier:);
		
		
		if ([targetClass instancesRespondToSelector:setSelector])
		{
			[inTouchBar performSelector:setSelector withObject:inIdentifier];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSTouchBar found but it does not implement 'setCustomizationIdentifier:'");
		}
	}
#else
	inTouchBar.customizationIdentifier = inIdentifier;
#endif
}// TouchBarSetCustomizationIdentifier


/*!
On OS 10.8 and later, "[aConnection remoteObjectProxyWithErrorHandler:]";
on earlier OS versions, no effect (returning "nil").

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
id
CocoaFuture_XPCConnectionRemoteObjectProxy	(NSXPCConnection*		inConnection,
											 void (^inHandler)(NSError*))
{
	id		result = nil;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCConnection");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCConnection class");
	}
	else
	{
		SEL		remoteObjectProxySelector = @selector(remoteObjectProxyWithErrorHandler:);
		
		
		if ([targetClass instancesRespondToSelector:remoteObjectProxySelector])
		{
			result = [inConnection performSelector:remoteObjectProxySelector withObject:inHandler];
			NSLog(@"returning remote object proxy %@ for connection %@", result, inConnection);
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCConnection found but it does not implement 'remoteObjectProxyWithErrorHandler:'");
		}
	}
#else
	result = [aConnection remoteObjectProxyWithErrorHandler:inHandler];
#endif
	
	return result;
}// XPCConnectionRemoteObjectProxy


/*!
On OS 10.8 and later, "[aConnection resume]"; on earlier OS
versions, no effect.

This work-around is necessary because older code bases will
not be able to directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionResume		(NSXPCConnection*		inConnection)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCConnection");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCConnection class");
	}
	else
	{
		SEL		resumeSelector = @selector(resume);
		
		
		if ([targetClass instancesRespondToSelector:resumeSelector])
		{
			//NSLog(@"resuming connection %@", inConnection);
			[inConnection performSelector:resumeSelector];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCConnection found but it does not implement 'resume'");
		}
	}
#else
	[inConnection resume];
#endif
}// XPCConnectionResume


/*!
On OS 10.8 and later, "aConnection.interruptionHandler = aBlock";
on earlier OS versions, no effect.

This work-around is necessary because older code bases will not
be able to directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionSetInterruptionHandler		(NSXPCConnection*		inConnection,
													 void					(^inBlock)())
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCConnection");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCConnection class");
	}
	else
	{
		SEL		setBlockSelector = @selector(setInterruptionHandler:);
		
		
		if ([targetClass instancesRespondToSelector:setBlockSelector])
		{
			//NSLog(@"setting interruption handler %@", inConnection);
			[inConnection performSelector:setBlockSelector withObject:inBlock];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCConnection found but it does not implement 'setInterruptionHandler:'");
		}
	}
#else
	inConnection.interruptionHandler = inBlock;
#endif
}// XPCConnectionSetInterruptionHandler


/*!
On OS 10.8 and later, "aConnection.invalidationHandler = aBlock";
on earlier OS versions, no effect.

This work-around is necessary because older code bases will not
be able to directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionSetInvalidationHandler		(NSXPCConnection*		inConnection,
													 void					(^inBlock)())
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCConnection");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCConnection class");
	}
	else
	{
		SEL		setBlockSelector = @selector(setInvalidationHandler:);
		
		
		if ([targetClass instancesRespondToSelector:setBlockSelector])
		{
			//NSLog(@"setting invalidation handler %@", inConnection);
			[inConnection performSelector:setBlockSelector withObject:inBlock];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCConnection found but it does not implement 'setInvalidationHandler:'");
		}
	}
#else
	inConnection.invalidationHandler = inBlock;
#endif
}// XPCConnectionSetInvalidationHandler


/*!
On OS 10.8 and later, "aConnection.remoteObjectInterface = anInterface";
on earlier OS versions, no effect.

This work-around is necessary because older code bases will not be able
to directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionSetRemoteObjectInterface	(NSXPCConnection*		inConnection,
													 NSXPCInterface*		inInterface)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCConnection");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCConnection class");
	}
	else
	{
		SEL		setInterfaceSelector = @selector(setRemoteObjectInterface:);
		
		
		if ([targetClass instancesRespondToSelector:setInterfaceSelector])
		{
			//NSLog(@"setting remote object interface %@", inConnection);
			[inConnection performSelector:setInterfaceSelector withObject:inInterface];
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCConnection found but it does not implement 'setRemoteObjectInterface:'");
		}
	}
#else
	inConnection.remoteObjectInterface = inInterface;
#endif
}// XPCConnectionSetRemoteObjectInterface


/*!
On OS 10.8 and later, "[NSXPCInterface interfaceWithProtocol:aProtocol]";
on earlier OS versions, "nil".

This work-around is necessary because older code bases will not be able
to directly refer to the class (it doesn’t exist).

(1.11)
*/
id
CocoaFuture_XPCInterfaceWithProtocol	(Protocol*		inProtocol)
{
	id		result = nil;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080 /* MAC_OS_X_VERSION_10_8 */
	id		targetClass = objc_getClass("NSXPCInterface");
	
	
	if (nil == targetClass)
	{
		Console_Warning(Console_WriteLine, "cannot find NSXPCInterface class");
	}
	else
	{
		SEL		interfaceWithProtocolSelector = @selector(interfaceWithProtocol:);
		
		
		if ([targetClass respondsToSelector:interfaceWithProtocolSelector])
		{
			id		autoreleasedInstance = [targetClass performSelector:interfaceWithProtocolSelector withObject:inProtocol];
			
			
			if (nil != autoreleasedInstance)
			{
				//NSLog(@"XPC interface object %@", autoreleasedInstance);
				result = autoreleasedInstance;
			}
		}
		else
		{
			Console_Warning(Console_WriteLine, "NSXPCInterface found but it does not implement 'interfaceWithProtocol:'");
		}
	}
#else
	result = [NSXPCInterface interfaceWithProtocol:inProtocol];
#endif
	
	return result;
}// XPCInterfaceWithProtocol

// BELOW IS REQUIRED NEWLINE TO END FILE
