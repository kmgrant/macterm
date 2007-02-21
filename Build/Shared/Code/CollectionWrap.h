/*!	\file CollectionWrap.h
	\brief Convenient wrapper class for a Collection Manager
	Collection.
	
	Use this to allow a Collection (normally constructed with C
	APIs) to use those APIs automatically upon instantiation of
	this type of object.  Useful to avoid having to write
	constructors and destructors for objects that use variables
	for Collections internally.
*/
/*###############################################################

	Data Access Library 1.3
	© 1998-2006 by Kevin Grant
	
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

#include "UniversalDefines.h"

#ifndef __COLLECTIONWRAP__
#define __COLLECTIONWRAP__



#pragma mark Types

class CollectionWrap
{
public:
	//! constructor
	inline
	CollectionWrap ();
	
	//! destructor
	inline
	~CollectionWrap ();
	
	//! returns the Collection that was created; use this with Collection Manager APIs
	inline Collection
	returnCollection () const;

protected:
	//! this class is not meant to be copied; this is declared but not implemented
	inline
	CollectionWrap (CollectionWrap const&);
	
	//! this class is not meant to be assigned to; this is declared but not implemented
	inline CollectionWrap&
	operator = (CollectionWrap const&);

private:
	Collection	this_collection;
};



#pragma mark Public Methods

CollectionWrap::
CollectionWrap ()
:
this_collection(nullptr)
{
	this_collection = NewCollection();
	assert(this_collection != nullptr);
}// default constructor


CollectionWrap::
~CollectionWrap ()
{
	DisposeCollection(this_collection);
}// destructor


Collection
CollectionWrap::
returnCollection ()
const
{
	return this_collection;
}// returnCollection

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
