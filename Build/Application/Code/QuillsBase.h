/*!	\file QuillsBase.h
	\brief Fundamental APIs exposed to scripting languages.
	
	This is the core header, containing APIs necessary for
	starting MacTelnet from a scripting environment.
	
	Information on these APIs is available through "pydoc".
*/
/*###############################################################

	MacTelnet
		© 1998-2010 by Kevin Grant.
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

#ifndef __QUILLSBASE__
#define __QUILLSBASE__

// standard-C++ includes
#include <string>
#include <vector>



#pragma mark Public Methods
namespace Quills {

#if SWIG
%feature("docstring",
"Core APIs for setup and teardown of Quills, and a version API.\n\
") Base;
#endif
class Base
{
public:
#if SWIG
%feature("docstring",
"Initialize every module, in dependency order.  May also trigger\n\
side effects such as displaying the splash screen.\n\
") all_init;
#endif
	static void all_init ();
	
#if SWIG
%feature("docstring",
"Tear down every module, in reverse dependency order.  Generally\n\
cripples Quills APIs, since their implementation modules will no\n\
longer be initialized.  (The exception is any that initialize\n\
just-in-time.)\n\
") all_done;
#endif
	static void all_done ();
	
#if SWIG
%feature("docstring",
"Return the CFBundleVersion field from the Info.plist of the\n\
main bundle.\n\
\n\
The string encoding is UTF-8.\n\
") version;
#endif
	static std::string version ();

private:
	Base (); // class is not instantiated
};

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
