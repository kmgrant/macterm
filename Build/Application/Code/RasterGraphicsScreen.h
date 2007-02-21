/*!	\file RasterGraphicsScreen.h
	\brief Routines for the interactive color raster
	graphics system’s UI.
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

#ifndef __RASTERGRAPHICSSCREEN__
#define __RASTERGRAPHICSSCREEN__

// Mac includes
#include <Carbon/Carbon.h>



#pragma mark Public Methods

void MacRGinit();
short MacRGnewwindow(char const* name, short x1, short y1, short x2, short y2);
void MacRGsetwindow(short wn);
void MacRGdestroy(short wn);
void MacRGremove(short wn);
short MacRGfindwind(WindowRef wind);
void MacRGcopy(WindowRef wind);
short MacRGupdate(WindowRef wind);
void MacRGsetdevice();
short MacRGfill(short x1, short y1, short x2, short y2);
short MacRGraster(char* data, short x1, short y1, short x2, short y2, short rowbytes);
short MacRGmap(short start, short length, char* data);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
