/*!	\file TektronixFont.h
	\brief A set of stroked fonts for use in TEK screens that
	must render text.
	
	Designed by Aaron Contorer and David Whittington.
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

#ifndef __TEKTRONIXFONT__
#define __TEKTRONIXFONT__



#pragma mark Variables

char* VGfont[95] =
{
	"", /* space */
	/* ! thru / */
	/* ! " */
	"4d1t1h1b1f3w5t",
	"4w4t4d4b",
	/* # $ % */
	"2w8h4w8f2e8b4d8t",
	"4d8t1x3d6f1v1b1n6h1n1b1v6f",
	"8y6a2f2b2h2t6c2f2b2h2t",
	/* & */
	"8d6r2f2t4h4v4b4h4y",
	/* ' */
	"2w3e3y",
	/* ( ) */
	"4d2r4t2y",
	"4d2y4t2r",
	/* * + , - */
	"4d8t1x3d6v6w6n",
	"4d2w4t2x3a6h",
	"2c3y",
	"3w8h",
	/* . */
	"2d2h2t2f2b",
	"8y",
	/* 0-9 */
	"2d4h2y4t2r4f2v4b2n2a8y",
	"2d4h2a8t2v",
	"8w6h2n2v4f2v2b8h",
	"6h2y2r4f4d2y2r6f",
	"6e2w8b2e8f6y",
	"6h2y2r6f4t8h",
	"2w2y4h2n2v4f2r4t2y4h",
	"8y8f",
	"2d4h2y2r4f2v2n4w2r2y4h2n2v",
	"2d4h2y4t2r4f2v2n4h2y",
	/* : thru @ */
	/* : */
	"2d2h2t2f2b4w2h2t2f2b",
	/* ; */
	"2c4y2w2t2f2b2h",
	"4d4r4y",
	"1w8h3w8f",
	"4y4r",
	"4d1h1t1f1b2w2t3h1y2t1r6f1v",
	"7w1y6h1n6b1v2f4t1x1r3f1v2b1n3h1y",
	/* A-Z */
	"4t4y4n4b2w8f",
	"6h2y2r6f6d2y2r6f8b",
	"6d2e2v4f2r4t2y4h2n",
	"6h2y4t2r6f8b",
	"8d8f8t8h4x8f",
	"8t8h4x8f",
	"7e1d1r5f2v4b2n4h2y2t3f",
	"8t4x8h4w8b",
	"8h4a8t4a8h",
	"2w2n4h2y6t",
	/* K */
	"8t4x4h4y4z4n",
	"8d8f8t",
	"8t4n4y8b",
	"8t8n8t",
	"2d4h2y4t2r4f2v4b2n",
	"8t7h1n2b1v7f",
	"2d2r4t2y4h2n4b2v4f2e4n",
	/* R */
	"8t7h1n2b1v7f4d4n",
	"6h2y2r4f2r2y6h",
	"4d8t4a8h",
	"8w6b2n4h2y6t",
	"8w4b4n4y4t",
	"8w8b4y4n8t",
	"8y8a8n",
	"4d4t4r4c4y",
	"8w8h8v8h",
	/* [ thru ` */
	"6d4f8t4h",
	"8w8n",
	"2d4h8t4f",
	"4w4y4n",
	"8h",
	"8w2d3n",
	/* a */
	"1w2t1y6h1n1t1x2b1n1q1v6f1r",
	"4w7h1n2b1v7f8t",
	"8d7f1r2t1y7h",
	"8e8b7f1r2t1y7h",
	"2w8h1t1r6f1v2b1n6h",
	/* f */
	"4w7h1d3w1r4f1v7b",
	"1d6h1y2t1r6f1v2b1n7h4w5b1v6f1r",
	"8t5x1y6h1n3b",
	"4d4t3w1t1h1b1f",
	"2n4h2y4t3w1t1f1b1h",
	/* k */
	"2d8w8b2w1h2y2z2n",
	/* l */
	"5d1f1r7t",
	"4t1x1y2h1n3b3w1y2h1n3b",
	"4w1h4b3w1y5h1n3b",
	"1d6h1y2t1r6f1v2b1n",
	/* p */
	"2x6t1x1y6h1n2b1v6f1r",
	"1d6h1y2t1r6f1v2b1n7d4w6b",
	"4w1h4b3w1y5h1n",
	"1d6h1y1r6f1r1y6h",
	"4w7h4w5a7b1n4h1y",
	"4w3b1n5h1y1c1f4t",
	/* v */
	"4w4n4y",
	"4w2b2n2y2n2y2t",
	"2d4y4a4n",
	"2x2d6y4z4r",
	"2w2e4h4v4h",
	/* end of lower-case letters */
	/* { thru ~ */
	"8d3f1r2t1r2f2d1y2t1y3h",
	"4d8t",
	"3h1y2t1y2h2a1r2t1r3f",
	"6w2y2n2y"
};

