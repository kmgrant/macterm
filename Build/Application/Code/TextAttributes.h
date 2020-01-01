/*!	\file TextAttributes.h
	\brief Manages the characteristics of a range of text in a
	terminal view.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// library includes
#include <Console.h>



#pragma mark Constants

/*!
Constant values used in attributes.  (These should be
consistent with documented bits below.)
*/
enum
{
	kTextAttributes_ValueDoubleHeightBottom		= 0x03,
	kTextAttributes_ValueDoubleHeightTop		= 0x02,
	kTextAttributes_ValueDoubleWidth			= 0x01
};

/*!
The limit on bitmap ID is imposed to avoid requiring a
large number of attribute bits.  See documentation on
TextAttributes_BitmapID.
*/
enum
{
	kTextAttributes_BitmapIDBits			= 22,
	kTextAttributes_BitmapIDMaximum			= ((1 << kTextAttributes_BitmapIDBits) - 1)
};

/*!
The limit on true color is imposed to avoid requiring a
large number of attribute bits.  See documentation on
TextAttributes_TrueColorID.
*/
enum
{
	kTextAttributes_TrueColorBits			= 11,
	kTextAttributes_TrueColorIDMaximum		= ((1 << kTextAttributes_TrueColorBits) - 1)
};

#pragma mark Types

/*!
As bitmap specifications are received through terminal
sequences, new bitmap definitions are created and assigned
ID numbers.  Renderers may determine the bitmap using the
Terminal_BitmapGetFromID() API.

There is a mask value applied in the attributes so the ID
may not actually support the same maximum size as its
integer type; see "kTextAttributes_BitmapIDMaximum".

NOTE:	Bitmap IDs are reused after a time.  In theory this
		could mean that images could change their rendering,
		especially for old scrollback lines in large buffers
		where lots of unique bitmaps are seen.  This is
		considered an acceptable trade-off to avoid a more
		complex scheme for remembering the bitmap ID values
		of every bitmap segment in the terminal.
*/
typedef UInt32 TextAttributes_BitmapID;

/*!
As color specifications are received through terminal
sequences, new color definitions are created and assigned
ID numbers.  Renderers may determine the color using the
Terminal_TrueColorGetFromID() API.

There is a mask value applied in the attributes so the ID
may not actually support the same maximum size as its
integer type; see "kTextAttributes_TrueColorIDMaximum".

This is intentionally more compact than the original 24-bit
specification (therefore limiting the total number of color
combinations in terminals).  The goal is to consume fewer
bits to associate a true color with its text.

NOTE:	Color IDs are reused after a time.  In theory this
		could mean that text could change its rendering,
		especially for old scrollback lines in large buffers
		where lots of unique colors are seen.  This is
		considered an acceptable trade-off to avoid a more
		complex scheme for remembering the true color values
		of every piece of text in the terminal.
*/
typedef UInt16 TextAttributes_TrueColorID;

