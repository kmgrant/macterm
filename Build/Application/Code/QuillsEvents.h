/*!	\file QuillsEvents.h
	\brief User interaction event APIs exposed to scripting
	languages.
	
	Use this class to interact with the user via the
	MacTelnet graphical user interface.
	
	Information on these APIs is available through "pydoc".
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

#ifndef __QUILLSEVENTS__
#define __QUILLSEVENTS__



#pragma mark Public Methods
namespace Quills {

class Events
{
public:
#if SWIG
%feature("docstring",
"Begin graphical user interface interaction.  Complete control\n\
is given to the user, any future callbacks to the code are made\n\
in response to actions such as clicks on menus and buttons.\n\
\n\
IMPORTANT: This call blocks until the user asks to quit.\n\
") run_loop;
#endif
	static void run_loop ();

private:
	Events (); // class is not instantiated
};

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
