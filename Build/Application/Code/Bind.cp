/*###############################################################

	Bind.cp
	
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

// MacTelnet includes
#include "Bind.h"



#pragma mark Public Methods

/*!
Creates a binding for the “primary on/off state”
of the given UI element, and the data model
callbacks specified.

This routine will return nullptr if there is any
problem creating the attachment; typically this
occurs because the request is illogical (e.g. if
you pass a checkbox or bevel button, those have
a clear primary on/off target; but a menu or a
window would not).

(3.1)
*/
Bind_AttachmentRef
Bind_NewPrimaryBooleanAttachment	(Bind_Token						inToWhichUIElement,
									 FourCharCode					inAttachmentID,
									 Bind_BooleanConverterProcPtr	inHowToRetrieveData,
									 Bind_BooleanReceiverProcPtr	inHowToHearAboutUserChanges,
									 Bind_NotificationRules			inWhenToHearAboutUserChanges)
{
	Bind_AttachmentRef		result = nullptr;
	
	
	assert(kBind_NotificationRuleAlways == inWhenToHearAboutUserChanges);
	switch (inToWhichUIElement.what)
	{
	case kBind_TokenTypeHIView:
		{
			OSStatus		error = noErr;
			ControlKind		controlKind;
			
			
			error = GetControlKind(inToWhichUIElement.as.viewRef, &controlKind);
			if (noErr == error)
			{
				switch (controlKind.signature)
				{
				case kControlKindSignatureApple:
					// standard Mac OS X control
					switch (controlKind.kind)
					{
					case kControlKindBevelButton: // these buttons have have sticky (on/off) behavior
					case kControlKindDisclosureButton: // “on” is “open”, “off” is “closed”
					case kControlKindDisclosureTriangle: // “on” is “open”, “off” is “closed”
					case kControlKindCheckGroupBox: // like a checkbox
					case kControlKindCheckBox:
						// these controls ARE essentially “convertible to Boolean”
						break;
					
					default:
						// illogical or unknown
						break;
					}
					break;
				
				default:
					// illogical
					break;
				}
			}
		}
		break;
	
	case kBind_TokenTypeMenu:
	case kBind_TokenTypeWindow:
	default:
		// illogical
		break;
	}
	return result;
}// NewPrimaryBooleanAttachment


/*!
Returns the attachment ID code given to the
specified attachment when it was constructed.
If the specified attachment is invalid, the
result is "kBind_InvalidAttachmentID".

(3.1)
*/
FourCharCode
Bind_AttachmentReturnID		(Bind_AttachmentRef		inTarget)
{
	FourCharCode	result = kBind_InvalidAttachmentID;
	
	
	return result;
}// AttachmentReturnID


/*!
Brings the user interface element(s) of the
specified attachment into sync; that is, the
data-gathering callback is invoked to retrieve
the latest value, and the appropriate APIs are
used to modify the element(s) accordingly.

NOTE:	Modifications in the other direction
		(user changes) are detected with
		Carbon Events and handled as they occur.

(3.1)
*/
Bind_Result
Bind_AttachmentSynchronize	(Bind_AttachmentRef		inTarget)
{
	Bind_Result		result = kBind_ResultOK;
	
	
	return result;
}// AttachmentSynchronize

// BELOW IS REQUIRED NEWLINE TO END FILE
