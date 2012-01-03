/*!	\file Releases.cp
	\brief Routines for decoding the return values from the
	Gestalt() system call.
*/
/*###############################################################

	Data Access Library 2.6
	© 1998-2012 by Kevin Grant
	
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

#include <Releases.h>
#include <UniversalDefines.h>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Internal Method Prototypes
namespace {

UInt8	nibblesToBase10		(UInt8);

} // anonymous namespace



#pragma mark Public Methods

/*!
Using the Gestalt() system call to obtain
version information returns a garbled,
long integer containing a ton of information.
To acquire the major revision number only
from a messy Gestalt version long integer,
call this method.

ex. Mac OS 8.5.1
           ^
		major revision number

(3.0)
*/
UInt8
Releases_ReturnMajorRevisionForVersion 	(long const		inVersion)
{
	long		scrap = ((inVersion >> 8) & 0x000000FF);
	UInt8		result = 0;
	
	
	result = nibblesToBase10(scrap);
	return result;
}// ReturnMajorRevisionForVersion


/*!
Using the Gestalt() system call to obtain
version information returns a garbled,
long integer containing a ton of information.
To acquire the minor revision number only
from a messy Gestalt version long integer,
call this method.

ex. Mac OS 8.5.1
             ^
		minor revision number

(3.0)
*/
UInt8
Releases_ReturnMinorRevisionForVersion 	(long const		inVersion)
{
	long		scrap = ((inVersion >> 4) & 0x0000000F);
	UInt8		result = 0;
	
	
	result = (scrap);
	return result;
}// ReturnMinorRevisionForVersion


/*!
Using the Gestalt() system call to obtain
version information returns a garbled,
long integer containing a ton of information.
To acquire the superminor revision number only
from a messy Gestalt version long integer,
call this method.

ex. Mac OS 8.5.1
               ^
		superminor revision number

(3.0)
*/
UInt8
Releases_ReturnSuperminorRevisionForVersion 	(long const		inVersion)
{
	long		scrap = (inVersion & 0x0000000F);
	UInt8		result = 0;
	
	
	result = (scrap);
	return result;
}// ReturnSuperminorRevisionForVersion


#pragma mark Internal Methods
namespace {

/*!
Interprets the two nibbles of a byte as if
they were each digits of a base-10 number,
and returns the base 10 equivalent.

Hack!  Is it just me, or does Mac OS X
respond to the version Gestalts incorrectly
by returning "10" as "1, 0" in hex slots,
thereby making this 16?  Oh well, this
method exists to set things straight.

(3.0)
*/
UInt8
nibblesToBase10		(UInt8		inNibbles)
{
	return ((inNibbles & 0x0F) + 10 * ((inNibbles >> 4) & 0x0F));
}// nibblesToBase10

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
