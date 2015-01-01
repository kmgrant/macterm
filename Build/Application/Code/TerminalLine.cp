/*!	\file TerminalLine.cp
	\brief Back-end storage for the lines of text in a terminal.
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

#include "TerminalLine.h"
#include <UniversalDefines.h>



#pragma mark Variables
namespace {


TerminalLine_AttributeInfo&		gEmptyLineAttributes ()		{ static TerminalLine_AttributeInfo x; return x; }


} // anonymous namespace


#pragma mark Public Methods

/*!
Creates a new screen buffer line.

(3.1)
*/
TerminalLine_Object::
TerminalLine_Object ()
:
textVectorBegin(REINTERPRET_CAST(std::malloc(kTerminalLine_MaximumCharacterCount * sizeof(UniChar)), UniChar*)),
textVectorEnd(textVectorBegin + kTerminalLine_MaximumCharacterCount),
textVectorSize(textVectorEnd - textVectorBegin),
textCFString(CFStringCreateMutableWithExternalCharactersNoCopy
				(kCFAllocatorDefault, textVectorBegin, kTerminalLine_MaximumCharacterCount,
					kTerminalLine_MaximumCharacterCount/* capacity */, kCFAllocatorMalloc/* reallocator/deallocator */),
				true/* is retained */),
attributeInfo(nullptr)
{
	assert(textCFString.exists());
	clearAttributes();
	structureInitialize();
}// TerminalLine_Object constructor


/*!
Creates a new screen buffer line by copying an
existing one.

(3.1)
*/
TerminalLine_Object::
TerminalLine_Object	(TerminalLine_Object const&		inCopy)
:
textVectorBegin(REINTERPRET_CAST(std::malloc(kTerminalLine_MaximumCharacterCount * sizeof(UniChar)), UniChar*)),
textVectorEnd(textVectorBegin + kTerminalLine_MaximumCharacterCount),
textVectorSize(textVectorEnd - textVectorBegin),
textCFString(CFStringCreateMutableWithExternalCharactersNoCopy
				(kCFAllocatorDefault, textVectorBegin, kTerminalLine_MaximumCharacterCount,
					kTerminalLine_MaximumCharacterCount/* capacity */, kCFAllocatorMalloc/* reallocator/deallocator */),
				true/* is retained */),
attributeInfo(nullptr)
{
	assert(textCFString.exists());
	this->copyAttributes(inCopy.attributeInfo);
	// it is important for the local CFMutableStringRef to have its own
	// internal buffer, which is why it was allocated separately and
	// is being copied directly below
	std::copy(inCopy.textVectorBegin, inCopy.textVectorEnd, this->textVectorBegin);
}// TerminalLine_Object copy constructor


/*!
Disposes of a screen buffer line.

(3.1)
*/
TerminalLine_Object::
~TerminalLine_Object ()
{
	this->clearAttributes();
}// TerminalLine_Object destructor


/*!
Reinitializes a screen buffer line from a
different one.

(3.1)
*/
TerminalLine_Object&
TerminalLine_Object::
operator =	(TerminalLine_Object const&		inCopy)
{
	if (this != &inCopy)
	{
		this->clearAttributes();
		this->copyAttributes(inCopy.attributeInfo);
		
		// since the CFMutableStringRef uses the internal buffer, overwriting
		// the buffer contents will implicitly update the CFStringRef as well;
		// also, since the lines are all the same size, there is no need to
		// copy the start/end and size information
		std::copy(inCopy.textVectorBegin, inCopy.textVectorEnd, this->textVectorBegin);
	}
	return *this;
}// TerminalLine_Object::operator =


/*!
Removes all attributes, possibly freeing allocated memory and
returning to a shared reference for attribute data.

(4.1)
*/
void
TerminalLine_Object::
clearAttributes ()
{
	if (&gEmptyLineAttributes() != this->attributeInfo)
	{
		delete this->attributeInfo, this->attributeInfo = nullptr;
	}
	
	// note: this applies both to initial values and to values
	// that become nullptr above after being deallocated
	if (nullptr == this->attributeInfo)
	{
		this->attributeInfo = &gEmptyLineAttributes();
	}
}// TerminalLine_Object::clearAttributes


/*!
Unlike createAttributes(), this will check the address of the
source and perform a shallow copy of any known shared sets
(otherwise, it calls createAttributes() and makes a deep copy).

WARNING:	This overwrites the current attribute allocation without
		checking the previous value.  In cases where the previous
		value might have been allocated, call clearAttributes()
		first (which will conditionally deallocate if the data
		was not shared).

(4.1)
*/
void
TerminalLine_Object::
copyAttributes	(TerminalLine_AttributeInfo const*	inSourceOrNull)
{
	TerminalLine_AttributeInfo*		mutablePtr = nullptr;
	
	
	if (isSharedAttributeSource(inSourceOrNull, &mutablePtr))
	{
		// source is shared; continue to share in the copy
		this->attributeInfo = mutablePtr;
	}
	else
	{
		// source is unknown; make a unique copy
		createAttributes(inSourceOrNull);
	}
}// TerminalLine_Object::copyAttributes


/*!
Allocates memory for new, unique attribute data, preventing the
use of any references to shared attribute data.  This should only
be done when a line definitely requires unique attributes.

If "inSourceOrNull" is not nullptr, the new attributes copy the
given source of existing attributes.

WARNING:	This overwrites the current attribute allocation without
		checking the previous value.  In cases where the previous
		value might have been allocated, call clearAttributes()
		first (which will conditionally deallocate if the data
		was not shared).

(4.1)
*/
void
TerminalLine_Object::
createAttributes	(TerminalLine_AttributeInfo const*		inSourceOrNull)
{
	this->attributeInfo = (nullptr == inSourceOrNull)
							? new TerminalLine_AttributeInfo()
							: new TerminalLine_AttributeInfo(*inSourceOrNull);
}// TerminalLine_Object::createAttributes


/*!
Returns true if the specified line attribute storage matches any
known shared source of attributes (such as the set of attributes
that describe lines with no attributes at all).  This determines
if returnMutableAttributeVector() will need to do any allocation.

If a source is shared, its non-"const" version is returned so
that it may be assigned.  This DOES NOT MEAN IT CAN BE CHANGED!!!
The new pointer is just useful to aid variable updates in certain
situations.

(4.1)
*/
bool
TerminalLine_Object::
isSharedAttributeSource		(TerminalLine_AttributeInfo const*		inSource,
							 TerminalLine_AttributeInfo**			outNonConstVersion)
const
{
	bool	result = false;
	
	
	if (&gEmptyLineAttributes() == inSource)
	{
		if (nullptr != outNonConstVersion)
		{
			*outNonConstVersion = &gEmptyLineAttributes();
		}
		result = true;
	}
	return result;
}// TerminalLine_Object::isSharedAttributeSource


/*!
Resets a line to its initial state (clearing all text and
removing attribute bits).

(3.1)
*/
void
TerminalLine_Object::
structureInitialize ()
{
	std::fill(textVectorBegin, textVectorEnd, ' ');
	clearAttributes();
}// TerminalLine_Object::structureInitialize

// BELOW IS REQUIRED NEWLINE TO END FILE
