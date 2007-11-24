/*!	\file Bind.h
	\brief Binds something to a user interface element, so that
	you can be notified of changes to the element (and update
	the element when your data changes).
	
	This currently works for simple UI elements, not all possible
	elements.
*/
/*###############################################################

	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __BIND__
#define __BIND__

// standard-C++ includes
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <ResultCode.template.h>



#pragma mark Constants

FourCharCode const	kBind_InvalidAttachmentID = '----';

typedef ResultCode< UInt32 >	Bind_Result;
Bind_Result const		kBind_ResultOK(0);		//!< no error

enum Bind_TokenType
{
	kBind_TokenTypeHIView		= 0,
	kBind_TokenTypeMenu			= 1,
	kBind_TokenTypeWindow		= 2
};

typedef UInt32 Bind_NotificationRules;
enum
{
	kBind_NotificationRuleAlways	= 0xFFFFFFFF,	//!< notify every time the user changes something
	kBind_NotificationRuleOnce		= 1 << 0		//!< notify only the first time something is changed
};

#pragma mark Types

/*!
Encapsulates a user interface element whose current
selection or value can be logically bound to a
generic data type in an unambiguous way.
*/
struct Bind_Token
{
	Bind_TokenType		what;
	union
	{
		MenuRef			menuRef;
		HIViewRef		viewRef;
		WindowRef		windowRef;
	} as;
};

typedef struct Bind_OpaqueAttachment*	Bind_AttachmentRef;
typedef std::vector< Bind_Token >		Bind_TokenList;
typedef Bind_TokenList::size_type		Bind_Index;
typedef std::vector< Bind_Index >		Bind_IndexList;

#pragma mark Callbacks

/*!
Boolean Converter

Required when binding data to user interface
elements that are capable of an on/off state
(such as a checkbox, but NOT a radio button).
The routine must return a true or false value
to be reflected in the state of the UI element.
*/
typedef Boolean (*Bind_BooleanConverterProcPtr)			(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID);

typedef void	(*Bind_BooleanReceiverProcPtr)			(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 Boolean				inValue);

/*!
CFRange Generator

Required when binding data to user interface
elements that are capable of a range of states
(such as a scroll bar or slider) or a selected
range (such as a list).  The routine must return
a CFRange - that is, a zero-based location and a
length value.
*/
typedef CFRange		(*Bind_CFRangeGeneratorProcPtr)		(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID);

typedef void		(*Bind_CFRangeReceiverProcPtr)		(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 CFRange				inValue);

/*!
CFString Generator

Required when binding data to a set of user
interface elements that have obvious string
targets (such as static text and editable text
fields).  The routine must return a Core Foundation
string, NOT RETAINED (this module will retain the
string if necessary) to be reflected in the
contents of the UI element.
*/
typedef CFStringRef (*Bind_CFStringGeneratorProcPtr)	(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID);

typedef void		(*Bind_CFStringReceiverProcPtr)		(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 CFStringRef			inValue);

/*!
Index Chooser

Required when binding data to a set of user
interface elements that have mutually exclusive
on/off states (such as a set of radio buttons or
a single data browser).  The appropriate return
value is the zero-based index, into the original
list of attachment tokens, that should be “on”;
OR, if attached to a single UI element, the
zero-based index of the selected thing within
that element (say, a list or menu item).
*/
typedef Bind_Index	(*Bind_IndexChooserProcPtr)			(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID);

typedef void		(*Bind_IndexReceiverProcPtr)		(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 Bind_Index				inValue);

/*!
Index Lister

Required when binding data to a set of user
interface elements that allow a disjoint set of
on/off states (such as a set of checkboxes, or
a single data browser).  The appropriate return
value is a list of zero or more zero-based indices,
into the original list of attachment tokens, that
should be “on”; OR, if attached to a single UI
element, a list of zero or more zero-based indices
of the selected things within that element (say, a
list).
*/
typedef void		(*Bind_IndexListerProcPtr)			(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 Bind_IndexList&		inoutList);

typedef void		(*Bind_IndexListReceiverProcPtr)	(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 Bind_IndexList const&	inValue);

/*!
Integer Generator

Required when binding data to a set of user
interface elements that have obvious integer
targets (such as static text and editable text
fields, sliders, and scroll bars).  The routine
must return a 32-bit integer to be reflected in
the contents of the UI element.  Logistics of
string conversion, etc. are automatically taken
care of when necessary.
*/
typedef SInt32		(*Bind_IntegerGeneratorProcPtr)		(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID);

