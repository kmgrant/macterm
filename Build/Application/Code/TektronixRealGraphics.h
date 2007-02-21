/*!	\file TektronixRealGraphics.h
	\brief UI elements for vector graphics screens.
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

#ifndef __TEKTRONIXREALGRAPHICS__
#define __TEKTRONIXREALGRAPHICS__

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

#include "tekdefs.h"



#pragma mark Public Methods

Boolean
	TektronixRealGraphics_IsRealGraphicsWindow	(WindowRef		inWindow,
												 SInt16*		outRG);

char const*RGMdevname();
short RGMnewwin();
void RGMinit();
short RGMgin(short w);
short RGMpencolor(short w, short color);
short RGMclrscr(short w);
short RGMclose(short w);
short RGMpoint(short w, short x, short y);
short RGMdrawline(short w, short x0, short y0, short x1, short y1);
void RGMinfo(short w, short v, short a, short b, short c, short d);
void RGMpagedone(short w);
void RGMdataline(short w, short data, short count);
void RGMcharmode(short w, short rotation, short size);
void RGMgmode();
void RGMtmode();
void RGMshowcur();
void RGMlockcur();
void RGMhidecur();
void RGMbell(short w);
void RGMuncover(short w);
short RGMoutfunc(short (*f )());
short RGsetwind(short dnum);
short RGfindbyVG(short vg);
short RGattach(short vg, SessionRef, char *name, TektronixMode TEKtype);
short RGdetach(short vg);
short RGfindbywind(WindowRef wind);
short RGupdate(WindowRef wind);
short RGsupdate(short i);
short RGgetVG(WindowRef wind);
short inSplash(Point *p1, Point *p2);
void VidSync();
void RGmousedown(WindowRef wind, Point *wherein);
void RGMgrowme(short myRGMnum, WindowRef window, long *where, short modifiers);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
