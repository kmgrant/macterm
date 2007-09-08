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

#ifndef __LISTENERMODEL__
#define __LISTENERMODEL__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
Possible return values from Listener Model module routines.
*/
enum ListenerModel_ResultCode
{
	kListenerModel_ResultCodeSuccess		= 0,	//!< no error occurred
	kListenerModel_InvalidModelReference	= 1,	//!< listener model is not recognized
	kListenerModel_InvalidListenerReference	= 2		//!< listener is not recognized
};

/*!
User-defined identifier to help distinguish models.
*/
typedef FourCharCode	ListenerModel_Descriptor;
enum
{
	kListenerModel_InvalidDescriptor = FOUR_CHAR_CODE('----')
};

/*!
How listeners are notified, and what kind of listeners
are allowed.
*/
enum ListenerModel_Style
{
	kListenerModel_StyleStandard = 0,				//!< requires Standard listeners; all listeners are always notified
	kListenerModel_StyleLogicalOR = 1,				//!< requires Boolean listeners; listeners notified until one of
													//!  the listeners returns "true"
	kListenerModel_StyleNonEventNotHandledErr = 2	//!< requires OSStatus listeners; listeners notified until one of
													//!  them returns something other than "eventNotHandledErr" (this
													//!  INCLUDES "noErr" and actual error codes)
};

#pragma mark Types

/*!
Values are arbitrary and defined by the model user.
*/
typedef FourCharCode	ListenerModel_Event;

typedef struct OpaqueListenerModel**	ListenerModel_Ref;
typedef struct OpaqueListener**			ListenerModel_ListenerRef;

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

/*!
OSStatus Listener Callback

Identical to a standard callback, except it has an OSStatus
return value.  This callback can only be used with a listener
model whose style accepts Boolean callbacks (the logical OR
style, for instance).  A logical "false" is defined as a
return value of "eventNotHandledErr"!!!
*/
typedef OSStatus (*ListenerModel_OSStatusProcPtr)	(ListenerModel_Ref		inFromWhichModel,
													 ListenerModel_Event	inEventThatOccurred,
													 void*					inEventContextPtr,
													 void*					inListenerContextPtr);
inline OSStatus
ListenerModel_InvokeOSStatusProc	(ListenerModel_OSStatusProcPtr	inUserRoutine,
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
	ListenerModel_NewOSStatusListener		(ListenerModel_OSStatusProcPtr	inCallback,
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

ListenerModel_ResultCode
	ListenerModel_RemoveListenerForEvent	(ListenerModel_Ref				inFromWhichModel,
											 ListenerModel_Event			inForWhichEvent,
											 ListenerModel_ListenerRef		inListenerToRemove);

ListenerModel_ResultCode
	ListenerModel_NotifyListenersOfEvent	(ListenerModel_Ref				inForWhichModel,
											 ListenerModel_Event			inEventThatOccurred,
											 void*							inContextPtr,
											 void*							outReturnValuePtrOrNull = nullptr);

//@}

//!\name Accessing Listeners
//@{

ListenerModel_ResultCode
	ListenerModel_GetDescriptor				(ListenerModel_Ref				inForWhichModel,
											 ListenerModel_Descriptor*		outDescriptorPtr);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
