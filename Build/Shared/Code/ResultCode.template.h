/*!	\file ResultCode.template.h
	\brief Implements error codes in a way that is not
	plagued with implicit type conversions.
	
	Use this instead of integers or enumerations when
	creating result codes for a module’s routines.  This
	is inherently safer because it creates something that
	cannot accidentally be assigned to or compared with
	a regular integer.  It also opens some flexibility if
	debug utilities are added, such as text equivalents.
*/
/*###############################################################

	Data Access Library
	© 1998-2016 by Kevin Grant
	
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

#ifndef __RESULTCODE__
#define __RESULTCODE__



#pragma mark Types

template < typename c_type >
class ResultCode
{
public:
	inline explicit ResultCode			(c_type const&);
	inline virtual ~ResultCode			();
	inline virtual bool operator ==		(ResultCode const&) const;
	inline virtual bool operator !=		(ResultCode const&) const;
	inline virtual bool ok				() const;
protected:
private:
	c_type		this_resultCode;
};



#pragma mark Public Methods

template < typename c_type >
ResultCode< c_type >::
ResultCode	(c_type const&		inCode)
:
this_resultCode(inCode)
{
}// one-argument constructor


template < typename c_type >
ResultCode< c_type >::
~ResultCode ()
{
}// destructor


template < typename c_type >
bool
ResultCode< c_type >::
operator ==		(ResultCode const&		inComparison)
const
{
	return (inComparison.this_resultCode == this_resultCode);
}// operator ==


template < typename c_type >
bool
ResultCode< c_type >::
operator !=		(ResultCode const&		inComparison)
const
{
	return !(this->operator ==(inComparison));
}// operator !=


/*!
Returns true only if the specified result code
indicates success.  The default implementation
compares your stored type with zero; it is
recommended that zero always be used for the
success value.

(1.4)
*/
template < typename c_type >
bool
ResultCode< c_type >::
ok ()
const
{
	return (0 == this_resultCode);
}// ok

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
