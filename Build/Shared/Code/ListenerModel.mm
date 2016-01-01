/*!	\file ListenerModel.mm
	\brief An implementation of the listener pattern.
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

#import <ListenerModel.h>
#import <UniversalDefines.h>

// standard-C++ includes
#import <algorithm>
#import <map>
#import <vector>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlockReferenceTracker.template.h>
#import <Registrar.template.h>



#pragma mark Constants
namespace {

typedef UInt16 ListenerModelCallbackType;
enum
{
	// what kind of listeners are allowed
	kListenerModelCallbackTypeStandard = 0,		// callback returns nothing
	kListenerModelCallbackTypeBoolean = 1,		// callback returns true or false
	kListenerModelCallbackTypeOSStatus = 2		// callback returns any Mac OS OSStatus value
};

typedef UInt16 ListenerModelBehavior;
enum
{
	// how listeners are notified
	kListenerModelBehaviorNotifyAllSequentially = 0,					// regardless of return value, always notify all listeners
	kListenerModelBehaviorNotifyUntilReturnedTrue = 1,					// stop as soon as a callback returns true
	kListenerModelBehaviorNotifyUntilReturnedNonEventNotHandledErr = 2	// continue as long as callbacks return "eventNotHandledErr"
};

} // anonymous namespace

#pragma mark Types
namespace {

typedef std::vector< ListenerModel_ListenerRef >			My_ListenerList;
typedef My_ListenerList*									My_ListenerListPtr;
typedef std::map< ListenerModel_Event, My_ListenerListPtr >	My_EventToListenerListPtrMap;

typedef MemoryBlockReferenceTracker< ListenerModel_Ref >			ListenerModelRefTracker;
typedef Registrar< ListenerModel_Ref, ListenerModelRefTracker >		ListenerModelRefRegistrar;

struct ListenerModel
{
	inline
	ListenerModel ();
	
	ListenerModelRefRegistrar		refValidator;			// ensures this reference is recognized as a valid one
	ListenerModel_Descriptor		descriptor;				// user-defined identifier for this model
	ListenerModelBehavior			notificationBehavior;	// a "kListenerModelBehavior..." constant describing how to notify listeners
	ListenerModelCallbackType		callbackType;			// what kind all listeners must be
	My_EventToListenerListPtrMap	eventListeners;			// associative array of one or more "Listener" structures per event type
};
typedef ListenerModel*		ListenerModelPtr;

typedef MemoryBlockPtrLocker< ListenerModel_Ref, ListenerModel >			ListenerModelPtrLocker;
typedef LockAcquireRelease< ListenerModel_Ref, ListenerModel >				ListenerModelAutoLocker;

typedef MemoryBlockReferenceTracker< ListenerModel_ListenerRef >			ListenerReferenceTracker;
typedef Registrar< ListenerModel_ListenerRef, ListenerReferenceTracker >	ListenerReferenceRegistrar;

struct Listener
{
	inline
	Listener ();
	
	ListenerReferenceRegistrar	refValidator;	// ensures this reference is recognized as a valid one
	ListenerModelCallbackType	callbackType;	// a "kListenerModelCallbackType..." constant specifying the legal union member
	void*						context;		// context assigned at creation time to help callback code figure out what, specifically, this is for
	union
	{
		ListenerModel_BooleanProcPtr	boolean;
		ListenerModel_OSStatusProcPtr	osStatus;
		ListenerModel_StandardProcPtr	standard;
	} callback;		// function to invoke
};
typedef Listener*		ListenerPtr;
typedef ListenerPtr*	ListenerHandle;

typedef MemoryBlockPtrLocker< ListenerModel_ListenerRef, Listener >			ListenerPtrLocker;
typedef LockAcquireRelease< ListenerModel_ListenerRef, Listener >			ListenerAutoLocker;
typedef MemoryBlockReferenceLocker< ListenerModel_ListenerRef, Listener >	ListenerReferenceLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void		objectiveCStandardListener	(ListenerModel_Ref, ListenerModel_Event, void*, void*);
Boolean		unitTest000_Begin			();
void		unitTest000_Callback1		(ListenerModel_Ref, ListenerModel_Event, void*, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModelPtrLocker&		gListenerModelPtrLocks ()	{ static ListenerModelPtrLocker x; return x; }
ListenerModelRefTracker&	gListenerModelValidRefs ()	{ static ListenerModelRefTracker x; return x; }
ListenerPtrLocker&			gListenerPtrLocks ()		{ static ListenerPtrLocker x; return x; }
ListenerReferenceLocker&	gListenerRefLocks ()		{ static ListenerReferenceLocker x; return x; }
ListenerReferenceTracker&	gListenerValidRefs ()		{ static ListenerReferenceTracker x; return x; }
SInt32						gUnitTest000_CallCount = 0;
ListenerModel_Ref			gUnitTest000_Model = nullptr;
Boolean						gUnitTest000_Result = false;

} // anonymous namespace


#pragma mark Functors

/*!
This is a functor designed to invoke a listener
of the Boolean type (that is, it has a return
value of true or false).

Model of STL Predicate.

(1.0)
*/
#pragma mark booleanListenerInvoker
class booleanListenerInvoker:
public std::unary_function< My_EventToListenerListPtrMap::value_type/* argument */, bool/* return */ >
{
public:
	booleanListenerInvoker		(ListenerModel_Ref		inForWhichModel,
								 ListenerModel_Event	inEventThatOccurred,
								 void*					inEventContextPtr)
	: _model(inForWhichModel), _event(inEventThatOccurred), _context(inEventContextPtr), _anyInvalidListeners(false)
	{
	}
	
	bool
	operator()	(ListenerModel_ListenerRef	inListener)
	{
		ListenerAutoLocker	listenerPtr(gListenerPtrLocks(), inListener);
		bool				result = false;
		
		
		if (gListenerValidRefs().end() == gListenerValidRefs().find(inListener))
		{
			Console_Warning(Console_WriteValueAddress, "attempt to notify nonexistent Boolean listener",
							inListener);
		}
		else
		{
			result = ListenerModel_InvokeBooleanProc(listenerPtr->callback.boolean, _model, _event,
														_context, listenerPtr->context);
		}
		return result;
	}
	
	bool
	anyInvalidListeners () const
	{
		return _anyInvalidListeners;
	}

protected:

private:
	ListenerModel_Ref		_model;
	ListenerModel_Event		_event;
	void*					_context;
	bool					_anyInvalidListeners;
};


