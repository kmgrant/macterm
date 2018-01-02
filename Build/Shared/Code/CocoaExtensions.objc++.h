/*!	\file CocoaExtensions.objc++.h
	\brief Declarations for methods that have been
	added to standard Cocoa classes.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2018 by Kevin Grant
	
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

// compile-time options
#ifndef COCOA_EXTENSIONS_SUPPORT_QUICKDRAW
#define COCOA_EXTENSIONS_SUPPORT_QUICKDRAW 0
#endif



#pragma mark Types

@interface NSColor (CocoaExtensions_NSColor) //{

// class methods
	+ (BOOL)
	searchResultColorsForForeground:(NSColor**)_
	background:(NSColor**)_;
	+ (BOOL)
	selectionColorsForForeground:(NSColor**)_
	background:(NSColor**)_;

// new methods: constructing color variations
	- (NSColor*)
	colorCloserToBlack;
	- (NSColor*)
	colorCloserToWhite;
	- (NSColor*)
	colorWithShading;

// new methods: setting color in graphics context
	- (void)
	setAsBackgroundInCGContext:(CGContextRef)_;
	- (void)
	setAsForegroundInCGContext:(CGContextRef)_;

#if COCOA_EXTENSIONS_SUPPORT_QUICKDRAW
// TEMPORARY AND DEPRECATED; USE ONLY AS NEEDED
	- (void)
	setAsBackgroundInQDCurrentPort;
	- (void)
	setAsForegroundInQDCurrentPort;
#endif

@end //}


@interface NSData (CocoaExtensions_NSData) //{

// initializers
	- (instancetype)
	initWithBase64EncodingOSImplementation:(NSString*)_;

@end //}


@interface NSImage (CocoaExtensions_NSImage) //{

// new methods
	- (NSImage*)
	imageFromSubRect:(NSRect)_;

@end //}


@interface NSInvocation (CocoaExtensions_NSInvocation) //{

// class methods
	+ (NSInvocation*)
	invocationWithSelector:(SEL)_
	target:(id)_;

@end //}


/*!
Since observers have the ridiculous property of
being extremely dependent on exactly how they
are installed in order to be removed correctly,
this object is used to capture state precisely.

When one of the helper methods below is used to
register an observer, an object of this type is
allocated and returned to capture the parameters
that were used.

Then, "removeObserverSpecifiedWith:" can be used
to precisely remove the observer later.
*/
@interface CocoaExtensions_ObserverSpec : NSObject //{
{
@private
	id			_observedObject;
	void*		_context;
	NSString*	_keyPath;
}

// accessors
	@property (assign) void*
	context;
	@property (strong) NSString*
	keyPath;
	@property (weak) id
	observedObject;

@end //}


@interface NSObject (CocoaExtensions_NSObject) //{

// new methods: simpler notifications
	- (void)
	postNote:(NSString*)_;
	- (void)
	postNote:(NSString*)_
	queued:(NSPostingStyle)_;
	- (void)
	postNote:(NSString*)_
	queued:(NSPostingStyle)_
	coalescing:(NSNotificationCoalescing)_;
	- (void)
	whenObject:(id)_
	postsNote:(NSString*)_
	performSelector:(SEL)_;
	- (void)
	ignoreWhenObject:(id)_
	postsNote:(NSString*)_;
	- (void)
	ignoreWhenObjectsPostNotes;

// new methods: simpler observers with easier cleanup
	- (CocoaExtensions_ObserverSpec*)
	newObserverFromKeyPath:(NSString*)_
	ofObject:(id)_
	options:(NSKeyValueObservingOptions)_
	context:(void*)_;
	- (CocoaExtensions_ObserverSpec*)
	newObserverFromSelector:(SEL)_;
	- (CocoaExtensions_ObserverSpec*)
	newObserverFromSelector:(SEL)_
	ofObject:(id)_
	options:(NSKeyValueObservingOptions)_
	context:(void*)_;
	- (void)
	removeObserverSpecifiedWith:(CocoaExtensions_ObserverSpec*)_;
	- (void)
	removeObserversSpecifiedInArray:(NSArray*)_;

@end //}


@interface NSValue (CocoaExtensions_NSValue) //{

// class methods: Core Graphics data
	+ (NSValue*)
	valueWithCGPoint:(CGPoint)_;
	+ (NSValue*)
	valueWithCGRect:(CGRect)_;
	+ (NSValue*)
	valueWithCGSize:(CGSize)_;

@end //}


@interface NSView (CocoaExtensions_NSView) //{

// new methods
	- (BOOL)
	isKeyboardFocusInSubtree;
	- (BOOL)
	isKeyboardFocusOnSelf;

@end //}


@interface NSWindow (CocoaExtensions_NSWindow) //{

// new methods: coordinate translation
	- (NSPoint)
	localToGlobalRelativeToTopForPoint:(NSPoint)_;

@end //}


#pragma mark New Methods

//!\name Key-Value Coding
//@{

// a macro for a highly-common comparison operation in observers
#define KEY_PATH_IS_SEL(aKeyPath,aSelector) \
	[(aKeyPath) isEqualToString:NSStringFromSelector(aSelector)]

//@}

//!\name Delayed Invocations
//@{

void
	CocoaExtensions_RunLater							(Float64						inDelayAsFractionOfSeconds,
														 void							(^inBlock)());

void
	CocoaExtensions_RunLaterInQueue						(dispatch_queue_t const&		inQueue,
														 Float64						inDelayAsFractionOfSeconds,
														 void							(^inBlock)());

//@}


/*!
Uses compile-time type deduction to invoke a selector on an
object that returns a single non-object parameter value and
takes no arguments.  The result is returned via parameter.
(When result is an object, use "performSelector:withObject:".)

Returns true only if the selector was found and invoked.

This is just a convenient way to use NSInvocation and set up
everything properly for a single-primitive-return-value case.

(2016.03)
*/
template < typename arg_type >
BOOL
CocoaExtensions_PerformSelectorOnTargetReturningValue	(SEL		inSelector,
														 id			inTarget,
														 arg_type*	outValuePtr)
{
	BOOL	result = [inTarget respondsToSelector:inSelector];
	
	
	if (result)
	{
		// (see NSInvocation extensions above for this method)
		NSInvocation*	methodInvoker = [NSInvocation invocationWithSelector:inSelector target:inTarget];
		
		
		// note: first “real” argument of target method is at #2
		[methodInvoker setReturnValue:outValuePtr];
		[methodInvoker invoke];
	}
	return result;
}// PerformSelectorOnTargetReturningValue


/*!
Uses compile-time type deduction to invoke a selector on an
object that requires a single non-object parameter value.
(If you have an object, use "performSelector:withObject:".)

Returns true only if the selector was found and invoked.

This is just a convenient way to use NSInvocation and set up
everything properly for a single-primitive-argument case.

(2016.03)
*/
template < typename arg_type >
BOOL
CocoaExtensions_PerformSelectorOnTargetWithValue	(SEL		inSelector,
													 id			inTarget,
													 arg_type	inSingleArgument)
{
	BOOL	result = [inTarget respondsToSelector:inSelector];
	
	
	if (result)
	{
		// (see NSInvocation extensions above for this method)
		NSInvocation*	methodInvoker = [NSInvocation invocationWithSelector:inSelector target:inTarget];
		
		
		// note: first “real” argument of target method is at #2
		[methodInvoker setArgument:&inSingleArgument atIndex:2];
		[methodInvoker invoke];
	}
	return result;
}// PerformSelectorOnTargetWithValue

// BELOW IS REQUIRED NEWLINE TO END FILE
