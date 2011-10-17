/*!	\file NIBLoader.h
	\brief Simplifies Interface Builder NIB loads in
	object-oriented environments.
	
	Particularly useful in C++ objects that have to load
	NIBs, as this will automatically load an underlying
	NIB file and return the true object it represents.
*/
/*###############################################################

	Data Access Library 2.6
	© 1998-2011 by Kevin Grant
	
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

#ifndef __NIBLOADER__
#define __NIBLOADER__

// standard-C includes
#include <strings.h>

// library includes
#include <HIViewWrap.h>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

/*!
Opens a NIB bundle in the application’s main bundle
that matches the given base name (i.e. bundle name
without a ".nib" extension).  On destruction, the
NIB reference is disposed of.
*/
class NIBLoader
{
public:
	inline NIBLoader	(CFBundleRef,
						 CFStringRef);
	
	inline ~NIBLoader	();
	
	//! returns true only if the NIB file was opened successfully
	inline bool
	isLoaded	() const;
	
	//! returns the NIB, or nullptr if it is not loaded
	inline IBNibRef
	returnNIB	() const;

protected:

private:
	IBNibRef	_nibRef;
};


/*!
Loads a NIB bundle, creates a specific window in it,
disposes of the loader and returns the new window.
Although creating an instance of this class creates
a new window, destroying the instance does not affect
the window.

A WindowRef operator is defined so that this class can
literally transparently load a window from a NIB and
initialize a WindowRef data member with it.
*/
class NIBWindow
{
	typedef NIBWindow&	(*manipulator)	(NIBWindow&);

public:
	inline NIBWindow	(CFBundleRef,
						 CFStringRef,
						 CFStringRef);
	
	//! makes a NIBWindow implicitly convertible to a WindowRef
	inline operator WindowRef () const;
	
	//! allow functions for NIBWindows to act as manipulators
	inline NIBWindow&
	operator <<		(NIBWindow::manipulator);
	
	//! returns true only if the window is valid
	inline bool
	exists	() const;
	
	//! like returnHIViewWithID(), but assumes the ID is 0 and the signature is as given
	inline HIViewWrap
	returnHIViewWithCode	(OSType) const;
	
	//! the compositing-window equivalent of returnControlWithID()
	inline HIViewWrap
	returnHIViewWithID		(HIViewID const&) const;

protected:

private:
	WindowRef	_window;
};



#pragma mark Inline Methods

/*!
A convenient manipulator routine (see operator <<())
that asserts - that is, in debug mode only - that
the specified window has a valid reference.

(1.4)
*/
inline NIBWindow&
NIBLoader_AssertWindowExists	(NIBWindow&		inoutWindow)
{
	assert(inoutWindow.exists());
	return inoutWindow;
}// AssertWindowExists


NIBLoader::
NIBLoader	(CFBundleRef	inBundle,
			 CFStringRef	inBaseName)
:
_nibRef(nullptr)
{
	OSStatus	error = noErr;
	
	
	error = CreateNibReferenceWithCFBundle(inBundle, inBaseName, &_nibRef);
	if (noErr != error) _nibRef = nullptr; // nullify just in case
}// NIBLoader 2-argument constructor


NIBLoader::
~NIBLoader ()
{
	if (nullptr != _nibRef) DisposeNibReference(_nibRef);
}// NIBLoader destructor


bool
NIBLoader::
isLoaded ()
const
{
	return (nullptr != _nibRef);
}// isLoaded


IBNibRef
NIBLoader::
returnNIB ()
const
{
	return _nibRef;
}// returnNIB


NIBWindow::
NIBWindow	(CFBundleRef	inBundle,
			 CFStringRef	inNIBBaseName,
			 CFStringRef	inWindowNameInNIB)
:
_window(nullptr)
{
	NIBLoader	loader(inBundle, inNIBBaseName);
	
	
	if (loader.isLoaded())
	{
		OSStatus	error = CreateWindowFromNib(loader.returnNIB(), inWindowNameInNIB, &_window);
		
		
		if (noErr != error) _window = nullptr; // nullify just in case
	}
}// NIBWindow 2-argument constructor


NIBWindow::
operator WindowRef ()
const
{
	return _window;
}// operator WindowRef


NIBWindow&
NIBWindow::
operator <<		(NIBWindow::manipulator		inManipulator)
{
	return (*inManipulator)(*this);
}// operator <<


bool
NIBWindow::
exists ()
const
{
	return (IsValidWindowRef(_window));
}// exists


HIViewWrap
NIBWindow::
returnHIViewWithCode	(OSType		inFourCharCode)
const
{
	HIViewID	tmpID;
	
	
	bzero(&tmpID, sizeof(tmpID));
	tmpID.signature = inFourCharCode;
	tmpID.id = 0;
	return returnHIViewWithID(tmpID);
}// returnHIViewWithCode


HIViewWrap
NIBWindow::
returnHIViewWithID	(HIViewID const&	inID)
const
{
	return HIViewWrap(inID, _window);
}// returnHIViewWithID

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
