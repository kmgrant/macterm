/*!	\file TerminalGlyphDrawing
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

#import "TerminalGlyphDrawing.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <QuartzCore/QuartzCore.h>

// library includes
#import <CFRetainRelease.h>
#import <CGContextSaveRestore.h>
#import <Console.h>



// these help to ensure that values land on pixel boundaries
// when they ought to for optimal rendering purposes (note:
// there are more supported ways to do this such as using an
// NSView and "centerScanRect:" but that is not practical
// right now for legacy reasons)
#define INCR_PIXEL(x) (round(x) + 0.5)
#define DECR_PIXEL(x) (round(x) - 0.5)


#pragma mark Constants
namespace {

typedef NSUInteger My_GlyphDrawingOptions;
enum
{
	kMy_GlyphDrawingOptionFill		= (1 << 0),		// fill the path with the target color
	kMy_GlyphDrawingOptionInset		= (1 << 1),		// inset target rectangle to avoid touching neighbor cells
	kMy_GlyphDrawingOptionNoStroke	= (1 << 2),		// do NOT stroke the path with the target color
	kMy_GlyphDrawingOptionThickLine	= (1 << 3),		// do NOT auto-scale line width beyond a very thick width
	kMy_GlyphDrawingOptionThinLine	= (1 << 4),		// do NOT auto-scale line width beyond a very thin width
};

CGAffineTransform* const	kMy_NoTransform = nullptr;

} // anonymous namespace

#pragma mark Types

@class TerminalGlyphDrawing_Metrics;

typedef void (^My_ShapeDefinitionBlock)(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);

/*!
The private class interface.
*/
@interface TerminalGlyphDrawing_Layer (TerminalGlyphDrawing_LayerInternal) //{

// accessors
	- (BOOL)
	isAntialiasingDisabled;
	- (BOOL)
	isBold;
	- (BOOL)
	isSmallSize;

// new methods: complete glyph definitions
	- (void)
	addLayersForOutsideRangeUnicodePoint:(UnicodeScalarValue)_;
	- (void)
	addLayersForRangeHex2500To2600UnicodePoint:(UnicodeScalarValue)_;

// new methods: partial glyph definitions
	- (void)
	addLayerUsingBlock:(My_ShapeDefinitionBlock)_;
	- (void)
	addLayerUsingBlock:(My_ShapeDefinitionBlock)_
	options:(My_GlyphDrawingOptions)_;
	- (void)
	addLayerUsingText:(NSString*)_;

// new methods: other
	- (CGSize)
	insetAmount;
	- (void)
	rebuildLayers;

@end //}


/*!
Captures common frame-based drawing measurements (for
example, a line width that scales with text and scaled
positions for certain box-drawing lines).  Consistency
across glyph renderers is important for alignment.

Rendering properties are not constant; many of them are
set according to the current frame rectangles of the
given layers.  If you intend to query several properties
and use them together, be sure to gather all the values
before changing any layer’s frame rectangle.
*/
@interface TerminalGlyphDrawing_Metrics : NSObject //{
{
@private
	CAShapeLayer*					targetLayer_;
	TerminalGlyphDrawing_Layer*		owningLayer_;
}

// class methods
	+ (instancetype)
	metricsWithTargetLayer:(CAShapeLayer*)_
	owningLayer:(TerminalGlyphDrawing_Layer*)_;

// initializers
	- (instancetype)
	initWithTargetLayer:(CAShapeLayer*)_
	owningLayer:(TerminalGlyphDrawing_Layer*)_;

// new methods
	- (void)
	standardBold;

// accessors: layers
	@property (strong, readonly) TerminalGlyphDrawing_Layer*
	owningLayer;
	@property (strong, readonly) CAShapeLayer*
	targetLayer;

// accessors: layer size
	@property (assign, readonly) CGFloat
	layerHalfHeight;
	@property (assign, readonly) CGFloat
	layerHalfWidth;
	@property (assign, readonly) CGFloat
	layerHeight;
	@property (assign, readonly) CGFloat
	layerWidth;

// accessors: default line size
	@property (assign, readonly) CGFloat
	lineHalfWidth;
	@property (assign, readonly) CGFloat
	lineWidth;

// accessors: rendering pairs of lines
	@property (assign, readonly) CGFloat
	doubleHorizontalFirstY;
	@property (assign, readonly) CGFloat
	doubleHorizontalSecondY;
	@property (assign, readonly) CGFloat
	doubleVerticalFirstX;
	@property (assign, readonly) CGFloat
	doubleVerticalSecondX;

// accessors: connecting lines to edges
	@property (assign, readonly) CGFloat
	squareLineBoldNormalJoin;
	@property (assign, readonly) CGFloat
	squareLineBottomEdge;
	@property (assign, readonly) CGFloat
	squareLineLeftEdge;
	@property (assign, readonly) CGFloat
	squareLineRightEdge;
	@property (assign, readonly) CGFloat
	squareLineTopEdge;

// accessors: text properties
	@property (assign, readonly) CGFloat
	baselineHint; // WARNING: this property is not accurate when "kMy_GlyphDrawingOptionInset" is in use

@end //}

