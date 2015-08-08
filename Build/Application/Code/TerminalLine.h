/*!	\file TerminalLine.h
	\brief Internal implementation of a line of a terminal screen.
	
	WARNING:	This a low-level API that exposes implementation
			details for efficiency and as such the API is
			unstable.  It is only expected to be used by other
			internal implementations.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#ifndef __TERMINALLINE__
#define __TERMINALLINE__

// standard-C++ includes
#include <list>
#include <vector>

// library includes
#include <CFRetainRelease.h>

// application includes
#include "TerminalTextAttributes.typedef.h"



#pragma mark Constants

enum
{
	// NOTE: this is a constant for historical reasons but that may change
	kTerminalLine_MaximumCharacterCount = 256		//!< maximum number of columns allowed; must be a multiple of "kMy_TabStop"
};

#pragma mark Types

typedef UniChar*								TerminalLine_TextIterator;
typedef std::vector< TerminalTextAttributes >   TerminalLine_TextAttributesList;


/*!
All the information required to represent the attributes
of characters on a single line of a terminal buffer.

This is a separate data structure because as an optimization
the terminal may share information for lines that have the
same fundamental attributes (especially if they are the
default values and they never changed).  In other words,
many lines may refer to one TerminalLine_AttributeInfo
instance, even if the lines themselves are unique.
*/
struct TerminalLine_AttributeInfo
{
	friend struct TerminalLine_Object;
	
	inline TerminalLine_AttributeInfo ();
	
	inline TerminalLine_AttributeInfo (TerminalLine_AttributeInfo const&);

private:
	TerminalTextAttributes				globalAttributes;   //!< attributes that apply to every character (e.g. double-sized text)
	TerminalLine_TextAttributesList		attributeVector;	//!< where character attributes exist
};


/*!
Represents a single line of the screen buffer of a
terminal, as well as attributes of its contents
(special styles, colors, highlighting, double-sized
text, etc.).  Since text and attributes are allocated
many lines at a time, a given line may or may not
point to the start of an allocated block, so flags
are also present to indicate which pointers should be
used to dispose memory blocks later.

The line list is an std::list (as opposed to some other
standard container like a vector or deque) because it
requires quick access to the start, end and middle, and
it requires iterators that remain valid after lines are
mucked with.  It also makes use of the splice() method
which is specific to a linked list.

NOTE:	Traditionally NCSA Telnet has used bits to
		represent the style of every single terminal
		cell.  This is memory-inefficient (albeit
		convenient at times), and also worsens linearly
		as the size of the screen increases.  It may be
		nice to implement a “style run”-based approach
		that sets attributes for ranges of text (which
		is pretty much how they’re defined anyway, when
		VT sequences arrive).  That would greatly
		reduce the number of attribute words in memory!
		The first part of this is implemented, in the
		sense that Terminal Views only see terminal data
		in terms of style runs (see the new routine
		Terminal_ForEachLikeAttributeRunDo()).
*/
struct TerminalLine_Object
{
	TerminalLine_TextIterator		textVectorBegin;	//!< where characters exist
	TerminalLine_TextIterator		textVectorEnd;		//!< for convenience; past-the-end of this buffer
	size_t							textVectorSize;		//!< for convenience; size of buffer
	CFRetainRelease					textCFString;		//!< mutable string object for which "textVectorBegin" is the storage,
														//!  so the buffer can be manipulated directly if desired
	
	TerminalLine_Object ();
	~TerminalLine_Object ();
	
	TerminalLine_Object (TerminalLine_Object const&);
	
	TerminalLine_Object&
	operator = (TerminalLine_Object const&);
	
	inline bool
	operator == (TerminalLine_Object const&  inLine) const;
	
	inline bool
	operator != (TerminalLine_Object const&  inLine) const;
	
	inline TerminalLine_TextAttributesList const&
	returnAttributeVector () const;
	
	inline TerminalTextAttributes
	returnGlobalAttributes () const;
	
	inline TerminalLine_TextAttributesList&
	returnMutableAttributeVector ();
	
	inline TerminalTextAttributes&
	returnMutableGlobalAttributes ();
	
	void
	structureInitialize ();

private:
	TerminalLine_AttributeInfo*		attributeInfo;
	
	void
	clearAttributes ();
	