typedef void		(*Bind_IntegerReceiverProcPtr)		(Bind_AttachmentRef		inBinding,
														 FourCharCode			inAttachmentID,
														 SInt32					inValue);



#pragma mark Public Methods

//!\name Creating and Destroying Attachments
//@{

Bind_AttachmentRef
	Bind_NewMaximumMinimumAttachment		(Bind_Token							inToWhichUIElement,
											 FourCharCode						inAttachmentID,
											 Bind_CFRangeGeneratorProcPtr		inHowToRetrieveData);

Bind_AttachmentRef
	Bind_NewPrimaryBooleanAttachment		(Bind_Token							inToWhichUIElement,
											 FourCharCode						inAttachmentID,
											 Bind_BooleanConverterProcPtr		inHowToRetrieveData,
											 Bind_BooleanReceiverProcPtr		inHowToHearAboutUserChanges,
											 Bind_NotificationRules				inWhenToHearAboutUserChanges =
																					kBind_NotificationRuleAlways);

Bind_AttachmentRef
	Bind_NewPrimaryIntegerAttachment		(Bind_Token							inToWhichUIElement,
											 FourCharCode						inAttachmentID,
											 Bind_IntegerGeneratorProcPtr		inHowToRetrieveData,
											 Bind_IntegerReceiverProcPtr		inHowToHearAboutUserChanges,
											 Bind_NotificationRules				inWhenToHearAboutUserChanges =
																					kBind_NotificationRuleAlways);

Bind_AttachmentRef
	Bind_NewPrimaryTextAttachment			(Bind_Token							inToWhichUIElement,
											 FourCharCode						inAttachmentID,
											 Bind_CFStringGeneratorProcPtr		inHowToRetrieveData,
											 Bind_CFStringReceiverProcPtr		inHowToHearAboutUserChanges,
											 Bind_NotificationRules				inWhenToHearAboutUserChanges =
																					kBind_NotificationRuleAlways);

Bind_AttachmentRef
	Bind_NewSelectOneIndexAttachment		(Bind_TokenList const&				inToWhichUIElements,
											 FourCharCode						inAttachmentID,
											 Bind_IndexChooserProcPtr			inHowToRetrieveData,
											 Bind_IndexReceiverProcPtr			inHowToHearAboutUserChanges,
											 Bind_NotificationRules				inWhenToHearAboutUserChanges =
																					kBind_NotificationRuleAlways);

Bind_AttachmentRef
	Bind_NewSelectRangeAttachment			(Bind_TokenList const&				inToWhichUIElements,
											 FourCharCode						inAttachmentID,
											 Bind_CFRangeGeneratorProcPtr		inHowToRetrieveData,
											 Bind_CFRangeReceiverProcPtr		inHowToHearAboutUserChanges,
											 Bind_NotificationRules				inWhenToHearAboutUserChanges =
																					kBind_NotificationRuleAlways);

Bind_AttachmentRef
	Bind_NewTitleAttachment					(Bind_Token							inToWhichUIElement,
											 FourCharCode						inAttachmentID,
											 Bind_CFStringGeneratorProcPtr		inHowToRetrieveData,
											 Bind_CFStringReceiverProcPtr		inHowToHearAboutUserChanges,
											 Bind_NotificationRules				inWhenToHearAboutUserChanges =
																					kBind_NotificationRuleAlways);

void
	Bind_DisposeAttachment					(Bind_AttachmentRef*				inoutRefPtr);

//@}

//!\name Wrapping Bindable User Interface Elements
//@{

inline Bind_Token
	Bind_ReturnMenuToken					(MenuRef							inMenu)
	{
		Bind_Token		result;
		
		
		result.what = kBind_TokenTypeMenu;
		result.as.menuRef = inMenu;
		return result;
	}

inline Bind_Token
	Bind_ReturnViewToken					(HIViewRef							inView)
	{
		Bind_Token		result;
		
		
		result.what = kBind_TokenTypeHIView;
		result.as.viewRef = inView;
		return result;
	}

inline Bind_Token
	Bind_ReturnWindowToken					(HIWindowRef						inWindow)
	{
		Bind_Token		result;
		
		
		result.what = kBind_TokenTypeWindow;
		result.as.windowRef = inWindow;
		return result;
	}

//@}

//!\name Working With Attachments
//@{

FourCharCode
	Bind_AttachmentReturnID					(Bind_AttachmentRef					inTarget);

Bind_Result
	Bind_AttachmentSynchronize				(Bind_AttachmentRef					inTarget);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