/*!
This is a functor designed to invoke a listener
of the OSStatus type (that is, it has a return
value of true as long as the result is something
other than "eventNotHandledErr").

Model of STL Predicate.

(1.0)
*/
#pragma mark osStatusListenerInvoker
class osStatusListenerInvoker:
public std::unary_function< My_EventToListenerListPtrMap::value_type/* argument */, bool/* return */ >
{
public:
	osStatusListenerInvoker		(ListenerModel_Ref		inForWhichModel,
								 ListenerModel_Event	inEventThatOccurred,
								 void*					inEventContextPtr)
	: _model(inForWhichModel), _event(inEventThatOccurred), _context(inEventContextPtr), _anyInvalidListeners(false)
	{
	}
	
	bool
	operator()	(ListenerModel_ListenerRef	inListener)
	{
		ListenerAutoLocker	listenerPtr(gListenerPtrLocks(), inListener);
		bool				result = false;
		
		
		if (gListenerValidRefs().end() == gListenerValidRefs().find(inListener))
		{
			Console_Warning(Console_WriteValueAddress, "attempt to notify nonexistent OSStatus listener",
							inListener);
		}
		else
		{
			result = (eventNotHandledErr != ListenerModel_InvokeOSStatusProc
											(listenerPtr->callback.osStatus, _model, _event,
												_context, listenerPtr->context));
		}
		return result;
	}
	
	bool
	anyInvalidListeners () const
	{
		return _anyInvalidListeners;
	}

protected:

private:
	ListenerModel_Ref		_model;
	ListenerModel_Event		_event;
	void*					_context;
	bool					_anyInvalidListeners;
};


/*!
This is a functor designed to invoke a listener
of the Standard type (that is, it has no return
value).

Model of STL Unary Function.

(1.0)
*/
#pragma mark standardListenerInvoker
class standardListenerInvoker:
public std::unary_function< My_EventToListenerListPtrMap::value_type/* argument */, void/* return */ >
{
public:
	standardListenerInvoker		(ListenerModel_Ref		inForWhichModel,
								 ListenerModel_Event	inEventThatOccurred,
								 void*					inEventContextPtr)
	: _model(inForWhichModel), _event(inEventThatOccurred), _context(inEventContextPtr), _anyInvalidListeners(false)
	{
	}
	
	void
	operator()	(ListenerModel_ListenerRef	inListener)
	{
		ListenerAutoLocker	listenerPtr(gListenerPtrLocks(), inListener);
		
		
		if (gListenerValidRefs().end() == gListenerValidRefs().find(inListener))
		{
			Console_Warning(Console_WriteValueAddress, "attempt to notify nonexistent standard listener",
							inListener);
		}
		else
		{
			ListenerModel_InvokeStandardProc(listenerPtr->callback.standard, _model, _event,
												_context, listenerPtr->context);
		}
	}
	
	bool
	anyInvalidListeners () const
	{
		return _anyInvalidListeners;
	}

protected:

private:
	ListenerModel_Ref		_model;
	ListenerModel_Event		_event;
	void*					_context;
	bool					_anyInvalidListeners;
};



#pragma mark Public Methods

