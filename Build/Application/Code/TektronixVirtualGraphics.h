/*!	\file TektronixVirtualGraphics.h
	\brief Takes Tektronix codes as input, and sends the output
	to real graphics devices.
	
	Developed by Aaron Contorer, with bug fixes by Tim Krauskopf.
	TEK 4105 additions by Dave Whittington.
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

#ifndef __TEKTRONIXVIRTUALGRAPHICS__
#define __TEKTRONIXVIRTUALGRAPHICS__

// MacTelnet includes
#include "SessionRef.typedef.h"



#pragma mark Public Methods

/* vgtek.c */
short detachGraphics(short dnum);
short getgraphcurs();
void setgraphcurs();
void unsetgraphcurs();
short VGalive(short dnum);
void destroyGraphics(short dnum);
SessionRef TektronixVirtualGraphics_FindSessionUsingGraphic (short vg);
short donothing();
short drawc(short vw, short c);
void TektronixVirtualGraphics_Init();
short VGdevice(short vw, short dev);
short VGnewwin(short device, SessionRef inSession);
void VGclrstor(short vw);
void VGdumpstore(short vw, short (*func )(short));
void VGdraw(short vw, char c);
void VGpage(short vw);
short VGpred(short vw, short dest);
void VGstopred(short vw);
void VGredraw(short vw, short dest);
void VGgiveinfo(short vw);
void VGzoom(short vw, short x0, short y0, short x1, short y1);
void VGwhatzoom(short vw, short *px0, short *py0, short *px1, short *py1);
void VGzcpy(short src, short dest);
void VGclose(short vw);
short VGwrite(short vw, char const* data, short count);
char const*VGrgname(short rgdev);
void VGtmode(short rgdev);
void VGgindata(short vw, unsigned short x, unsigned short y, char c, char *a);
SessionRef VGgetVS(short theVGnum);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