/*!
Terminal Attribute Bits

IMPORTANT:	Do not directly access these bits, use the masks
			and accessors defined below.  If the bits must
			change, be sure to fix the accessors!

These bits are used to define the current text attributes when
rendering, and are primarily used in the emulator data loop and
in terminal views.

Attributes that are “line global” in nature, such as double-size
text, are represented for convenience when ascertaining style of
chunks of text that do not span an entire line; however, the
implementation should not allow line-global attributes to vary
for any chunk of text on the same line.

IMPORTANT:	The bit ranges documented below should match the
			corresponding constant definitions.

Upper 32-bit range ("_upper" field):
<pre>
[BACKGROUND]                       [FOREGROUND]                          [B][F]  [T][UNUSED]  [UNUSED][INV.]
31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16    15 14 13 12  11 10  9  8   7  6  5  4   3  2  1  0
─┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼─────┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼─
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  └─── 0: if set, all bits are INVALID
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  └──┴───┴──┴──┴────── 5-1: UNDEFINED; set to 0
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  └───────── 6: color index is TextAttributes_BitmapID?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   └──────────── 7: color index is TextAttributes_TrueColorID?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  └─── 8: use custom foreground color index (bits 20-10)?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  └────── 9: use custom background color index (bits 31-21)?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  └───┴──┴──┴──┴─────┴──┴──┴──┴───┴──┴───────── 20-10: index for unique foreground color from a palette [1];
 │  │  │  │   │  │  │  │   │  │  │                                                       or, if bit 7 is set, a TextAttributes_TrueColorID;
 │  │  │  │   │  │  │  │   │  │  │                                                       or, if bit 6 is set, lower bits of TextAttributes_BitmapID
 │  │  │  │   │  │  │  │   │  │  │                                                       (it may not be a combination of these)
 │  │  │  │   │  │  │  │   │  │  │
 └──┴──┴──┴───┴──┴──┴──┴───┴──┴──┴─────────────────── 31-21: index for unique background color from a palette [1];
                                                             or, if bit 7 is set, a TextAttributes_TrueColorID;
                                                             or, if bit 6 is set, upper bits of TextAttributes_BitmapID
                                                             (it may not be a combination of these)
</pre>

Lower 32-bit range ("_lower" field):
<pre>
[UNUSED]     [UNUSED]     [UNUSED]     [UNUSED]     [E][SL][SR][GR] [DBL][UNUSED][STYLE BITS]
31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16    15 14 13 12  11 10  9  8   7  6  5  4   3  2  1  0
─┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼─────┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼─
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  └─── 0: bold?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  └────── 1: UNDEFINED; set to 0
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  └───────── 2: italic?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   └──────────── 3: underlined?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  └─── 4: blinking?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  └────── 5: UNDEFINED; set to 0
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  └───────── 6: inverse video?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   └──────────── 7: concealed (invisible)?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  └──┴─── 9-8:   UNDEFINED; set to 0
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   └──┴───────── 11─10: double text mode (2 bits, see [2]); LINE-GLOBAL
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  └─── 12: VT graphics enabled?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  └────── 13: is selected as a search result?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  └───────── 14: is selected by the user (for copy, print, etc.)?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     └──────────── 15: is prohibited from being erased by selective erases
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │
 └──┴──┴──┴───┴──┴──┴──┴───┴──┴──┴──┴───┴──┴──┴──┴─── 31-16: UNDEFINED; set to 0

[1] The base 8 colors are 3-bit ANSI color values that can be one
of the following (the exact RGB components of which may be
customized by the user):
	000 (0)		black			100 (4)		blue
	001 (1)		red				101 (5)		magenta
	010 (2)		green			110 (6)		cyan
	011 (3)		yellow			111 (7)		white
The values 8-15 are also recognized for the “bold” color values.
On terminals that support 256 colors, any number from 0 to 255 is
valid, referring to the 256-color table maintained by the terminal.
Space has been allocated for larger index values in the future.
Note that if the “color index is TextAttributes_TrueColorID” bit is
set, this field is actually an TextAttributes_TrueColorID that can
be resolved to a set of high-precision color components.  And, if
the “color index is TextAttributes_BitmapID” bit is set, the bits
of both the foreground and background portions combine to represent
a TextAttributes_BitmapID indicating that the terminal cell renders
a portion of a bitmap image.

[2] The 2-bit double text mode values can be one of the following
(but please use defined constants instead of these numbers):
	00 (0)		normal			10 (2)		text is top half of double height
	01 (1)		double width	11 (3)		text is bottom half of double height
</pre>
*/
struct TextAttributes_Object
{
	//! Although not strictly necessary, this class makes it easier
	//! to keep shifts and masks consistent across all uses (and
	//! it will not conflict with TextAttributes_Object constructors
	//! that accept integers).  It also provides a good place for
	//! helper methods that can handle two 32-bit halves.
	struct BitRange
	{
		explicit inline
		BitRange	(UInt32, UInt8);
		
		inline void
		addExclusivelyTo	(UInt32&, UInt32&, UInt32) const;
		
		inline void
		addTo	(UInt32&, UInt32&, UInt32) const;
		
		inline void
		clearFrom	(UInt32&, UInt32&) const;
		
		inline UInt32
		returnValue		(UInt32, UInt32) const;
		
		inline void
		removeFrom	(UInt32&, UInt32&, UInt32) const;
		
		UInt32	_mask;
		UInt8	_shift;
	};
	
	
	explicit inline
	TextAttributes_Object ();
	
