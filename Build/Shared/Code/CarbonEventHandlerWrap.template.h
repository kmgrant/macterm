/*!	\file CarbonEventHandlerWrap.template.h
	\brief Simplifies installation and removal of event
	handlers in object-oriented environments.
	
	Particularly useful in C++ objects that have event
	handler members, this class handles the details of
	allocating and installing the handler, and removing
	and disposing of it when finished.
*/
/*###############################################################

	Data Access Library
	© 1998-2014 by Kevin Grant
	
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

#ifndef __CARBONEVENTHANDLERWRAP__
#define __CARBONEVENTHANDLERWRAP__

// standard-C++ includes
#include <set>
#include <utility>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

/*!
Wraps the event class type to avoid accidentally specifying
a type code as a class instead.
*/
class CarbonEventClass
{
public:
	//! creates instance wrapper representing the given class;
	//! note that if this constructor were not explicit, there
	//! would be no advantage (no point) to having this class
	explicit inline
	CarbonEventClass	(UInt32		inClass);
	
	inline
	operator UInt32		() const;

protected:

private:
	UInt32	_class;
};


typedef std::pair< UInt32, UInt32 >		CarbonEventType;

/*!
Describes one or more Carbon Events in a way that is convenient
for object-oriented interfaces.

Technically this is an STL container, so you can modify it
post-construction to have as many elements as you want.  But
for convenience in defining other constructors, this class has
constructors to receive the most common (small) numbers of
event types.
*/
class CarbonEventSet: public std::set< CarbonEventType >
{
public:
	inline
	CarbonEventSet	(CarbonEventType const&		inSingleType);
	
	inline
	CarbonEventSet	(CarbonEventType const&		inType1,
					 CarbonEventType const&		inType2);
	
	inline
	CarbonEventSet	(CarbonEventType const&		inType1,
					 CarbonEventType const&		inType2,
					 CarbonEventType const&		inType3);
	
	inline
	CarbonEventSet	(CarbonEventType const&		inType1,
					 CarbonEventType const&		inType2,
					 CarbonEventType const&		inType3,
					 CarbonEventType const&		inType4);

protected:

private:
};


/*!
Simplifies event specifications further by allowing the
class to be “common” to multiple kind specifications.
*/
class CarbonEventSetInClass: public CarbonEventSet
{
public:
	inline
	CarbonEventSetInClass	(CarbonEventClass	inClass,
							 UInt32				inKind);
	
	inline
	CarbonEventSetInClass	(CarbonEventClass	inClass,
							 UInt32				inKind1,
							 UInt32				inKind2);
	
	inline
	CarbonEventSetInClass	(CarbonEventClass	inClass,
							 UInt32				inKind1,
							 UInt32				inKind2,
							 UInt32				inKind3);
	
	inline
	CarbonEventSetInClass	(CarbonEventClass	inClass,
							 UInt32				inKind1,
							 UInt32				inKind2,
							 UInt32				inKind3,
							 UInt32				inKind4);

protected:

private:
};


/*!
Handles the details of allocating and installing an event
handler, as well as removing it and cleaning it up when
finished.

Event types are given as an STL container of STL pairs of
integers, in the order “event class, event kind”.  The
recommended input is a CarbonEventSet, but if you have
other STL containers they should also work fine.  If
CarbonEventSet is used, see CarbonEventHandlerWrap.
*/
template < typename event_type_container >
class CarbonEventHandlerWrapGeneric
{
public:
	inline
	CarbonEventHandlerWrapGeneric ();
	
	inline
	CarbonEventHandlerWrapGeneric	(EventTargetRef					inTarget,
									 EventHandlerProcPtr			inProcPtr,
									 event_type_container const&	inEvents,
									 void*							inDataToPassToHandler = nullptr);
	
	inline ~CarbonEventHandlerWrapGeneric	();
	
	inline bool
	install		(EventTargetRef					inTarget,
				 EventHandlerProcPtr			inProcPtr,
				 event_type_container const&	inEvents,
				 void*							inDataToPassToHandler = nullptr);
	
	//! returns true only if the event handler was allocated and installed successfully
	inline bool
	isInstalled () const;
	
