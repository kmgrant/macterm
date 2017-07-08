/*!	\file Registrar.template.h
	\brief Automatically adds and removes addresses at
	construction and destruction time, respectively.
	Useful for keeping track of valid pointers.
*/
/*###############################################################

	Data Access Library
	© 1998-2017 by Kevin Grant
	
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

#include <UniversalDefines.h>

#pragma once



#pragma mark Types

/*!
Automatically adds the specified reference to a set when
constructed, and removes it when destructed.

Typically, you make one of these the first data member of
an internal class whose pointer is tracked by an opaque
reference type.  This way, when the class is constructed
its reference is added to the list and when it is destroyed
it is removed.

Another application is to declare one of these within the
code of a constructor or destructor, attached to a different
list of references that track “unstable” pointers.  This is
useful for debugging if code is called using a pointer to a
data structure that is technically partially defined due to
construction or destruction.
*/
template < typename address_type, typename set_type >
class Registrar
{
public:
	Registrar	(address_type, set_type&);
	~Registrar	();

private:
	// copying is not allowed
	Registrar	(Registrar< address_type, set_type > const&);
	
	// reassignment is not allowed
	Registrar< address_type, set_type >&
	operator =	(Registrar< address_type, set_type > const&);
	
	address_type	_ref;
	set_type&		_registry;
};



#pragma mark Public Methods

template < typename address_type, typename set_type >
Registrar< address_type, set_type >::
Registrar	(address_type	inRef,
			 set_type&		inoutRegistry)
:
_ref(inRef),
_registry(inoutRegistry)
{
	_registry.insert(_ref);
}// Registrar default constructor


template < typename address_type, typename set_type >
Registrar< address_type, set_type >::
~Registrar ()
{
	_registry.erase(_ref);
}// Registrar destructor

// BELOW IS REQUIRED NEWLINE TO END FILE
