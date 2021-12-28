/*!	\file BoundName.objc++.h
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

// Mac includes
@import Cocoa;



#pragma mark Types

/*!
Since some older versions of the OS do not bind "description"
reliably, this exposes a string property that always has the
same meaning on any version of the OS.  It is recommended
that user interface elements use "boundName" for bindings
instead of "description".
*/
@interface BoundName_Object : NSObject //{

// initializers
	- (instancetype _Nullable)
	init;
	- (instancetype _Nullable)
	initWithBoundName:(NSString* _Nullable)_ NS_DESIGNATED_INITIALIZER;

// accessors
	@property (strong, nullable) NSString*
	boundName;
	@property (strong, nullable) NSString*
	description;

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