	void
	copyAttributes (TerminalLine_AttributeInfo const*);
	
	void
	createAttributes (TerminalLine_AttributeInfo const*);
	
	bool
	isSharedAttributeSource (TerminalLine_AttributeInfo const*,
							 TerminalLine_AttributeInfo** = nullptr) const;
	
	inline TerminalLine_AttributeInfo const&
	returnAttributeInfo () const;
	
	inline TerminalLine_AttributeInfo&
	returnMutableAttributeInfo ();
};


/*!
Semantically this is like a pointer to TerminalLine_Object
except that it has special nullability support.

By default, ALL handles that are not constructed with a
particular pointer will be assigned to the SAME global,
shared, empty-line data.

*/
struct TerminalLine_Handle
{
	TerminalLine_Handle ();
	~TerminalLine_Handle ();
	
	TerminalLine_Handle	(TerminalLine_Handle const&);
	
	TerminalLine_Handle&
	operator = (TerminalLine_Handle const&);
	
	inline TerminalLine_Object const&
	operator * () const;
	
	TerminalLine_Object&
	operator * (); // note: this is not inlined, it is more complex
	
	inline TerminalLine_Object const*
	operator -> () const;
	
	inline TerminalLine_Object*
	operator -> ();
	
	inline bool
	operator == (TerminalLine_Handle const&  inHandle) const;
	
	inline bool
	operator == (TerminalLine_Object const*  inObjectPtr) const;
	
	inline bool
	operator != (TerminalLine_Handle const&  inHandle) const;
	
	inline bool
	operator != (TerminalLine_Object const*  inObjectPtr) const;
	
	bool
	isDefault () const;
	
	void
	reset ();

private:
	mutable TerminalLine_Object*	linePtr;
};



#pragma mark Inline Methods

/*!
Initializes a structure that contains attribute data for
a single line of a terminal buffer.

(4.1)
*/
TerminalLine_AttributeInfo::
TerminalLine_AttributeInfo ()
:
globalAttributes(kTerminalTextAttributesAllOff),
attributeVector(kTerminalLine_MaximumCharacterCount)
{
}// TerminalLine_AttributeInfo constructor


/*!
Creates a new set of line attribute information by copying an
existing one.

(4.1)
*/
TerminalLine_AttributeInfo::
TerminalLine_AttributeInfo	(TerminalLine_AttributeInfo const&	inCopy)
:
globalAttributes(inCopy.globalAttributes),
attributeVector(inCopy.attributeVector)
{
}// TerminalLine_AttributeInfo copy constructor


/*!
Returns true only if the specified line is considered
equal to this line.

(3.1)
*/
bool
TerminalLine_Object::
operator == (TerminalLine_Object const&  inLine)
const
{
	return (&inLine == this);
}// TerminalLine_Object::operator ==


/*!
Returns the opposite of "operator ==".

(4.1)
*/
bool
TerminalLine_Object::
operator != (TerminalLine_Object const&  inLine)
const
{
	return (false == (this->operator ==(inLine)));
}// TerminalLine_Object::operator !=


/*!
Returns the character-by-character and line-global attributes that
apply to this screen buffer line.  The data is not guaranteed to be
unique for all lines (as an optimization, common data may be shared
until something is modified).

(4.1)
*/
TerminalLine_AttributeInfo const&
TerminalLine_Object::
returnAttributeInfo ()
const
{
	return *(this->attributeInfo);
}// TerminalLine_Object::returnAttributeInfo


/*!
Returns the set of attributes that applies to the line.  This set
is not guaranteed to be unique for all lines (as an optimization,
common sets may be shared until they are modified).

(4.1)
*/
TerminalLine_TextAttributesList const&
TerminalLine_Object::
returnAttributeVector ()
const
{
	return this->returnAttributeInfo().attributeVector;
}// TerminalLine_Object::returnAttributeVector


/*!
Returns the set of attributes that applies to the entire line by
default.  This is for information only, and it is sometimes used
to initialize the actual attribute vector.  It is a way to
remember changes that should always apply to the line as a whole,
such as a “double-size” mode.

(4.1)
*/
TerminalTextAttributes
TerminalLine_Object::
returnGlobalAttributes ()
const
{
	return this->returnAttributeInfo().globalAttributes;
}// TerminalLine_Object::returnGlobalAttributes


