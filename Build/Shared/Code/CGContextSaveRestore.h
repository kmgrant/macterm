/*!	\file CGContextSaveRestore.h
	\brief Convenient wrapper class for the graphics state of
	a Quartz context.
	
	Use this to ensure CGContextSaveGState() is automatically
	called at object construction or duplication time, and
	CGContextRestoreGState is automatically called when the
	object is destroyed.  This ensures you will not forget to
	make these calls at the correct times, and leads to code
	that is also exception-safe, etc.
*/
/*###############################################################

	Data Access Library
	© 1998-2020 by Kevin Grant
	
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
Use instead of a regular Core Foundation reference in
order to have the reference automatically retained with
CFRetain() when constructed, assigned or copied, and
released with CFRelease() when it goes out of scope or
is assigned, etc.

It is possible to have a nullptr value, and no CFRetain()
or CFRelease() occurs in this case.  It is therefore safe
to initialize to nullptr and later assign a value that
should be retained and released, etc.
*/
class CGContextSaveRestore
{
public:
	inline
	CGContextSaveRestore (CGContextSaveRestore const&);
	
	inline
	CGContextSaveRestore (CGContextRef);
	
	virtual inline
	~CGContextSaveRestore ();
	
	// IMPORTANT: It is clearer to call setCGContextRef(), that is recommended.
	// This assignment operator only exists to satisfy STL container implementations
	// and other generic constructs that could not know about specific class methods.
	virtual inline CGContextSaveRestore&
	operator = (CGContextSaveRestore const&);
	
	inline CGContextRef
	returnCGContextRef () const;
	
	inline void
	setCGContextRef (CGContextRef);

protected:

private:
	CGContextRef	_context;
};



#pragma mark Public Methods

/*!
Creates a new save state using the value of an
existing one.  CGContextSaveGState() is called
on the reference.

(2.0)
*/
CGContextSaveRestore::
CGContextSaveRestore	(CGContextSaveRestore const&	inCopy)
{
	_context = inCopy._context;
	assert(_context == inCopy._context);
	if (nullptr != _context)
	{
		CGContextSaveGState(_context);
	}
}// copy constructor


/*!
Creates a new reference using the value of an
existing one that is a generic Core Foundation
type.  CFRetain() is called on the reference.

(2.0)
*/
CGContextSaveRestore::
CGContextSaveRestore	(CGContextRef	inContext)
:
_context(inContext)
{
	if (nullptr != _context)
	{
		CGContextSaveGState(_context);
	}
}// context constructor


/*!
Calls CGContextRestoreGState() on the reference kept
by this class instance, if any.

(2.0)
*/
CGContextSaveRestore::
~CGContextSaveRestore ()
{
	if (nullptr != _context) CGContextRestoreGState(_context);
}// destructor


/*!
Equivalent to using the copy constructor to make
a new instance, but updates this instance.

(2.0)
*/
CGContextSaveRestore&
CGContextSaveRestore::
operator =	(CGContextSaveRestore const&	inCopy)
{
	if (&inCopy != this)
	{
		setCGContextRef(inCopy.returnCGContextRef());
	}
	return *this;
}// operator =


/*!
Returns the graphics context managed by this instance,
or nullptr if none.

(2.0)
*/
CGContextRef
CGContextSaveRestore::
returnCGContextRef ()
const
{
	return _context;
}// returnCGContextRef


/*!
Changes the context managed by this instance.  Any
previous context has its graphics state restored.

(1.1)
*/
void
CGContextSaveRestore::
setCGContextRef		(CGContextRef	inNewContext)
{
	if (nullptr != _context) CGContextRestoreGState(_context);
	_context = inNewContext;
	if (nullptr != _context) CGContextSaveGState(_context);
}// setCGContextRef

// BELOW IS REQUIRED NEWLINE TO END FILE
