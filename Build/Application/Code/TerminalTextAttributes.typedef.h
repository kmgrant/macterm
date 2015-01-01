/*!	\file TerminalTextAttributes.typedef.h
	\brief Type definition header used to define a type that is
	used too frequently to be declared in any module’s header.
	
	This header also describes in detail the text attribute bits
	and their valid values.
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

#ifndef __TERMINALTEXTATTRIBUTES__
#define __TERMINALTEXTATTRIBUTES__



#pragma mark Types

/*!
Terminal Attribute Bits

IMPORTANT:	Do not directly access these bits, use the masks
			and accessors defined below.  If the bits must
			change, be sure to fix the accessors!

These bits are used to define the current text attributes when
rendering data, and are primarily used in the emulator data
loop and when rendering terminal screens.  The values are
somewhat important for legacy reasons, so you can’t easily shift
bits around.

Attributes that are “line global” in nature, such as double-size
text, are represented for convenience when ascertaining style of
chunks of text that do not span an entire line; however, the
implementation should not allow line-global attributes to vary
for any chunk of text on the same line.

The lower 8 bits represent text style and things that were
traditionally supported by NCSA Telnet 2.6.  They are generally
designed to coincide with VT-assigned values, so you shouldn’t
move the bits around.

The bit values allow up to 256 possible custom colors; indices
occupy the upper 16 bits, to allow for a possible future
optimization to save memory when a terminal does not use any
custom colors.

<pre>
[BACKGROUND]              [FOREGROUND]                   [S.][GR.][DBL.][COLOR]  [STYLE BITS]
31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16    15 14 13 12  11 10  9  8   7  6  5  4   3  2  1  0
-|--|--|--|---|--|--|--|---|--|--|--|---|--|--|--|-----|--|--|--|---|--|--|--|---|--|--|--|---|--|--|--|-
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  +--- 0: bold?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |  |  +------ 1: UNDEFINED - set to 0
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |  +--------- 2: italic?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |   +------------ 3: underlined?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |  +--- 4: blinking?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |  +------ 5: UNDEFINED - set to 0
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |  +--------- 6: inverse video?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |   +------------ 7: concealed (invisible)?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |  +--- 8:  use custom foreground color index (bits 23-16)?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |  +------ 9:  use custom background color index (bits 31-24)?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   +--+--------- 11-10: double text mode (2 bits, see [2]); LINE-GLOBAL
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  +--- 12: VT graphics enabled?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  +------ 13: is selected as a search result?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  +--------- 14: is selected by the user (for copy, print, etc.)?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     +------------ 15: is prohibited from being erased by selective erases
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   +--+--+--+---+--+--+--+--- 23-16: index selecting one of up to 256 foreground colors (sometimes fewer, e.g. 8, [1])
 |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |
 +--+--+--+---+--+--+--+----------------------------- 31-24: index selecting one of up to 256 background colors (sometimes fewer, e.g. 8, [1])

[1] The base 8 colors are 3-bit ANSI color values that can be one
of the following (the exact RGB components of which may be
customized by the user):
	000 (0)		black			100 (4)		blue
	001 (1)		red				101 (5)		magenta
	010 (2)		green			110 (6)		cyan
	011 (3)		yellow			111 (7)		white
On terminals that do not support all 256 colors, only these color
indices (as well as 8-15 for “bold” equivalents) are recognized.
Otherwise, any number from 0 to 255 is valid.  Note that since
256-color support was added, there is now an explicit way to
request an emphasized version of one of the 8 base colors (add 8,
as opposed to the “bold” bit).

[2] The 2-bit double text mode values can be one of the following
(but please use defined constants instead of these numbers):
	00 (0)		normal			10 (2)		text is top half of double height
	01 (1)		double width	11 (3)		text is bottom half of double height
</pre>
*/
typedef UInt32 TerminalTextAttributes;
enum
{
	kTerminalTextAttributesAllOff			= 0L,					//!< indicates that all attributes are “off”
	kInvalidTerminalTextAttributes			= 0xFFFFFFFF,			//!< indicates the attribute bits are undefined
	kAllStyleOrColorTerminalTextAttributes	= 0xFFFF03FF,			//!< specify ALL bits that control font style or color
	kTerminalTextAttributeInverseVideo		= 0x00000040,			//!< are foreground and background colors flipped?
	kTerminalTextAttributeEnableBackground	= 0x00000200,			//!< if set, the background color index applies
	kTerminalTextAttributeEnableForeground	= 0x00000100,			//!< if set, the foreground color index applies
	kMaskTerminalTextAttributeBackground	= 0xFF000000,			//!< MASK ONLY; bits that specify the color to use
	kMaskTerminalTextAttributeForeground	= 0x00FF0000,			//!< MASK ONLY; bits that specify the color to use
	kMaskTerminalTextAttributeDoubleText	= 0x00000C00,			//!< MASK ONLY
		kTerminalTextAttributeDoubleWidth		= 0x00000400,		//!< if masking "kTerminalTextAttributeDoubleText" yields EXACTLY this
																	//!  value, then double-width, single-height text is to be rendered
		kTerminalTextAttributeDoubleHeightTop	= 0x00000800,		//!< if masking "kTerminalTextAttributeDoubleText" yields EXACTLY this
																	//!  value, then the top half of double-width and double-height text
																	//!  is to be rendered
		kTerminalTextAttributeDoubleHeightBottom	= 0x00000C00,	//!< if masking "kTerminalTextAttributeDoubleText" yields EXACTLY this
																	//!  value, then the bottom half of double-width and double-height
																	//!  text is to be rendered
	kTerminalTextAttributeVTGraphics		= 0x00001000,			//!< should VT graphics be used? (exact glyphs depend on the current
																	//!  terminal; for example, graphics are different for VT52 than VT100)
	kTerminalTextAttributeSearchResult		= 0x00002000,			//!< is text highlighted as being part of a search result?
	kTerminalTextAttributeSelected			= 0x00004000,			//!< is text highlighted as being part of the selection?
	kTerminalTextAttributeCannotErase		= 0x00008000,			//!< is text marked as do-not-touch by selective erase sequences?
};


