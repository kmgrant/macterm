/*!	\file CocoaFuture.mm
	\brief Methods available only in newer OS versions.
	
	Most symbols are only declared, to ease compilation.
	But this file exists in case it is necessary to offer
	implementations (using different names) of certain
	functionality on older OSes.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.10
	© 2008-2012 by Kevin Grant
	
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
		//Console_Warning(Console_WriteLine, "cannot find NSUserNotification class");
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
			//Console_Warning(Console_WriteLine, "NSUserNotification found, but it does not implement 'alloc' and/or 'init'");
		}
	}
	
	return result;
}// CocoaFuture_AllocInitUserNotification


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
			//Console_Warning(Console_WriteLine, "NSUserNotificationCenter found, but it does not implement 'defaultUserNotificationCenter'");
		}
	}
	
	return result;
}// CocoaFuture_DefaultUserNotificationCenter

// BELOW IS REQUIRED NEWLINE TO END FILE
