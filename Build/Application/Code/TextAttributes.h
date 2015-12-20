/*!	\file TextAttributes.h
	\brief Manages the characteristics of a range of text in a
	terminal view.
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

#include <UniversalDefines.h>

#ifndef __TEXTATTRIBUTES__
#define __TEXTATTRIBUTES__

// library includes
#include <Console.h>



#pragma mark Types

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
[BACKGROUND]                    [FOREGROUND]                       [B][F] [UNU.] [UNUSED]     [UNUSED]
31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16    15 14 13 12  11 10  9  8   7  6  5  4   3  2  1  0
─┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼─────┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼───┼──┼──┼──┼─
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │  └──┴───┴──┴──┴──┴───┴──┴──┴──┴─── 9-0: UNDEFINED; set to 0
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │  └───────── 10: use custom foreground color index (bits 21-12)?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │   └──────────── 11: use custom background color index (bits 31-22)?
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │
 │  │  │  │   │  │  │  │   │  │  │  │   │  │  │  │     │  │  │  │
 │  │  │  │   │  │  │  │   │  │  └──┴───┴──┴──┴──┴─────┴──┴──┴──┴─── 21-12: index selecting a unique foreground color from a palette [1]
 │  │  │  │   │  │  │  │   │  │
 │  │  │  │   │  │  │  │   │  │
 └──┴──┴──┴───┴──┴──┴──┴───┴──┴────────────────────── 31-22: index selecting a unique background color from a palette [1]
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

[2] The 2-bit double text mode values can be one of the following
(but please use defined constants instead of these numbers):
	00 (0)		normal			10 (2)		text is top half of double height
	01 (1)		double width	11 (3)		text is bottom half of double height
</pre>
*/
struct TextAttributes_Object
{
	inline
	TextAttributes_Object ();
	
	inline
	TextAttributes_Object	(UInt32);
	
	inline
	TextAttributes_Object	(UInt32, UInt32);
	
	inline bool
	operator ==	(TextAttributes_Object const&) const;
	
	inline bool
	operator !=	(TextAttributes_Object const&) const;
	
	inline void
	addAttributes	(TextAttributes_Object);
	
	inline void
	clear ();
	
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
	hasAttributesIdenticalInRange	(TextAttributes_Object, TextAttributes_Object) const;
	
	inline bool
	hasAttributesInRange	(TextAttributes_Object) const;
	
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
	removeStyleAndColorRelatedAttributes ();

private:
	UInt32	_upper;
	UInt32	_lower;
};

//
// IMPORTANT: The constant bit ranges chosen below should match
// the bit-range documentation block above.
//

//! indicates the attribute bits are undefined
TextAttributes_Object const		kTextAttributes_Invalid
								(0xFFFFFFFF,	0xFFFFFFFF);

//! is text marked as do-not-touch by selective erase sequences?
TextAttributes_Object const		kTextAttributes_CannotErase
								(0,				0x00008000);

//! the background color index
TextAttributes_Object const		kTextAttributes_ColorIndexBackground
								(0,				0xFF000000);

//! the foreground color index
TextAttributes_Object const		kTextAttributes_ColorIndexForeground
								(0,				0x00FF0000);

//! if masking "kTextAttributes_RangeDoubleAny" yields EXACTLY this
//! value, then the bottom half of double-width and double-height
//! text is to be rendered
TextAttributes_Object const		kTextAttributes_DoubleHeightBottom
								(0,				0x00000C00);

//! if masking "kTextAttributes_RangeDoubleAny" yields EXACTLY this
//! value, then the top half of double-width and double-height text
//! is to be rendered
TextAttributes_Object const		kTextAttributes_DoubleHeightTop
								(0,				0x00000800);

//! if masking "kTextAttributes_RangeDoubleAny" yields EXACTLY this
//! value, then double-width, single-height text is to be rendered
TextAttributes_Object const		kTextAttributes_DoubleWidth
								(0,				0x00000400);

//! if set, the background color index applies
TextAttributes_Object const		kTextAttributes_EnableBackground
								(0x00000200,	0);

//! if set, the foreground color index applies
TextAttributes_Object const		kTextAttributes_EnableForeground
								(0x00000100,	0);

//! a mask that should encompass all possible background color index values
TextAttributes_Object const		kTextAttributes_RangeColorIndexBackground
								(0xFFC00000,	0);

//! a mask that should encompass all possible foreground color index values
TextAttributes_Object const		kTextAttributes_RangeColorIndexForeground
								(0x003FF000,	0);

//! a mask that should encompass all possible double-size attributes
TextAttributes_Object const		kTextAttributes_RangeDoubleAny
								(0,				0x00000C00);

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
}// TextAttributes_Object 2-argument constructor


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
Returns the background-index portion of the attributes.