	explicit inline
	TextAttributes_Object	(UInt32);
	
	inline
	TextAttributes_Object	(UInt32, UInt32);
	
	inline
	TextAttributes_Object	(BitRange, UInt32);
	
	inline bool
	operator ==	(TextAttributes_Object const&) const;
	
	inline bool
	operator !=	(TextAttributes_Object const&) const;
	
	inline void
	addAttributes	(TextAttributes_Object);
	
	inline TextAttributes_BitmapID
	bitmapID () const;
	
	inline void
	bitmapIDSet	(TextAttributes_BitmapID);
	
	inline void
	clear ();
	
	inline TextAttributes_TrueColorID
	colorIDBackground () const;
	
	inline void
	colorIDBackgroundSet	(TextAttributes_TrueColorID);
	
	inline TextAttributes_TrueColorID
	colorIDForeground () const;
	
	inline void
	colorIDForegroundSet	(TextAttributes_TrueColorID);
	
	inline UInt16
	colorIndexBackground () const;
	
	inline void
	colorIndexBackgroundCopyFrom	(TextAttributes_Object);
	
	inline void
	colorIndexBackgroundSet		(UInt16);
	
	inline UInt16
	colorIndexForeground () const;
	
	inline void
	colorIndexForegroundSet		(UInt16);
	
	inline bool
	hasAttributes	(TextAttributes_Object) const;
	
	inline bool
	hasBitmap () const;
	
	inline bool
	hasBlink () const;
	
	inline bool
	hasBold () const;
	
	inline bool
	hasConceal () const;
	
	inline bool
	hasDoubleAny () const;
	
	inline bool
	hasDoubleHeightBottom () const;
	
	inline bool
	hasDoubleHeightTop () const;
	
	inline bool
	hasDoubleWidth () const;
	
	inline bool
	hasItalic () const;
	
	inline bool
	hasSearchHighlight () const;
	
	inline bool
	hasSelection () const;
	
	inline bool
	hasUnderline () const;
	
	inline void
	removeAttributes	(TextAttributes_Object);
	
	inline void
	removeImageRelatedAttributes ();
	
	inline void
	removeStyleAndColorRelatedAttributes ();
	
	inline UInt32
	returnValueInRange	(TextAttributes_Object::BitRange) const;

private:
	UInt32	_upper;
	UInt32	_lower;
};

//! the mask and shift for the bits required to represent any double-text value
TextAttributes_Object::BitRange const	kTextAttributes_MaskDoubleText(0x03, 10 + 0);

//! the mask and shift for the bits required to represent any bitmap ID value
TextAttributes_Object::BitRange const	kTextAttributes_MaskBitmapID(kTextAttributes_BitmapIDMaximum, 64 - kTextAttributes_BitmapIDBits);

//! the mask and shift for the bits required to represent any color index value
//! (this must be at least as large as TextAttributes_TrueColorID)
TextAttributes_Object::BitRange const	kTextAttributes_MaskColorIndexBackground(kTextAttributes_TrueColorIDMaximum, 64 - kTextAttributes_TrueColorBits);
TextAttributes_Object::BitRange const	kTextAttributes_MaskColorIndexForeground(kTextAttributes_TrueColorIDMaximum, 64 - 2 * kTextAttributes_TrueColorBits);

//
// IMPORTANT: The constant bit ranges chosen below should match
// the bit-range documentation block above.
//

//! indicates that ALL the attribute bits are undefined
TextAttributes_Object const		kTextAttributes_Invalid
								(0x00000001,	0);

//! is text marked as do-not-touch by selective erase sequences?
TextAttributes_Object const		kTextAttributes_CannotErase
								(0,				0x00008000);

//! if set, foreground AND background index combine to form TextAttributes_BitmapID values
TextAttributes_Object const		kTextAttributes_ColorIndexIsBitmapID
								(0x00000040,	0);

//! if set, foreground and background index are TextAttributes_TrueColorID values
TextAttributes_Object const		kTextAttributes_ColorIndexIsTrueColorID
								(0x00000080,	0);

