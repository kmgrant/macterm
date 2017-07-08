/*!	\file HIViewWrapManip.h
	\brief Manipulators for HIViewWrap class instances,
	which can be used via operator <<().
	
	Note that the operator <<() member of HIViewWrap
	allows any basic functor that takes and returns an
	HIViewWrap reference to act as a manipulator.  But,
	when manipulators need arguments, special wrappers
	must exist to enable them.
	
	This file provides such wrappers.  They only exist
	for common functionality likely to be useful when
	initializing a new view (indeed, the whole point of
	the operator is to allow a chain of initializers at
	object setup time).
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

// standard-C++ includes
#include <iostream>

// library includes
#include <Console.h>
#include <HIViewWrap.h>



#pragma mark Public Methods

/*!
A convenient manipulator routine (see operator <<())
that asserts - that is, in debug mode only - that
the specified view has a valid reference.

NOTE:	A manipulator that takes only a class reference
		argument does not require its own operator,
		because the HIViewWrap class implements this
		particular operator <<() on its own.

(1.4)
*/
inline HIViewWrap&
HIViewWrap_AssertExists		(HIViewWrap&	inoutView)
{
	assert(inoutView.exists());
	return inoutView;
}// AssertExists


/*!
A convenient manipulator routine (see operator <<())
that alters the size of a view.

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_DeltaSize
{
	Float32		_deltaX;
	Float32		_deltaY;
	Boolean		_debug;
};

inline __HIViewWrap_DeltaSize
HIViewWrap_DeltaSize	(Float32	inDeltaX,
						 Float32	inDeltaY,
						 Boolean	inDebug = false)
{
	__HIViewWrap_DeltaSize	result;
	result._deltaX = inDeltaX;
	result._deltaY = inDeltaY;
	result._debug = inDebug;
	return result;
}// DeltaSize

inline HIViewWrap&
operator <<		(HIViewWrap&				inoutView,
				 __HIViewWrap_DeltaSize		inData)
{
	OSStatus	error = noErr;
	HIRect		viewFrame;
	
	
	error = HIViewGetFrame(inoutView, &viewFrame);
	if (inData._debug)
	{
		Rect	windowBounds;
		
		
		error = GetWindowBounds(GetControlOwner(inoutView), kWindowContentRgn, &windowBounds);
		assert_noerr(error);
		std::cout
		<< "HIViewWrap_DeltaSize: Delta X, delta Y are "
		<< inData._deltaX << ", "
		<< inData._deltaY
		<< std::endl
		;
		std::cout
		<< "HIViewWrap_DeltaSize: I think the view is in a window whose origin and size are "
		<< windowBounds.left << ", "
		<< windowBounds.top << ", "
		<< windowBounds.right - windowBounds.left << ", "
		<< windowBounds.bottom - windowBounds.top << "; "
		<< "I think the view origin and size are "
		<< viewFrame.origin.x << ", "
		<< viewFrame.origin.y << ", "
		<< viewFrame.size.width << ", "
		<< viewFrame.size.height
		<< std::endl
		;
	}
	assert_noerr(error);
	viewFrame.size.width += inData._deltaX;
	viewFrame.size.height += inData._deltaY;
	error = HIViewSetFrame(inoutView, &viewFrame);
	assert_noerr(error);
	if (inData._debug)
	{
		std::cout
		<< "HIViewWrap_DeltaSize: The view origin and size have changed to "
		<< viewFrame.origin.x << ", "
		<< viewFrame.origin.y << ", "
		<< viewFrame.size.width << ", "
		<< viewFrame.size.height
		<< std::endl
		;
	}
	return inoutView;
}// operator <<


/*!
A convenient manipulator routine (see operator <<())
that installs an arbitrary key filter routine on a
control.

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_InstallKeyFilter
{
	ControlKeyFilterUPP		_installedUPP;
};

inline __HIViewWrap_InstallKeyFilter
HIViewWrap_InstallKeyFilter		(ControlKeyFilterUPP	inUPP)
{
	__HIViewWrap_InstallKeyFilter	result;
	result._installedUPP = inUPP;
	return result;
}// InstallKeyFilter

inline HIViewWrap&
operator <<		(HIViewWrap&					inoutView,
				 __HIViewWrap_InstallKeyFilter	inData)
{
	OSStatus	error = SetControlData(inoutView, kControlNoPart, kControlKeyFilterTag,
										sizeof(inData._installedUPP), &inData._installedUPP);
	
	
	assert_noerr(error);
	return inoutView;
}// operator <<


/*!
A convenient manipulator routine (see operator <<())
that embeds a view in a parent (that is, it makes the
target view a subview of the specified view).

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_EmbedIn
{
	HIViewRef	_parentView;
};

inline __HIViewWrap_EmbedIn
HIViewWrap_EmbedIn		(HIViewRef		inParentView)
{
	__HIViewWrap_EmbedIn	result;
	result._parentView = inParentView;
	return result;
}// EmbedIn

inline HIViewWrap&
operator <<		(HIViewWrap&			inoutView,
				 __HIViewWrap_EmbedIn	inData)
{
	OSStatus	error = HIViewAddSubview(inData._parentView, inoutView);
	
	
	assert_noerr(error);
	return inoutView;
}// operator <<


/*!
A convenient manipulator routine (see operator <<())
that alters the location of a view within its parent.

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_MoveBy
{
	Float32		_deltaX;
	Float32		_deltaY;
};

inline __HIViewWrap_MoveBy
HIViewWrap_MoveBy	(Float32	inDeltaX,
					 Float32	inDeltaY)
{
	__HIViewWrap_MoveBy	result;
	result._deltaX = inDeltaX;
	result._deltaY = inDeltaY;
	return result;
}// MoveBy

inline HIViewWrap&
operator <<		(HIViewWrap&				inoutView,
				 __HIViewWrap_MoveBy		inData)
{
	OSStatus	error = noErr;
	
	
	error = HIViewMoveBy(inoutView, inData._deltaX, inData._deltaY);
	assert_noerr(error);
	return inoutView;
}// operator <<


/*!
A convenient manipulator routine (see operator <<())
that alters the size of a view.

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_Resize
{
	Float32		_width;
	Float32		_height;
};

inline __HIViewWrap_Resize
HIViewWrap_Resize	(Float32	inWidth,
					 Float32	inHeight)
{
	__HIViewWrap_Resize		result;
	result._width = inWidth;
	result._height = inHeight;
	return result;
}// Resize

inline HIViewWrap&
operator <<		(HIViewWrap&			inoutView,
				 __HIViewWrap_Resize	inData)
{
	OSStatus	error = noErr;
	HIRect		viewFrame;
	
	
	error = HIViewGetFrame(inoutView, &viewFrame);
	assert_noerr(error);
	viewFrame.size.width = inData._width;
	viewFrame.size.height = inData._height;
	error = HIViewSetFrame(inoutView, &viewFrame);
	assert_noerr(error);
	return inoutView;
}// operator <<


/*!
A convenient manipulator routine (see operator <<())
that makes a control active or inactive.

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_SetActiveState
{
	Boolean		_makeActive;
};

inline __HIViewWrap_SetActiveState
HIViewWrap_SetActiveState		(Boolean	inActivate)
{
	__HIViewWrap_SetActiveState	result;
	result._makeActive = inActivate;
	return result;
}// SetActiveState

inline HIViewWrap&
operator <<		(HIViewWrap&					inoutView,
				 __HIViewWrap_SetActiveState	inData)
{
	OSStatus	error = noErr;
	
	
	if (false == inData._makeActive) error = DeactivateControl(inoutView);
	else error = ActivateControl(inoutView);
	
	
	assert_noerr(error);
	return inoutView;
}// operator <<


/*!
A convenient manipulator routine (see operator <<())
that makes a control visible or invisible.

NOTE:	Implementing a manipulator that takes arguments
		requires 3 declarations (below): a class to
		uniquify the operator <<(), the front end function
		that the user would actually call, and finally
		the operator <<() that makes it all work.

(1.4)
*/
struct __HIViewWrap_SetVisibleState
{
	Boolean		_makeVisible;
};

inline __HIViewWrap_SetVisibleState
HIViewWrap_SetVisibleState		(Boolean	inMakeVisible)
{
	__HIViewWrap_SetVisibleState	result;
	result._makeVisible = inMakeVisible;
	return result;
}// SetVisibleState

inline HIViewWrap&
operator <<		(HIViewWrap&					inoutView,
				 __HIViewWrap_SetVisibleState	inData)
{
	OSStatus	error = HIViewSetVisible(inoutView, inData._makeVisible);
	
	
	assert_noerr(error);
	return inoutView;
}// operator <<

// BELOW IS REQUIRED NEWLINE TO END FILE
