/*!	\file TerminalRangeDescription.typedef.h
	\brief Type definition header used to define a type that is
	used too frequently to be declared in any module’s header.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#ifndef __TERMINALRANGEDESCRIPTION__
#define __TERMINALRANGEDESCRIPTION__

#include "TerminalScreenRef.typedef.h"

#pragma mark Types

struct Terminal_RangeDescription
{
	TerminalScreenRef	screen;				//!< the screen for which this text range applies
	SInt32				firstRow;			//!< zero-based row number where range occurs; 0 is topmost main screen line,
											//!  a negative line number is in the scrollback buffer
	UInt16				firstColumn;		//!< zero-based column number where range begins
	UInt16				columnCount;		//!< number of columns wide the range is; if 0, the range is empty
	UInt32				rowCount;			//!< number of rows the range covers (it is rectangular, not flush to the edges)
};
typedef Terminal_RangeDescription const*	Terminal_RangeDescriptionConstPtr;

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
