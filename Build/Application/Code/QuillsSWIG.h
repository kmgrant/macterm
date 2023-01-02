/*!	\file QuillsSWIG.h
	\brief SWIG-specific code used by several modules.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

#pragma once

// standard-C++ includes
#include <sstream>
#include <stdexcept>



#pragma mark Public Methods

// Helper macro for common case of throwing a std::runtime_error by
// composing a message from a std::ostringstream.
#define QUILLS_THROW_MSG(sequence_of_print_operators) \
	do \
	{ \
		std::ostringstream	messageStream; \
		messageStream << sequence_of_print_operators; \
		throw std::runtime_error(messageStream.str()); \
	} while (0)

// Originally exception-handling code would use SWIG_CATCH_UNKNOWN
// to write a block similar to the following.  Unfortunately that
// would not work well with SWIG_CATCH_STDEXCEPT because SWIG would
// generate multiple catch-blocks for std::exception.  The solution
// is to not use SWIG_CATCH_UNKNOWN, instead implementing the final
// catch-all with the custom #define below.  SWIG_CATCH_STDEXCEPT
// is also compatible with this code.
#define QUILLS_CATCH_ALL \
	catch (...) \
	{ \
		SWIG_exception_fail(SWIG_UnknownError, "unknown exception"); \
	}

// BELOW IS REQUIRED NEWLINE TO END FILE
