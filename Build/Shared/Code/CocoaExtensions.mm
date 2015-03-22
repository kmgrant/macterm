/*!	\file CocoaExtensions.mm
	\brief Methods added to standard classes.
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

#import <CocoaExtensions.objc++.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <ColorUtilities.h>



#pragma mark Public Methods

#pragma mark -
@implementation NSColor (CocoaExtensions_NSColor)


/*!
Returns a color that is similar to the current color but
with an arbitrarily-darker shade.

(1.10)
*/
- (NSColor*)
colorCloserToBlack
{
	NSColor*	result = [self blendedColorWithFraction:0.5/* arbitrary */ ofColor:[NSColor blackColor]];
	
	
	if (nil == result)
	{
		result = [[self retain] autorelease];
	}
	return result;
}// colorCloserToBlack


/*!
Returns a color that is similar to the current color but
with an arbitrarily-lighter shade.

(1.10)
*/
- (NSColor*)
colorCloserToWhite
{
	NSColor*	result = [self blendedColorWithFraction:0.5/* arbitrary */ ofColor:[NSColor whiteColor]];
	
	
	if (nil == result)
	{
		result = [[self retain] autorelease];
	}
	return result;
}// colorCloserToWhite


/*!
Returns a color that is a noticeably different shade; if the
color is very dark, this will be like "colorCloserToWhite";
otherwise it will be like "colorCloserToBlack".

(1.10)
*/
- (NSColor*)
colorWithShading
{
	NSColor*	asRGB = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	NSColor*	result = asRGB;
	
	
	if (nil != asRGB)
	{
		Float32 const	kTolerance = 0.5; // color intensities can vary by this much and still be considered black or white
		
		
		if ([asRGB brightnessComponent] < kTolerance)
		{
			// dark; make whiter
			result = [self colorCloserToWhite];
		}
		else
		{
			// light; make blacker
			result = [self colorCloserToBlack];
		}
	}
	
	if (nil == result)
	{
		result = [[self retain] autorelease];
	}
	
	return result;
}// colorWithShading


