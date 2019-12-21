/*!	\file ListenerModel.h
	\brief An implementation of the listener pattern.
	
	This module allows you to write generic code that notifies
	a potentially anonymous list of “listeners” (callback
	functions) when specific events take place.  You can use a
	single model to register many, many unique event types,
	each of which can have a unique list of callbacks.
	
	In addition, there are a few different kinds of rules you
	can apply when notifying listeners.  For example, you can
	just notify all listeners in order, blindly.  Or, you can
	request that each listener return a flag value, and stop
	notifying listeners as soon as one returns "true".
	
	NSNotificationCenter is a reasonable replacement for this
	module but most code has not been converted to Objective-C.
	As an interim measure, Objective-C message-passing has
	been added to this module as an option, and longer-term
	any code that supports listeners may change to use Cocoa
	exclusively.
*/
/*###############################################################

	Data Access Library
	© 1998-2019 by Kevin Grant
	
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

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <RetainRelease.template.h>



#pragma mark Constants

/*!
Possible return values from Listener Model module routines.
*/
enum ListenerModel_Result
{
	kListenerModel_ResultOK							= 0,	//!< no error occurred
	kListenerModel_ResultInvalidModelReference		= 1,	//!< listener model is not recognized
	kListenerModel_ResultInvalidListenerReference	= 2		//!< listener is not recognized
};

/*!
User-defined identifier to help distinguish models.
*/
typedef FourCharCode	ListenerModel_Descriptor;
enum
{
	kListenerModel_InvalidDescriptor = '----'
};

/*!
How listeners are notified, and what kind of listeners
are allowed.
*/
enum ListenerModel_Style
{
	kListenerModel_StyleStandard = 0,				//!< requires Standard listeners; all listeners are always notified
	kListenerModel_StyleLogicalOR = 1				//!< requires Boolean listeners; listeners notified until one of
													//!  the listeners returns "true"
};

#pragma mark Types

/*!
Values are arbitrary and defined by the model user.
*/
typedef FourCharCode	ListenerModel_Event;

typedef struct OpaqueListenerModel**	ListenerModel_Ref;
typedef struct OpaqueListener**			ListenerModel_ListenerRef;

#ifdef __OBJC__

/*!
An object that allows you to forward listener events to a
target object instead of having a separate C function.

Typically you allocate an object of this type in another
Objective-C object, whose "self" you pass in as the target.

To install this listener via the APIs of other modules that
may depend on listener references, use the "listenerRef"
method.

NOTE:	The underlying listener that is created uses an
		internal C function as a callback, and sets that
		callback’s “listener context” to be the wrapper
		object itself.  If you require listener-specific
		context information, just make sure that your
		target object can access the data that it needs.
*/
@interface ListenerModel_StandardListener : NSObject < NSCopying > //{
{
	ListenerModel_ListenerRef	listenerRef;
	NSInvocation*				methodInvoker;
}

// initializers
	- (instancetype)
	initWithTarget:(id)_
	eventFiredSelector:(SEL)_;

// new methods
	- (void)
	listenerModel:(ListenerModel_Ref)_
	firedEvent:(ListenerModel_Event)_
	context:(void*)_; // forwards an event to the selector on the target

// accessors
	- (ListenerModel_ListenerRef)
	listenerRef; // for compatibility with older APIs
	- (void)
	setEventFiredSelector:(SEL)_; // change selector given at initialization time
	- (void)
	setTarget:(id)_; // change target given at initialization time

@end //}

#endif // __OBJC__

#pragma mark Callbacks

/*!
Standard Listener Callback

These routines are notifiers that are attached to listener
models.  When a particular kind of event occurs, a callback
such as this may be invoked.

There are two context parameters: the first varies only on
a per-event basis (that is, it is specified at notification
time), but the second varies on a per-callback basis (that
is, it is specified at callback construction time).  In
most cases, the former is defined by an external module
providing notification services and the latter is defined
by the receiver to be whatever is useful (often a reference
type that can be used to retrieve or change data).
*/
typedef void (*ListenerModel_StandardProcPtr)	(ListenerModel_Ref		inFromWhichModel,
												 ListenerModel_Event	inEventThatOccurred,
												 void*					inEventContextPtr,
												 void*					inListenerContextPtr);
