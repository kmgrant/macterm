/*!	\file TerminalLine.cp
	\brief Back-end storage for the lines of text in a terminal.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

// library includes
#include <Console.h>



#pragma mark Variables
namespace {


TerminalLine_AttributeInfo&		gEmptyLineAttributes ()		{ static TerminalLine_AttributeInfo x; return x; }
TerminalLine_Object const&		gEmptyLineData ()			{ static TerminalLine_Object x; return x; }


} // anonymous namespace


#pragma mark Public Methods

/*!
Creates a new screen buffer line.

(3.1)
*/
TerminalLine_Object::
TerminalLine_Object ()
:
textVectorBegin(REINTERPRET_CAST(malloc(kTerminalLine_MaximumCharacterCount * sizeof(UniChar)), UniChar*)),
textVectorEnd(textVectorBegin + kTerminalLine_MaximumCharacterCount),
textCFString(CFStringCreateMutableWithExternalCharactersNoCopy
				(kCFAllocatorDefault, textVectorBegin, kTerminalLine_MaximumCharacterCount,
					kTerminalLine_MaximumCharacterCount/* capacity */, kCFAllocatorMalloc/* reallocator/deallocator */),
				CFRetainRelease::kAlreadyRetained),
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
textVectorBegin(REINTERPRET_CAST(malloc(kTerminalLine_MaximumCharacterCount * sizeof(UniChar)), UniChar*)),
textVectorEnd(textVectorBegin + kTerminalLine_MaximumCharacterCount),
textCFString(CFStringCreateMutableWithExternalCharactersNoCopy
				(kCFAllocatorDefault, textVectorBegin, kTerminalLine_MaximumCharacterCount,
					kTerminalLine_MaximumCharacterCount/* capacity */, kCFAllocatorMalloc/* reallocator/deallocator */),
				CFRetainRelease::kAlreadyRetained),
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


/*!
Creates a new screen buffer line handle by making it point
to a shared, immutable empty-line data structure.  This
ensures that the vast majority of lines consume minimal
resources until they are actually changed (at which time
unique allocations are performed).

(4.1)
*/
TerminalLine_Handle::
TerminalLine_Handle ()
:
linePtr(nullptr) // see below
{
	this->reset(); // set to shared empty-line data
}// TerminalLine_Handle constructor


/*!
Handles copy construction by making the data pointer
unique if necessary.  (Should be consistent with all
other code that mutates the pointer value.)

(4.1)
*/
TerminalLine_Handle::
TerminalLine_Handle		(TerminalLine_Handle const&		inOther)
:
linePtr(nullptr) // see below
{
	if (inOther.isDefault())
	{
		this->reset(); // set to shared empty-line data
	}
	else
	{
		// make unique (should be consistent with the allocation
		// behavior of the non-const "operator *()")
		linePtr = new TerminalLine_Object();
		assert(false == this->isDefault());
		//Console_WriteValueAddress("upon copy, allocating unique line data for handle", this); // debug
	}
}// TerminalLine_Handle copy constructor


/*!
Creates a new screen buffer line handle by making it point
to a shared, immutable empty-line data structure.  This
ensures that the vast majority of lines consume minimal
resources until they are actually changed (at which time
unique allocations are performed).

(4.1)
*/
TerminalLine_Handle::
~TerminalLine_Handle ()
{
	if (false == this->isDefault())
	{
		// IMPORTANT: these semantics may change; for instance,
		// they may simply *mark* an allocated line as available
		// and not necessarily deallocate (the behavior of the
		// non-const "operator *()" should be consistent)
		delete linePtr, linePtr = nullptr;
	}
}// TerminalLine_Handle destructor


/*!
Handles assignment by making the data pointer unique
if necessary.  (Should be consistent with all other
code that mutates the pointer value.)

(4.1)
*/
TerminalLine_Handle&
TerminalLine_Handle::
operator = (TerminalLine_Handle const&		inOther)
{
	if (inOther.isDefault())
	{
		this->reset(); // set to shared empty-line data
	}
	else
	{
		// make unique (should be consistent with the allocation
		// behavior of the non-const "operator *()")
		linePtr = new TerminalLine_Object();
		assert(false == this->isDefault());
		//Console_WriteValueAddress("upon assignment, allocating unique line data for handle", this); // debug
	}
	return *this;
}// TerminalLine_Handle::operator =


/*!
Returns the line data that this handle refers to.

Since this is the mutating version, if the handle is in a reset
(nulled) state then this will CHANGE the internal pointer to
allocate a unique value and return that.  Always use "const"
references to read lines if you do not need to make changes so
that handles will share the empty line as long as possible.

NOTE: The "const" version is trivial and inlined, as it does
not have the same complex semantics.

(4.1)
*/
TerminalLine_Object&
TerminalLine_Handle::
operator * ()
{
	if (this->isDefault())
	{
		// copy-on-write semantics; auto-allocate a unique version
		// IMPORTANT: should match logic of destructor
		linePtr = new TerminalLine_Object();
		assert(false == this->isDefault());
		//Console_WriteValueAddress("allocating unique line data for handle", this); // debug
	}
	return *linePtr;
}// TerminalLine_Handle::operator * (non-const)


/*!
Returns true if this handle is currently relying on shared,
default data (in other words, it is an empty line that has
never been modified).

(4.1)
*/
bool
TerminalLine_Handle::
isDefault ()
const
{
	return (&gEmptyLineData() == this->linePtr);
}// TerminalLine_Handle::isDefault


/*!
Conceptually the same as deleting a heap-allocated line structure
except that the resulting object may be marked for reuse instead
(and the line data may not even be on the heap).

(4.1)
*/
void
TerminalLine_Handle::
reset ()
{
	// IMPORTANT: immutability of shared empty-line data is ensured
	// by the code in this class; in any case, the shared empty line
	// must NEVER be modified (otherwise all empty lines would change)
	this->linePtr = CONST_CAST(&gEmptyLineData(), TerminalLine_Object*);
	assert(this->isDefault());
}// TerminalLine_Handle::reset

// BELOW IS REQUIRED NEWLINE TO END FILE
