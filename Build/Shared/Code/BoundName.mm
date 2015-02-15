/*!	\file BoundName.mm
	\brief An object with a stable name-string binding.
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

#import <BoundName.objc++.h>

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Public Methods

@implementation BoundName_Object


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
		[self setBoundName:aString];
	}
	return self;
}// initWithBoundName:


/*!
Destructor.

(1.9)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Accessor.

(1.9)
*/
- (NSString*)
boundName
{
	return [[boundNameString_ retain] autorelease];
}
- (void)
setBoundName:(NSString*)	aString
{
	if (boundNameString_ != aString)
	{
		[boundNameString_ release];
		boundNameString_ = [aString copy];
	}
}// setBoundName:


/*!
Accessor.

(1.9)
*/
- (NSString*)
description
{
	return [[boundNameString_ retain] autorelease];
}
- (void)
setDescription:(NSString*)		aString
{
	if (boundNameString_ != aString)
	{
		[boundNameString_ release];
		boundNameString_ = [aString copy];
	}
}// setDescription:


@end // BoundName_Object

// BELOW IS REQUIRED NEWLINE TO END FILE
