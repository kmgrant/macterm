/*!	\file CocoaFuture.mm
	\brief Methods available only in newer OS versions.
	
	Most symbols are only declared, to ease compilation.
	But this file exists in case it is necessary to offer
	implementations (using different names) of certain
	functionality on older OSes.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2016 by Kevin Grant
	
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



#pragma mark Public Methods

/*!
On Mac OS X 10.8 and later, "[[NSUserNotification alloc] init]";
on earlier Mac OS X versions, "nil".

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
On Mac OS X 10.10 and later, "[[NSVisualEffectView alloc] initWithFrame:]";
on earlier Mac OS X versions, "nil".

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.10)
*/
id
CocoaFuture_AllocInitVisualEffectViewWithFrame		(CGRect		inFrame)
{
	id		result = nil;
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
	
	return result;
}// AllocInitVisualEffectViewWithFrame


/*!
On Mac OS X 10.8 and later, "[[NSXPCConnection alloc] initWithServiceName:aName]";
on earlier Mac OS X versions, "nil".

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
id
CocoaFuture_AllocInitXPCConnectionWithServiceName	(NSString*		inName)
{
	id		result = nil;
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
	
	return result;
}// AllocInitXPCConnectionWithServiceName


/*!
On Mac OS X 10.8 and later, asks the Objective-C runtime for
a handle to the NSUserNotificationCenter class and returns the
result of the "defaultUserNotificationCenter" class method.
Earlier Mac OS X versions will return "nil".

This work-around is necessary because older code bases will
not be able to directly refer to the class (as it doesn’t
exist).

(1.10)
*/
id
CocoaFuture_DefaultUserNotificationCenter ()
{
	id		result = nil;
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
	
	return result;
}// DefaultUserNotificationCenter


/*!
On Mac OS X 10.8 and later, "[aConnection remoteObjectProxyWithErrorHandler]";
on earlier Mac OS X versions, no effect (returning "nil").

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
id
CocoaFuture_XPCConnectionRemoteObjectProxy	(NSXPCConnection*		inConnection,
											 void (^inHandler)(NSError*))
{
	id		result = nil;
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
	
	return result;
}// XPCConnectionRemoteObjectProxy


/*!
On Mac OS X 10.8 and later, "[aConnection resume]"; on earlier Mac OS X
versions, no effect.

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionResume		(NSXPCConnection*		inConnection)
{
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
}// XPCConnectionResume


/*!
On Mac OS X 10.8 and later, "aConnection.interruptionHandler = aBlock";
on earlier Mac OS X versions, no effect.

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionSetInterruptionHandler		(NSXPCConnection*		inConnection,
													 void					(^inBlock)())
{
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
}// XPCConnectionSetInterruptionHandler


/*!
On Mac OS X 10.8 and later, "aConnection.invalidationHandler = aBlock";
on earlier Mac OS X versions, no effect.

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionSetInvalidationHandler		(NSXPCConnection*		inConnection,
													 void					(^inBlock)())
{
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
}// XPCConnectionSetInvalidationHandler


/*!
On Mac OS X 10.8 and later, "aConnection.remoteObjectInterface = anInterface";
on earlier Mac OS X versions, no effect.

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
void
CocoaFuture_XPCConnectionSetRemoteObjectInterface	(NSXPCConnection*		inConnection,
													 NSXPCInterface*		inInterface)
{
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
}// XPCConnectionSetRemoteObjectInterface


/*!
On Mac OS X 10.8 and later, "[NSXPCInterface interfaceWithProtocol:aProtocol]";
on earlier Mac OS X versions, "nil".

This work-around is necessary because older code bases will not be able to
directly refer to the class (it doesn’t exist).

(1.11)
*/
id
CocoaFuture_XPCInterfaceWithProtocol	(Protocol*		inProtocol)
{
	id		result = nil;
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
	
	return result;
}// XPCInterfaceWithProtocol

// BELOW IS REQUIRED NEWLINE TO END FILE