//! if the bits in the range "kTextAttributes_MaskDoubleText" are
//! equal to this, the bottom half of double-width and double-height
//! text is to be rendered
TextAttributes_Object const		kTextAttributes_DoubleHeightBottom
								(kTextAttributes_MaskDoubleText, kTextAttributes_ValueDoubleHeightBottom);

//! if the bits in the range "kTextAttributes_MaskDoubleText" are
//! equal to this, the top half of double-width and double-height
//! text is to be rendered
TextAttributes_Object const		kTextAttributes_DoubleHeightTop
								(kTextAttributes_MaskDoubleText, kTextAttributes_ValueDoubleHeightTop);

//! if the bits in the range "kTextAttributes_MaskDoubleText" are
//! equal to this, double-width, single-height text is to be rendered
TextAttributes_Object const		kTextAttributes_DoubleWidth
								(kTextAttributes_MaskDoubleText, kTextAttributes_ValueDoubleWidth);

//! for convenience, bits to cover all possible double-text settings
TextAttributes_Object const		kTextAttributes_DoubleTextAll
								(kTextAttributes_MaskDoubleText, 0x03);

//! if set, the background color index applies
TextAttributes_Object const		kTextAttributes_EnableBackground
								(0x00000100,	0);

//! if set, the foreground color index applies
TextAttributes_Object const		kTextAttributes_EnableForeground
								(0x00000200,	0);

//! is text highlighted as being part of a search result?
TextAttributes_Object const		kTextAttributes_SearchHighlight
								(0,				0x00002000);

//! is text highlighted as being part of the selection?
TextAttributes_Object const		kTextAttributes_Selected
								(0,				0x00004000);

//! is text blinking, using blinking colors?
TextAttributes_Object const		kTextAttributes_StyleBlinking
								(0,				0x00000010);

//! is text in boldface, using emphasized colors?
TextAttributes_Object const		kTextAttributes_StyleBold
								(0,				0x00000001);

//! is text using same foreground and background?
TextAttributes_Object const		kTextAttributes_StyleConceal
								(0,				0x00000080);

//! are foreground and background colors flipped?
TextAttributes_Object const		kTextAttributes_StyleInverse
								(0,				0x00000040);

//! is text in italics?
TextAttributes_Object const		kTextAttributes_StyleItalic
								(0,				0x00000004);

//! is text underlined?
TextAttributes_Object const		kTextAttributes_StyleUnderline
								(0,				0x00000008);

//! should VT graphics be used? (exact glyphs depend on the current
//! terminal; for example, graphics are different for VT52 than VT100)
TextAttributes_Object const		kTextAttributes_VTGraphics
								(0,				0x00001000);


#pragma mark Public Methods

/*!
Describes a range of bits within a larger space that can be
as wide as 64 bits.

IMPORTANT:	Although you can store any mask/shift combination,
			helper methods do not handle any bit combinations
			that would straddle the boundaries between two
			32-bit halves.  For maximum convenience, the shift
			must keep the mask value within one 32-bit half.
*/
TextAttributes_Object::BitRange::BitRange	(UInt32		inMask,
											 UInt8		inShift)
: _mask(inMask)
, _shift(inShift)
{
	assert(inShift < 63);
}// TextAttributes_Object::BitRange constructor


/*!
A short-cut for calling clearFrom() and addTo(): existing
bits in the mask range are all cleared before the new value
is added with bitwise-OR.  Other bits are untouched.

IMPORTANT:	This does not handle shift/mask combinations
			that would straddle the boundaries between the
			two halves.  The shift must not cause the mask
			value to exceed one 32-bit half or it will be
			clipped.

(4.1)
*/
void
TextAttributes_Object::BitRange::addExclusivelyTo	(UInt32&	inoutUpper,
													 UInt32&	inoutLower,
													 UInt32		inValue)
const
{
	clearFrom(inoutUpper, inoutLower);
	addTo(inoutUpper, inoutLower, inValue);
}// addExclusivelyTo


/*!
Performs a bitwise-OR of the specified value with the given
target upper/lower ranges, applying the mask and shift as
appropriate.  If the shift is greater than or equal to 32,
the value is applied to the upper argument; otherwise, it
applies to the lower argument.

IMPORTANT:	This does not handle shift/mask combinations
			that would straddle the boundaries between the
			two halves.  The shift must not cause the mask
			value to exceed one 32-bit half or it will be
			clipped.

(4.1)
*/
void
TextAttributes_Object::BitRange::addTo	(UInt32&	inoutUpper,
										 UInt32&	inoutLower,
										 UInt32		inValue)