	//! removes the handler; subsequent calls to isInstalled() will return false, but install() can be called again
	inline void
	remove ();

protected:
	static inline void
	remove	(EventHandlerUPP&, EventHandlerRef&);

private:
	EventHandlerUPP		_procUPP;
	EventHandlerRef		_handler;
	bool				_installed;
};


/*!
Handles the details of allocating and installing an event
handler, as well as removing it and cleaning it up when
finished.

Event types are given as a CarbonEventSet.  See also
CarbonEventHandlerWrapGeneric.
*/
class CarbonEventHandlerWrap: public CarbonEventHandlerWrapGeneric< CarbonEventSet >
{
public:
	inline
	CarbonEventHandlerWrap ();
	
	inline
	CarbonEventHandlerWrap	(EventTargetRef				inTarget,
							 EventHandlerProcPtr		inProcPtr,
							 CarbonEventSet const&		inEvents,
							 void*						inDataToPassToHandler = nullptr);

protected:

private:
};



#pragma mark Public Methods

CarbonEventClass::
CarbonEventClass	(UInt32		inClass)
:
_class(inClass)
{
}// CarbonEventClass 1-argument constructor


CarbonEventClass::
operator UInt32 ()
const
{
	return _class;
}// operator UInt32


CarbonEventSet::
CarbonEventSet	(CarbonEventType const&		inSingleType)
:
std::set< CarbonEventType >()
{
	this->insert(inSingleType);
	assert(1 == this->size());
}// CarbonEventSet 1-argument constructor


CarbonEventSet::
CarbonEventSet	(CarbonEventType const&		inType1,
				 CarbonEventType const&		inType2)
:
std::set< CarbonEventType >()
{
	this->insert(inType1);
	this->insert(inType2);
	assert(2 == this->size());
}// CarbonEventSet 2-argument constructor


CarbonEventSet::
CarbonEventSet	(CarbonEventType const&		inType1,
				 CarbonEventType const&		inType2,
				 CarbonEventType const&		inType3)
:
std::set< CarbonEventType >()
{
	this->insert(inType1);
	this->insert(inType2);
	this->insert(inType3);
	assert(3 == this->size());
}// CarbonEventSet 3-argument constructor


CarbonEventSet::
CarbonEventSet	(CarbonEventType const&		inType1,
				 CarbonEventType const&		inType2,
				 CarbonEventType const&		inType3,
				 CarbonEventType const&		inType4)
:
std::set< CarbonEventType >()
{
	this->insert(inType1);
	this->insert(inType2);
	this->insert(inType3);
	this->insert(inType4);
	assert(4 == this->size());
}// CarbonEventSet 4-argument constructor


CarbonEventSetInClass::
CarbonEventSetInClass	(CarbonEventClass	inClass,
						 UInt32				inKind)
:
CarbonEventSet(std::make_pair(inClass, inKind))
{
}// CarbonEventSetInClass 1-argument constructor


CarbonEventSetInClass::
CarbonEventSetInClass	(CarbonEventClass	inClass,
						 UInt32				inKind1,
						 UInt32				inKind2)
:
CarbonEventSet(std::make_pair(inClass, inKind1),
				std::make_pair(inClass, inKind2))
{
}// CarbonEventSetInClass 2-argument constructor


CarbonEventSetInClass::
CarbonEventSetInClass	(CarbonEventClass	inClass,
						 UInt32				inKind1,
						 UInt32				inKind2,
						 UInt32				inKind3)
:
CarbonEventSet(std::make_pair(inClass, inKind1),
				std::make_pair(inClass, inKind2),
				std::make_pair(inClass, inKind3))
{
}// CarbonEventSetInClass 3-argument constructor


CarbonEventSetInClass::
CarbonEventSetInClass	(CarbonEventClass	inClass,
						 UInt32				inKind1,
						 UInt32				inKind2,
						 UInt32				inKind3,
						 UInt32				inKind4)
:
CarbonEventSet(std::make_pair(inClass, inKind1),
				std::make_pair(inClass, inKind2),
				std::make_pair(inClass, inKind3),
				std::make_pair(inClass, inKind4))
{
}// CarbonEventSetInClass 4-argument constructor