#pragma mark Internal Method Prototypes
namespace {

void	extendPath						(CGMutablePathRef, CGFloat, CGFloat, CGFloat, CGFloat);
void	extendPath						(CGMutablePathRef, CGFloat, CGFloat, CGFloat, CGFloat,
										 CGFloat, CGFloat);
void	extendPath						(CGMutablePathRef, CGFloat, CGFloat, CGFloat, CGFloat,
										 CGFloat, CGFloat, CGFloat, CGFloat);
void	extendWithCenterHorizontal		(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithCenterVertical		(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithMidLeftBottomHook		(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithMidLeftTopHook		(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithMidRightBottomHook	(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithMidRightTopHook		(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithSegmentDown			(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithSegmentLeft			(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithSegmentRight			(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
void	extendWithSegmentUp				(CGMutablePathRef, TerminalGlyphDrawing_Metrics*);
BOOL	extendWithText					(CGMutablePathRef, TerminalGlyphDrawing_Metrics*,
										 CFAttributedStringRef);

} // anonymous namespace



#pragma mark Public Methods


#pragma mark -
@implementation TerminalGlyphDrawing_Cache //{


#pragma mark Class Methods


/*!
Returns an autoreleased cache object for the given code point.

Note that caches may be captured such that they are never
really released.

(4.1)
*/
+ (instancetype)
cacheWithUnicodePoint:(UnicodeScalarValue)		aUnicodePoint
{
	static NSMutableDictionary*		pointCaches = [[NSMutableDictionary alloc] init];
	NSNumber*						numberKey = [NSNumber numberWithUnsignedLong:aUnicodePoint];
	TerminalGlyphDrawing_Cache*		result = STATIC_CAST([pointCaches objectForKey:numberKey], decltype(result));
	
	
	if (nil == result)
	{
		result = [[[self.class alloc] initWithUnicodePoint:aUnicodePoint] autorelease];
		[pointCaches setObject:result forKey:numberKey];
	}
	return result;
}// cacheWithUnicodePoint:


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithUnicodePoint:(UnicodeScalarValue)	aUnicodePoint
{
	self = [super init];
	if (nil != self)
	{
		self->unicodePoint_ = aUnicodePoint;
		self->normalPlainLayer_ = nil;
		self->normalBoldLayer_ = nil;
		self->smallPlainLayer_ = nil;
		self->smallBoldLayer_ = nil;
	}
	return self;
}// initWithUnicodePoint:


#pragma mark New Methods


/*!
Returns a new or cached layer that has the specified
characteristics.

The color will be used either to stroke or to fill,
depending on what the glyph handler does.

(4.1)
*/
- (TerminalGlyphDrawing_Layer*)
layerWithOptions:(TerminalGlyphDrawing_Options)		anOptionFlagSet
color:(CGColorRef)									aColor
{
	TerminalGlyphDrawing_Layer*		result = nil;
	TerminalGlyphDrawing_Layer**	choice = nullptr;
	
	
	if (anOptionFlagSet & kTerminalGlyphDrawing_OptionSmallSize)
	{
		if (anOptionFlagSet & kTerminalGlyphDrawing_OptionBold)
		{
			choice = &smallBoldLayer_;
		}
		else
		{
			choice = &smallPlainLayer_;
		}
	}
	else
	{
		if (anOptionFlagSet & kTerminalGlyphDrawing_OptionBold)
		{
			choice = &normalBoldLayer_;
		}
		else
		{
			choice = &normalPlainLayer_;
		}
	}
	
	// if the corresponding type of layer has not been created
	// before, create it now and cache it for later use
	if (nil == *choice)
	{
		*choice = [[TerminalGlyphDrawing_Layer alloc]
					initWithUnicodePoint:unicodePoint_
											options:anOptionFlagSet];
	}
	
	result = *choice;
	assert(nil != result);
	
	result.color = aColor;
	
	return result;
}// layerWithOptions:color:


@end //} TerminalGlyphDrawing_Cache


#pragma mark -
@implementation TerminalGlyphDrawing_Layer //{


@synthesize baselineHint = baselineHint_;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithUnicodePoint:(UnicodeScalarValue)	aUnicodePoint
options:(TerminalGlyphDrawing_Options)		anOptionFlagSet
{
	self = [super init];
	if (nil != self)
	{
		self->unicodePoint_ = aUnicodePoint;
		self->baselineHint_ = 0; // set later
		self->options_ = anOptionFlagSet;
		self->filledSublayerFlags_ = 0;
		self->noStrokeSublayerFlags_ = 0;
		self->insetSublayerFlags_ = 0;
		self->thickLineSublayerFlags_ = 0;
		self->thinLineSublayerFlags_ = 0;
		self->sublayerBlocks_ = [[NSMutableArray alloc] init];
		
		self.geometryFlipped = YES;
		
		// note: the Terminal View and Terminal Screen modules must also be
		// aware of any supported Unicode values
		if ((aUnicodePoint >= 0x2500) && (aUnicodePoint < 0x2600))
		{
			[self addLayersForRangeHex2500To2600UnicodePoint:aUnicodePoint];
		}
		else
		{
			// this catch-all function should implement any remaining
			// supported glyph renderings that do not fit in one of
			// the ranges above
			[self addLayersForOutsideRangeUnicodePoint:aUnicodePoint];
		}
		
		[self rebuildLayers];
	}
	return self;
}// initWithUnicodePoint:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[sublayerBlocks_ release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Draws the layer in the frame rectangle, with the specified
baseline offset (from the top) as a hint to renderers that
create text-like output.  If you do not know the baseline
then you should set this to "aFrame.size.height".

Use this instead of calling "renderInContext:" directly.
Also note that currently this class is not set up to
recognize direct changes to the frame; rather, the frame
is updated as a side effect of using this method.

(4.1)
*/
- (void)
renderInContext:(CGContextRef)	aDrawingContext
frame:(CGRect)					aFrame
baselineHint:(CGFloat)			aBaselineOffsetFromTop
{
	CGContextSaveRestore	_(aDrawingContext);
	
	
	self.baselineHint = aBaselineOffsetFromTop;
	if (false == CGRectEqualToRect(self.frame, aFrame))
	{
		// layer shapes have the ability to adapt to their container so
		// they are reconstructed whenever the target region changes
		self.frame = aFrame;
		[self rebuildLayers];
	}
	
	// place the drawing in the right part of the window
	CGContextTranslateCTM(aDrawingContext, self.frame.origin.x, self.frame.origin.y);
	
	// if necessary disable anti-aliasing for the drawing; note that
	// this does not call CGContextSetShouldAntialias() because that
	// method will only affect some anti-aliasing (in particular,
	// edge effects on the update rectangle may still be displayed,
	// which can prevent lines from connecting properly)
	CGContextSetAllowsAntialiasing(aDrawingContext, (NO == self.isAntialiasingDisabled));
	
	[super renderInContext:aDrawingContext];
	
	// this flag is not part of the state so it must be restored manually
	CGContextSetAllowsAntialiasing(aDrawingContext, true);
}// renderInContext:frame:baselineHint:


#pragma mark Accessors


/*!
Accesses the color that is used to draw the glyph.
This color may be used to stroke or to fill, depending
on what the glyph definition requires.

(4.1)
*/
- (CGColorRef)
color
{
	CGColorRef		result = nullptr;
	
	
	for (CALayer* layer in self.sublayers)
	{
		if ([layer isKindOfClass:CAShapeLayer.class])
		{
			CAShapeLayer*	asShapeLayer = STATIC_CAST(layer, decltype(asShapeLayer));
			
			
			result = asShapeLayer.fillColor;
			if (nil == result)
			{
				result = asShapeLayer.strokeColor;
			}
			break;
		}
	}
	return result;
}
- (void)
setColor:(CGColorRef)		aColor
{
	NSUInteger	i = 0;
	for (CALayer* layer in self.sublayers)
	{
		assert([layer isKindOfClass:CAShapeLayer.class]);
		NSUInteger const	kFlagMask = (1 << i);
		CAShapeLayer*		asShapeLayer = STATIC_CAST(layer, decltype(asShapeLayer));
		
		
		// sublayers can use the color for stroke or fill
		asShapeLayer.fillColor = (0 != (filledSublayerFlags_ & kFlagMask))
									? aColor
									: nil;
		asShapeLayer.strokeColor = (0 != (noStrokeSublayerFlags_ & kFlagMask))
									? nil
									: aColor;
		++i;
	}
}// setColor:


@end //} TerminalGlyphDrawing_Layer


#pragma mark Internal Methods
namespace {


/*!
An overloaded function that extends the specified path
by moving to the first point location and adding lines
through all the given points.

This is just a short-cut for CGPathAddLines().

(4.1)
*/
void
extendPath	(CGMutablePathRef	inoutPath,
			 CGFloat			inX1,
			 CGFloat			inY1,
			 CGFloat			inX2,
			 CGFloat			inY2)
{
	CGPoint		points[] =
				{
					{ inX1, inY1 },
					{ inX2, inY2 }
				};
	
	
	CGPathAddLines(inoutPath, kMy_NoTransform, points, sizeof(points) / sizeof(*points));
}// extendPath (5 arguments)


/*!
An overloaded function that extends the specified path
by moving to the first point location and adding lines
through all the given points.

This is just a short-cut for CGPathAddLines().

(4.1)
*/
void
extendPath	(CGMutablePathRef	inoutPath,
			 CGFloat			inX1,
			 CGFloat			inY1,
			 CGFloat			inX2,
			 CGFloat			inY2,
			 CGFloat			inX3,
			 CGFloat			inY3)
{
	CGPoint		points[] =
				{
					{ inX1, inY1 },
					{ inX2, inY2 },
					{ inX3, inY3 }
				};
	
	
	CGPathAddLines(inoutPath, kMy_NoTransform, points, sizeof(points) / sizeof(*points));
}// extendPath (7 arguments)


/*!
An overloaded function that extends the specified path
by moving to the first point location and adding lines
through all the given points.

This is just a short-cut for CGPathAddLines().

(4.1)
*/
void
extendPath	(CGMutablePathRef	inoutPath,
			 CGFloat			inX1,
			 CGFloat			inY1,
			 CGFloat			inX2,
			 CGFloat			inY2,
			 CGFloat			inX3,
			 CGFloat			inY3,
			 CGFloat			inX4,
			 CGFloat			inY4)
{
	CGPoint		points[] =
				{
					{ inX1, inY1 },
					{ inX2, inY2 },
					{ inX3, inY3 },
					{ inX4, inY4 }
				};
	
	
	CGPathAddLines(inoutPath, kMy_NoTransform, points, sizeof(points) / sizeof(*points));
}// extendPath (9 arguments)


/*!
Adds a line from the left edge to the right edge
that is centered in the frame.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithCenterHorizontal	(CGMutablePathRef				inoutPath,
							 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.squareLineLeftEdge, inMetrics.layerHalfHeight,
							inMetrics.squareLineRightEdge, inMetrics.layerHalfHeight);
}// extendWithCenterHorizontal


/*!
Adds a line from the top edge to the bottom edge
that is centered in the frame.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithCenterVertical	(CGMutablePathRef				inoutPath,
							 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.layerHalfWidth, inMetrics.squareLineTopEdge,
							inMetrics.layerHalfWidth, inMetrics.squareLineBottomEdge);
}// extendWithCenterVertical


/*!
Adds a line from the left edge to the center, and
from the center to the bottom edge.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithMidLeftBottomHook		(CGMutablePathRef				inoutPath,
								 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.squareLineLeftEdge, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.squareLineBottomEdge);
}// extendWithMidLeftBottomHook


/*!
Adds a line from the left edge to the center, and
from the center to the top edge.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithMidLeftTopHook	(CGMutablePathRef				inoutPath,
							 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.squareLineLeftEdge, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.squareLineTopEdge);
}// extendWithMidLeftTopHook


/*!
Adds a line from the right edge to the center, and
from the center to the bottom edge.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithMidRightBottomHook	(CGMutablePathRef				inoutPath,
								 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.squareLineRightEdge, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.squareLineBottomEdge);
}// extendWithMidRightBottomHook


/*!
Adds a line from the right edge to the center, and
from the center to the top edge.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithMidRightTopHook	(CGMutablePathRef				inoutPath,
							 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.squareLineRightEdge, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.squareLineTopEdge);
}// extendWithMidRightTopHook


/*!
Adds a line from the bottom edge to the center.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithSegmentDown	(CGMutablePathRef				inoutPath,
						 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.squareLineBottomEdge);
}// extendWithSegmentDown


/*!
Adds a line from the left edge to the center.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithSegmentLeft	(CGMutablePathRef				inoutPath,
						 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.squareLineLeftEdge, inMetrics.layerHalfHeight);
}// extendWithSegmentLeft


/*!
Adds a line from the right edge to the center.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithSegmentRight	(CGMutablePathRef				inoutPath,
						 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.squareLineRightEdge, inMetrics.layerHalfHeight);
}// extendWithSegmentRight


/*!
Adds a line from the top edge to the center.

The exact coordinates depend on the specified
metrics object.

(4.1)
*/
void
extendWithSegmentUp	(CGMutablePathRef				inoutPath,
					 TerminalGlyphDrawing_Metrics*	inMetrics)
{
	extendPath(inoutPath, inMetrics.layerHalfWidth, inMetrics.layerHalfHeight,
							inMetrics.layerHalfWidth, inMetrics.squareLineTopEdge);
}// extendWithSegmentUp


/*!
Generates the glyphs of the specified text and adds them to
the path, translated to the given location.  Returns true
only if the text is handled successfully.

NOTE:	This is not really meant to draw long strings, it
		would be restricted to whatever fits in a cell.
		Use it for cases where a font contains the exact
		rendering that you require.

(4.1)
*/
BOOL
extendWithText		(CGMutablePathRef				inoutPath,
					 TerminalGlyphDrawing_Metrics*	inMetrics,
					 CFAttributedStringRef			inText)
{
	CFRetainRelease		coreTextLine(STATIC_CAST(CTLineCreateWithAttributedString(inText), CFTypeRef),
										true/* is retained */);
	BOOL				horizontallyCentered = YES; // currently not a parameter
	BOOL				result = NO;
	
	
	if (coreTextLine.exists())
	{
		CTLineRef		asLine = REINTERPRET_CAST(coreTextLine.returnCFTypeRef(), CTLineRef);
		NSArray*		lineRuns = BRIDGE_CAST(CTLineGetGlyphRuns(asLine), NSArray*);
		
		
		// only one run is expected but an iteration seems necessary regardless
		[lineRuns enumerateObjectsUsingBlock:^(id object, NSUInteger UNUSED_ARGUMENT(index), BOOL* UNUSED_ARGUMENT(stop))
		{
			CTRunRef			asRun = BRIDGE_CAST(object, CTRunRef);
			CTFontRef			currentFont = REINTERPRET_CAST(CFDictionaryGetValue
																(CTRunGetAttributes(asRun), kCTFontAttributeName),
																CTFontRef);
			NSUInteger const		kNumGlyphs = CTRunGetGlyphCount(asRun);
			
			
			// although this is iterating over glyphs, the expectation is
			// that there is one glyph (centered about the origin-X)
			for (NSUInteger i = 0; i < kNumGlyphs; ++i)
			{
				CFRange		oneGlyphRange = CFRangeMake(i, 1);
				CGGlyph		glyphFontIndex = kCGFontIndexInvalid;
				CGPoint		glyphPosition;
				CGSize		glyphAdvance;
				CGPathRef	glyphPath = nullptr;
				
				
				CTRunGetGlyphs(asRun, oneGlyphRange, &glyphFontIndex);
				CTRunGetPositions(asRun, oneGlyphRange, &glyphPosition);
				CTRunGetAdvances(asRun, oneGlyphRange, &glyphAdvance);
				
				// generate a subpath for this glyph
				glyphPath = CTFontCreatePathForGlyph(currentFont, glyphFontIndex, kMy_NoTransform);
				{
					CGSize const		kCellSize = inMetrics.targetLayer.frame.size;
					CGRect const		kCellFrame = CGRectMake(0, 0, kCellSize.width, kCellSize.height);
					CGRect const		kFontRelativeFrame = CTFontGetBoundingRectsForGlyphs(currentFont, kCTFontHorizontalOrientation,
																							&glyphFontIndex, nullptr/* sub-rectangles */,
																							1/* number of items */);
					CGFloat const		kOffsetX = (((horizontallyCentered)
													? (-glyphAdvance.width / 2.0)
													: 0) * CGRectGetWidth(kCellFrame) / CGRectGetWidth(kFontRelativeFrame));
					CGAffineTransform	offsetFlipMatrix = CGAffineTransformConcat
															(CGAffineTransformMakeScale
																(kCellSize.width / CGRectGetWidth(kFontRelativeFrame),
																	-kCellSize.height / CGRectGetHeight(kFontRelativeFrame)),
																CGAffineTransformMakeTranslation
																(kCellFrame.origin.x + kCellSize.width / 2.0 + kOffsetX, kCellSize.height));
					
					
					// finally, extend the path to include a drawing of this glyph
					CGPathAddPath(inoutPath, &offsetFlipMatrix, glyphPath);
					//CGPathAddRect(inoutPath, &offsetFlipMatrix, kFontRelativeFrame); // debug
					//CGPathAddRect(inoutPath, kMy_NoTransform, kCellFrame); // debug
				}
				CGPathRelease(glyphPath), glyphPath = nullptr;
			}
		}];
		result = YES;
	}
	
	return result;
}// extendWithText


} // anonymous namespace


#pragma mark -
@implementation TerminalGlyphDrawing_Layer (TerminalGlyphDrawing_LayerInternal) //{


#pragma mark Accessors


/*!
Returns true only if the options given at construction time
included "kTerminalGlyphDrawing_OptionAntialiasingDisabled".

(4.1)
*/
- (BOOL)
isAntialiasingDisabled
{
	return (0 != (options_ & kTerminalGlyphDrawing_OptionAntialiasingDisabled));
}// isAntialiasingDisabled


/*!
Returns true only if the options given at construction time
included "kTerminalGlyphDrawing_OptionBold".

(4.1)
*/
- (BOOL)
isBold
{
	return (0 != (options_ & kTerminalGlyphDrawing_OptionBold));
}// isBold


/*!
Returns true only if the options given at construction time
included "kTerminalGlyphDrawing_OptionSmallSize".

(4.1)
*/
- (BOOL)
isSmallSize
{
	return (0 != (options_ & kTerminalGlyphDrawing_OptionSmallSize));
}// isSmallSize


#pragma mark New Methods: Complete Glyph Definitions


/*!
Adds drawing layers for Unicode points that are not handled by range
methods such as "addLayersForRangeHex2500To2600UnicodePoint:".

This is called by the initializer.

(4.1)
*/
- (void)
addLayersForOutsideRangeUnicodePoint:(UnicodeScalarValue)	aUnicodePoint
{
	if ((aUnicodePoint >= 0x2500) && (aUnicodePoint < 0x2600))
	{
		// value is not in the supported range for this function
		assert(false && "should call addLayersForRangeHex2500To2600UnicodePoint: for this value");
	}
	else
	{
		// value is in the expected range
		/*__weak*/ TerminalGlyphDrawing_Layer*	weakSelf = self;
		
		
		switch (aUnicodePoint)
		{
		case '=': // equal to
		case 0x2260: // not equal to
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// this should be similar to the "equivalent to" rendering (0x2261)
					CGFloat const	kLeft = metrics.squareLineLeftEdge; // note inset option below
					CGFloat const	kRight = metrics.squareLineRightEdge; // note inset option below
					CGFloat const	kPosition1 = metrics.doubleHorizontalFirstY + 1; // arbitrary
					CGFloat const	kPosition2 = metrics.doubleHorizontalSecondY + 1; // arbitrary
					
					
					extendPath(aPath, kLeft, kPosition1, kRight, kPosition1);
					extendPath(aPath, kLeft, kPosition2, kRight, kPosition2);
					if ('=' != aUnicodePoint)
					{
						CGFloat		slashLeft = (kLeft + metrics.lineWidth);
						CGFloat		slashRight = (kRight - metrics.lineWidth);
						
						
						// at small sizes this could be better placed...
						if (slashLeft > metrics.layerHalfWidth)
						{
							slashLeft = kLeft;
						}
						
						if (slashRight < metrics.layerHalfWidth)
						{
							slashRight = kRight;
						}
						
						extendPath(aPath, slashLeft, kPosition2 + metrics.lineWidth * 2.0,
											slashRight, kPosition1 - metrics.lineWidth * 2.0);
					}
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2026: // ellipsis (three dots)
		case 0x22EF: // middle ellipsis (three dots, centered)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerWidth / 9.0);
					CGFloat const	kSegmentLength = (kSpaceLength * 2.0);
					CGFloat const	kYPosition = ((0x22EF == aUnicodePoint)
													? metrics.layerHalfHeight
													: DECR_PIXEL(metrics.baselineHint - metrics.lineHalfWidth));
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, offset, kYPosition,
										offset + kSegmentLength, kYPosition);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, kYPosition,
										offset + kSegmentLength, kYPosition);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, kYPosition,
										offset + kSegmentLength, kYPosition);
				}];
			}
			break;
		
		case 0x00B7: // middle dot
		case 0x2027: // centered dot (hyphenation point)
		case 0x2219: // bullet operator (smaller than bullet)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddEllipseInRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, metrics.layerHalfHeight,
																				metrics.lineWidth, metrics.lineWidth));
				} options:(kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2022: // bullet (technically bigger than middle dot and circular)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* UNUSED_ARGUMENT(metrics))
				{
					CGPathAddEllipseInRect(aPath, kMy_NoTransform, CGRectInset(weakSelf.bounds, CGRectGetWidth(weakSelf.bounds) / 2.0,
																				CGRectGetHeight(weakSelf.bounds) / 2.0));
				}];
			}
			break;
		
		case 0x2190: // leftwards arrow
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerWidth / 4.0, metrics.layerHeight / 4.0);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerWidth / 4.0, metrics.layerHeight / 4.0 * 3.0);
					extendWithCenterHorizontal(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2191: // upwards arrow
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kLeft = metrics.layerWidth / 4.0;
					CGFloat const	kRight = metrics.layerWidth / 4.0 * 3.0;
					
					
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										kLeft, metrics.layerHeight / 4.0);
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										kRight, metrics.layerHeight / 4.0);
					extendWithCenterVertical(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2192: // rightwards arrow
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerWidth / 4.0 * 3.0, metrics.layerHeight / 4.0);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerWidth / 4.0 * 3.0, metrics.layerHeight / 4.0 * 3.0);
					extendWithCenterHorizontal(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2193: // downwards arrow
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kLeft = metrics.layerWidth / 4.0;
					CGFloat const	kRight = metrics.layerWidth / 4.0 * 3.0;
					
					
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										kLeft, metrics.layerHeight / 4.0 * 3.0);
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										kRight, metrics.layerHeight / 4.0 * 3.0);
					extendWithCenterVertical(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x21B5: // new line (international symbol is an arrow that hooks from mid-top to mid-left)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.squareLineTopEdge,
										metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.squareLineLeftEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.layerHeight / 4.0);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.layerHeight / 4.0 * 3.0);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x21DF: // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kLeft = metrics.layerWidth / 4.0;
					CGFloat const	kRight = metrics.layerWidth / 4.0 * 3.0;
					
					
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										kLeft, metrics.layerHeight / 4.0 * 3.0);
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										kRight, metrics.layerHeight / 4.0 * 3.0);
					extendPath(aPath, kLeft, metrics.layerHeight / 4.0,
										kRight, metrics.layerHeight / 4.0);
					extendPath(aPath, kLeft, metrics.layerHalfHeight,
										kRight, metrics.layerHalfHeight);
					extendWithCenterVertical(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x21E5: // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge + metrics.lineWidth, metrics.layerHeight / 4.0,
										metrics.squareLineRightEdge + metrics.lineWidth, metrics.layerHeight / 4.0 * 3.0);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerWidth / 4.0 * 3.0, metrics.layerHeight / 4.0);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerWidth / 4.0 * 3.0, metrics.layerHeight / 4.0 * 3.0);
					extendWithCenterHorizontal(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x221A: // square root left edge
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.squareLineLeftEdge + metrics.lineWidth/* arbitrary */, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.squareLineRightEdge, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x2261: // equivalent to (three horizontal lines)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// this should be similar to the "equal to" rendering ('=')
					CGFloat const	kLeft = metrics.squareLineLeftEdge; // note inset option below
					CGFloat const	kRight = metrics.squareLineRightEdge; // note inset option below
					CGFloat const	kPosition1 = metrics.layerHalfHeight - metrics.lineWidth * 2.0; // arbitrary
					CGFloat const	kPosition2 = metrics.layerHalfHeight; // arbitrary
					CGFloat const	kPosition3 = metrics.layerHalfHeight + metrics.lineWidth * 2.0; // arbitrary
					
					
					extendPath(aPath, kLeft, kPosition1, kRight, kPosition1);
					extendPath(aPath, kLeft, kPosition2, kRight, kPosition2);
					extendPath(aPath, kLeft, kPosition3, kRight, kPosition3);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2320: // integral sign (elongated S), top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kCurveRadius = (metrics.layerHeight / 4.0); // arbitrary (should match lower integral sign)
					CGPoint const	kCurlStartPoint = CGPointMake(metrics.squareLineRightEdge, metrics.squareLineTopEdge + kCurveRadius);
					CGPoint const	kCurlEndPoint = CGPointMake(metrics.layerHalfWidth, metrics.squareLineTopEdge + kCurveRadius);
					CGPoint const	kStraightLineStartPoint = CGPointMake(metrics.layerHalfWidth, kCurlEndPoint.y + metrics.lineHalfWidth);
					CGPathMoveToPoint(aPath, kMy_NoTransform, kCurlStartPoint.x, kCurlStartPoint.y);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform,
												metrics.layerWidth / 4.0 * 3.0, metrics.squareLineTopEdge + metrics.lineHalfWidth,
												kCurlEndPoint.x, kCurlEndPoint.y);
					extendPath(aPath, kStraightLineStartPoint.x, kStraightLineStartPoint.y,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2321: // integral sign (elongated S), bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kCurveRadius = (metrics.layerHeight / 4.0); // arbitrary (should match upper integral sign)
					CGPoint const	kCurlStartPoint = CGPointMake(metrics.layerWidth / 4.0, metrics.squareLineBottomEdge - kCurveRadius);
					CGPoint const	kCurlEndPoint = CGPointMake(metrics.layerHalfWidth, metrics.squareLineBottomEdge - kCurveRadius);
					CGPoint const	kStraightLineEndPoint = CGPointMake(metrics.layerHalfWidth, kCurlEndPoint.y - metrics.lineHalfWidth);
					CGPathMoveToPoint(aPath, kMy_NoTransform, kCurlStartPoint.x, kCurlStartPoint.y);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform,
												metrics.layerWidth / 4.0, metrics.squareLineBottomEdge - metrics.lineHalfWidth,
												kCurlEndPoint.x, kCurlEndPoint.y);
					extendPath(aPath, kStraightLineEndPoint.x, kStraightLineEndPoint.y,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x2325: // option key
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kYPosition1 = (metrics.layerHeight / 4.0);
					CGFloat const	kYPosition2 = (metrics.layerHeight / 4.0 * 3.0);
					
					
					extendPath(aPath, metrics.squareLineRightEdge, kYPosition1,
										metrics.layerWidth / 3.0 * 2.0, kYPosition1);
					extendPath(aPath, metrics.squareLineRightEdge, kYPosition2,
										metrics.layerWidth / 3.0 * 2.0, kYPosition2,
										metrics.layerWidth / 3.0, kYPosition1);
					extendPath(aPath, metrics.layerWidth / 3.0, kYPosition1,
										metrics.squareLineLeftEdge, kYPosition1);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2387: // alternate key
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kYPosition1 = (metrics.layerHeight / 4.0);
					CGFloat const	kYPosition2 = (metrics.layerHeight / 4.0 * 3.0);
					
					
					extendPath(aPath, metrics.squareLineRightEdge, kYPosition2,
										metrics.layerWidth / 3.0 * 2.0, kYPosition2);
					extendPath(aPath, metrics.squareLineRightEdge, kYPosition1,
										metrics.layerWidth / 3.0 * 2.0, kYPosition1,
										metrics.layerWidth / 3.0, kYPosition2);
					extendPath(aPath, metrics.layerWidth / 3.0, kYPosition2,
										metrics.squareLineLeftEdge, kYPosition2);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x239B: // left parenthesis, upper
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineBottomEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineRightEdge, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x239C: // left parenthesis extension
		case 0x239F: // right parenthesis extension
		case 0x23A2: // left square bracket extension
		case 0x23A5: // right square bracket extension
		case 0x23AA: // curly brace extension
		case 0x23AE: // integral extension
		case 0x23B8: // left vertical box line
		case 0x23B9: // right vertical box line
		case 0x23D0: // vertical line extension
			{
				// in MacTerm, all of these are just a vertical line
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
				}];
			}
			break;
		
		case 0x239D: // left parenthesis, lower
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineRightEdge, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x239E: // right parenthesis, upper
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineBottomEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineLeftEdge, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x23A0: // right parenthesis, lower
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineLeftEdge, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x23A1: // left square bracket, upper
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x23A3: // left square bracket, lower
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x23A4: // right square bracket, upper
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x23A6: // right square bracket, lower
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x23A7: // left curly brace, upper
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth, metrics.layerHeight / 4.0);
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHeight / 4.0);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge,
												metrics.squareLineRightEdge, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x23A8: // left curly brace, middle
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineLeftEdge, metrics.layerHalfHeight);
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineBottomEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineLeftEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x23A9: // left curly brace, lower
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.layerHeight / 4.0 * 3.0);
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHeight / 4.0 * 3.0);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
												metrics.squareLineRightEdge, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x23AB: // right curly brace, upper
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth, metrics.layerHeight / 4.0);
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHeight / 4.0);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge,
												metrics.squareLineLeftEdge, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x23AC: // right curly brace, middle
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineRightEdge, metrics.layerHalfHeight);
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineBottomEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x23AD: // right curly brace, lower
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.layerHeight / 4.0 * 3.0);
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHeight / 4.0 * 3.0);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
												metrics.squareLineLeftEdge, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x23B2: // large sigma (summation), top half
			{
				// this drawing mixes both edge-connected parts and inset parts
				// so "insetAmount" is invoked to connect the results
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.squareLineTopEdge + metrics.lineWidth,
										metrics.squareLineRightEdge, metrics.squareLineTopEdge,
										metrics.squareLineLeftEdge - [metrics.owningLayer insetAmount].width,
										metrics.squareLineTopEdge);
				} options:(kMy_GlyphDrawingOptionInset)/* this segment should not touch neighbor cells */];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					if (NO == self.isSmallSize)
					{
						[metrics standardBold];
					}
					extendPath(aPath, metrics.squareLineLeftEdge,
										metrics.squareLineTopEdge + [metrics.owningLayer insetAmount].height,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x23B3: // large sigma (summation), bottom half
			{
				// this drawing mixes both edge-connected parts and inset parts
				// so "insetAmount" is invoked to connect the results
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					if (NO == self.isSmallSize)
					{
						[metrics standardBold];
					}
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.squareLineLeftEdge,
										metrics.squareLineBottomEdge - [metrics.owningLayer insetAmount].height);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge - [metrics.owningLayer insetAmount].width,
										metrics.squareLineBottomEdge,
										metrics.squareLineRightEdge, metrics.squareLineBottomEdge,
										metrics.squareLineRightEdge, metrics.squareLineBottomEdge - metrics.lineWidth);
				} options:(kMy_GlyphDrawingOptionInset)/* this segment should not touch neighbor cells */];
			}
			break;
		
		case 0x23B7: // square root bottom, centered
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					// part of the shape is meant to touch neighbor cells and part is not;
					// therefore, some of the offsets must be done manually
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.squareLineTopEdge,
										metrics.layerHalfWidth - metrics.squareLineBoldNormalJoin,
										metrics.squareLineBottomEdge);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// unlike 0x221A, this is meant to connect straight upward
					// so the line must remain vertically centered
					extendWithCenterVertical(aPath, metrics);
				}];
			}
			break;
		
		case 0x23BA: // top line
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.lineHalfWidth,
										metrics.squareLineRightEdge, metrics.lineHalfWidth);
				}];
			}
			break;
		
		case 0x23BB: // line between top and middle regions
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight / 2.0,
										metrics.squareLineRightEdge, metrics.layerHalfHeight / 2.0);
				}];
			}
			break;
		
		case 0x23BC: // line between middle and bottom regions
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHeight - metrics.layerHalfHeight / 2.0,
										metrics.squareLineRightEdge, metrics.layerHeight - metrics.layerHalfHeight / 2.0);
				}];
			}
			break;
		
		case 0x23BD: // bottom line
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHeight - metrics.lineHalfWidth,
										metrics.squareLineRightEdge, metrics.layerHeight - metrics.lineHalfWidth);
				}];
			}
			break;
		
		case 0x26A1: // online/offline lightning bolt
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// draw triangle pointing to upper right
					CGFloat const	kTriangleOffset = ([self isSmallSize] ? 0 : (metrics.lineWidth / 4.0));
					
					
					extendPath(aPath, metrics.layerWidth / 3.0 * 2.0, metrics.layerHeight / 3.0,
										metrics.layerWidth / 4.0, metrics.layerHalfHeight + kTriangleOffset,
										metrics.layerWidth / 3.0, metrics.layerHalfHeight + kTriangleOffset,
										metrics.layerWidth / 3.0 * 2.0, metrics.layerHeight / 3.0);
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionThinLine)];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kTriangleOffset = ([self isSmallSize] ? 0 : (metrics.lineWidth / 4.0));
					
					
					// draw triangle pointing to lower left
					extendPath(aPath, metrics.layerWidth / 3.0, metrics.layerHeight / 3.0 * 2.0,
										metrics.layerWidth / 3.0 * 2.0, metrics.layerHalfHeight - kTriangleOffset,
										metrics.layerWidth / 4.0 * 3.0, metrics.layerHalfHeight - kTriangleOffset,
										metrics.layerWidth / 3.0, metrics.layerHeight / 3.0 * 2.0);
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionThinLine)];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kTriangleOffset = ([self isSmallSize] ? 0 : (metrics.lineWidth / 4.0));
					
					
					// fill in the middle
					extendPath(aPath, metrics.layerWidth / 3.0 + metrics.lineHalfWidth, metrics.layerHalfHeight + kTriangleOffset,
										metrics.layerWidth / 3.0 * 2.0 - metrics.lineHalfWidth, metrics.layerHalfHeight + kTriangleOffset);
					extendPath(aPath, metrics.layerWidth / 3.0 + metrics.lineHalfWidth, metrics.layerHalfHeight - kTriangleOffset,
										metrics.layerWidth / 3.0 * 2.0 - metrics.lineHalfWidth, metrics.layerHalfHeight - kTriangleOffset);
				} options:(kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x2713: // check mark
		case 0x2714: // check mark, bold
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapRound;
					extendPath(aPath, metrics.layerWidth / 5.0, metrics.layerHalfHeight,
										metrics.layerWidth / 3.0, metrics.layerHeight / 6.0 * 4.0,
										metrics.layerWidth / 6.0 * 5.0, metrics.layerHeight / 6.0);
				}];
			}
			break;
		
		case 0x2718: // X mark
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerWidth / 4.0, metrics.layerHeight / 4.0,
										metrics.layerWidth / 4.0 * 3.0, metrics.layerHeight / 4.0 * 3.0 + metrics.lineHalfWidth);
					extendPath(aPath, metrics.layerWidth / 4.0 * 3.0, metrics.layerHeight / 4.0,
										metrics.layerWidth / 4.0, metrics.layerHeight / 4.0 * 3.0);
				}];
			}
			break;
		
		case 0x27A6: // curve-to-right arrow (used for detached-from-head in "powerline")
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// do not draw the curve-arrow; instead, draw something a
					// bit more similar to the “version control branch” icon
					// (except show the detachment)
					CGFloat const	kXPosition1 = (metrics.layerWidth / 4.0);
					CGFloat const	kXPosition2 = (metrics.layerWidth / 4.0 * 3.0);
					
					
					// left line
					extendPath(aPath, kXPosition1, metrics.layerHeight / 6.0,
										kXPosition1, metrics.layerHeight / 6.0 * 5.0);
					
					// arrow head
					extendPath(aPath, kXPosition2 - metrics.lineHalfWidth, metrics.squareLineTopEdge + metrics.lineWidth * 2.0,
										kXPosition2, metrics.squareLineTopEdge,
										kXPosition2 + metrics.lineHalfWidth, metrics.squareLineTopEdge + metrics.lineWidth * 2.0);
					extendPath(aPath, kXPosition2, metrics.layerHeight / 3.0 * 2.0,
										kXPosition2, metrics.layerHeight / 6.0);
				} options:(kMy_GlyphDrawingOptionThickLine)];
			}
			break;
		
		case 0x2913: // vertical tab (international symbol is a down-pointing arrow with a terminating line)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kLeft = metrics.layerWidth / 4.0;
					CGFloat const	kRight = metrics.layerWidth / 4.0 * 3.0;
					
					
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										kLeft, metrics.layerHeight / 4.0 * 3.0);
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										kRight, metrics.layerHeight / 4.0 * 3.0);
					extendPath(aPath, kLeft, metrics.squareLineBottomEdge + metrics.lineWidth,
										kRight, metrics.squareLineBottomEdge + metrics.lineWidth);
					extendWithCenterVertical(aPath, metrics);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0xE0A0: // "powerline" version control branch
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kXPosition1 = (metrics.layerWidth / 4.0);
					CGFloat const	kXPosition2 = (metrics.layerWidth / 4.0 * 3.0);
					
					
					// left line
					extendPath(aPath, kXPosition1, metrics.layerHeight / 6.0,
										kXPosition1, metrics.layerHeight / 6.0 * 5.0);
					
					// arrow head
					extendPath(aPath, kXPosition2 - metrics.lineHalfWidth, metrics.squareLineTopEdge + metrics.lineWidth * 2.0,
										kXPosition2, metrics.squareLineTopEdge,
										kXPosition2 + metrics.lineHalfWidth, metrics.squareLineTopEdge + metrics.lineWidth * 2.0);
				} options:(kMy_GlyphDrawingOptionThickLine)];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kXPosition1 = (metrics.layerWidth / 4.0);
					CGFloat const	kXPosition2 = (metrics.layerWidth / 4.0 * 3.0);
					
					
					// this element of the path is separate so that the
					// line overlap/intersection is smoother
					metrics.targetLayer.lineCap = kCALineCapRound;
					extendPath(aPath, kXPosition1, metrics.layerHeight / 3.0 * 2.0,
										kXPosition2, metrics.layerHeight / 3.0,
										kXPosition2, metrics.layerHeight / 6.0);
				} options:(kMy_GlyphDrawingOptionThickLine)];
			}
			break;
		
		case 0xE0A1: // "powerline" line (LN) marker
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kLLeft = (metrics.squareLineLeftEdge + 1);
					CGFloat const	kLRight = (metrics.layerWidth / 5.0 * 3.0);
					CGFloat const	kLTop = (metrics.squareLineTopEdge + 1);
					CGFloat const	kLBottom = ((metrics.layerHeight / 5.0 * 2.0) - 1);
					CGFloat const	kNLeft = (metrics.layerWidth / 3.0);
					CGFloat const	kNRight = (metrics.squareLineRightEdge - 1);
					CGFloat const	kNTop = ((metrics.layerHeight / 5.0 * 3.0));
					CGFloat const	kNBottom = (metrics.squareLineBottomEdge - 2);
					
					
					//metrics.targetLayer.lineCap = kCALineCapRound;
					extendPath(aPath, kLLeft, kLTop, kLLeft, kLBottom,
										kLRight, kLBottom);
					extendPath(aPath, kNLeft, kNBottom, kNLeft, kNTop,
										kNRight, kNBottom, kNRight, kNTop);
				}];
			}
			break;
		
		case 0xE0A2: // "powerline" closed padlock
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform,
												metrics.layerWidth / 5.0, metrics.layerHalfHeight);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform,
												metrics.layerWidth / 5.0, metrics.layerHeight / 5.0,
												metrics.layerHalfWidth, metrics.layerHeight / 5.0);
					CGPathMoveToPoint(aPath, kMy_NoTransform,
												metrics.layerWidth / 5.0 * 4.0, metrics.layerHalfHeight);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform,
												metrics.layerWidth / 5.0 * 4.0, metrics.layerHeight / 5.0,
												metrics.layerHalfWidth, metrics.layerHeight / 5.0);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThickLine)];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerWidth / 6.0,
											metrics.layerHalfHeight, metrics.layerWidth / 6.0 * 4.0, metrics.layerHeight / 3.0));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionInset)];
			}
			break;
		
		case 0xE0B0: // "powerline" rightward triangle
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					//metrics.targetLayer.lineCap = kCALineCapRound;
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.squareLineTopEdge,
										metrics.squareLineRightEdge - metrics.lineWidth, metrics.layerHalfHeight,
										metrics.squareLineLeftEdge, metrics.squareLineBottomEdge,
										metrics.squareLineLeftEdge, metrics.squareLineTopEdge);
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0xE0B1: // "powerline" rightward arrowhead
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.squareLineTopEdge,
										metrics.squareLineRightEdge - metrics.lineWidth, metrics.layerHalfHeight,
										metrics.squareLineLeftEdge, metrics.squareLineBottomEdge);
				} options:(kMy_GlyphDrawingOptionThickLine)];
			}
			break;
		
		case 0xE0B2: // "powerline" leftward triangle
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapRound;
					extendPath(aPath, metrics.squareLineRightEdge, metrics.squareLineTopEdge,
										metrics.squareLineLeftEdge + metrics.lineWidth, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.squareLineBottomEdge,
										metrics.squareLineRightEdge, metrics.squareLineTopEdge);
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0xE0B3: // "powerline" leftward arrowhead
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.squareLineTopEdge,
										metrics.squareLineLeftEdge + metrics.lineWidth, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.squareLineBottomEdge);
				} options:(kMy_GlyphDrawingOptionThickLine)];
			}
			break;
		
		case 0xFFFD: // replacement character
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// draw a hollow diamond
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.squareLineLeftEdge, metrics.layerHalfHeight);
				} options:(kMy_GlyphDrawingOptionThinLine)];
				
				// draw a question mark inside the diamond
				[self addLayerUsingText:@"?"];
			}
			break;
		
		default:
			// ???
			Console_Warning(Console_WriteValueUnicodePoint, "TerminalGlyphDrawing_Layer not implemented for specified code point", aUnicodePoint);
			break;
		}
	}
}// addLayersForOutsideRangeUnicodePoint:


/*!
Adds drawing layers for Unicode points in the range 0x2500 (inclusive)
to 0x2600 (exclusive).

This is called by the initializer.

(4.1)
*/
- (void)
addLayersForRangeHex2500To2600UnicodePoint:(UnicodeScalarValue)		aUnicodePoint
{
	if ((aUnicodePoint < 0x2500) || (aUnicodePoint >= 0x2600))
	{
		// value is not in the supported range for this function
		assert(false && "incorrect glyph layer initialization method called");
	}
	else
	{
		// value is in the expected range
		switch (aUnicodePoint)
		{
		case 0x2500: // middle line
		case 0x2501: // middle line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterHorizontal(aPath, metrics);
				}];
			}
			break;
		
		case 0x2502: // vertical line
		case 0x2503: // vertical line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
				}];
			}
			break;
		
		case 0x2504: // horizontal triple-dashed line
		case 0x2505: // horizontal triple-dashed line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerWidth / 9.0);
					CGFloat const	kSegmentLength = (kSpaceLength * 2.0);
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2506: // vertical triple-dashed line
		case 0x2507: // vertical triple-dashed line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerHeight / 9.0);
					CGFloat const	kSegmentLength = (kSpaceLength * 2.0);
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
				}];
			}
			break;
		
		case 0x2508: // horizontal quadruple-dashed line
		case 0x2509: // horizontal quadruple-dashed line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerWidth / 16.0 * 1.5);
					CGFloat const	kSegmentLength = (metrics.layerWidth / 16.0 * 2.5);
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x250A: // vertical quadruple-dashed line
		case 0x250B: // vertical quadruple-dashed line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerHeight / 16.0 * 1.5);
					CGFloat const	kSegmentLength = (metrics.layerHeight / 16.0 * 2.5);
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
				}];
			}
			break;
		
		case 0x250C: // hook mid-right to mid-bottom
		case 0x250F: // hook mid-right to mid-bottom, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightBottomHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x250D: // hook mid-right to mid-bottom, bold right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth + metrics.lineHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight - metrics.squareLineBoldNormalJoin,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x250E: // hook mid-right to mid-bottom, bold bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth - metrics.squareLineBoldNormalJoin, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight + metrics.lineHalfWidth,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2510: // hook mid-left to mid-bottom
		case 0x2513: // hook mid-left to mid-bottom, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2511: // hook mid-left to mid-bottom, bold left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth - metrics.lineHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight - metrics.squareLineBoldNormalJoin,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2512: // hook mid-left to mid-bottom, bold bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth + metrics.squareLineBoldNormalJoin, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight + metrics.lineHalfWidth,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2514: // hook mid-top to mid-right
		case 0x2517: // hook mid-top to mid-right, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightTopHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2515: // hook mid-top to mid-right, bold right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth + metrics.lineHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight + metrics.squareLineBoldNormalJoin,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x2516: // hook mid-top to mid-right, bold top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth - metrics.squareLineBoldNormalJoin, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight - metrics.lineHalfWidth,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x2519: // hook mid-top to mid-left, bold left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth - metrics.lineHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight + metrics.squareLineBoldNormalJoin,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x251A: // hook mid-top to mid-left, bold top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth + metrics.squareLineBoldNormalJoin, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight - metrics.lineHalfWidth,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
			}
			break;
		
		case 0x2518: // hook mid-top to mid-left
		case 0x251B: // hook mid-top to mid-left, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftTopHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x251C: // cross minus the left piece
		case 0x2523: // cross minus the left piece, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x251D: // cross minus the left piece, bold right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x251E: // cross minus the left piece, bold top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentUp(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightBottomHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x251F: // cross minus the left piece, bold bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentDown(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightTopHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2520: // cross minus the left piece, bold vertical
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterVertical(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x2521: // cross minus the left piece, bold hook mid-top to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidRightTopHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2522: // cross minus the left piece, bold hook mid-bottom to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineTopEdge);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidRightBottomHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2524: // cross minus the right piece
		case 0x252B: // cross minus the right piece, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x2525: // cross minus the right piece, bold left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x2526: // cross minus the right piece, bold top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x2527: // cross minus the right piece, bold bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x2528: // cross minus the right piece, bold vertical
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterVertical(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x2529: // cross minus the right piece, bold hook mid-top to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidLeftTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x252A: // cross minus the right piece, bold hook mid-bottom to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x252C: // cross minus the top piece
		case 0x2533: // cross minus the top piece, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x252D: // cross minus the top piece, bold left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x252E: // cross minus the top piece, bold right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x252F: // cross minus the top piece, bold horizontal
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterHorizontal(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x2530: // cross minus the top piece, bold bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterHorizontal(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x2531: // cross minus the top piece, bold hook mid-bottom to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x2532: // cross minus the top piece, bold hook mid-bottom to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidRightBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x2534: // cross minus the bottom piece
		case 0x253B: // cross minus the bottom piece, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2535: // cross minus the bottom piece, bold left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x2536: // cross minus the bottom piece, bold right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x2537: // cross minus the bottom piece, bold horizontal
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterHorizontal(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x2538: // cross minus the bottom piece, bold top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterHorizontal(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x2539: // cross minus the bottom piece, bold hook mid-top to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidLeftTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x253A: // cross minus the bottom piece, bold hook mid-top to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidRightTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x253C: // cross
		case 0x254B: // cross, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendWithCenterHorizontal(aPath, metrics);
				}];
			}
			break;
		
		case 0x253D: // cross, bold left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendWithSegmentRight(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x253E: // cross, bold right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendWithSegmentLeft(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x253F: // cross, bold horizontal
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterHorizontal(aPath, metrics);
				}];
			}
			break;
		
		case 0x2540: // cross, bold top
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterHorizontal(aPath, metrics);
					extendWithSegmentDown(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x2541: // cross, bold bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterHorizontal(aPath, metrics);
					extendWithSegmentUp(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x2542: // cross, bold vertical
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterHorizontal(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterVertical(aPath, metrics);
				}];
			}
			break;
		
		case 0x2543: // cross, bold hook mid-top to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidLeftTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightBottomHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2544: // cross, bold hook mid-top to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidRightTopHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2545: // cross, bold hook mid-bottom to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidLeftBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidRightTopHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2546: // cross, bold hook mid-bottom to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithMidRightBottomHook(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithMidLeftTopHook(aPath, metrics);
				}];
			}
			break;
		
		case 0x2547: // cross, bold T-up
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterHorizontal(aPath, metrics);
					extendWithSegmentUp(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x2548: // cross, bold T-down
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterHorizontal(aPath, metrics);
					extendWithSegmentDown(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x2549: // cross, bold T-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterVertical(aPath, metrics);
					extendWithSegmentLeft(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x254A: // cross, bold T-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendWithCenterVertical(aPath, metrics);
					extendWithSegmentRight(aPath, metrics);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x254C: // horizontal double-dashed line
		case 0x254D: // horizontal double-dashed line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerWidth / 6.0);
					CGFloat const	kSegmentLength = (kSpaceLength * 2.0);
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, offset, metrics.layerHalfHeight,
										offset + kSegmentLength, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x254E: // vertical double-dashed line
		case 0x254F: // vertical double-dashed line, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kSpaceLength = (metrics.layerHeight / 6.0);
					CGFloat const	kSegmentLength = (kSpaceLength * 2.0);
					CGFloat			offset = kSpaceLength;
					
					
					metrics.targetLayer.lineCap = kCALineCapButt;
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
					offset += (kSegmentLength + kSpaceLength);
					extendPath(aPath, metrics.layerHalfWidth, offset,
										metrics.layerHalfWidth, offset + kSegmentLength);
				}];
			}
			break;
		
		case 0x2550: // middle line, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2551: // vertical line, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2552: // hook mid-right to mid-bottom, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY,
										metrics.layerHalfWidth, metrics.doubleHorizontalFirstY,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY,
										metrics.layerHalfWidth, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2553: // hook mid-right to mid-bottom, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.doubleVerticalFirstX, metrics.layerHalfHeight,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.layerHalfHeight,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2554: // hook mid-right to mid-bottom, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2555: // hook mid-left to mid-bottom, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.layerHalfWidth, metrics.doubleHorizontalFirstY,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.layerHalfWidth, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2556: // hook mid-left to mid-bottom, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.doubleVerticalSecondX, metrics.layerHalfHeight,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.layerHalfHeight,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2557: // hook mid-left to mid-bottom, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2558: // hook mid-top to mid-right, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
					extendPath(aPath, metrics.layerHalfWidth, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
				}];
			}
			break;
		
		case 0x2559: // hook mid-top to mid-right, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x255A: // hook mid-top to mid-right, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x255B: // hook mid-top to mid-left, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.doubleHorizontalSecondY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY);
					extendPath(aPath, metrics.layerHalfWidth, metrics.doubleHorizontalFirstY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY);
				}];
			}
			break;
		
		case 0x255C: // hook mid-top to mid-left, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.layerHalfHeight,
										metrics.squareLineLeftEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x255D: // hook mid-top to mid-left, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalFirstY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalSecondY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x255E: // cross minus the left piece, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendPath(aPath, metrics.layerHalfWidth, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.layerHalfWidth, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x255F: // cross minus the left piece, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2560: // cross minus the left piece, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2561: // cross minus the right piece, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithCenterVertical(aPath, metrics);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.layerHalfWidth, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.layerHalfWidth, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2562: // cross minus the right piece, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.doubleVerticalFirstX, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2563: // cross minus the right piece, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalFirstY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalSecondY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2564: // cross minus the top piece, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
					extendPath(aPath, metrics.layerHalfWidth, metrics.doubleHorizontalSecondY,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2565: // cross minus the top piece, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.layerHalfHeight,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.layerHalfHeight,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2566: // cross minus the top piece, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalSecondY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2567: // cross minus the bottom piece, double-horizontal-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x2568: // cross minus the bottom piece, double-vertical-only version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.layerHalfHeight);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.layerHalfHeight);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2569: // cross minus the bottom piece, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalFirstY,
										metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
				}];
			}
			break;
		
		case 0x256A: // cross, double-horizontal-only
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY);
					extendWithCenterVertical(aPath, metrics);
				}];
			}
			break;
		
		case 0x256B: // cross, double-vertical-only
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.doubleVerticalFirstX, metrics.squareLineTopEdge,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.doubleVerticalSecondX, metrics.squareLineTopEdge,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
					extendWithCenterHorizontal(aPath, metrics);
				}];
			}
			break;
		
		case 0x256C: // cross, double-line version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalFirstX, metrics.squareLineTopEdge);
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalFirstX, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalFirstX, metrics.squareLineBottomEdge);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalFirstY,
										metrics.doubleVerticalSecondX, metrics.squareLineTopEdge);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalSecondX, metrics.doubleHorizontalSecondY,
										metrics.doubleVerticalSecondX, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x256D: // curved mid-right to mid-bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.squareLineRightEdge, metrics.layerHalfHeight);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x256E: // curved mid-left to mid-bottom
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.squareLineLeftEdge, metrics.layerHalfHeight);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x256F: // curved mid-top to mid-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineLeftEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2570: // curved mid-top to mid-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathMoveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.squareLineTopEdge);
					CGPathAddQuadCurveToPoint(aPath, kMy_NoTransform, metrics.layerHalfWidth, metrics.layerHalfHeight,
												metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x2571: // diagonal line from top-right to bottom-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// target a region outside the cell (clipping occurs)
					// so that the line has a reasonable angle when
					// joining other cells that have diagonal lines
					extendPath(aPath,  metrics.squareLineLeftEdge - metrics.lineWidth, metrics.squareLineBottomEdge + metrics.lineWidth,
										metrics.squareLineRightEdge + metrics.lineWidth, metrics.squareLineTopEdge - metrics.lineWidth);
				}];
			}
			break;
		
		case 0x2572: // diagonal line from top-left to bottom-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// target a region outside the cell (clipping occurs)
					// so that the line has a reasonable angle when
					// joining other cells that have diagonal lines
					extendPath(aPath,  metrics.squareLineLeftEdge - metrics.lineWidth, metrics.squareLineTopEdge - metrics.lineWidth,
										metrics.squareLineRightEdge + metrics.lineWidth, metrics.squareLineBottomEdge + metrics.lineWidth);
				}];
			}
			break;
		
		case 0x2573: // diagonal lines from each corner crossing in the center
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// target a region outside the cell (clipping occurs)
					// so that the line has a reasonable angle when
					// joining other cells that have diagonal lines
					extendPath(aPath,  metrics.squareLineLeftEdge - metrics.lineWidth, metrics.squareLineTopEdge - metrics.lineWidth,
										metrics.squareLineRightEdge + metrics.lineWidth, metrics.squareLineBottomEdge + metrics.lineWidth);
					extendPath(aPath,  metrics.squareLineLeftEdge - metrics.lineWidth, metrics.squareLineBottomEdge + metrics.lineWidth,
										metrics.squareLineRightEdge + metrics.lineWidth, metrics.squareLineTopEdge - metrics.lineWidth);
				}];
			}
			break;
		
		case 0x2574: // cross, left segment only
		case 0x2578: // cross, left segment only, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentLeft(aPath, metrics);
				}];
			}
			break;
		
		case 0x2575: // cross, top segment only
		case 0x2579: // cross, top segment only, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentUp(aPath, metrics);
				}];
			}
			break;
		
		case 0x2576: // cross, right segment only
		case 0x257A: // cross, right segment only, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentRight(aPath, metrics);
				}];
			}
			break;
		
		case 0x2577: // cross, bottom segment only
		case 0x257B: // cross, bottom segment only, bold version
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendWithSegmentDown(aPath, metrics);
				}];
			}
			break;
		
		case 0x257C: // horizontal line, bold right half
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth + metrics.lineHalfWidth, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x257D: // vertical line, bold bottom half
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight + metrics.lineHalfWidth,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x257E: // horizontal line, bold left half
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth - metrics.lineHalfWidth, metrics.layerHalfHeight);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
				}];
			}
			break;
		
		case 0x257F: // vertical line, bold top half
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					[metrics standardBold];
					extendPath(aPath, metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth, metrics.layerHalfHeight - metrics.lineHalfWidth);
				}];
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					extendPath(aPath, metrics.layerHalfWidth, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge);
				}];
			}
			break;
		
		case 0x2580: // upper-half block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x2581: // 1/8 bottom block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 8.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHeight - kBlockHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2582: // 2/8 (1/4) bottom block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 4.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHeight - kBlockHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2583: // 3/8 bottom block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 8.0 * 3.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHeight - kBlockHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2584: // lower-half block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 8.0 * 4.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHalfHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x2585: // 5/8 bottom block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 8.0 * 5.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHeight - kBlockHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2586: // 6/8 (3/4) bottom block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 4.0 * 3.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHeight - kBlockHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2587: // 7/8 bottom block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 8.0 * 7.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHeight - kBlockHeight, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2588: // solid block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2589: // 7/8 left block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 8.0 * 7.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x258A: // 6/8 (3/4) left block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 4.0 * 3.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x258B: // 5/8 left block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 8.0 * 5.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x258C: // left-half block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerHalfWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x258D: // 3/8 left block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 8.0 * 3.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x258E: // 2/8 (1/4) left block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 4.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x258F: // 1/8 left block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 8.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2590: // right-half block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, 0, metrics.layerHalfWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2594: // 1/8 top block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockHeight = (metrics.layerHeight / 8.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerWidth, kBlockHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2595: // 1/8 right block
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGFloat const	kBlockWidth = (metrics.layerWidth / 8.0);
					
					
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerWidth - kBlockWidth, 0, kBlockWidth, metrics.layerHeight));
				} options:(kMy_GlyphDrawingOptionFill)];
			}
			break;
		
		case 0x2596: // quadrant lower-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x2597: // quadrant lower-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x2598: // quadrant upper-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x2599: // block minus upper-right quadrant
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x259A: // quadrants upper-left and lower-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x259B: // block minus lower-right quadrant
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x259C: // block minus lower-left quadrant
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x259D: // quadrant upper-right
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x259E: // quadrants upper-right and lower-left
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x259F: // block minus upper-left quadrant
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, 0, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerHalfWidth, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(0, metrics.layerHalfHeight, metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x25A0: // black square
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					CGPathAddRect(aPath, kMy_NoTransform, CGRectMake(metrics.layerWidth / 4.0, metrics.layerHeight / 4.0,
																		metrics.layerHalfWidth, metrics.layerHalfHeight));
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionNoStroke)];
			}
			break;
		
		case 0x25C6: // black diamond
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// draw a filled diamond (should be similar to 0x25C7)
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.squareLineLeftEdge, metrics.layerHalfHeight);
				} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x25C7: // white diamond
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// draw a hollow diamond (should be similar to 0x25C6)
					extendPath(aPath, metrics.squareLineLeftEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.squareLineRightEdge, metrics.layerHalfHeight);
					extendPath(aPath, metrics.squareLineRightEdge, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.squareLineLeftEdge, metrics.layerHalfHeight);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		case 0x25CA: // lozenge (narrower white diamond)
			{
				[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
				{
					// draw a hollow diamond (should be similar to 0x25C6)
					extendPath(aPath, metrics.layerHalfWidth / 2.0, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineTopEdge,
										metrics.layerHalfWidth / 2.0 * 3.0, metrics.layerHalfHeight);
					extendPath(aPath, metrics.layerHalfWidth / 2.0 * 3.0, metrics.layerHalfHeight,
										metrics.layerHalfWidth, metrics.squareLineBottomEdge,
										metrics.layerHalfWidth / 2.0, metrics.layerHalfHeight);
				} options:(kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
			}
			break;
		
		default:
			// ???
			Console_Warning(Console_WriteValueUnicodePoint, "TerminalGlyphDrawing_Layer not implemented for range [0x2500, 0x2600) and specified code point", aUnicodePoint);
			break;
		}
	}
}// addLayersForRangeHex2500To2600UnicodePoint:


