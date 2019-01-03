/*!	\file PreferencesContextRef.typedef.h
	\brief Type definition header used to define a type that is
	used too frequently to be declared in any module’s header.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#pragma mark Types

/*!
A context represents a source of preferences.  It is very
flexible; a context could represent just temporary data in
memory, or full-blown CFPreferences defaults in the application
domain, or some other domain, or even the “factory defaults”
in "DefaultPreferences.plist".  It is possible to manipulate
and transfer data between contexts regardless of what they
represent underneath.  A context can be independent, or it can
be directly associated with a known and named collection that
is saved to disk.
*/
typedef struct Preferences_OpaqueContext*	Preferences_ContextRef;

// BELOW IS REQUIRED NEWLINE TO END FILE