#pragma mark Public Methods

/*!
TerminalTextAttributes Bit Accessors

The bits that these accessors refer to are documented above.
*/
static inline Boolean STYLE_BOLD						(TerminalTextAttributes x) { return ((x & 0x00000001) != 0); }
static inline Boolean STYLE_ITALIC						(TerminalTextAttributes x) { return ((x & 0x00000004) != 0); }
static inline Boolean STYLE_UNDERLINE					(TerminalTextAttributes x) { return ((x & 0x00000008) != 0); }
static inline Boolean STYLE_BLINKING					(TerminalTextAttributes x) { return ((x & 0x00000010) != 0); }
static inline Boolean STYLE_INVERSE_VIDEO				(TerminalTextAttributes x) { return ((x & 0x00000040) != 0); }
static inline Boolean STYLE_CONCEALED					(TerminalTextAttributes x) { return ((x & 0x00000080) != 0); }

static inline Boolean STYLE_USE_FOREGROUND_INDEX		(TerminalTextAttributes x) { return ((x & kTerminalTextAttributeEnableForeground) != 0); }
static inline UInt8 STYLE_FOREGROUND_INDEX				(TerminalTextAttributes x) { return ((x >> 16) & 0xFF); }
static inline TerminalTextAttributes& STYLE_SET_FOREGROUND_INDEX	(TerminalTextAttributes& x, UInt8 y)
{
	x = (x & ~kMaskTerminalTextAttributeForeground) | ((UInt32)y << 16) | kTerminalTextAttributeEnableForeground;
	return x;
}
static inline TerminalTextAttributes& STYLE_CLEAR_FOREGROUND_INDEX	(TerminalTextAttributes& x)
{
	x = (x & (~kMaskTerminalTextAttributeForeground | ~kTerminalTextAttributeEnableForeground));
	return x;
}
static inline Boolean STYLE_USE_BACKGROUND_INDEX		(TerminalTextAttributes x) { return ((x & kTerminalTextAttributeEnableBackground) != 0); }
static inline UInt8 STYLE_BACKGROUND_INDEX				(TerminalTextAttributes x) { return ((x >> 24) & 0xFF); }
static inline TerminalTextAttributes& STYLE_SET_BACKGROUND_INDEX	(TerminalTextAttributes& x, UInt8 y)
{
	x = (x & ~kMaskTerminalTextAttributeBackground) | ((UInt32)y << 24) | kTerminalTextAttributeEnableBackground;
	return x;
}
static inline TerminalTextAttributes& STYLE_CLEAR_BACKGROUND_INDEX	(TerminalTextAttributes& x)
{
	x = (x & (~kMaskTerminalTextAttributeBackground | ~kTerminalTextAttributeEnableBackground));
	return x;
}
static inline TerminalTextAttributes& STYLE_COPY_BACKGROUND		(TerminalTextAttributes x, TerminalTextAttributes& y)
{
	y &= ~(kMaskTerminalTextAttributeBackground | kTerminalTextAttributeEnableBackground);
	y |= (x & (kMaskTerminalTextAttributeBackground | kTerminalTextAttributeEnableBackground));
	return y;
}

static inline Boolean STYLE_IS_DOUBLE_ANY				(TerminalTextAttributes x) { return (0 != (x & kMaskTerminalTextAttributeDoubleText)); }
// careful, when testing a multiple-bit field, make sure only the desired values are set to 1!
static inline Boolean STYLE_IS_DOUBLE_WIDTH_ONLY		(TerminalTextAttributes x) { return ((x & kMaskTerminalTextAttributeDoubleText) ==
																								kTerminalTextAttributeDoubleWidth); }
static inline Boolean STYLE_IS_DOUBLE_HEIGHT_TOP		(TerminalTextAttributes x) { return ((x & kMaskTerminalTextAttributeDoubleText) ==
																								kTerminalTextAttributeDoubleHeightTop); }
static inline Boolean STYLE_IS_DOUBLE_HEIGHT_BOTTOM		(TerminalTextAttributes x) { return ((x & kMaskTerminalTextAttributeDoubleText) ==
																								kTerminalTextAttributeDoubleHeightBottom); }

static inline Boolean STYLE_USE_VT_GRAPHICS				(TerminalTextAttributes x) { return ((x & kTerminalTextAttributeVTGraphics) != 0); }

static inline Boolean STYLE_SEARCH_RESULT				(TerminalTextAttributes x) { return ((x & kTerminalTextAttributeSearchResult) != 0); }

static inline Boolean STYLE_SELECTED					(TerminalTextAttributes x) { return ((x & kTerminalTextAttributeSelected) != 0); }

static inline Boolean STYLE_CANNOT_ERASE				(TerminalTextAttributes x) { return ((x & kTerminalTextAttributeCannotErase) != 0); }

static inline TerminalTextAttributes& STYLE_ADD			(TerminalTextAttributes& x, TerminalTextAttributes y) { x |= y; return x; }
static inline TerminalTextAttributes& STYLE_REMOVE		(TerminalTextAttributes& x, TerminalTextAttributes y) { x &= ~y; return x; }



#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