const
{
	if (_shift >= 32)
	{
		inoutUpper |= ((inValue & _mask) << (_shift - 32));
	}
	else
	{
		inoutLower |= ((inValue & _mask) << _shift);
	}
}// addTo


/*!
This is a short-cut for calling removeFrom() using a value
of the mask; in other words, it clears the masked bits and
leaves other bits untouched.

(4.1)
*/
void
TextAttributes_Object::BitRange::clearFrom	(UInt32&	inoutUpper,
											 UInt32&	inoutLower)
const
{
	removeFrom(inoutUpper, inoutLower, _mask);
}// clearFrom


/*!
Returns the value of the masked region of the given bits,
after shifting.  If the shift is greater than or equal to
32, the value comes from the upper argument; otherwise, it
comes from the lower argument.

IMPORTANT:	This does not handle shift/mask combinations
			that would straddle the boundaries between the
			two halves.  The shift must not cause the mask
			value to exceed one 32-bit half or it will be
			clipped.

(4.1)
*/
UInt32
TextAttributes_Object::BitRange::returnValue	(UInt32		inUpper,
												 UInt32		inLower)
const
{
	UInt32		result = 0;
	
	
	if (_shift >= 32)
	{
		result = ((inUpper >> (_shift - 32)) & _mask);
	}
	else
	{
		result = ((inLower >> _shift) & _mask);
	}
	
	return result;
}// returnValue


/*!
Performs a bitwise-AND of the negation of the specified value
with the given target upper/lower ranges, applying the mask
and shift as appropriate.  If the shift is greater than or
equal to 32, the value is applied to the upper argument;
otherwise, it applies to the lower argument.

IMPORTANT:	This does not handle shift/mask combinations
			that would straddle the boundaries between the
			two halves.  The shift must not cause the mask
			value to exceed one 32-bit half or it will be
			clipped.

(4.1)
*/
void
TextAttributes_Object::BitRange::removeFrom		(UInt32&	inoutUpper,
												 UInt32&	inoutLower,
												 UInt32		inValue)
const
{
	if (_shift >= 32)
	{
		inoutUpper &= ~((inValue & _mask) << (_shift - 32));
	}
	else
	{
		inoutLower &= ~((inValue & _mask) << _shift);
	}
}// removeFrom


/*!
Sets all bits to zero.

(4.1)
*/
TextAttributes_Object::TextAttributes_Object ()
: TextAttributes_Object(0, 0)
{
}// TextAttributes_Object default constructor


/*!
Sets the lower bits as specified, and the upper bits to zero.

(4.1)
*/
TextAttributes_Object::TextAttributes_Object	(UInt32		inLower)
: TextAttributes_Object(0, inLower)
{
}// TextAttributes_Object 1-argument constructor


/*!
Sets all the bits as specified.

Designated initializer.

(4.1)
*/
TextAttributes_Object::TextAttributes_Object	(UInt32		inUpper,
												 UInt32		inLower)
: _upper(inUpper)
, _lower(inLower)
{
}// TextAttributes_Object 2-argument constructor (UInt32, UInt32)


/*!
Initializes bits by shifting the given value into the specified range.

WARNING:	This does NOT yet support shifts that cross the boundary
			between the upper 32 bits and the lower 32 bits.  The
			mask may span more than one bit and the shift may be
			greater than 32 but a “wide” mask should not be shifted
			such that part of the range would straddle the boundary
			between the upper and lower 32-bit ranges.  If you have
			bits that require this, use a more explicit constructor
			that specifies exact upper and lower values.

(4.1)
*/
TextAttributes_Object::TextAttributes_Object	(BitRange	inRange,
												 UInt32		inValue)
: _upper(0)
, _lower(0)
{
	inRange.addTo(_upper, _lower, inValue);
}// TextAttributes_Object 2-argument constructor (BitRange, UInt32)