/*!
Provides “highlighted for search results” versions of the given
foreground and background colors, overwriting the colors pointed to
by each argument.  Returns true unless there was a problem changing
one or both colors.

(1.10)
*/
+ (BOOL)
searchResultColorsForForeground:(NSColor**)		inoutForegroundColor
background:(NSColor**)							inoutBackgroundColor
{
	BOOL	result = YES;
	
	
	if ((nullptr == inoutForegroundColor) || (nullptr == inoutBackgroundColor))
	{
		result = NO;
	}
	else
	{
		Float32 const	kTolerance = 0.5; // color intensities can vary by this much and still be considered black or white
		NSColor*		foregroundRGB = [*inoutForegroundColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
		NSColor*		backgroundRGB = [*inoutBackgroundColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
		
		
		if ((nil != foregroundRGB) && (nil != backgroundRGB))
		{
			//NSColor*	searchHighlightColor = [NSColor yellowColor];
			NSColor*	searchHighlightColor = [NSColor colorWithCalibratedRed:0.92 green:1.0 blue:0.0 alpha:1.0];
			
			
			if (([foregroundRGB brightnessComponent] < kTolerance) &&
				([backgroundRGB brightnessComponent] > (1.0 - kTolerance)))
			{
				// approximately monochromatic; in this case it’s safe to pick a
				// target color because the appearance against black/white is known
				*inoutForegroundColor = [NSColor blackColor];
				*inoutBackgroundColor = searchHighlightColor;
			}
			else if ([backgroundRGB brightnessComponent] < kTolerance)
			{
				// very dark background; therefore, darkening the background will
				// not make it clear where the result is; brighten it instead
				*inoutForegroundColor = [foregroundRGB blendedColorWithFraction:0.9/* arbitrary */ ofColor:[NSColor whiteColor]];
				*inoutBackgroundColor = [backgroundRGB blendedColorWithFraction:0.3/* arbitrary */ ofColor:[NSColor blackColor]];
			}
			else
			{
				// typical case; some combination of colors, just find darker
				// colors to show results in this case
				*inoutForegroundColor = [foregroundRGB blendedColorWithFraction:0.8/* arbitrary */ ofColor:[NSColor blackColor]];
				*inoutBackgroundColor = [backgroundRGB blendedColorWithFraction:0.8/* arbitrary */ ofColor:searchHighlightColor];
			}
		}
		else
		{
			// error; leave unchanged
			result = NO;
		}
	}
	
	return result;
}// searchResultColorsForForeground:background:


/*!
Provides “selected” versions of the given foreground and background
colors, overwriting the colors pointed to by each argument.  Returns
true unless there was a problem changing one or both colors.

Since traditional selection schemes assume that white is the
background color, this routine examines the background color and
will only use the system-wide highlight color if the background is
approximately white.  Otherwise, the actual colors are used to find
an appropriate selection color (which will usually be slightly
“darker” than the given colors, unless the colors are close to
black).

(1.10)
*/
+ (BOOL)
selectionColorsForForeground:(NSColor**)	inoutForegroundColor
background:(NSColor**)						inoutBackgroundColor
{
	BOOL	result = YES;
	
	
	if ((nullptr == inoutForegroundColor) || (nullptr == inoutBackgroundColor))
	{
		result = NO;
	}
	else
	{
		Float32 const	kTolerance = 0.5; // color intensities can vary by this much and still be considered black or white
		NSColor*		foregroundRGB = [*inoutForegroundColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
		NSColor*		backgroundRGB = [*inoutBackgroundColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
		
		
		if ((nil != foregroundRGB) && (nil != backgroundRGB))
		{
			if (([foregroundRGB brightnessComponent] < kTolerance) &&
				([backgroundRGB brightnessComponent] > (1.0 - kTolerance)))
			{
				// approximately monochromatic; this is what the highlight color
				// is “traditionally” intended for, so it should look okay; use it
				*inoutForegroundColor = foregroundRGB;
				*inoutBackgroundColor = [NSColor selectedTextBackgroundColor];
			}
			else if ([backgroundRGB brightnessComponent] < kTolerance)
			{
				// very dark background; therefore, darkening the background will
				// not make it clear where the selection is; brighten it instead
				*inoutForegroundColor = [foregroundRGB blendedColorWithFraction:0.2/* arbitrary */ ofColor:[NSColor whiteColor]];
				*inoutBackgroundColor = [backgroundRGB blendedColorWithFraction:0.33/* arbitrary */ ofColor:[NSColor whiteColor]];
			}
			else
			{
				// typical case; some combination of colors, just find darker
				// colors to show highlighted text in this case
				*inoutForegroundColor = [foregroundRGB blendedColorWithFraction:0.2/* arbitrary */ ofColor:[NSColor blackColor]];
				*inoutBackgroundColor = [backgroundRGB blendedColorWithFraction:0.33/* arbitrary */ ofColor:[NSColor blackColor]];
			}
		}
		else
		{
			// error; leave unchanged
			result = NO;
		}
	}
	
	return result;
}// selectionColorsForForeground:background:


/*!
Sets this NSColor value as the background RGB color
of the specified Core Graphics context.

(4.1)
*/
- (void)
setAsBackgroundInCGContext:(CGContextRef)	aDrawingContext
{
	NSColor*		asRGB = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	
	
	CGContextSetRGBFillColor(aDrawingContext, [asRGB redComponent], [asRGB greenComponent],
								[asRGB blueComponent], [asRGB alphaComponent]);
}// setAsBackgroundInCGContext:


/*!
TEMPORARY.  FOR TRANSITIONAL CODE ONLY.  DEPRECATED.

Sets this NSColor value as the background RGB color
of the current QuickDraw graphics port.

(4.1)
*/
- (void)
setAsBackgroundInQDCurrentPort
{
	CGDeviceColor	asDeviceColor; // not really a device color, just convenient for RGB floats
	RGBColor		asQuickDrawRGB;
	NSColor*		asRGB = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	
	
	asDeviceColor.red = [asRGB redComponent];
	asDeviceColor.green = [asRGB greenComponent];
	asDeviceColor.blue = [asRGB blueComponent];
	asQuickDrawRGB = ColorUtilities_QuickDrawColorMake(asDeviceColor);
	RGBBackColor(&asQuickDrawRGB);
}// setAsBackgroundInQDCurrentPort


/*!
Sets this NSColor value as the foreground RGB color
of the specified Core Graphics context.

(4.1)
*/
- (void)
setAsForegroundInCGContext:(CGContextRef)	aDrawingContext
{
	NSColor*		asRGB = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	
	
	CGContextSetRGBStrokeColor(aDrawingContext, [asRGB redComponent], [asRGB greenComponent],
								[asRGB blueComponent], [asRGB alphaComponent]);
}// setAsForegroundInCGContext:


/*!
TEMPORARY.  FOR TRANSITIONAL CODE ONLY.  DEPRECATED.

Sets this NSColor value as the foreground RGB color
of the current QuickDraw graphics port.

(4.1)
*/
- (void)
setAsForegroundInQDCurrentPort
{
	CGDeviceColor	asDeviceColor; // not really a device color, just convenient for RGB floats
	RGBColor		asQuickDrawRGB;
	NSColor*		asRGB = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	
	
	asDeviceColor.red = [asRGB redComponent];
	asDeviceColor.green = [asRGB greenComponent];
	asDeviceColor.blue = [asRGB blueComponent];
	asQuickDrawRGB = ColorUtilities_QuickDrawColorMake(asDeviceColor);
	RGBForeColor(&asQuickDrawRGB);
}// setAsForegroundInQDCurrentPort


@end // NSColor (CocoaExtensions_NSColor)


#pragma mark -
@implementation NSInvocation (CocoaExtensions_NSInvocation)


/*!
Creates an NSInvocation object using the usual autorelease
class method, but automatically pulls the method signature
from the target object and automatically calls "setTarget:"
and "setSelector:" accordingly.

You must still call setArgument:atIndex: for any other
arguments that the selector requires.

(1.9)
*/
+ (NSInvocation*)
invocationWithSelector:(SEL)	aSelector
target:(id)						aTarget
{
	NSInvocation*	result = [NSInvocation invocationWithMethodSignature:
											[aTarget methodSignatureForSelector:aSelector]];
	
	
	[result setTarget:aTarget]; // index 0 implicitly
	[result setSelector:aSelector]; // index 1 implicitly
	
	return result;
}// invocationWithSelector:target:


@end // NSInvocation (CocoaExtensions_NSInvocation)


#pragma mark -
@implementation NSObject (CocoaExtensions_NSObject)


/*!
Given a property method name, such as fooBar, returns the
conventional name for a selector to indicate whether or not
changes to that key will automatically notify observers; in
this case, autoNotifyOnChangeToFooBar.

The idea is to keep property information close to the
accessors for those properties, instead of making the
"automaticallyNotifiesObserversForKey:" method big and ugly.

Related: NSObject(NSKeyValueObservingCustomization).

(1.9)
*/
+ (NSString*)
selectorNameForKeyChangeAutoNotifyFlag:(NSString*)	aPropertySelectorName
{
	NSString*			result = nil;
	NSMutableString*	nameOfAccessor = [[[NSMutableString alloc] initWithString:aPropertySelectorName]
											autorelease];
	NSString*			accessorFirstChar = [nameOfAccessor substringToIndex:1];
	
	
	[nameOfAccessor replaceCharactersInRange:NSMakeRange(0, 1/* length */)
												withString:[accessorFirstChar uppercaseString]];
	result = [@"autoNotifyOnChangeTo" stringByAppendingString:nameOfAccessor];
	return result;
}// selectorNameForKeyChangeAutoNotifyFlag:


/*!
Given a selector, such as @selector(fooBar), returns the
conventional selector for a method that would determine whether
or not property changes will automatically notify any observers;
in this case, @selector(autoNotifyOnChangeToFooBar).

The signature of the validator is expected to be:
+ (id) autoNotifyOnChangeToFooBar;
Due to limitations in performSelector:, the result is not a
BOOL, but rather an object of type NSNumber, whose "boolValue"
method is called.  Validators can return @(YES) or @(NO) for
compile-time constant cases.

See selectorNameForKeyChangeAutoNotifyFlag: and
automaticallyNotifiesObserversForKey:.

Related: NSObject(NSKeyValueObservingCustomization).

(1.9)
*/
+ (SEL)
selectorToReturnKeyChangeAutoNotifyFlag:(SEL)	anAccessor
{
	SEL		result = NSSelectorFromString([self.class selectorNameForKeyChangeAutoNotifyFlag:NSStringFromSelector(anAccessor)]);
	
	
	return result;
}// selectorToReturnKeyChangeAutoNotifyFlag:


@end // NSObject (CocoaExtensions_NSObject)


#pragma mark -
@implementation NSWindow (CocoaExtensions_NSWindow)


/*!
Translates coordinates from the local coordinate system of
the window into the screen coordinate system, while “flipping”
to make the coordinates relative to the top of the screen.

This is not recommended.  It is useful when translating code
that is still based in the QuickDraw and Carbon environments.

(1.9)
*/
- (NSPoint)
localToGlobalRelativeToTopForPoint:(NSPoint)	aLocalPoint
{
	NSPoint		result = aLocalPoint;
	NSPoint		windowGlobalPosition = [self frame].origin;
	
	
	// flip local Y
	result.y = (self.frame.size.height - result.y);
	
	// flip global Y
	windowGlobalPosition.y = ([self screen].frame.size.height - self.frame.origin.y - self.frame.size.height);
	
	// offset final coordinates based on flipped global
	result.x += windowGlobalPosition.x;
	result.y += windowGlobalPosition.y;
	
	return result;
}// localToGlobalRelativeToTopForPoint


/*!
Given an NSArray with 4 floating-point numbers in the order
X, Y, width and height, calls "setFrame:display:" on the
window (with a display of YES).

The array would be constructed using code such as:
@[
	[NSNumber numberWithFloat:[window frame].origin.x],
	[NSNumber numberWithFloat:[window frame].origin.y],
	[NSNumber numberWithFloat:[window frame].size.width],
	[NSNumber numberWithFloat:[window frame].size.height]
];

This is useful for calls that require an object, such as
"performSelector:withObject:afterDelay:".

(1.9)
*/
- (void)
setFrameWithArray:(id)		anArray
{
	NSRect		newFrame = [self frame];
	size_t		i = 0;
	
	
	for (NSNumber* numberObject in anArray)
	{
		if (0 == i)
		{
			newFrame.origin.x = [numberObject floatValue];
		}
		else if (1 == i)
		{
			newFrame.origin.y = [numberObject floatValue];
		}
		else if (2 == i)
		{
			newFrame.size.width = [numberObject floatValue];
		}
		else if (3 == i)
		{
			newFrame.size.height = [numberObject floatValue];
		}
		++i;
	}
	[self setFrame:newFrame display:YES];
}// setFrameWithArray:


@end // NSWindow (CocoaExtensions_NSWindow)

// BELOW IS REQUIRED NEWLINE TO END FILE
