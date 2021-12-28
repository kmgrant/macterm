/*!	\file BoundName.mm
	\brief An object with a stable name-string binding.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2021 by Kevin Grant
	
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

#import <BoundName.objc++.h>

// Mac includes
@import Cocoa;



#pragma mark Public Methods

#pragma mark -
@implementation BoundName_Object


#pragma mark Initializers


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
init
{
	return [self initWithBoundName:@""];
}// init


/*!
Designated initializer.

(1.9)
*/
- (instancetype)
initWithBoundName:(NSString*)	aString
{
	self = [super init];
	if (nil != self)
	{
		_boundName = [aString copy];
	}
	return self;
}// initWithBoundName:


/*!
Accessor.

(1.9)
*/
- (NSString*)
description
{
	return self.boundName;
}
- (void)
setDescription:(NSString*)		aString
{
	self.boundName = aString;
}// setDescription:


@end // BoundName_Object

// BELOW IS REQUIRED NEWLINE TO END FILE