#pragma mark New Methods: Partial Glyph Definitions


/*!
Equivalent to "addLayerUsingBlock:options:" where the fill flag is
not set and the stroke flag is set.  (Stroked paths are overwhelmingly
the most common so this form exists for convenience.)

(4.1)
*/
- (void)
addLayerUsingBlock:(My_ShapeDefinitionBlock)	aBlock
{
	[self addLayerUsingBlock:aBlock options:0];
}// addLayerUsingBlock:


/*!
Adds the specified block to a list of blocks that define layers.
The block is invoked whenever it is necessary to redefine the
layer’s content (if scaling is disabled) but it is invoked at
least once up front to set the initial appearance of the layer.

The fill flag determines if the layer's shape is filled, and the
stroke flag determines if the shape is stroked (if you only fill
then the shape will appear smaller than it would otherwise).  See
"addLayerUsingBlock:" as a short-cut for the common case of
clearing these flags.

NOTE:	This approach allows for complex shapes to be composed.
		For instance, simply having two lines of different widths
		would require two different layer definitions: one with
		the thinner line width and one with the thicker line.

(4.1)
*/
- (void)
addLayerUsingBlock:(My_ShapeDefinitionBlock)	aBlock
options:(My_GlyphDrawingOptions)				aFlagSet
{
	NSUInteger const	kFlagMask = (1 << sublayerBlocks_.count);
	
	
	assert(sublayerBlocks_.count < 32); // array size cannot exceed number of bits available
	
	//
	// since layers can be destroyed and rebuilt for new targets,
	// it is necessary to remember all of their flag values; each
	// layer’s settings are allowed to differ from siblings
	//
	
	if (aFlagSet & kMy_GlyphDrawingOptionFill)
	{
		filledSublayerFlags_ |= kFlagMask;
	}
	else
	{
		filledSublayerFlags_ &= ~kFlagMask;
	}
	
	if (aFlagSet & kMy_GlyphDrawingOptionNoStroke)
	{
		noStrokeSublayerFlags_ |= kFlagMask;
	}
	else
	{
		noStrokeSublayerFlags_ &= ~kFlagMask;
	}
	
	if (aFlagSet & kMy_GlyphDrawingOptionInset)
	{
		insetSublayerFlags_ |= kFlagMask;
	}
	else
	{
		insetSublayerFlags_ &= ~kFlagMask;
	}
	
	if (aFlagSet & kMy_GlyphDrawingOptionThickLine)
	{
		thickLineSublayerFlags_ |= kFlagMask;
	}
	else
	{
		thickLineSublayerFlags_ &= ~kFlagMask;
	}
	
	if (aFlagSet & kMy_GlyphDrawingOptionThinLine)
	{
		thinLineSublayerFlags_ |= kFlagMask;
	}
	else
	{
		thinLineSublayerFlags_ &= ~kFlagMask;
	}
	
	// TEMPORARY: using the 10.6 SDK (older buggy compiler) a block
	// that originated on the stack would be added to an NSArray as
	// a type of object that crashes when released; to avoid this
	// for now, a useless heap copy of the block is made
	[sublayerBlocks_ addObject:[[aBlock copy] autorelease]];
}// addLayerUsingBlock:options:


/*!
A convenience method for adding a path with the most common
options for drawing a single glyph as text: namely, the path
of the glyph is filled, it uses inset metrics to not touch
neighbor cells, and it has a thin line limit.

(4.1)
*/
- (void)
addLayerUsingText:(NSString*)	aGlyph
{
	[self addLayerUsingBlock:^(CGMutablePathRef aPath, TerminalGlyphDrawing_Metrics* metrics)
	{
		NSMutableAttributedString*		attributedString = [[[NSMutableAttributedString alloc]
															initWithString:aGlyph] autorelease];
		BOOL							renderOK = false;
		
		
		metrics.targetLayer.frame = CGRectInset(metrics.targetLayer.frame, metrics.lineWidth, metrics.lineWidth);
		renderOK = extendWithText(aPath, metrics, BRIDGE_CAST(attributedString, CFAttributedStringRef));
		if (NO == renderOK)
		{
			static BOOL		haveWarned = NO;
			
			
			if (NO == haveWarned)
			{
				Console_Warning(Console_WriteLine, "unexpectedly failed to extend a path using text");
				haveWarned = YES;
			}
		}
	} options:(kMy_GlyphDrawingOptionFill | kMy_GlyphDrawingOptionInset | kMy_GlyphDrawingOptionThinLine)];
}// addLayerUsingText:


#pragma mark New Methods: Other


/*!
If a sublayer is marked with "kMy_GlyphDrawingOptionInset",
this specifies the difference between the sublayer’s frame
and the normal frame.

This amount is occasionally useful when drawing, as a glyph
may want to have some portions that are consistent with a
neighbor-connected frame (the default) and others that are
consistent with an inset frame.

This method is also called to inset frames where appropriate.

(4.1)
*/
- (CGSize)
insetAmount
{
	return CGSizeMake(CGRectGetWidth(self.frame) / 4.0, CGRectGetHeight(self.frame) / 5.0);
}// insetAmount


/*!
Removes all sublayers (if any are defined) and rebuilds
them.  This can be called multiple times, typically
when the target rectangle changes.

The definition of a glyph is represented as a series of
blocks so that the build process can be as complex as
necessary.  While that kind of flexibility could also
have been achieved using subclasses, the block approach
requires a lot less code for all the different cases
and the blocks are only created if the corresponding
glyphs are used.

Note that technically a layer can be transformed at will
to suit any target rectangle but this would also produce
a relatively-low-quality rendering.  When layers are
rebuilt, using metrics recalculated for the new target
rectangle, the results are much better than for simple
scaling.  This is especially true for shapes that make
use of flags such as "isSmallSize", because the shape
itself may not even be exactly the same anymore.

(4.1)
*/
- (void)
rebuildLayers
{
	// the "color" property depends on the ability to
	// look at layers (which will be destroyed shortly)
	// so any original value is captured as needed
	CGColorRef const	kOldColor = self.color;
	NSMutableArray*		toDelete = [self.sublayers copy];
	
	
	// use a side array to avoid delete-during-iteration problems
	for (CALayer* layer in toDelete)
	{
		[layer removeFromSuperlayer];
	}
	[toDelete release], toDelete = nil;
	self.sublayers = nil;
	
	NSUInteger	i = 0;
	for (id object in sublayerBlocks_)
	{
		NSUInteger const			kFlagMask = (1 << i);
		My_ShapeDefinitionBlock		asBlock = STATIC_CAST(object, decltype(asBlock));
		CAShapeLayer*				layer = [CAShapeLayer layer];
		CGMutablePathRef			mutablePath = CGPathCreateMutable();
		
		
		layer.frame = CGRectMake(0, 0, CGRectGetWidth(self.frame), CGRectGetHeight(self.frame));
		if (0 != (insetSublayerFlags_ & kFlagMask))
		{
			// the inset amount is designed to avoid neighbor cells;
			// it is for convenience in renderers that are NOT trying
			// to create continuous, touching lines and want an easy
			// way to render a shape “comfortably inside” the cell;
			// note, this should not be changed without looking at how
			// renderers currently rely on its value (look for layers
			// added with "kMy_GlyphDrawingOptionInset")
			CGSize const	kInsets = [self insetAmount];
			
			
			layer.frame = CGRectInset(layer.frame, kInsets.width, kInsets.height);
		}
		
		// note: these layer properties are only defaults; any
		// shape definition block can override any setting
		CGFloat const	kMaxLineWidth = ((0 != (thickLineSublayerFlags_ & kFlagMask))
											? 4.0 // arbitrary
											: ((0 != (thinLineSublayerFlags_ & kFlagMask))
												? 2.0 // arbitrary
												: (self.frame.size.width / 5.0))); // arbitrary
		layer.lineWidth = ((self.frame.size.width * self.frame.size.height) / 240); // arbitrary; make lines thicker when text is larger
		if (layer.lineWidth > kMaxLineWidth)
		{
			layer.lineWidth = kMaxLineWidth;
		}
		if (NO == self.isAntialiasingDisabled)
		{
			// make anti-aliased lines slightly thicker so that they
			// blend better with glyphs that are not anti-aliased
			layer.lineWidth *= 1.4;
		}
		if (layer.lineWidth < 1.0)
		{
			layer.lineWidth = 1.0;
		}
		if (self.isBold)
		{
			// should match formula used in "standardBold" method
			layer.lineWidth *= 1.5;
		}
		layer.lineCap = kCALineCapSquare;
		layer.lineJoin = kCALineJoinMiter;
		layer.fillColor = (0 != (filledSublayerFlags_ & kFlagMask))
							? kOldColor
							: nil;
		layer.strokeColor = (0 != (noStrokeSublayerFlags_ & kFlagMask))
							? nil
							: kOldColor;
		
		// use the block to draw the path itself (and possibly
		// override any of the default layer settings above)
		{
			/*__weak*/ TerminalGlyphDrawing_Layer*	weakSelf = self;
			TerminalGlyphDrawing_Metrics*			metrics = [TerminalGlyphDrawing_Metrics
																metricsWithTargetLayer:layer owningLayer:weakSelf];
			
			
			(asBlock)(mutablePath, metrics);
		}
		layer.path = mutablePath;
		CGPathRelease(mutablePath), mutablePath = nullptr;
		
		[self addSublayer:layer];
		++i;
	}
}// rebuildLayers


