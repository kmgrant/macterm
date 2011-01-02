/*!	\file RasterGraphicsKernel.h
	\brief Routines for the interactive color raster graphics
	system.
*/
/*###############################################################

	MacTelnet
		© 1998-2011 by Kevin Grant.
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

#ifndef	__RASTERGRAPHICSKERNEL__
#define __RASTERGRAPHICSKERNEL__

// saved argument values in parser
union arg
{
	int		a_num;				// integer number; WARNING, must be exactly "int" due to scanf() usage elsewhere
	char	a_ptr[100];			// string pointer
};

// format of a window entry
struct VRwin
{
	char			w_name[100];		// window’s name, assigned on creation
	SInt16			w_used;				// flag - is this name an old duplicate?
	SInt16			w_left;				// left edge
	SInt16			w_top;				// top edge
	SInt16			w_width;			// width
	SInt16			w_height;			// height
	SInt16			w_display;			// hardware display number of window
	struct
	{
		SInt16			wn;		// window number
	} w_rr;								// machine-dependent info for window
	struct VRwin*	w_next;				// pointer to next link in the list
};
typedef struct VRwin	VRW;



#pragma mark Public Methods

short
	RasterGraphicsKernel_Init		();

void decode0();
short decode1(char c);
short VRwrite(char *b, short len);
short VRwindow(union arg av[], char *unused);
void	VRdestroybyName(char *name);
short VRdestroy(union arg av[], char *unused);
short VRmap(union arg av[], char *data);
short VRpixel(union arg av[], char *data);
short VRimp(union arg av[], char *data);
void unimcomp(UInt8 in[], UInt8 out[], short xdim, short xmax);
short unrleit(UInt8 *buf, UInt8 *bufto, short inlen, short outlen);
short VRrle(union arg av[], char *data);
short VRfile(union arg av[], char *unused);
short VRclick(union arg av[], char *unused);
short VRmsave(union arg av[], char *unused);
VRW *VRlookup(char *name);
short VRcleanup();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