/*!
A unit test for this module.  This should always
be run before a release, after any substantial
changes are made, or if you suspect bugs!  It
should also be EXPANDED as new functionality is
proposed (ideally, a test is written before the
functionality is added).

(1.4)
*/
void
ListenerModel_RunTests ()
{
	UInt16		totalTests = 0;
	UInt16		failedTests = 0;
	
	
	++totalTests; if (false == unitTest000_Begin()) ++failedTests;
	
	Console_WriteUnitTestReport("ListenerModel", failedTests, totalTests);
}// RunTests


/*!
Creates a new listener model.  If any problems
occur, nullptr is returned.

The listener model style defines both the type
of callbacks you can install into the model,
and how those callbacks are used when notifying
callbacks of events.  You cannot add a listener
to a model if the listener’s type is inconsistent
with the style of the model.

The model cannot be used for anything until you
add listeners to it.  Use methods such as
ListenerModel_AddListener() to manage the
listeners in the model, and respond to events
with ListenerModel_NotifyListenersOfEvent().

(1.0)
*/
ListenerModel_Ref
ListenerModel_New	(ListenerModel_Style		inStyle,
					 ListenerModel_Descriptor	inDescriptor)
{
	ListenerModel_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new ListenerModel, ListenerModel_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	
	if (nullptr != result)
	{
		ListenerModelAutoLocker		ptr(gListenerModelPtrLocks(), result);
		
		
		ptr->descriptor = inDescriptor;
		switch (inStyle)
		{
		case kListenerModel_StyleLogicalOR:
			ptr->notificationBehavior = kListenerModelBehaviorNotifyUntilReturnedTrue;
			ptr->callbackType = kListenerModelCallbackTypeBoolean;
			break;
		
		case kListenerModel_StyleNonEventNotHandledErr:
			ptr->notificationBehavior = kListenerModelBehaviorNotifyUntilReturnedNonEventNotHandledErr;
			ptr->callbackType = kListenerModelCallbackTypeOSStatus;
			break;
		
		case kListenerModel_StyleStandard:
		default:
			ptr->notificationBehavior = kListenerModelBehaviorNotifyAllSequentially;
			ptr->callbackType = kListenerModelCallbackTypeStandard;
			break;
		}
	}
	return result;
}// New