char* VGTEKfont[106] =
{
	"", /* space */
	/* ! thru / */
	/* ! " */
	"2d0t2w4t",
	"1d6w2b2h2t",
	/* # $ % */
	"1d6t2d6b1h2w4f2w4h",
	"1w3h1y1r2f1r1y3h1w2a6b",
	"1w4y3a1f1t1h6x2d1h1t1f",
	/* & */
	"4d4r1t1y1n1b2v1b2n1h2y",
	/* ' */
	"1d4w2y",
	/* ( ) */
	"3d1r4t1y",
	"1d1y4t1r",
	/* * + , - */
	"1w4y4a4n1x2a6t",
	"3w4h2r4b",
	"1d2x2y1t",
	"3w4h",
	/* . */
	"2d1h1t1f",
	"1w4y",
	/* 0-9 */
	"1w4t1y2h1n4b1v2f1r4y",
	"1d2h1a6t1x1f",
	"6w1y2h1n1b1v2f1v2b4h",
	"1w1n2h1y1t1r1f1d1y1t1r2f1v",
	"3d6t3v1x4h",
	"1w1n2h1y2t1r3f2t4h",
	"3d6w1f2v3b1n2h1y1t1r3f",
	"1d2t3y1t4f",
	"1d2h1y1t1r1f1d1y1t1r2f1v1b1n1h1a1v1b1n",
	"1d1h2y3t1r2f1v1b1n3h",
	/* : thru @ */
	/* : */
	"1d1h1t1f2w1h1t1r",
	/* ; */
	"1x1c2y1t2w2t",
	"3d3r3y",
	"2w4h2w4f",
	"3y3r",
	"2d0t2w2y1t1r2f1v",
	"4d3f1r5t1y2h1n4b2f1t2y",
	/* A-Z */
	"4t2y2n4b2w4f",
	"6t3h1n1b1v3f3d1n1b1v3f",
	"4d1w1v2f1r4t1y2h1n",
	"6t2h2n2b2v2f",
	"4d4f6t4h1z2x3f",
	"6t4h3x1a3f",
	"3d3w1h3b3f1r4t1y2h1n",
	"6t4d6b3w4f",
	"1d2h1a6t1d2f",
	"1w1n2h1y5t",
	/* K */
	"6t4d3v3n",
	"6w6b4h",
	"6t2n1b1w2y6b",
	"6t4n2x6t",
	"1w1n2h1y4t1r2f1v4b",
	"6t3h1n1b1v3f",
	"1w1n2h1y4t1r2f1v4b2d1b1d1b1h",
	/* R */
	"6t3h1n1b1v3f1d3n",
	"1w1n2h1y1t1r2f1r1t1y2h1n",
	"2d6t2a4h",
	"6w5b1n2h1y5t",
	"6w4b2n2y4t",
	"6w6b2y1t1x2n6t",
	"1t4y1t4a1b4n1b",
	"2d3t2y1t4a1b2n",
	"6w4h1b4v1b4h",
	/* [ thru ` */
	"2d2f6t2h",
	"5w4n",
	"2d2h6t2f",
	"4w2y2n",
	"3x4h",
	"5w4n",
	/* a */
	"1d4w2h1n3b3f1r1y3h",
	"6t4x2y1h1n2b1v1f2r",
	"4d3f1r2t1y3h",
	"4d6t4b2r1f1v2b1n1h2y",
	"3d2f1r2t1y2h1n1b4f",
	/* f */
	"1d5t1y1h1n2x2a2f",
	"1x1n2h1y5t1z1r1f1v1b1n1h1y1t",
	"6t4x2y1h1n3b",
	"1d2h1a4t1f1e1w0h",
	"1x1n1h1y4t2w0h",
	/* k */
	"6t4x2h2y4x2r",
	/* l */
	"1d2h1a6t1f",
	"4t1h1n2b2w1y1n3b",
	"4t2x2y1h1n3b",
	"1w1n2h1y2t1r2f1v2b",
	/* p */
	"2x6t2x2y1h1n2b1v1f2r",
	"2x4d6t2x2r1f1v2b1n1h2y",
	"4t2x2y1h1n",
	"3h1y1r2f1r1y2h",
	"1d4w2h1f2w5b1n1y",
	"4w3b1n1h2y2x4t",
	/* v */
	"4w2b2n2y2t",
	"4w3b1n1y2t2x1n1y3t",
	"4y4a4n",
	"4w2b1n1h2y1w5b1v2f1r",
	"4w4h4v4h",
	/* end of lower-case letters */
	/* { thru ~ */
	"3d1r1t1r1y1t1y",
	"2d6t",
	"1d1y1t1y1r1t1r",
	"5w1y2n1y",
	/* marker definitions */
	"0t",
	"1x2t1c2f",
	"2x4t2c4f",
	"3x6t2d1x4v4d4r",
	"3x1a2h1e4t1q2f1z4b",
	"3x2a1t4y1t4a1b4n1b",
	"2z4t4h4b4f",
	"2a2y2n2v2r",
	"0t2z4t4h4b4f",
	"0t2x2r2y2n2v",
	"2z4t4h4b4f4y4x4r"
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
