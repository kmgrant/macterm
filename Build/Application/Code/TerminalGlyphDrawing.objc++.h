/*!	\file TerminalGlyphDrawing.objc++.h
	\brief Used to draw special VT graphics glyphs in terminals.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

// Mac includes
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

// library includes
#import <CocoaFuture.objc++.h>

// application includes
#import "TerminalViewRef.typedef.h"



#pragma mark Constants

typedef NSUInteger TerminalGlyphDrawing_Options;
enum
{
	kTerminalGlyphDrawing_OptionAntialiasingDisabled = (1 << 0),	//!< antialiasing should be disabled while rendering
	kTerminalGlyphDrawing_OptionBold = (1 << 1),					//!< glyph is meant to represent a boldface character
	kTerminalGlyphDrawing_OptionSmallSize = (1 << 2),				//!< rendering area may be too small for some details
};

#pragma mark Types

/*!
This object captures the potentially-complex process of
rendering a particular special graphics character.

NOTE:	Normally this object is constructed only by a
		cache that holds similar objects; see the class
		"TerminalGlyphDrawing_Cache".
*/
@interface TerminalGlyphDrawing_Layer : CALayer //{
{
@private
	UnicodeScalarValue				unicodePoint_;
	CGFloat							baselineHint_;
	TerminalGlyphDrawing_Options	options_;
	NSUInteger						filledSublayerFlags_; // array index is bit number; sublayers that fill
	NSUInteger						noStrokeSublayerFlags_; // sublayers that exclusively fill (no stroke)
	NSUInteger						insetSublayerFlags_; // sublayers that use an inset frame instead of the default
	NSUInteger						thickLineSublayerFlags_; // sublayers that do not scale beyond a thick line width
	NSUInteger						thinLineSublayerFlags_; // sublayers that do not scale beyond a thin line width
	NSMutableArray*					sublayerBlocks_;
}

// initializers
	- (instancetype)
	initWithUnicodePoint:(UnicodeScalarValue)_
	options:(TerminalGlyphDrawing_Options)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (void)
	renderInContext:(CGContextRef)_
	frame:(CGRect)_
	baselineHint:(CGFloat)_;

// accessors
	@property (assign) CGFloat
	baselineHint;
	@property (assign) CGColorRef
	color;

@end //}


/*!
This class caches related layers with small variations.
*/
@interface TerminalGlyphDrawing_Cache : NSObject //{
{
@private
	UnicodeScalarValue				unicodePoint_;
	TerminalGlyphDrawing_Layer*		normalPlainLayer_;
	TerminalGlyphDrawing_Layer*		normalBoldLayer_;
	TerminalGlyphDrawing_Layer*		smallPlainLayer_;
	TerminalGlyphDrawing_Layer*		smallBoldLayer_;
}

// class methods
	+ (instancetype)
	cacheWithUnicodePoint:(UnicodeScalarValue)_;

// initializers
	- (instancetype)
	initWithUnicodePoint:(UnicodeScalarValue)_;

// new methods
	- (TerminalGlyphDrawing_Layer*)
	layerWithOptions:(TerminalGlyphDrawing_Options)_
	color:(CGColorRef)_;

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