/*!
Equality check.

(4.1)
*/
bool
TextAttributes_Object::operator ==	(TextAttributes_Object const&	inOther)
const
{
	return ((_upper == inOther._upper) && (_lower == inOther._lower));
}// operator ==


/*!
Not-equal check.

(4.1)
*/
bool
TextAttributes_Object::operator !=	(TextAttributes_Object const&	inOther)
const
{
	return (false == this->operator ==(inOther));
}// operator !=


/*!
Changes this object’s attributes to include the specified
attributes.

(4.1)
*/
void
TextAttributes_Object::addAttributes	(TextAttributes_Object	inAttributes)
{
	_upper |= inAttributes._upper;
	_lower |= inAttributes._lower;
}// addAttributes


/*!
Returns the bitmap ID for rendering a portion of an image.

IMPORTANT:	You must only call this for attributes that set
			the "kTextAttributes_ColorIndexIsBitmapID" bit.

(2017.12)
*/
TextAttributes_BitmapID
TextAttributes_Object::bitmapID ()
const
{
	assert(this->hasAttributes(kTextAttributes_ColorIndexIsBitmapID));
	return STATIC_CAST(this->returnValueInRange(kTextAttributes_MaskBitmapID), TextAttributes_BitmapID);
}// bitmapID


/*!
Changes the bitmap ID for rendering a portion of an image,
adding the "kTextAttributes_ColorIndexIsBitmapID" bit.

(2017.12)
*/
void
TextAttributes_Object::bitmapIDSet		(TextAttributes_BitmapID	inID)
{
	kTextAttributes_MaskBitmapID.addExclusivelyTo(_upper, _lower, inID);
	this->addAttributes(kTextAttributes_ColorIndexIsBitmapID);
	assert(bitmapID() == inID); // debug
}// bitmapIDSet


/*!
Sets all attributes to zero.

(4.1)
*/
void
TextAttributes_Object::clear ()
{
	_upper = 0;
	_lower = 0;
}// clear


/*!
Returns the true-color ID for rendering the background (cell).

IMPORTANT:	You must only call this for attributes that set
			the "kTextAttributes_ColorIndexIsTrueColorID" bit.

(4.1)
*/
TextAttributes_TrueColorID
TextAttributes_Object::colorIDBackground ()
const
{
	assert(this->hasAttributes(kTextAttributes_ColorIndexIsTrueColorID));
	return STATIC_CAST(colorIndexBackground(), TextAttributes_TrueColorID);
}// colorIDBackground


/*!
Changes the true-color ID for rendering the foreground (text),
adding the "kTextAttributes_ColorIndexIsTrueColorID" bit.

(4.1)
*/
void
TextAttributes_Object::colorIDBackgroundSet		(TextAttributes_TrueColorID		inID)
{
	colorIndexBackgroundSet(STATIC_CAST(inID, UInt16));
	this->addAttributes(kTextAttributes_ColorIndexIsTrueColorID);
	assert(colorIDBackground() == inID);
}// colorIDBackgroundSet


/*!
Returns the true-color ID for rendering the foreground (text).

IMPORTANT:	You must only call this for attributes that set
			the "kTextAttributes_ColorIndexIsTrueColorID" bit.

(4.1)
*/
TextAttributes_TrueColorID
TextAttributes_Object::colorIDForeground ()
const
{
	assert(this->hasAttributes(kTextAttributes_ColorIndexIsTrueColorID));
	return STATIC_CAST(colorIndexForeground(), TextAttributes_TrueColorID);
}// colorIDForeground


/*!
Changes the true-color ID for rendering the foreground (text),
adding the "kTextAttributes_ColorIndexIsTrueColorID" bit.

(4.1)
*/
void
TextAttributes_Object::colorIDForegroundSet		(TextAttributes_TrueColorID		inID)
{
	colorIndexForegroundSet(STATIC_CAST(inID, UInt16));
	this->addAttributes(kTextAttributes_ColorIndexIsTrueColorID);
	assert(colorIDForeground() == inID);
}// colorIDForegroundSet


/*!
Returns the background-index portion of the attributes.

(4.1)
*/
UInt16
TextAttributes_Object::colorIndexBackground ()
const
{
	return STATIC_CAST(this->returnValueInRange(kTextAttributes_MaskColorIndexBackground), UInt16);
}// colorIndexBackground


