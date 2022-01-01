/*!	\file Emulation.h
	\brief Terminal emulators.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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



#pragma mark Constants

typedef UInt32 Emulation_FullType;
typedef UInt32 Emulation_BaseType; // part of Emulation_FullType
typedef UInt32 Emulation_Variant; // part of Emulation_FullType

enum
{
	// These masks chop up the 16-bit emulator type into two parts,
	// the terminal type and the variant of it; this allows up to 256
	// terminal types, and 256 variants (for example, VT is a type,
	// and VT100 and VT220 are variants of the VT terminal type).
	//
	// Standardizing on this approach will make it *much* easier to
	// implement future terminal types - for example, many variants
	// of terminals share identical features, so you can check if
	// ANY variant of a particular terminal is in use just by
	// isolating the upper byte.  For convenience, two macros below
	// are included to isolate the upper or lower byte for you.
	// Use them!!!
	kEmulation_BaseTypeByteShift		= 8,
	kEmulation_BaseTypeMask				= (0x000000FF << kEmulation_BaseTypeByteShift),
	kEmulation_VariantByteShift			= 0,
	kEmulation_VariantMask				= (0x000000FF << kEmulation_VariantByteShift)
};
enum
{
	// use these constants only when you need to determine the terminal emulator family
	// (and if you add support for new terminal types, add constants to this list in
	// the same way as shown below)
	kEmulation_BaseTypeVT = ((0 << kEmulation_BaseTypeByteShift) & kEmulation_BaseTypeMask),
		kEmulation_VariantVT100 = ((0x00 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantVT102 = ((0x01 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantVT220 = ((0x02 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantVT320 = ((0x03 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantVT420 = ((0x04 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
	kEmulation_BaseTypeXTerm = ((1 << kEmulation_BaseTypeByteShift) & kEmulation_BaseTypeMask),
		kEmulation_VariantXTermOriginal = ((0x00 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantXTermColor = ((0x01 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantXTerm256Color = ((0x02 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
	kEmulation_BaseTypeDumb = ((2 << kEmulation_BaseTypeByteShift) & kEmulation_BaseTypeMask),
		kEmulation_VariantDumb1 = ((0x00 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
	kEmulation_BaseTypeANSI = ((3 << kEmulation_BaseTypeByteShift) & kEmulation_BaseTypeMask),
		kEmulation_VariantANSIBBS = ((0x00 << kEmulation_VariantByteShift) & kEmulation_VariantMask),
		kEmulation_VariantANSISCO = ((0x01 << kEmulation_VariantByteShift) & kEmulation_VariantMask)
};
enum
{
	// refer to a terminal type using these simpler constants
	kEmulation_FullTypeANSIBBS = kEmulation_BaseTypeANSI | kEmulation_VariantANSIBBS, // PC (“ANSI”) terminals
	kEmulation_FullTypeANSISCO = kEmulation_BaseTypeANSI | kEmulation_VariantANSISCO,
	kEmulation_FullTypeVT100 = kEmulation_BaseTypeVT | kEmulation_VariantVT100, // VT terminals
	kEmulation_FullTypeVT102 = kEmulation_BaseTypeVT | kEmulation_VariantVT102,
	kEmulation_FullTypeVT220 = kEmulation_BaseTypeVT | kEmulation_VariantVT220,
	kEmulation_FullTypeVT320	= kEmulation_BaseTypeVT | kEmulation_VariantVT320,
	kEmulation_FullTypeVT420 = kEmulation_BaseTypeVT | kEmulation_VariantVT420,
	kEmulation_FullTypeXTermOriginal = kEmulation_BaseTypeXTerm | kEmulation_VariantXTermOriginal, // xterm terminals
	kEmulation_FullTypeXTermColor = kEmulation_BaseTypeXTerm | kEmulation_VariantXTermColor,
	kEmulation_FullTypeXTerm256Color = kEmulation_BaseTypeXTerm | kEmulation_VariantXTerm256Color,
	kEmulation_FullTypeDumb = kEmulation_BaseTypeDumb | kEmulation_VariantDumb1 // “dumb” terminals
};

// BELOW IS REQUIRED NEWLINE TO END FILE
