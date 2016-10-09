/*!	\file HIViewWrap.h
	\brief Puts a ControlRef or HIViewRef in a class,
	mostly useful for implementing operator overloads.
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

#ifndef __HIVIEWWRAP__
#define __HIVIEWWRAP__

// library includes
#include <CFRetainRelease.h>
#include <HIViewWrap.fwd.h>



#pragma mark Types

/*!
Convenient wrapper for an HIViewID (ControlID).  This is very
convenient for automatically constructing objects.

A HIViewID operator is defined so that this class can act as
an HIViewID where that is required (i.e. OS APIs).
*/
class HIViewIDWrap
{
public:
	//! constructor based on signature only (ID is 0)
	inline HIViewIDWrap		(OSType				inFourCharCode);
	
	//! constructor based on ID
	inline HIViewIDWrap		(HIViewID const&	inID);
	
	//! constructor based on signature and ID
	inline HIViewIDWrap		(OSType				inFourCharCode,
							 SInt32				inIndex);
	
	//! makes a HIViewIDWrap implicitly convertible to an HIViewID
	inline operator HIViewID () const;
	
	//! equality with an ID structure
	inline bool
	operator ==	(HIViewID const&) const;
	
	//! inequality with an ID structure
	inline bool
	operator !=	(HIViewID const&) const;

protected:

private:
	HIViewID	_id;
};


/*!
Convenient wrapper for an HIView.  C++ will occasionally
require classes (e.g. with operator overloading), where
this is convenient.  This is also a quicker way to retrieve
views by ID, etc.

An HIViewRef operator is defined so that this class can
literally transparently retrieve a view with a given ID
and initialize an HIViewRef data member with it.
*/
class HIViewWrap:
public CFRetainRelease
{
	typedef HIViewWrap&		(*manipulator)	(HIViewWrap&);

public:
	//! constructor with nullptr reference
	inline HIViewWrap	();
	
	//! constructor based on view reference
	inline HIViewWrap	(HIViewRef		inView);
	
	//! constructor based on ID in window
	inline HIViewWrap	(HIViewID const&	inID,
						 WindowRef			inParentWindow);
	
	//! you should prefer setWithRetain(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);
	
	//! makes an HIViewWrap implicitly convertible to an HIViewRef
	inline operator HIViewRef () const;
	
	//! allows functions for HIViewWraps to act as manipulators
	inline HIViewWrap&
	operator <<		(HIViewWrap::manipulator	inManipulator);
	
	//! creates or returns existing accessibility object with ID zero (0)
	inline AXUIElementRef
	acquireAccessibilityObject ();
	
	//! returns true only if the view is valid
	inline bool
	exists () const;
	
	//! returns the ID of this view
	inline HIViewID
	identifier () const;

protected:

private:
	CFRetainRelease		_accessibilityObject;
};



#pragma mark Inline Methods

HIViewIDWrap::
HIViewIDWrap	(OSType		inFourCharCode)
:
_id()
{
	_id.signature = inFourCharCode;
	_id.id = 0;
}// HIViewIDWrap OSType constructor


HIViewIDWrap::
HIViewIDWrap	(HIViewID const&	inID)
:
_id(inID)
{
}// HIViewIDWrap HIViewID constructor


HIViewIDWrap::
HIViewIDWrap	(OSType		inFourCharCode,
				 SInt32		inID)
:
_id()
{
	_id.signature = inFourCharCode;
	_id.id = inID;
}// HIViewIDWrap 2-argument constructor


HIViewIDWrap::
operator HIViewID ()
const
{
	return _id;
}// operator HIViewID


bool
HIViewIDWrap::
operator ==		(HIViewID const&	inID)
const
{
	return ((_id.signature == inID.signature) && (_id.id == inID.id));
}// operator ==


bool
HIViewIDWrap::
operator !=		(HIViewID const&	inID)
const
{
	return ! operator == (inID);
}// operator !=


HIViewWrap::
HIViewWrap ()
:
CFRetainRelease()
{
}// HIViewWrap default constructor


HIViewWrap::
HIViewWrap		(HIViewRef		inView)
:
CFRetainRelease(inView, CFRetainRelease::kNotYetRetained)
{
}// HIViewWrap 1-argument constructor


HIViewWrap::
HIViewWrap		(HIViewID const&	inID,
				 WindowRef			inWindow)
:
CFRetainRelease()
{
	OSStatus	error = noErr;
	HIViewRef	view = nullptr;
	
	
	error = GetControlByID(inWindow, &inID, &view);
	if (noErr != error) view = nullptr; // nullify just in case
	this->setWithRetain(view);
}// HIViewWrap 2-argument constructor


CFRetainRelease&
HIViewWrap::
operator =	(CFRetainRelease const&		inCopy)
{
	return CFRetainRelease::operator =(inCopy);
}// operator =


HIViewWrap::
operator HIViewRef ()
const
{
	return REINTERPRET_CAST(returnHIObjectRef(), HIViewRef);
}// operator HIViewRef


HIViewWrap&
HIViewWrap::
operator <<		(HIViewWrap::manipulator	inManipulator)
{
	return (*inManipulator)(*this);
}// operator <<


AXUIElementRef
HIViewWrap::
acquireAccessibilityObject ()
{
	AXUIElementRef		result = nullptr;
	
	
	if (_accessibilityObject.exists())
	{
		result = REINTERPRET_CAST(_accessibilityObject.returnCFTypeRef(), AXUIElementRef);
	}
	else
	{
		result = AXUIElementCreateWithHIObjectAndIdentifier(this->returnHIObjectRef(), 0/* sub-part identifier */);
		_accessibilityObject.setWithNoRetain(result);
	}
	return result;
}// acquireAccessibilityObject


bool
HIViewWrap::
exists ()
const
{
	HIViewRef	view = this->operator HIViewRef();
	
	
	// TEMPORARY - is there a better Mac OS X call to determine valid HIObjects?
	return ((nullptr != view) && IsValidControlHandle(view));
}// exists


HIViewID
HIViewWrap::
identifier ()
const
{
	HIViewID	result = { '----', 0 };
	
	
	UNUSED_RETURN(OSStatus)GetControlID(this->operator HIViewRef(), &result);
	return result;
}// identifier

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
