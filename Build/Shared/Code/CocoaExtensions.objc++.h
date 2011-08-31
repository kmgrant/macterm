/*!	\file CocoaExtensions.objc++.h
	\brief Declarations for methods that have been
	added to standard Cocoa classes.
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
#import <Cocoa/Cocoa.h>



#pragma mark Types

@interface NSObject (CocoaExtensions_NSObject)

// helpers for key-value observing customization

+ (NSString*)
selectorNameForKeyChangeAutoNotifyFlag:(NSString*)_;

+ (SEL)
selectorToReturnKeyChangeAutoNotifyFlag:(SEL)_;

@end


@interface NSWindow (CocoaExtensions_NSWindow)

// helpers for setting frames with a delay

- (void)
setFrameWithArray:(id)_;

@end

// BELOW IS REQUIRED NEWLINE TO END FILE