@end //} TerminalGlyphDrawing_Layer (TerminalGlyphDrawing_LayerInternal)


#pragma mark -
@implementation TerminalGlyphDrawing_Metrics


@synthesize owningLayer = owningLayer_;
@synthesize targetLayer = targetLayer_;


#pragma mark Class Methods


/*!
Returns an autoreleased metrics object with the given
configuration.

(4.1)
*/
+ (instancetype)
metricsWithTargetLayer:(CAShapeLayer*)		aTargetLayer
owningLayer:(TerminalGlyphDrawing_Layer*)	anOwningLayer
{
	return [[[self.class allocWithZone:NULL]
				initWithTargetLayer:aTargetLayer owningLayer:anOwningLayer]
			autorelease];
}// metricsWithTargetLayer:owningLayer:


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithTargetLayer:(CAShapeLayer*)			aTargetLayer
owningLayer:(TerminalGlyphDrawing_Layer*)	anOwningLayer
{
	self = [super init];
	if (nil != self)
	{
		owningLayer_ = anOwningLayer;
		targetLayer_ = aTargetLayer;
	}
	return self;
}// initWithTargetLayer:owningLayer:


#pragma mark New Methods


/*!
Changes the line width of the "targetLayer" object to
be thicker than before.

Use this in blocks that wish to perform partial-bold
rendering.  (If the entire shape should be bold, simply
use "kTerminalGlyphDrawing_OptionBold" at the time the
owning layer is constructed, which will automatically
set the layer line width appropriately.)

(4.1)
*/
- (void)
standardBold
{
	self.targetLayer.lineWidth *= 1.5;
}// standardBold


