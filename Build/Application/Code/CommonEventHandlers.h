/*!	\file CommonEventHandlers.h
	\brief Simplifies writing handlers for very common
	kinds of events.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#ifndef __COMMONEVENTHANDLERS__
#define __COMMONEVENTHANDLERS__

// Mac includes
#include <Carbon/Carbon.h>



#pragma mark Constants

typedef UInt16 CommonEventHandlers_ChangedBoundsEdges;
enum
{
	kCommonEventHandlers_ChangedBoundsEdgeLeft			= (1 << 0),		//!< set if you want to know when a view’s left edge changes
	kCommonEventHandlers_ChangedBoundsEdgeTop			= (1 << 1),		//!< set if you want to know when a view’s top edge changes
	kCommonEventHandlers_ChangedBoundsEdgeRight			= (1 << 2),		//!< set if you want to know when a view’s right edge changes
	kCommonEventHandlers_ChangedBoundsEdgeBottom		= (1 << 3),		//!< set if you want to know when a view’s bottom edge changes
	kCommonEventHandlers_ChangedBoundsEdgeSeparationH	= (1 << 4),		//!< set if you want to know when a view’s width changes
	kCommonEventHandlers_ChangedBoundsEdgeSeparationV	= (1 << 5),		//!< set if you want to know when a view’s height changes
	kCommonEventHandlers_ChangedBoundsAnyEdge			= kCommonEventHandlers_ChangedBoundsEdgeLeft |
															kCommonEventHandlers_ChangedBoundsEdgeTop |
															kCommonEventHandlers_ChangedBoundsEdgeRight |
															kCommonEventHandlers_ChangedBoundsEdgeBottom
};

#pragma mark Callbacks

/*!
HIView Resize Handler

Invoked by a Carbon Event handler internal to
this module after a bounds-change event is shown
to be resizing a monitored view.  The input is
the change in width and height of the view, where
positive values increase down and right and
negative values decrease up and left.
*/
typedef void (*CommonEventHandlers_HIViewResizeProcPtr)	(HIViewRef		inView,
														 Float32		inDeltaX,
														 Float32		inDeltaY,
														 void*			inContext);
inline void
CommonEventHandlers_InvokeHIViewResizeProc	(CommonEventHandlers_HIViewResizeProcPtr	inProc,
											 HIViewRef									inView,
											 Float32									inDeltaX,
											 Float32									inDeltaY,
											 void*										inContext)
{
	(*inProc)(inView, inDeltaX, inDeltaY, inContext);
}

/*!
HIWindow Resize Handler

Invoked by a Carbon Event handler internal to
this module after a bounds-change event is shown
to be resizing a monitored window.  The input is
the change in width and height of the window,
where positive values increase down and right and
negative values decrease up and left.
*/
typedef void (*CommonEventHandlers_HIWindowResizeProcPtr)	(HIWindowRef	inWindow,
															 Float32		inDeltaX,
															 Float32		inDeltaY,
															 void*			inContext);
inline void
CommonEventHandlers_InvokeHIWindowResizeProc	(CommonEventHandlers_HIWindowResizeProcPtr	inProc,
												 HIWindowRef								inWindow,
												 Float32									inDeltaX,
												 Float32									inDeltaY,
												 void*										inContext)
{
	(*inProc)(inWindow, inDeltaX, inDeltaY, inContext);
}

#pragma mark Types

typedef struct CommonEventHandlers_OpaqueNumericalFieldArrows*	CommonEventHandlers_NumericalFieldArrowsRef;
typedef struct CommonEventHandlers_OpaquePopUpMenuArrows*		CommonEventHandlers_PopUpMenuArrowsRef;

/*!
Automatically installs Carbon Event handlers for all typical
HIView size events, and provides a simpler entry point - functions
of the form "CommonEventHandlers_HIViewResizeProcPtr" - to
communicate with your custom code.

IMPORTANT:	Upon destruction, the handlers are removed.  It
follows that you should have at most one instance of this class
per HIView!
*/
class CommonEventHandlers_HIViewResizerImpl;
class CommonEventHandlers_HIViewResizer
{
public:
	inline CommonEventHandlers_HIViewResizer ();
	
	inline
	CommonEventHandlers_HIViewResizer	(HIViewRef									inForWhichView,
										 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
										 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
										 void*										inContext);
	
	inline ~CommonEventHandlers_HIViewResizer ();
	
	inline bool
	install		(HIViewRef									inForWhichView,
				 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
				 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
				 void*										inContext);
	
	inline bool
	isInstalled () const;

protected:
	static OSStatus
	installAs	(HIViewRef									inForWhichView,
				 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
				 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
				 void*										inContext,
				 CommonEventHandlers_HIViewResizerImpl*&	outHandlerPtr);
	
	static void
	remove	(CommonEventHandlers_HIViewResizerImpl*&	inoutImplPtr);

private:
	CommonEventHandlers_HIViewResizerImpl*	_impl;
	bool									_installed;
};


/*!
Automatically installs Carbon Event handlers for all typical
window size events, and provides a simpler entry point - functions
of the form "CommonEventHandlers_WindowResizeProcPtr" - to
communicate with your custom code.

IMPORTANT:	Upon destruction, the handlers are removed.  It
follows that you should have at most one instance of this class
per Mac OS window!
*/
class CommonEventHandlers_WindowResizerImpl;
class CommonEventHandlers_WindowResizer
{
public:
	inline CommonEventHandlers_WindowResizer ();
	
	inline
	CommonEventHandlers_WindowResizer	(WindowRef									inForWhichWindow,
										 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
										 void*										inContext,
										 Float32									inMinimumWidth,
										 Float32									inMinimumHeight,
										 Float32									inMaximumWidth,
										 Float32									inMaximumHeight);
	
