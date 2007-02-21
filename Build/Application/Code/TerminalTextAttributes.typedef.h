/*!	\file TerminalTextAttributes.typedef.h
	\brief Type definition header used to define a type that is
	used too frequently to be declared in any module’s header.
	
	This header also describes in detail the text attribute bits
	and their valid values.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __TERMINALTEXTATTRIBUTES__
#define __TERMINALTEXTATTRIBUTES__



#pragma mark Types

/*!
Terminal Attribute Bits

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

The lower 16 bits represent text style and things that were
traditionally supported by NCSA Telnet 2.6.  They are generally
designed to coincide with VT-assigned values, so you shouldn’t
move the bits around.

The upper 16 bits are new in MacTelnet 3.0 and finally give
some flexibility where it was badly needed.

<pre>
[RESERVED]                [AGE]    [S.][GR.] [DBL.]   [ANSI BG.]   [ANSI FG.]    [STYLE BITS]
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
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |  +--+--+--- 10-8: ANSI foreground color index (3 bits, see [1])
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |   +------------ 11:   use ANSI foreground color?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |  +--+--+--- 14-12: ANSI background color index (3 bits, see [1])
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |     +------------ 15:    use ANSI background color?
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  |  +--+--- 17-16: double text mode (2 bits, see [2]); LINE-GLOBAL
 |  |  |  |   |  |  |  |   |  |  |  |   |  |
 |  |  |  |   |  |  |  |   |  |  |  |   |  +--------- 18: VT graphics enabled?
 |  |  |  |   |  |  |  |   |  |  |  |   |
 |  |  |  |   |  |  |  |   |  |  |  |   +------------ 19: is high ASCII used anywhere on the line; LINE-GLOBAL
 |  |  |  |   |  |  |  |   |  |  |  |
 |  |  |  |   |  |  |  |   |  |  |  +--- 20: is selected?
 |  |  |  |   |  |  |  |   |  |  |
 |  |  |  |   |  |  |  |   +--+--+--- 23-21: text age, non-linear (3 bits, see [3])
 |  |  |  |   |  |  |  |
 +--+--+--+---+--+--+--+------------- 31-24: UNDEFINED - set to 0

[1] The 3-bit ANSI color values can be one of the following
(but please use defined constants instead of these numbers):
	000 (0)		black			100 (4)		blue
	001 (1)		red				101 (5)		magenta
	010 (2)		green			110 (6)		cyan
	011 (3)		yellow			111 (7)		white

[2] The 2-bit double text mode values can be one of the following
(but please use defined constants instead of these numbers):
	00 (0)		normal			10 (2)		text is top half of double height
	01 (1)		double width	11 (3)		text is bottom half of double height

[3] Text aging is not implemented and may be removed.
</pre>
*/
typedef UInt32 TerminalTextAttributes;
enum
{
	kNoTerminalTextAttributes				= 0L,					//!< indicates that all attributes are “off”
	kInvalidTerminalTextAttributes			= 0xFFFFFFFF,			//!< indicates the attribute bits are undefined
	kMaskTerminalTextAttributeDoubleText	= 0x00030000,			//!< MASK ONLY
		kTerminalTextAttributeDoubleWidth		= 0x00010000,		//!< if masking "kTerminalTextAttributeDoubleText" yields EXACTLY this
																	//!  value, then double-width, single-height text is to be rendered
		kTerminalTextAttributeDoubleHeightTop	= 0x00020000,		//!< if masking "kTerminalTextAttributeDoubleText" yields EXACTLY this
																	//!  value, then the top half of double-width and double-height text
																	//!  is to be rendered
		kTerminalTextAttributeDoubleHeightBottom	= 0x00030000,	//!< if masking "kTerminalTextAttributeDoubleText" yields EXACTLY this
																	//!  value, then the bottom half of double-width and double-height
																	//!  text is to be rendered
	kTerminalTextAttributeVTGraphics		= 0x00040000,			//!< should VT graphics be used? (exact glyphs depend on the current
																	//!  terminal; for example, graphics are different for VT52 than VT100)
	kTerminalTextAttributeHighASCII			= 0x00080000,			//!< does any byte on the line require 8 bits, i.e. greater than 127?
	kTerminalTextAttributeSelected			= 0x00100000,			//!< is text highlighted as being part of the selection?
	kMaskTerminalTextAttributeAge			= 0x00E00000			//!< at what stage of decay is the text?
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