(4.1)
*/
UInt16
TextAttributes_Object::colorIndexBackground ()
const
{
	// upper/lower, bit shift and mask need to be consistent with kTextAttributes_ColorIndexBackground
	return ((_upper >> 22) & 0x03FF);
}// colorIndexBackground


/*!
Sets the background-index portion of the attributes by
copying the relevant bits from another set of attributes.

(4.1)
*/
void
TextAttributes_Object::colorIndexBackgroundCopyFrom		(TextAttributes_Object		inSourceAttributes)
{
	_upper &= ~(kTextAttributes_RangeColorIndexBackground._upper | kTextAttributes_EnableBackground._upper);
	_upper |= (inSourceAttributes._upper & (kTextAttributes_RangeColorIndexBackground._upper | kTextAttributes_EnableBackground._upper));
}// colorIndexBackgroundCopyFrom


/*!
Sets the background-index portion of the attributes.

(4.1)
*/
void
TextAttributes_Object::colorIndexBackgroundSet	(UInt16		inIndex)
{
	// upper/lower, bit shift and mask need to be consistent with kTextAttributes_ColorIndexBackground
	_upper &= ~(kTextAttributes_RangeColorIndexBackground._upper);
	_upper |= (kTextAttributes_EnableBackground._upper | ((STATIC_CAST(inIndex, UInt32) & 0x03FF) << 22));
}// colorIndexBackgroundSet


/*!
Returns the foreground-index portion of the attributes.

(4.1)
*/
UInt16
TextAttributes_Object::colorIndexForeground ()
const
{
	// upper/lower, bit shift and mask need to be consistent with kTextAttributes_ColorIndexForeground
	return ((_upper >> 12) & 0x03FF);
}// colorIndexForeground


/*!
Sets the foreground-index portion of the attributes.

(4.1)
*/
void
TextAttributes_Object::colorIndexForegroundSet	(UInt16		inIndex)
{
	// upper/lower, bit shift and mask need to be consistent with kTextAttributes_ColorIndexForeground
	_upper &= ~(kTextAttributes_RangeColorIndexForeground._upper);
	_upper |= (kTextAttributes_EnableForeground._upper | ((STATIC_CAST(inIndex, UInt32) & 0x03FF) << 12));
}// colorIndexForegroundSet


/*!
Returns true if this object’s attributes include all of
the specified attribute bits.

See also hasAttributesInRange().

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
Returns true if the attributes from this object that fall inside
the given masked range are identical to the given attributes.

(4.1)
*/
bool
TextAttributes_Object::hasAttributesIdenticalInRange	(TextAttributes_Object		inMask,
														 TextAttributes_Object		inAttributes)
const
{
	UInt32 const	kUpperTest = (_upper & inMask._upper);
	UInt32 const	kLowerTest = (_lower & inMask._lower);
	
	
	return ((inAttributes._upper == kUpperTest) && (inAttributes._lower == kLowerTest));
}// hasAttributesIdenticalInRange


/*!
Returns true if any bit of this object’s attributes is set
from the given masked range.

(4.1)
*/
bool
TextAttributes_Object::hasAttributesInRange		(TextAttributes_Object		inMask)
const
{
	return ((0 != (_upper & inMask._upper)) || (0 != (_lower & inMask._lower)));
}// hasAttributesInRange


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
	return this->hasAttributesInRange(kTextAttributes_RangeDoubleAny);
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
	return this->hasAttributesIdenticalInRange(kTextAttributes_RangeDoubleAny, kTextAttributes_DoubleHeightBottom);
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
	return this->hasAttributesIdenticalInRange(kTextAttributes_RangeDoubleAny, kTextAttributes_DoubleHeightTop);
}// hasDoubleHeightTop


/*!
Returns true if this object sets the attribute for double-width.

(4.1)
*/
bool
TextAttributes_Object::hasDoubleWidth ()
const
{
	return this->hasAttributesIdenticalInRange(kTextAttributes_RangeDoubleAny, kTextAttributes_DoubleWidth);
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
Removes all attributes related to styles or colors.  (This is
occasionally important in terminal operations.)

(4.1)
*/
void
TextAttributes_Object::removeStyleAndColorRelatedAttributes ()
{
	// specify ALL bits that control styles or colors
	_upper &= ~(kTextAttributes_RangeColorIndexBackground._upper |
				kTextAttributes_RangeColorIndexForeground._upper |
				kTextAttributes_EnableBackground._upper |
				kTextAttributes_EnableForeground._upper);
	_lower &= ~(kTextAttributes_StyleBlinking._lower |
				kTextAttributes_StyleBold._lower |
				kTextAttributes_StyleConceal._lower |
				kTextAttributes_StyleInverse._lower |
				kTextAttributes_StyleItalic._lower |
				kTextAttributes_StyleUnderline._lower);
}// removeStyleAndColorRelatedAttributes


#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