/*!
Sets the background-index portion of the attributes by
copying the relevant bits from another set of attributes.

(4.1)
*/
void
TextAttributes_Object::colorIndexBackgroundCopyFrom		(TextAttributes_Object		inSourceAttributes)
{
	_upper &= ~(kTextAttributes_EnableBackground._upper | kTextAttributes_ColorIndexIsTrueColorID._upper);
	_upper |= (inSourceAttributes._upper & (kTextAttributes_EnableBackground._upper | kTextAttributes_ColorIndexIsTrueColorID._upper));
	kTextAttributes_MaskColorIndexBackground.addExclusivelyTo(_upper, _lower,
																inSourceAttributes.returnValueInRange(kTextAttributes_MaskColorIndexBackground));
}// colorIndexBackgroundCopyFrom


/*!
Sets the background-index portion of the attributes, clearing
the "kTextAttributes_ColorIndexIsTrueColorID" bit.

If this should remain true-color, use colorIDBackgroundSet().

(4.1)
*/
void
TextAttributes_Object::colorIndexBackgroundSet	(UInt16		inIndex)
{
	_upper &= ~(kTextAttributes_ColorIndexIsTrueColorID._upper);
	kTextAttributes_MaskColorIndexBackground.addExclusivelyTo(_upper, _lower, inIndex);
	_upper |= (kTextAttributes_EnableBackground._upper);
	assert(colorIndexBackground() == inIndex); // debug
}// colorIndexBackgroundSet


/*!
Returns the foreground-index portion of the attributes.

(4.1)
*/
UInt16
TextAttributes_Object::colorIndexForeground ()
const
{
	return STATIC_CAST(this->returnValueInRange(kTextAttributes_MaskColorIndexForeground), UInt16);
}// colorIndexForeground


/*!
Sets the foreground-index portion of the attributes, clearing
the "kTextAttributes_ColorIndexIsTrueColorID" bit.

If this should remain true-color, use colorIDForegroundSet().

(4.1)
*/
void
TextAttributes_Object::colorIndexForegroundSet	(UInt16		inIndex)
{
	_upper &= ~(kTextAttributes_ColorIndexIsBitmapID._upper);
	_upper &= ~(kTextAttributes_ColorIndexIsTrueColorID._upper);
	kTextAttributes_MaskColorIndexForeground.addExclusivelyTo(_upper, _lower, inIndex);
	_upper |= (kTextAttributes_EnableForeground._upper);
	assert(colorIndexForeground() == inIndex); // debug
}// colorIndexForegroundSet


/*!
Returns true if this object’s attributes include all of
the specified attribute bits.

See also returnValueInRange().

(4.1)
*/
bool
TextAttributes_Object::hasAttributes	(TextAttributes_Object		inAttributes)
const
{
	return ((inAttributes._upper == (_upper & inAttributes._upper)) &&
			(inAttributes._lower == (_lower & inAttributes._lower)));
}// hasAttributes


/*!
Returns true if the "kTextAttributes_ColorIndexIsBitmapID" attribute
is set.

(2017.12)
*/
bool
TextAttributes_Object::hasBitmap ()
const
{
	return this->hasAttributes(kTextAttributes_ColorIndexIsBitmapID);
}// hasBitmap


/*!
Returns true if the "kTextAttributes_StyleBlinking" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasBlink ()
const
{
	return this->hasAttributes(kTextAttributes_StyleBlinking);
}// hasBlink


/*!
Returns true if the "kTextAttributes_StyleBold" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasBold ()
const
{
	return this->hasAttributes(kTextAttributes_StyleBold);
}// hasBold


/*!
Returns true if the "kTextAttributes_StyleConceal" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasConceal ()
const
{
	return this->hasAttributes(kTextAttributes_StyleConceal);
}// hasConceal


/*!
Returns true if this object includes any attributes related to
double-size text.

(4.1)
*/
bool
TextAttributes_Object::hasDoubleAny ()
const
{
	return (0 != this->returnValueInRange(kTextAttributes_MaskDoubleText));
}// hasDoubleAny