/*!
Destroys a listener model created with the
ListenerModel_New() routine.

(1.0)
*/
void
ListenerModel_Dispose	(ListenerModel_Ref*		inoutRefPtr)
{
	if (gListenerModelPtrLocks().isLocked(*inoutRefPtr))
	{
		//Console_Warning(Console_WriteValue, "attempt to dispose of locked listener model; outstanding locks",
		//				gListenerModelPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, ListenerModel**));
		*inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Creates a new predication listener, a special callback function
that is invoked to determine the truth or falsehood of some
condition, and locks it automatically (so, an implicit
ListenerModel_RetainListener() call is made).  If any problems
occur, nullptr is returned.

The specified context is passed as the listener context at
notification time (the event context is also passed, but that
is determined at event notification time).  It may be useful
to define a context here to tell you, for example, the specific
thing that this callback is for (e.g. responding to a click in
a particular control).

(1.0)
*/
ListenerModel_ListenerRef
ListenerModel_NewBooleanListener	(ListenerModel_BooleanProcPtr	inCallback,
									 void*							inContextOrNull)
{
	ListenerModel_ListenerRef	result = REINTERPRET_CAST(new Listener, ListenerModel_ListenerRef);
	
	
	if (nullptr != result)
	{
		ListenerAutoLocker	ptr(gListenerPtrLocks(), result);
		
		
		ListenerModel_RetainListener(result);
		ptr->callbackType = kListenerModelCallbackTypeBoolean;
		ptr->context = inContextOrNull;
		ptr->callback.boolean = inCallback;
	}
	return result;
}// NewBooleanListener


/*!
Creates a new Mac OS event listener, a special callback function
that is invoked to determine the success or failure of some
OS-dependent operation, and locks it automatically (so, an
implicit ListenerModel_RetainListener() call is made).  If any
problems occur, nullptr is returned.

The result "eventNotHandledErr" has special significance, and
is like a logical "false" for a Boolean listener; you MAY return
"noErr" and other error codes to indicate logical "true".  Be
sure you remember this!!!

(1.0)
*/
ListenerModel_ListenerRef
ListenerModel_NewOSStatusListener	(ListenerModel_OSStatusProcPtr	inCallback,
									 void*							inContextOrNull)
{
	ListenerModel_ListenerRef	result = REINTERPRET_CAST(new Listener, ListenerModel_ListenerRef);
	
	
	if (nullptr != result)
	{
		ListenerAutoLocker	ptr(gListenerPtrLocks(), result);
		
		
		ListenerModel_RetainListener(result);
		ptr->callbackType = kListenerModelCallbackTypeOSStatus;
		ptr->context = inContextOrNull;
		ptr->callback.osStatus = inCallback;
	}
	return result;
}// NewOSStatusListener


/*!
Creates a new listener, a special callback function that
is invoked when one or more events occur, and locks it
automatically (so, an implicit ListenerModel_RetainListener()
call is made).  If any problems occur, nullptr is returned.

You can’t directly add callback functions to a listener
model; you must first create an abstract representation
of the callback by calling this routine, and then pass
the returned reference to other functions that work
with listeners.

(1.0)
*/
ListenerModel_ListenerRef
ListenerModel_NewStandardListener	(ListenerModel_StandardProcPtr	inCallback,
									 void*							inContextOrNull)
{
	ListenerModel_ListenerRef	result = REINTERPRET_CAST(new Listener, ListenerModel_ListenerRef);
	
	
	if (nullptr != result)
	{
		ListenerAutoLocker	ptr(gListenerPtrLocks(), result);
		
		
		ListenerModel_RetainListener(result);
		ptr->callbackType = kListenerModelCallbackTypeStandard;
		ptr->context = inContextOrNull;
		ptr->callback.standard = inCallback;
	}
	return result;
}// NewStandardListener


/*!
Adds an additional lock on the specified reference.
This indicates that you are using the model for
some reason, so attempts by anyone else to delete
the model with ListenerModel_ReleaseListener() will
fail until you release your lock (and everyone else
releases locks they may have).

Note that you may wish to use an instance of the
ListenerModel_ListenerWrap class instead of calling
this directly.

(1.1)
*/
void
ListenerModel_RetainListener	(ListenerModel_ListenerRef		inRef)
{
	if ((nullptr == inRef) ||
		(gListenerValidRefs().end() == gListenerValidRefs().find(inRef)))
	{
		Console_Warning(Console_WriteValueAddress, "attempt to retain a nonexistent listener", inRef);
	}
	else
	{
		gListenerRefLocks().acquireLock(inRef);
	}
}// RetainListener


/*!
Releases one lock on the specified listener created with a
ListenerModel_NewXYZListener() routine, and deletes the
listener *if* no other locks remain.  Your copy of the
reference is set to nullptr.

Note that you may wish to use an instance of the
ListenerModel_ListenerWrap class instead of calling this
directly.

(1.0)
*/
void
ListenerModel_ReleaseListener	(ListenerModel_ListenerRef*		inoutRefPtr)
{
	if (nullptr != inoutRefPtr)
	{
		if ((nullptr == *inoutRefPtr) ||
			(gListenerValidRefs().end() == gListenerValidRefs().find(*inoutRefPtr)))
		{
			Console_Warning(Console_WriteValueAddress, "attempt to release a nonexistent listener",
							*inoutRefPtr);
		}
		else
		{
			gListenerRefLocks().releaseLock(*inoutRefPtr);
			unless (gListenerRefLocks().isLocked(*inoutRefPtr))
			{
				delete (REINTERPRET_CAST(*inoutRefPtr, Listener*));
			}
		}
		*inoutRefPtr = nullptr;
	}
}// ReleaseListener


/*!
Installs a new callback routine in the given model that shall be
invoked whenever ListenerModel_NotifyListenersOfEvent() is invoked
on the model for the specified event.

IMPORTANT:	A listener will fail to be installed unless its type is
			consistent with the style of the model.  For example, a
			“logical OR” model can’t work if its callbacks return no
			values.

(1.0)
*/
OSStatus
ListenerModel_AddListenerForEvent	(ListenerModel_Ref			inToWhichModel,
									 ListenerModel_Event		inForWhichEvent,
									 ListenerModel_ListenerRef	inListenerToAdd)
{
	ListenerModelAutoLocker		ptr(gListenerModelPtrLocks(), inToWhichModel);
	OSStatus					result = noErr;
	
	
	if (nullptr == ptr)
	{
		Console_Warning(Console_WriteValueFourChars, "attempt to add listener to nonexistent model for event",
						inForWhichEvent);
		result = memPCErr;
	}
	else
	{
		ListenerAutoLocker	listenerPtr(gListenerPtrLocks(), inListenerToAdd);
		
		
		if (nullptr == listenerPtr) result = memPCErr;
		else
		{
			// it is only legal to add callbacks that are consistent with
			// the requirements of the style of the list
			if (listenerPtr->callbackType != ptr->callbackType) result = paramErr;
			else
			{
				if (ptr->eventListeners.end() == ptr->eventListeners.find(inForWhichEvent))
				{
					// list does not yet exist - create it
					try
					{
						My_ListenerList*	newListPtr = new My_ListenerList();
						
						
						ptr->eventListeners.insert(My_EventToListenerListPtrMap::value_type(inForWhichEvent, newListPtr));
						assert(ptr->eventListeners.end() != ptr->eventListeners.find(inForWhichEvent));
					}
					catch (std::bad_alloc)
					{
						// not enough memory?!?!?
					}
				}
				if ((ptr->eventListeners.end() == ptr->eventListeners.find(inForWhichEvent)) ||
						(nullptr == ptr->eventListeners[inForWhichEvent]))
				{
					result = memFullErr;
				}
				else
				{
					// add to the sequence of callbacks for this event
					ptr->eventListeners[inForWhichEvent]->push_back(inListenerToAdd);
				}
			}
		}
	}
	return result;
}// AddListenerForEvent


/*!
Returns the unique ID assigned to the specified Listener
Model when it was constructed.  You might use this to
help determine what kind of object an event came from.

(1.0)
*/
ListenerModel_Result
ListenerModel_GetDescriptor		(ListenerModel_Ref			inForWhichModel,
								 ListenerModel_Descriptor*	outDescriptorPtr)
{
	ListenerModelAutoLocker		ptr(gListenerModelPtrLocks(), inForWhichModel);
	ListenerModel_Result		result = kListenerModel_ResultOK;
	
	
	if (nullptr != outDescriptorPtr) *outDescriptorPtr = ptr->descriptor;
	return result;
}// GetDescriptor


/*!
Returns "true" only if there is at least one listener
installed in the given model for the specified event.
You might use this to determine whether there is any
point in notifying listeners in response to an event.

(1.1)
*/
Boolean
ListenerModel_IsAnyListenerForEvent		(ListenerModel_Ref		inForWhichModel,
										 ListenerModel_Event	inEventThatOccurred)
{
	ListenerModelAutoLocker		ptr(gListenerModelPtrLocks(), inForWhichModel);
	auto						toEventToListenerListPtr = ptr->eventListeners.find(inEventThatOccurred);
	Boolean						result = false;
	
	
	if (ptr->eventListeners.end() != toEventToListenerListPtr)
	{
		// are any listeners in this list?
		result = (false == toEventToListenerListPtr->second->empty());
	}
	return result;
}// IsAnyListenerForEvent


/*!
Invokes callback routines registered as listeners for
the given event.  Depending upon the style of the
listener model, this may have different effects:

- for "kListenerModelBehaviorNotifyAllSequentially",
  every listener is notified regardless of what its
  return value is

- for "kListenerModelBehaviorNotifyUntilReturnedTrue",
  the return value of each listener is examined and
  the first listener that returns "true" is the last
  listener that will be notified (others are skipped)

- for "kListenerModelBehaviorNotifyUntilReturnedNonEventNotHandledErr",
  listeners are notified as long as they return the
  value "eventNotHandledErr"; as soon as one of them
  returns something different, no remaining listeners
  are notified

The "outReturnValuePtrOrNull" argument, if not nullptr,
can sometimes be used to receive a return value that
indicates whether all listeners were notified.  The type
of data it points to varies based on the model style:

- for "kListenerModelBehaviorNotifyAllSequentially",
  the type is undefined and reserved

- for "kListenerModelBehaviorNotifyUntilReturnedTrue",
  the return value points to a Boolean that is "true"
  only if *some* listener in the model returned "true"
  (the result is "false" if all listeners are notified
  and none of them returns true)

- for "kListenerModelBehaviorNotifyUntilReturnedNonEventNotHandledErr",
  the return value points to a Boolean that is "true"
  only if *some* listener in the model returned a value
  besides "eventNotHandledErr" (the result is "false"
  if all listeners are notified and none of them returns
  "eventNotHandledErr", even if some return "noErr")

\retval kListenerModel_ResultOK
if no errors occur

\retval kListenerModel_InvalidModelReference
if "inForWhichModel" is not valid

\retval kListenerModel_InvalidListenerReference
if some listener found in the model is no longer valid;
this does not prevent remaining listeners from being
considered, it is purely an informational return value
(but in the future, this routine may automatically remove
listeners found to be invalid)

(1.1)
*/
ListenerModel_Result
ListenerModel_NotifyListenersOfEvent	(ListenerModel_Ref		inForWhichModel,
										 ListenerModel_Event	inEventThatOccurred,
										 void*					inEventContextPtr,
										 void*					outReturnValuePtrOrNull)
{
	ListenerModelAutoLocker		ptr(gListenerModelPtrLocks(), inForWhichModel);
	ListenerModel_Result		result = kListenerModel_ResultOK;
	
	
	if ((nullptr == ptr) ||
		(gListenerModelValidRefs().end() == gListenerModelValidRefs().find(inForWhichModel)))
	{
		Console_Warning(Console_WriteValueFourChars, "attempt to notify listeners in nonexistent model of event",
						inEventThatOccurred);
		result = kListenerModel_ResultInvalidModelReference;
	}
	else
	{
		switch (ptr->callbackType)
		{
		case kListenerModelCallbackTypeStandard:
			{
				auto	toEventToListenerListPtr = ptr->eventListeners.find(inEventThatOccurred);
				
				
				if (ptr->eventListeners.end() != toEventToListenerListPtr)
				{
					standardListenerInvoker		perListenerFunction(inForWhichModel, inEventThatOccurred,
																	inEventContextPtr);
					
					
					// invoke each Standard listener in turn
					perListenerFunction = std::for_each(toEventToListenerListPtr->second->begin(),
														toEventToListenerListPtr->second->end(), perListenerFunction);
					
					if (perListenerFunction.anyInvalidListeners())
					{
						result = kListenerModel_ResultInvalidListenerReference;
					}
				}
			}
			break;
		
		case kListenerModelCallbackTypeBoolean:
			{
				auto	toEventToListenerListPtr = ptr->eventListeners.find(inEventThatOccurred);
				
				
				if (ptr->eventListeners.end() != toEventToListenerListPtr)
				{
					// invoke each Boolean listener and stop as soon as one returns true;
					// the fact that the callback invoker is modeled as a Predicate allows
					// the STL find_if() algorithm to be exploited to do the right thing here
					booleanListenerInvoker		perListenerFunction(inForWhichModel, inEventThatOccurred,
																			inEventContextPtr);
					auto						toListener = std::find_if(toEventToListenerListPtr->second->begin(),
																			toEventToListenerListPtr->second->end(),
																			perListenerFunction);
					Boolean						someListenerReturnedTrue = false;
					
					
					someListenerReturnedTrue = (toEventToListenerListPtr->second->end() != toListener);
					if (nullptr != outReturnValuePtrOrNull)
					{
						*(REINTERPRET_CAST(outReturnValuePtrOrNull, Boolean*)) = someListenerReturnedTrue;
					}
					
					if (perListenerFunction.anyInvalidListeners())
					{
						result = kListenerModel_ResultInvalidListenerReference;
					}
				}
			}
			break;
		
		case kListenerModelCallbackTypeOSStatus:
			{
				auto	toEventToListenerListPtr = ptr->eventListeners.find(inEventThatOccurred);
				
				
				if (ptr->eventListeners.end() != toEventToListenerListPtr)
				{
					// invoke each OSStatus listener and continue while "eventNotHandledErr" is
					// returned; the fact that the callback invoker is modeled as a Predicate allows
					// the STL find_if() algorithm to be exploited to do the right thing here
					osStatusListenerInvoker		perListenerFunction(inForWhichModel, inEventThatOccurred,
																	inEventContextPtr);
					auto						toListener = std::find_if(toEventToListenerListPtr->second->begin(),
																			toEventToListenerListPtr->second->end(),
																			perListenerFunction);
					Boolean						someListenerReturnedNonEventNotHandledErr = false;
					
					
					someListenerReturnedNonEventNotHandledErr = (toEventToListenerListPtr->second->end() != toListener);
					if (nullptr != outReturnValuePtrOrNull)
					{
						*(REINTERPRET_CAST(outReturnValuePtrOrNull, Boolean*)) = someListenerReturnedNonEventNotHandledErr;
					}
					
					if (perListenerFunction.anyInvalidListeners())
					{
						result = kListenerModel_ResultInvalidListenerReference;
					}
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// NotifyListenersOfEvent


/*!
Removes a callback routine from the given model, so it will no
longer be invoked when ListenerModel_NotifyListenersOfEvent() is
invoked on the model for the specified event.

Note that this is no longer strictly necessary, unless you are
planning to remove an event before destroying it.  Now, when you
destroy a listener object it is automatically removed from all
models it is a member of.

\retval kListenerModel_ResultOK
if no errors occur

\retval kListenerModel_InvalidModelReference
if "inFromWhichModel" is not valid

\retval kListenerModel_InvalidListenerReference
if "inListenerToRemove" is not valid

(1.0)
*/
ListenerModel_Result
ListenerModel_RemoveListenerForEvent	(ListenerModel_Ref			inFromWhichModel,
										 ListenerModel_Event		inForWhichEvent,
										 ListenerModel_ListenerRef	inListenerToRemove)
{
	ListenerModelAutoLocker		ptr(gListenerModelPtrLocks(), inFromWhichModel);
	ListenerModel_Result		result = kListenerModel_ResultOK;
	
	
	if ((nullptr == ptr) ||
		(gListenerModelValidRefs().end() == gListenerModelValidRefs().find(inFromWhichModel)))
	{
		Console_Warning(Console_WriteValueFourChars, "attempt to remove listener from nonexistent model for event",
						inForWhichEvent);
		result = kListenerModel_ResultInvalidModelReference;
	}
	else if ((nullptr == inListenerToRemove) ||
				(gListenerValidRefs().end() == gListenerValidRefs().find(inListenerToRemove)))
	{
		Console_Warning(Console_WriteValueFourChars, "attempt to remove nonexistent listener for event",
						inForWhichEvent);
		result = kListenerModel_ResultInvalidListenerReference;
	}
	else
	{
		auto	toEventToListenerListPtr = ptr->eventListeners.find(inForWhichEvent);
		
		
		if (ptr->eventListeners.end() != toEventToListenerListPtr)
		{
			My_EventToListenerListPtrMap::mapped_type	listenerListPtr = toEventToListenerListPtr->second;
			
			
			// delete occurrences of the given listener in the list for this event
			listenerListPtr->erase(std::remove(listenerListPtr->begin(), listenerListPtr->end(), inListenerToRemove),
									listenerListPtr->end());
		}
	}
	
	return result;
}// RemoveListenerForEvent


#pragma mark Internal Methods
namespace {

/*!
Initializes a new Listener instance and registers
its address in a table of valid references.  At
destruction time, the address is automatically
unregistered.
*/
Listener::
Listener ()
:
refValidator(REINTERPRET_CAST(this, ListenerModel_ListenerRef), gListenerValidRefs()),
callbackType(kListenerModelCallbackTypeStandard),
context(nullptr)
{
	callback.standard = nullptr;
}// Listener default constructor


/*!
Initializes a new ListenerModel instance and
registers its address in a table of valid
references.  At destruction time, the address
is automatically unregistered.
*/
ListenerModel::
ListenerModel ()
:
refValidator(REINTERPRET_CAST(this, ListenerModel_Ref), gListenerModelValidRefs()),
descriptor(kListenerModel_InvalidDescriptor),
notificationBehavior(kListenerModelBehaviorNotifyAllSequentially),
callbackType(kListenerModelCallbackTypeStandard),
eventListeners()
{
}// ListenerModel default constructor


/*!
This C-based callback is invoked by a listener model in the
usual way, and it forwards the event to a particular object
instead.

(2.6)
*/
void
objectiveCStandardListener	(ListenerModel_Ref		inModel,
							 ListenerModel_Event	inEvent,
							 void*					inEventContext,
							 void*					inStandardListener)
{
	ListenerModel_StandardListener*		asStandardListener = REINTERPRET_CAST(inStandardListener,
																				ListenerModel_StandardListener*);
	
	
	[asStandardListener listenerModel:inModel firedEvent:inEvent context:inEventContext];
}// objectiveCStandardListener

} // anonymous namespace


#pragma mark Internal Methods: Unit Tests
namespace {

/*!
Tests construction and call of standard listeners
that receive a context.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messagess should be
printed for ALL assertion failures regardless.

(1.4)
*/
Boolean
unitTest000_Begin ()
{
	Boolean						result = true;
	ListenerModel_ListenerWrap	callbackWrapper = nullptr;
	ListenerModel_Result		modelError = kListenerModel_ResultOK;
	
	
	// create a listener, and arbitrarily pass in the address of this
	// function as the event context (the listener code should assert
	// that it received the right event context)
	callbackWrapper.setRef(ListenerModel_NewStandardListener
							(unitTest000_Callback1, REINTERPRET_CAST(unitTest000_Begin, void*)/* event context */),
							true/* is retained */);
	result &= Console_Assert("wrapper constructed", nullptr != callbackWrapper.returnRef());
	
	// create a model, for controlling event notifications
	gUnitTest000_Model = ListenerModel_New(kListenerModel_StyleStandard, 'u000');
	{
		ListenerModel_Descriptor	desc = '----';
		
		
		modelError = ListenerModel_GetDescriptor(gUnitTest000_Model, &desc);
		result &= Console_Assert("no errors getting model descriptor", kListenerModel_ResultOK == modelError);
		result &= Console_Assert("proper model descriptor", 'u000' == desc);
	}
	
	// install the listener on specific events
	{
		OSStatus	installError = noErr;
		
		
		installError = ListenerModel_AddListenerForEvent(gUnitTest000_Model, 'test', callbackWrapper.returnRef());
		result &= Console_Assert("no errors installing handler", noErr == installError);
	}
	
	// notify listeners a certain number of times; arbitrarily pass a fake
	// address as the listener context (the listener code should have
	// appropriate assertions)
	gUnitTest000_CallCount = 0;
	gUnitTest000_Result = true; // could be used by callbacks to raise assertions
	modelError = ListenerModel_NotifyListenersOfEvent(gUnitTest000_Model, 'test',
														REINTERPRET_CAST(0x54321234, void*)/* event context */);
	result &= Console_Assert("no errors on notify", kListenerModel_ResultOK == modelError);
	modelError = ListenerModel_NotifyListenersOfEvent(gUnitTest000_Model, 'test',
														REINTERPRET_CAST(0x54321234, void*)/* event context */);
	result &= Console_Assert("no errors on notify", kListenerModel_ResultOK == modelError);
	modelError = ListenerModel_NotifyListenersOfEvent(gUnitTest000_Model, 'test',
														REINTERPRET_CAST(0x54321234, void*)/* event context */);
	result &= Console_Assert("no errors on notify", kListenerModel_ResultOK == modelError);
	modelError = ListenerModel_NotifyListenersOfEvent(gUnitTest000_Model, 'test',
														REINTERPRET_CAST(0x54321234, void*)/* event context */);
	result &= Console_Assert("no errors on notify", kListenerModel_ResultOK == modelError);
	modelError = ListenerModel_NotifyListenersOfEvent(gUnitTest000_Model, 'test',
														REINTERPRET_CAST(0x54321234, void*)/* event context */);
	result &= Console_Assert("no errors on notify", kListenerModel_ResultOK == modelError);
	modelError = ListenerModel_NotifyListenersOfEvent(gUnitTest000_Model, 'test',
														REINTERPRET_CAST(0x54321234, void*)/* event context */);
	result &= Console_Assert("no errors on notify", kListenerModel_ResultOK == modelError);
	
	// incorporate any callback-raised failures
	result &= gUnitTest000_Result;
	
	// the callback should have been invoked the right number of times
	result &= Console_Assert("proper number of callback invocations", 6 == gUnitTest000_CallCount);
	
	// callback and model are no longer needed
	ListenerModel_Dispose(&gUnitTest000_Model);
	result &= Console_Assert("model nullified", nullptr == gUnitTest000_Model);
	
	return result;
}// unitTest000_Begin


/*!
Test callback used with unitTest000_Begin().

(1.4)
*/
void
unitTest000_Callback1	(ListenerModel_Ref		inModel,
						 ListenerModel_Event	inEvent,
						 void*					inEventContext,
						 void*					inListenerContext)
{
	// the model should match what the test created
	gUnitTest000_Result &= Console_Assert("proper model", inModel == gUnitTest000_Model);
	
	// the event should match what the test set up
	gUnitTest000_Result &= Console_Assert("proper event", 'test' == inEvent);
	
	// the fake address 0x54321234 is passed as the event context (arbitrarily)
	gUnitTest000_Result &= Console_Assert("proper event context",
											REINTERPRET_CAST(0x54321234, void*) == inEventContext);
	
	// the address of the unit test function is passed as the listener context (arbitrarily)
	gUnitTest000_Result &= Console_Assert("proper listener context", unitTest000_Begin == inListenerContext);
	
	// increment a global to indicate the callback was invoked
	++gUnitTest000_CallCount;
}// unitTest000_Callback1

}// anonymous namespace


#pragma mark -
@implementation ListenerModel_StandardListener


/*!
Constructs a “standard listener” (that has no return value)
to invoke the specified selector on the given target object
whenever the listener is called by some listener model.
You can call "listenerRef" to help install it in a model.

The "eventFiredSelector:" must have the following form:
	- (void)
	listenerModel:(ListenerModel_Ref)_
	firedEvent:(ListenerModel_Event)_
	context:(void*)_;

Since the target object can act as a listener-specific
context, no 4th parameter is given (unlike the traditional
C-based listeners).

(2.6)
*/
- (instancetype)
initWithTarget:(id)			aTarget
eventFiredSelector:(SEL)	aSelector
{
	self = [super init];
	if (nil != self)
	{
		self->listenerRef = ListenerModel_NewStandardListener(objectiveCStandardListener, self/* context */);
		self->methodInvoker = [[NSInvocation invocationWithSelector:aSelector target:aTarget] retain];
	}
	return self;
}// initWithTarget:eventFiredSelector:


/*!
Destructor.

(2.6)
*/
- (void)
dealloc
{
	ListenerModel_ReleaseListener(&listenerRef);
	[self->methodInvoker release];
	[super dealloc];
}// dealloc


/*!
Invokes the selector on the target object.  The meaning
of the context varies according to the type of event
that has occurred (therefore it is caller-defined).

(2.6)
*/
- (void)
listenerModel:(ListenerModel_Ref)	aModel
firedEvent:(ListenerModel_Event)	anEvent
context:(void*)						aContext
{
	[self->methodInvoker setArgument:&aModel atIndex:2];
	[self->methodInvoker setArgument:&anEvent atIndex:3];
	[self->methodInvoker setArgument:&aContext atIndex:4];
	[self->methodInvoker invoke];
}// listenerModel:firedEvent:context:


/*!
Returns a reference to a listener, which is needed when
installing this callback via an API that expects such a
reference instead of an Objective-C object.

(2.6)
*/
- (ListenerModel_ListenerRef)
listenerRef
{
	return self->listenerRef;
}// listenerRef

@end // ListenerModel_StandardListener

// BELOW IS REQUIRED NEWLINE TO END FILE
