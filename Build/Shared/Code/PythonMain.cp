/*###############################################################

	Python Main
	© 2009-2019 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

// library includes
#include <Python.h>

/*!
In every respect, this is like running "python" from the
command line.  However, the existence of an in-bundle
executable allows Mac OS X to assign the correct bundle
ID to the interpreter, which is absolutely critical!
*/
int
main	(int	argc,
		 char*	argv[])
{
	return Py_Main(argc, argv);
}

// BELOW IS REQUIRED NEWLINE TO END FILE