/*!
Returns true if this object sets the attribute for double-height
text (bottom half).

(4.1)
*/
bool
TextAttributes_Object::hasDoubleHeightBottom ()
const
{
	return (kTextAttributes_ValueDoubleHeightBottom == this->returnValueInRange(kTextAttributes_MaskDoubleText));
}// hasDoubleHeightBottom


/*!
Returns true if this object sets the attribute for double-height
text (top half).

(4.1)
*/
bool
TextAttributes_Object::hasDoubleHeightTop ()
const
{
	return (kTextAttributes_ValueDoubleHeightTop == this->returnValueInRange(kTextAttributes_MaskDoubleText));
}// hasDoubleHeightTop


/*!
Returns true if this object sets the attribute for double-width.

(4.1)
*/
bool
TextAttributes_Object::hasDoubleWidth ()
const
{
	return (kTextAttributes_ValueDoubleWidth == this->returnValueInRange(kTextAttributes_MaskDoubleText));
}// hasDoubleWidth


/*!
Returns true if the "kTextAttributes_StyleItalic" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasItalic ()
const
{
	return this->hasAttributes(kTextAttributes_StyleItalic);
}// hasItalic


/*!
Returns true if the "kTextAttributes_SearchHighlight" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasSearchHighlight ()
const
{
	return this->hasAttributes(kTextAttributes_SearchHighlight);
}// hasSearchHighlight


/*!
Returns true if the "kTextAttributes_Selected" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasSelection ()
const
{
	return this->hasAttributes(kTextAttributes_Selected);
}// hasSelection


/*!
Returns true if the "kTextAttributes_StyleUnderline" attribute is set.

(4.1)
*/
bool
TextAttributes_Object::hasUnderline ()
const
{
	return this->hasAttributes(kTextAttributes_StyleUnderline);
}// hasUnderline


/*!
Changes this object’s attributes to exclude the specified
attributes.

(4.1)
*/
void
TextAttributes_Object::removeAttributes		(TextAttributes_Object	inAttributes)
{
	_upper &= ~(inAttributes._upper);
	_lower &= ~(inAttributes._lower);
}// removeAttributes


/*!
Removes all attributes related to bitmaps.  (This is important when
attributes are copied, as usually the new copy should not continue
to have the same image as the original.)

(2017.12)
*/
void
TextAttributes_Object::removeImageRelatedAttributes ()
{
	if (hasAttributes(kTextAttributes_ColorIndexIsBitmapID))
	{
		bitmapIDSet(0);
		removeAttributes(kTextAttributes_ColorIndexIsBitmapID);
	}
}// removeImageRelatedAttributes


/*!
Removes all attributes related to styles or colors.  (This is
occasionally important in terminal operations.)

(4.1)
*/
void
TextAttributes_Object::removeStyleAndColorRelatedAttributes ()
{
	// specify ALL bits that control styles or colors
	kTextAttributes_MaskColorIndexBackground.clearFrom(_upper, _lower);
	kTextAttributes_MaskColorIndexForeground.clearFrom(_upper, _lower);
	_upper &= ~(kTextAttributes_ColorIndexIsTrueColorID._upper |
				kTextAttributes_EnableBackground._upper |
				kTextAttributes_EnableForeground._upper);
	_lower &= ~(kTextAttributes_StyleBlinking._lower |
				kTextAttributes_StyleBold._lower |
				kTextAttributes_StyleConceal._lower |
				kTextAttributes_StyleInverse._lower |
				kTextAttributes_StyleItalic._lower |
				kTextAttributes_StyleUnderline._lower);
}// removeStyleAndColorRelatedAttributes


/*!
Returns the shifted, masked value of the specified range of bits.
The argument should be recognized range constant such as
"kTextAttributes_MaskColorIndexBackground".

NOTE:	Normally you should rely on more specific accessors such as
		colorIndexForeground(), hasBold(), etc.  See also the generic
		bit accessor hasAttributes().

(4.1)
*/
UInt32
TextAttributes_Object::returnValueInRange	(TextAttributes_Object::BitRange	inRange)
const
{
	return inRange.returnValue(_upper, _lower);
}// returnValueInRange

// BELOW IS REQUIRED NEWLINE TO END FILE