/*!
Returns the character-by-character and line-global attributes that
apply to this screen buffer line, in a form that can be directly
modified.  If the data was previously shared, it becomes unique and
memory is allocated (therefore, use this judiciously; ideally only
when you are sure that the line requires unique attributes).  See
also the read-only version, returnAttributeInfo().

(4.1)
*/
TerminalLine_AttributeInfo&
TerminalLine_Object::
returnMutableAttributeInfo ()
{
	if (this->isSharedAttributeSource(this->attributeInfo))
	{
		this->createAttributes(this->attributeInfo);
	}
	return *(this->attributeInfo);
}// TerminalLine_Object::returnMutableAttributeInfo


/*!
Returns the set of attributes that applies to the line, in a form
that can be directly modified.  This has the same potential side
effects as returnMutableAttributeInfo().

See also the read-only version, returnAttributeVector(), and the
line-global version, returnMutableGlobalAttributes().

NOTE:	Although technically this does not need a different name
		(the "const" variation is sufficient), it has a separate
		name so that it is easier to see when the mutable variant
		is in use and when memory allocation may be occurring.

(4.1)
*/
TerminalLine_TextAttributesList&
TerminalLine_Object::
returnMutableAttributeVector ()
{
	return this->returnMutableAttributeInfo().attributeVector;
}// TerminalLine_Object::returnMutableAttributeVector


/*!
Use instead of returnGlobalAttributes() if it is necessary to
change the global attribute values.  This has the same potential side
effects as returnMutableAttributeInfo().

See also the read-only version, returnGlobalAttributes(), and the
character-by-character version, returnMutableAttributeVector().

(4.1)
*/
TerminalTextAttributes&
TerminalLine_Object::
returnMutableGlobalAttributes ()
{
	return this->returnMutableAttributeInfo().globalAttributes;
}// TerminalLine_Object::returnMutableGlobalAttributes


/*!
Returns the line data that this handle refers to.  If the handle
is in a reset state, the line is blank and the returned pointer
will always refer to the shared, immutable blank line data; as
such the underlying object references are NOT all unique, even
though handles themselves are always unique.

See also the non-const version (not inlined), which actually has
copy-on-write semantics.

(4.1)
*/
TerminalLine_Object const&
TerminalLine_Handle::
operator * ()
const
{
	return *linePtr;
}// TerminalLine_Handle::operator * (const)


/*!
Calls "operator *".

(4.1)
*/
TerminalLine_Object const*
TerminalLine_Handle::
operator -> ()
const
{
	return &(this->operator *());
}// TerminalLine_Handle::operator * (const)


/*!
Calls "operator *", with the same special semantics as
the non-const version of "operator *".

(4.1)
*/
TerminalLine_Object*
TerminalLine_Handle::
operator -> ()
{
	return &(this->operator *());
}// TerminalLine_Handle::operator -> (non-const)


/*!
Returns true only if the given handle refers to the
same line data as this handle.

(4.1)
*/
bool
TerminalLine_Handle::
operator == (TerminalLine_Handle const&  inHandle)
const
{
	return (inHandle.linePtr == this->linePtr);
}// TerminalLine_Handle::operator == (TerminalLine_Handle const&)


/*!
Returns true only if the given line data pointer is the
same as the one managed by this handle.

This is for convenience, as it is often the case that
terminal code will have ready access to the line data
but not the handle that it originally came from.

(4.1)
*/
bool
TerminalLine_Handle::
operator == (TerminalLine_Object const*  inObjectPtr)
const
{
	return (inObjectPtr == this->linePtr);
}// TerminalLine_Handle::operator == (TerminalLine_Object const*)


/*!
Returns the opposite of "operator ==".

(4.1)
*/
bool
TerminalLine_Handle::
operator != (TerminalLine_Handle const&  inHandle)
const
{
	return (false == (this->operator ==(inHandle)));
}// TerminalLine_Handle::operator != (TerminalLine_Handle const&)


/*!
Returns the opposite of "operator ==".

(4.1)
*/
bool
TerminalLine_Handle::
operator != (TerminalLine_Object const*  inObjectPtr)
const
{
	return (false == (this->operator ==(inObjectPtr)));
}// TerminalLine_Handle::operator != (TerminalLine_Object const*)


#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