inline void
ListenerModel_InvokeStandardProc	(ListenerModel_StandardProcPtr	inUserRoutine,
									 ListenerModel_Ref				inFromWhichModel,
									 ListenerModel_Event			inEventThatOccurred,
									 void*							inEventContextPtr,
									 void*							inListenerContextPtr)
{
	(*inUserRoutine)(inFromWhichModel, inEventThatOccurred, inEventContextPtr, inListenerContextPtr);
}

/*!
Boolean Listener Callback

Identical to a standard callback, except it has a Boolean
return value.  This callback can only be used with a
listener model whose style accepts Boolean callbacks (the
logical OR style, for instance).
*/
typedef Boolean (*ListenerModel_BooleanProcPtr)	(ListenerModel_Ref		inFromWhichModel,
												 ListenerModel_Event	inEventThatOccurred,
												 void*					inEventContextPtr,
												 void*					inListenerContextPtr);
inline Boolean
ListenerModel_InvokeBooleanProc		(ListenerModel_BooleanProcPtr	inUserRoutine,
									 ListenerModel_Ref				inFromWhichModel,
									 ListenerModel_Event			inEventThatOccurred,
									 void*							inEventContextPtr,
									 void*							inListenerContextPtr)
{
	return (*inUserRoutine)(inFromWhichModel, inEventThatOccurred, inEventContextPtr, inListenerContextPtr);
}



#pragma mark Public Methods

//!\name Module Tests
//@{

void
	ListenerModel_RunTests					();

//@}

//!\name Creating and Destroying Listener Models
//@{

ListenerModel_Ref
	ListenerModel_New						(ListenerModel_Style			inStyle,
											 ListenerModel_Descriptor		inDescriptor);

void
	ListenerModel_Dispose					(ListenerModel_Ref*				inoutRefPtr);

//@}

//!\name Creating and Destroying Listeners
//@{

ListenerModel_ListenerRef
	ListenerModel_NewBooleanListener		(ListenerModel_BooleanProcPtr	inCallback,
											 void*							inContextOrNull = nullptr);

ListenerModel_ListenerRef
	ListenerModel_NewStandardListener		(ListenerModel_StandardProcPtr	inCallback,
											 void*							inContextOrNull = nullptr);

void
	ListenerModel_RetainListener			(ListenerModel_ListenerRef		inCallback);

void
	ListenerModel_ReleaseListener			(ListenerModel_ListenerRef*		inoutRefPtr);

//@}

//!\name Manipulating Listener Models
//@{

OSStatus
	ListenerModel_AddListenerForEvent		(ListenerModel_Ref				inToWhichModel,
											 ListenerModel_Event			inForWhichEvent,
											 ListenerModel_ListenerRef		inListenerToAdd);

Boolean
	ListenerModel_IsAnyListenerForEvent		(ListenerModel_Ref				inForWhichModel,
											 ListenerModel_Event			inEventThatOccurred);

ListenerModel_Result
	ListenerModel_RemoveListenerForEvent	(ListenerModel_Ref				inFromWhichModel,
											 ListenerModel_Event			inForWhichEvent,
											 ListenerModel_ListenerRef		inListenerToRemove);

ListenerModel_Result
	ListenerModel_NotifyListenersOfEvent	(ListenerModel_Ref				inForWhichModel,
											 ListenerModel_Event			inEventThatOccurred,
											 void*							inContextPtr,
											 void*							outReturnValuePtrOrNull = nullptr);

//@}

//!\name Accessing Listeners
//@{

ListenerModel_Result
	ListenerModel_GetDescriptor				(ListenerModel_Ref				inForWhichModel,
											 ListenerModel_Descriptor*		outDescriptorPtr);

//@}



#pragma mark Types Dependent on Method Names

// DO NOT USE DIRECTLY.
struct _ListenerModel_ListenerRefMgr
{
	typedef ListenerModel_ListenerRef	reference_type;
	
	static void
	retain	(reference_type		inRef)
	{
		ListenerModel_RetainListener(inRef);
	}
	
	static void
	release	(reference_type		inRef)
	{
		ListenerModel_ReleaseListener(&inRef);
	}
};

/*!
Allows RAII-based automatic retain and release of a listener, so
you don’t have to call ListenerModel_RetainListener() or
ListenerModel_ReleaseListener() yourself.  Simply declare a
variable of this type (in a data structure, say), initialize it
as appropriate, and your reference is safe.  Note that there is
a constructor that allows you to store pre-retained (e.g. newly
allocated) listeners too.
*/
typedef RetainRelease<_ListenerModel_ListenerRefMgr>	ListenerModel_ListenerWrap;

// BELOW IS REQUIRED NEWLINE TO END FILE
