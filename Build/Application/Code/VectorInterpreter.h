/*!	\file VectorInterpreter.h
	\brief Takes Tektronix codes as input, and sends the output
	to real graphics devices.
	
	Developed by Aaron Contorer, with bug fixes by Tim Krauskopf.
	TEK 4105 additions by Dave Whittington.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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



#pragma mark Constants

typedef SInt16 VectorInterpreter_ID;
enum
{
	kVectorInterpreter_InvalidID				= -1
};

typedef SInt16 VectorInterpreter_Target;
enum
{
	kVectorInterpreter_TargetScreenPixels		= 0,
	kVectorInterpreter_TargetQuickDrawPicture	= 1
};



#pragma mark Public Methods

//!\name Initialization
//@{

void
	VectorInterpreter_Init					();

//@}

short detachGraphics(VectorInterpreter_ID dnum);
VectorInterpreter_ID VGnewwin(VectorInterpreter_Target, SessionRef);
void VGpage(VectorInterpreter_ID vw);
short VGpred(VectorInterpreter_ID vw, short dest);
void VGstopred(VectorInterpreter_ID vw);
void VGredraw(VectorInterpreter_ID vw, short dest);
void VGgiveinfo(VectorInterpreter_ID vw);
void VGzoom(VectorInterpreter_ID vw, short x0, short y0, short x1, short y1);
void VGzcpy(short src, short dest);
void VGclose(VectorInterpreter_ID vw);
short VGwrite(VectorInterpreter_ID vw, char const* data, short count);
void VGgindata(VectorInterpreter_ID vw, unsigned short x, unsigned short y, char c, char *a);
SessionRef VGgetVS(VectorInterpreter_ID theVGnum);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