	inline ~CommonEventHandlers_WindowResizer ();
	
	OSStatus
	getWindowMaximumSize	(Float32&	outMaximumWidth,
							 Float32&	outMaximumHeight);
	
	inline bool
	install		(WindowRef									inForWhichWindow,
				 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
				 void*										inContext,
				 Float32									inMinimumWidth,
				 Float32									inMinimumHeight,
				 Float32									inMaximumWidth,
				 Float32									inMaximumHeight);
	
	inline bool
	isInstalled () const;
	
	OSStatus
	setWindowIdealSize		(Float32	inIdealWidth,
							 Float32	inIdealHeight);
	
	OSStatus
	setWindowMaximumSize	(Float32	inMaximumWidth,
							 Float32	inMaximumHeight);
	
	OSStatus
	setWindowMinimumSize	(Float32	inMinimumWidth,
							 Float32	inMinimumHeight);

protected:
	static OSStatus
	installAs	(WindowRef									inForWhichWindow,
				 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
				 void*										inContext,
				 Float32									inMinimumWidth,
				 Float32									inMinimumHeight,
				 Float32									inMaximumWidth,
				 Float32									inMaximumHeight,
				 CommonEventHandlers_WindowResizerImpl*&	outHandlerPtr);
	
	static void
	remove	(CommonEventHandlers_WindowResizerImpl*&	inoutImplPtr);

private:
	CommonEventHandlers_WindowResizerImpl*	_impl;
	bool									_installed;
};



#pragma mark Inline Methods

/*!
Initializes an instance without installing any
event handlers.

(3.1)
*/
CommonEventHandlers_HIViewResizer::	
CommonEventHandlers_HIViewResizer ()
:
_impl(nullptr),
_installed(false)
{
}// default constructor


/*!
Installs various Carbon Event handlers to handle all
typical view resize related events.  The handler is
called only when the specified edges are changed.

You can call isInstalled() to see if the handlers were
installed successfully.
*/
CommonEventHandlers_HIViewResizer::
CommonEventHandlers_HIViewResizer	(HIViewRef									inForWhichView,
									 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
									 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
									 void*										inContext)
:
_impl(nullptr),
_installed(false)
{
	install(inForWhichView, inEdgesOfInterest, inResizeProc, inContext);
}// 4-argument constructor


/*!
Removes any installed handler.

(3.1)
*/
CommonEventHandlers_HIViewResizer::	
~CommonEventHandlers_HIViewResizer ()
{
	remove(_impl);
}// destructor


/*!
Installs various Carbon Event handlers to handle all
typical view resize related events.  The handler is
called only when the specified edges are changed.

Returns true only if the handler is successfully
installed.  (You may also call isInstalled() later
to inspect the same result.)

(3.1)
*/
bool
CommonEventHandlers_HIViewResizer::
install		(HIViewRef									inForWhichView,
			 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
			 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
			 void*										inContext)
{
	if (nullptr != _impl) remove(_impl);
	_installed = (noErr == installAs(inForWhichView, inEdgesOfInterest, inResizeProc, inContext, _impl));
	return _installed;
}// install


/*!
Returns true only if a handler is installed for this
instance.  (For example, if the default constructor
was used, nothing will have been installed.)

(3.1)
*/
bool
CommonEventHandlers_HIViewResizer::
isInstalled ()
const
{
	return _installed;
}// isInstalled


/*!
Installs various Carbon Event handlers to handle all
typical window resize related events.  The handler
enforces the given size constraints.

You can call isInstalled() to see if the handlers were
installed successfully.

(3.1)
*/
CommonEventHandlers_WindowResizer::	
CommonEventHandlers_WindowResizer	(HIWindowRef								inForWhichWindow,
									 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
									 void*										inContext,
									 Float32									inMinimumWidth,
									 Float32									inMinimumHeight,
									 Float32									inMaximumWidth,
									 Float32									inMaximumHeight)
:
_impl(nullptr),
_installed(false)
{
	install(inForWhichWindow, inResizeProc, inContext, inMinimumWidth, inMinimumHeight,
			inMaximumWidth, inMaximumHeight);
}// 7-argument constructor


/*!
Initializes an instance without installing any
event handlers.

(3.1)
*/
CommonEventHandlers_WindowResizer::	
CommonEventHandlers_WindowResizer ()
:
_impl(nullptr),
_installed(false)
{
}// default constructor


/*!
Initializes an instance without installing any
event handlers.

(3.1)
*/
CommonEventHandlers_WindowResizer::	
~CommonEventHandlers_WindowResizer ()
{
	remove(_impl);
	_installed = false;
}// default constructor


/*!
Installs various Carbon Event handlers to handle all
typical window resize related events.  The handler
enforces the given size constraints.

Returns true only if the handler is successfully
installed.  (You may also call isInstalled() later
to inspect the same result.)

(3.1)
*/
bool
CommonEventHandlers_WindowResizer::
install		(WindowRef									inForWhichWindow,
			 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
			 void*										inContext,
			 Float32									inMinimumWidth,
			 Float32									inMinimumHeight,
			 Float32									inMaximumWidth,
			 Float32									inMaximumHeight)
{
	if (nullptr != _impl) remove(_impl);
	_installed = (noErr == installAs(inForWhichWindow, inResizeProc, inContext, inMinimumWidth, inMinimumHeight,
										inMaximumWidth, inMaximumHeight, _impl));
	return _installed;
}// install


/*!
Returns true only if a handler is installed for this
instance.  (For example, if the default constructor
was used, nothing will have been installed.)

(3.1)
*/
bool
CommonEventHandlers_WindowResizer::
isInstalled ()
const
{
	return _installed;
}// isInstalled

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
