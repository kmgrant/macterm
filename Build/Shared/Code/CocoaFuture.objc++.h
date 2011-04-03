/*!	\file CocoaFuture.h
	\brief Declarations for certain methods available only
	on newer OSes, to aid compilations for older OSes.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.6
	© 2008-2011 by Kevin Grant
	
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
#include <Cocoa/Cocoa.h>



#pragma mark Types

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5

/*!
Methods that are implemented ONLY on Mac OS X 10.5 and beyond.
These declarations should match the latest SDK.

WARNING:	You MUST use "respondsToSelector:" or an equivalent
			mechanism to guard against use of these methods on
			older OSes.  The advantage of importing this method
			is that you can directly invoke the target method
			(in an if-statement, say) without seeing compiler
			warnings.  Note that "performSelector:" is also an
			option, but that is much more cumbersome for APIs
			that take or return non-objects.
*/
@interface NSWindow (NSWindowExtensionsFromLeopard)

- (void)
setCollectionBehavior:(unsigned int)_;

@end

#endif // MAC_OS_X_VERSION_10_5

// BELOW IS REQUIRED NEWLINE TO END FILE