template < typename event_type_container >
CarbonEventHandlerWrapGeneric< event_type_container >::
CarbonEventHandlerWrapGeneric ()
:
_procUPP(nullptr),
_handler(nullptr),
_installed(false)
{
}// CarbonEventHandlerWrapGeneric default constructor


template < typename event_type_container >
CarbonEventHandlerWrapGeneric< event_type_container >::
CarbonEventHandlerWrapGeneric	(EventTargetRef					inTarget,
								 EventHandlerProcPtr			inProcPtr,
								 event_type_container const&	inEvents,
								 void*							inDataToPassToHandler)
:
_procUPP(nullptr),
_handler(nullptr),
_installed(false)
{
	install(inTarget, inProcPtr, inEvents, inDataToPassToHandler);
}// CarbonEventHandlerWrapGeneric 4-argument constructor


template < typename event_type_container >
CarbonEventHandlerWrapGeneric< event_type_container >::
~CarbonEventHandlerWrapGeneric ()
{
	remove();
}// CarbonEventHandlerWrapGeneric destructor


template < typename event_type_container >
bool
CarbonEventHandlerWrapGeneric< event_type_container >::
install		(EventTargetRef					inTarget,
			 EventHandlerProcPtr			inProcPtr,
			 event_type_container const&	inEvents,
			 void*							inDataToPassToHandler)
{
	remove();
	_procUPP = NewEventHandlerUPP(inProcPtr);
	if (nullptr != _procUPP)
	{
		typename event_type_container::size_type const	kEventTypeSpecsLength = inEvents.size();
		
		
		try
		{
			// create a C array of EventTypeSpec structures, as required by the Carbon API
			EventTypeSpec*	eventTypeSpecs = new EventTypeSpec[kEventTypeSpecsLength];
			
			
			if (nullptr != eventTypeSpecs)
			{
				OSStatus										error = noErr;
				size_t											i = 0;
				typename event_type_container::const_iterator	pairIter;
				
				
				// copy data from the given STL container into the C array
				for (pairIter = inEvents.begin(); pairIter != inEvents.end(); ++pairIter)
				{
					eventTypeSpecs[i].eventClass = pairIter->first;
					eventTypeSpecs[i].eventKind = pairIter->second;
					++i;
				}
				
				// attempt to install the handler
				error = InstallEventHandler(inTarget, _procUPP, kEventTypeSpecsLength, eventTypeSpecs,
											inDataToPassToHandler, &_handler);
				if (noErr != error) _handler = nullptr; // nullify, just in case...
				else _installed = true;
				
				delete [] eventTypeSpecs, eventTypeSpecs = nullptr;
			}
		}
		catch (std::bad_alloc)
		{
			// ignore (user can call isInstalled() to find failures)
		}
	}
	return _installed;
}// install


template < typename event_type_container >
bool
CarbonEventHandlerWrapGeneric< event_type_container >::
isInstalled	()
const
{
	return _installed;
}// isInstalled


template < typename event_type_container >
void
CarbonEventHandlerWrapGeneric< event_type_container >::
remove ()
{
	if (nullptr != _handler)
	{
		remove(_procUPP, _handler);
	}
	_installed = false;
}// remove (no arguments)


template < typename event_type_container >
void
CarbonEventHandlerWrapGeneric< event_type_container >::
remove	(EventHandlerUPP&	inoutUPP,
		 EventHandlerRef&	inoutHandler)
{
	if (nullptr != inoutHandler) RemoveEventHandler(inoutHandler), inoutHandler = nullptr;
	if (nullptr != inoutUPP) DisposeEventHandlerUPP(inoutUPP), inoutUPP = nullptr;
}// remove (2 arguments)


CarbonEventHandlerWrap::
CarbonEventHandlerWrap ()
:
CarbonEventHandlerWrapGeneric< CarbonEventSet >()
{
}// CarbonEventHandlerWrap default constructor


CarbonEventHandlerWrap::
CarbonEventHandlerWrap	(EventTargetRef				inTarget,
						 EventHandlerProcPtr		inProcPtr,
						 CarbonEventSet const&		inEvents,
						 void*						inDataToPassToHandler)
:
CarbonEventHandlerWrapGeneric< CarbonEventSet >
(inTarget, inProcPtr, inEvents, inDataToPassToHandler)
{
}// CarbonEventHandlerWrap 4-argument constructor

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
