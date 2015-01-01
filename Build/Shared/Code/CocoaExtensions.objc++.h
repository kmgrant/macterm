/*!	\file CocoaExtensions.objc++.h
	\brief Declarations for methods that have been
	added to standard Cocoa classes.
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

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Types

@interface NSColor (CocoaExtensions_NSColor) //{

// class methods
	+ (BOOL)
	searchResultColorsForForeground:(NSColor**)_
	background:(NSColor**)_;
	+ (BOOL)
	selectionColorsForForeground:(NSColor**)_
	background:(NSColor**)_;

// helpers for constructing color variations
	- (NSColor*)
	colorCloserToBlack;
	- (NSColor*)
	colorCloserToWhite;
	- (NSColor*)
	colorWithShading;

// setting the colors of a graphics context
	- (void)
	setAsBackgroundInCGContext:(CGContextRef)_;
	- (void)
	setAsForegroundInCGContext:(CGContextRef)_;

// TEMPORARY AND DEPRECATED; USE ONLY AS NEEDED
	- (void)
	setAsBackgroundInQDCurrentPort;
	- (void)
	setAsForegroundInQDCurrentPort;

@end //}


@interface NSInvocation (CocoaExtensions_NSInvocation) //{

// class methods: helpers for constructing from selectors on target objects
	+ (NSInvocation*)
	invocationWithSelector:(SEL)_
	target:(id)_;

@end //}


@interface NSObject (CocoaExtensions_NSObject) //{

// class methods: helpers for key-value observing customization
	+ (NSString*)
	selectorNameForKeyChangeAutoNotifyFlag:(NSString*)_;
	+ (SEL)
	selectorToReturnKeyChangeAutoNotifyFlag:(SEL)_;

@end //}


@interface NSWindow (CocoaExtensions_NSWindow) //{

// new methods: coordinate translation
	- (NSPoint)
	localToGlobalRelativeToTopForPoint:(NSPoint)_;

// new methods: helpers for setting frames with a delay
	- (void)
	setFrameWithArray:(id)_;

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
