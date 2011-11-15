/*!	\file AutoPool.objc++.h
	\brief An Objective-C++ class that allocates and
	releases an NSAutoreleasePool automatically within
	a scope.
	
	This pool is needed to bind Cocoa to Carbon, and
	greatly simplifies C++ code for the bound APIs.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.9
	© 2008 by Kevin Grant
	
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



#pragma mark Types

class AutoPool
{
public:
	inline
	AutoPool	();
	
	inline
	~AutoPool	();

protected:

private:
	NSAutoreleasePool*		_pool;
};



#pragma mark Public Methods

AutoPool::
AutoPool (): _pool([[NSAutoreleasePool alloc] init])
{
}// AutoPool constructor


AutoPool::
~AutoPool ()
{
	[_pool release];
}// AutoPool destructor

// BELOW IS REQUIRED NEWLINE TO END FILE
