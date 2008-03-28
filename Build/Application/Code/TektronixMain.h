/*!	\file tekstor.proto.h
	\brief TEK backing store.
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

#ifndef __TEKSTORPROTO__
#define __TEKSTORPROTO__

// MacTelnet includes
#include "tekdefs.h"



TEKSTOREP	newTEKstore();
void freeTEKstore(TEKSTOREP s);
short	addTEKstore(TEKSTOREP s, char d);
void	topTEKstore(TEKSTOREP s);
short	nextTEKitem(TEKSTOREP s);
short	TEKunstore(TEKSTOREP s);

#ifdef AARON_CONTORER_HAS_A_BRAIN		// Never true
STOREP newstore();
void freestore(STOREP s);
int addstore(STOREP s, char d);
int topstore(STOREP s);
int nextitem(STOREP s);
int unstore(STOREP s);
#endif

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