#pragma mark Accessors: Layer Size


/*!
Returns half the height in pixels of the frame.

(4.1)
*/
- (CGFloat)
layerHalfHeight
{
	//return DECR_PIXEL(CGRectGetHeight(self->targetLayer_.frame) / 2.0);
	return (CGRectGetHeight(self->targetLayer_.frame) / 2.0);
}// layerHalfHeight


/*!
Returns half the width in pixels of the frame.

(4.1)
*/
- (CGFloat)
layerHalfWidth
{
	//return DECR_PIXEL(CGRectGetWidth(self->targetLayer_.frame) / 2.0);
	return (CGRectGetWidth(self->targetLayer_.frame) / 2.0);
}// layerHalfWidth


/*!
Returns the height in pixels of the frame.

(4.1)
*/
- (CGFloat)
layerHeight
{
	return CGRectGetHeight(self->targetLayer_.frame);
}// layerHeight


/*!
Returns the width in pixels of the frame.

(4.1)
*/
- (CGFloat)
layerWidth
{
	return CGRectGetWidth(self->targetLayer_.frame);
}// layerWidth


#pragma mark Accessors: Default Line Size


/*!
Returns half of a “normal” line width, which is primarily
useful for aligning lines that overlap perpendicularly.

(4.1)
*/
- (CGFloat)
lineHalfWidth
{
	return (self->targetLayer_.lineWidth / 2.0);
}// lineHalfWidth


/*!
Returns the “normal” thickness of drawn lines (as
opposed to metrics for boldface or other uses).

(4.1)
*/
- (CGFloat)
lineWidth
{
	return self->targetLayer_.lineWidth;
}// lineWidth


#pragma mark Accessors: Rendering Pairs of Lines


/*!
Returns the vertical location of the upper horizontal
line in a double-horizontal-line scenario.

(4.1)
*/
- (CGFloat)
doubleHorizontalFirstY
{
	return DECR_PIXEL(self.layerHalfHeight - self.lineWidth);
}// doubleHorizontalFirstY


/*!
Returns the vertical location of the lower horizontal
line in a double-horizontal-line scenario.

(4.1)
*/
- (CGFloat)
doubleHorizontalSecondY
{
	CGFloat		result = 0;
	
	
	if (self->owningLayer_.isAntialiasingDisabled)
	{
		// without anti-aliasing, arbitrarily force a single-pixel space
		// compared to the other line so that the split is legible
		result = (self.doubleHorizontalFirstY + std::max(2.0 * self.lineWidth + 1, 1.0));
	}
	else
	{
		CGFloat		extraOffset = 0;
		
		
		if (self->owningLayer_.isBold)
		{
			// make sure the thicker lines are still separated
			extraOffset += 0.2 * self.lineWidth;
		}
		
		result = INCR_PIXEL(self.doubleHorizontalFirstY + 1.5 * self.lineWidth + extraOffset);
	}
	
	return result;
}// doubleHorizontalSecondY


/*!
Returns the horizontal location of the left horizontal
line in a double-vertical-line scenario.

(4.1)
*/
- (CGFloat)
doubleVerticalFirstX
{
	return DECR_PIXEL(self.layerHalfWidth - self.lineWidth);
}// doubleVerticalFirstX


/*!
Returns the horizontal location of the right horizontal
line in a double-vertical-line scenario.

(4.1)
*/
- (CGFloat)
doubleVerticalSecondX
{
	CGFloat		result = 0;
	
	
	if (self->owningLayer_.isAntialiasingDisabled)
	{
		// without anti-aliasing, arbitrarily force a single-pixel space
		// compared to the other line so that the split is legible
		result = (self.doubleVerticalFirstX + std::max(2.0 * self.lineWidth + 1, 1.0));
	}
	else
	{
		CGFloat		extraOffset = 0;
		
		
		if (self->owningLayer_.isBold)
		{
			// make sure the thicker lines are still separated
			extraOffset += 0.2 * self.lineWidth;
		}
		
		result = INCR_PIXEL(self.doubleVerticalFirstX + 1.5 * self.lineWidth + extraOffset);
	}
	
	return result;
}// doubleVerticalSecondX


#pragma mark Accessors: Connecting Lines to Cell Edges


/*!
Use this offset whenever drawing the ends of lines that are
intended to appear adjacent to a different-width line (a
normal-width layer adjacent to a bold one, or vice-versa).

(4.1)
*/
- (CGFloat)
squareLineBoldNormalJoin
{
	// this should be half as wide as the thickness chosen
	// for lines in "standardBold"
	return (self.lineWidth * 0.75);
}// squareLineBoldNormalJoin


/*!
Returns the vertical coordinate that should be used to
start drawing a squared-off line from the bottom edge of
the frame (with the intent to produce a line that starts
from the bottom edge of the visible glyph rectangle).

This is “outside” the cell because clipping is assumed
and because lines are usually supposed to connect to a
neighboring line seamlessly.  Any edge anti-aliasing will
be removed by the clipping of the overrun line.

(4.1)
*/
- (CGFloat)
squareLineBottomEdge
{
	//return DECR_PIXEL(self.layerHeight - self.lineHalfWidth);
	return (self.layerHeight - self.lineHalfWidth);
}// squareLineBottomEdge


/*!
Returns the horizontal coordinate that should be used to
start drawing a squared-off line from the left edge of
the frame (with the intent to produce a line that starts
from the left edge of the visible glyph rectangle).

This is “outside” the cell because clipping is assumed
and because lines are usually supposed to connect to a
neighboring line seamlessly.  Any edge anti-aliasing will
be removed by the clipping of the overrun line.

(4.1)
*/
- (CGFloat)
squareLineLeftEdge
{
	//return INCR_PIXEL(self.lineHalfWidth);
	return self.lineHalfWidth;
}// squareLineLeftEdge


/*!
Returns the horizontal coordinate that should be used to
start drawing a squared-off line from the right edge of
the frame (with the intent to produce a line that starts
from the right edge of the visible glyph rectangle).

This is “outside” the cell because clipping is assumed
and because lines are usually supposed to connect to a
neighboring line seamlessly.  Any edge anti-aliasing will
be removed by the clipping of the overrun line.

(4.1)
*/
- (CGFloat)
squareLineRightEdge
{
	//return DECR_PIXEL(self.layerWidth - self.lineHalfWidth);
	return (self.layerWidth - self.lineHalfWidth);
}// squareLineRightEdge


/*!
Returns the vertical coordinate that should be used to
start drawing a squared-off line from the top edge of
the frame (with the intent to produce a line that starts
from the top of the visible glyph rectangle).

This is “outside” the cell because clipping is assumed
and because lines are usually supposed to connect to a
neighboring line seamlessly.  Any edge anti-aliasing will
be removed by the clipping of the overrun line.

(4.1)
*/
- (CGFloat)
squareLineTopEdge
{
	//return INCR_PIXEL(self.lineHalfWidth);
	return self.lineHalfWidth;
}// squareLineTopEdge


#pragma mark Accessors: Text Properties


/*!
Returns the "baselineHint" property of the owning layer,
which is relative to the base Y coordinate used for drawing.
This allows vector graphics to appear aligned with text.

WARNING:	This property is not accurate when the metrics
			have been inset by "kMy_GlyphDrawingOptionInset"
			because the baseline hint came from the original
			frame rectangle.  The "insetAmount" method of the
			TerminalGlyphDrawing_Layer class can be used to
			bridge drawing that must consider both regions.

(4.1)
*/
- (CGFloat)
baselineHint
{
	return self.owningLayer.baselineHint;
}// baselineHint


@end //} TerminalGlyphDrawing_Metrics

// BELOW IS REQUIRED NEWLINE TO END FILE
