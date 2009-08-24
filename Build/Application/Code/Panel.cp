/*###############################################################

	Panel.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <FlagManager.h>
#include <IconManager.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>

// resource includes
#include "SpacingConstants.r"

// MacTelnet includes
#include "ConstantsRegistry.h"
#include "Panel.h"



#pragma mark Types

struct Panel
{
	struct
	{
		void*					auxiliaryDataPtr;	// arbitrary data
		Panel_ChangeProcPtr		changedProc;		// called when anything changes in the panel
		HIViewRef				container;			// the super-control of every panel control
		CFRetainRelease			descriptor;			// an identifier that helps distinguish panels
		
		IconManagerIconRef		icon;
		CFRetainRelease			name;				// a label for this panel
		CFRetainRelease			description;		// a long description for this panel
		UInt32					showCommandID;		// the four character command ID to show the panel
	} customizerWritable;
	
	struct
	{
		HIWindowRef				owningWindow;		// the window associated with panel controls
		HISize					preferredSize;		// user-specified dimensions of the panel
	} utilizerWritable;
	
	Panel_Ref	selfRef;	// redundant, convenient storage to reference type for this data
};
typedef struct Panel	Panel;
typedef Panel*			PanelPtr;

typedef MemoryBlockPtrLocker< Panel_Ref, Panel >	PanelPtrLocker;
typedef LockAcquireRelease< Panel_Ref, Panel >		PanelAutoLocker;

#pragma mark Internal Method Prototypes

static SInt32		panelChanged	(PanelPtr, Panel_Message, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	PanelPtrLocker&		gPanelPtrLocks ()	{ static PanelPtrLocker x; return x; }
}



#pragma mark Public Methods

/*!
Creates a new Panel.  If any problems occur,
nullptr is returned.

Your dialog panel cannot function without a
change notification procedure.  This routine
is the entry point that dialog boxes use
(indirectly, by invoking one or more methods
from the Panel module) to communicate with
panels in an abstract way.

(3.0)
*/
Panel_Ref
Panel_New	(Panel_ChangeProcPtr	inProc)
{
	Panel_Ref	result = nullptr;
	
	
	if (nullptr != inProc) result = REINTERPRET_CAST(new Panel, Panel_Ref);
	if (nullptr != result)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), result);
		
		
		ptr->customizerWritable.auxiliaryDataPtr = nullptr;
		ptr->customizerWritable.changedProc = inProc;
		ptr->customizerWritable.container = nullptr;
		ptr->customizerWritable.descriptor.setCFTypeRef(kPanel_InvalidKind);
		ptr->customizerWritable.icon = IconManager_NewIcon();
		ptr->utilizerWritable.owningWindow = nullptr;
		ptr->utilizerWritable.preferredSize = CGSizeMake(0, 0);
		ptr->selfRef = result;
	}
	return result;
}// New


/*!
Destroys a Panel previously created with
Panel_New().  Your copy of the reference
is automatically set to nullptr.

(3.0)
*/
void
Panel_Dispose	(Panel_Ref*		inoutRefPtr)
{
	if (nullptr != inoutRefPtr)
	{
		{
			PanelAutoLocker		ptr(gPanelPtrLocks(), *inoutRefPtr);
			
			
			(SInt32)panelChanged(ptr, kPanel_MessageDestroyed,
									Panel_ReturnImplementation(*inoutRefPtr));
			IconManager_DisposeIcon(&ptr->customizerWritable.icon);
		}
		
		// delete outside the block so the reference will be unlocked
		delete *(REINTERPRET_CAST(inoutRefPtr, PanelPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
For a tab frame that fills an entire panel edge-to-edge
with tabs pointing upward, this routine calculates the
exterior boundaries of the entire tab frame.

(3.1)
*/
void
Panel_CalculateTabFrame		(HIViewRef	inPanelContainerView,
							 Point*		outTabFrameTopLeft,
							 Point*		outTabFrameWidthHeight)
{
	if ((nullptr != outTabFrameTopLeft) && (nullptr != outTabFrameWidthHeight))
	{
		HIRect	containerFrame;
		
		
		assert_noerr(HIViewGetFrame(inPanelContainerView, &containerFrame));
		
		SetPt(outTabFrameTopLeft, containerFrame.origin.x + HSP_TAB_AND_DIALOG,
				containerFrame.origin.y + VSP_TAB_AND_DIALOG);
		SetPt(outTabFrameWidthHeight,
				containerFrame.size.width - INTEGER_DOUBLED(HSP_TAB_AND_DIALOG),
				containerFrame.size.height - INTEGER_DOUBLED(VSP_TAB_AND_DIALOG));
	}
}// CalculateTabFrame (3 arguments)


/*!
For a tab frame that fills an entire panel edge-to-edge
with tabs pointing upward, this routine calculates the
exterior boundaries of the entire tab frame.

(3.1)
*/
void
Panel_CalculateTabFrame		(Float32	inPanelContainerWidth,
							 Float32	inPanelContainerHeight,
							 HIPoint&	outTabFrameTopLeft,
							 HISize&	outTabFrameWidthHeight)
{
	outTabFrameTopLeft.x = HSP_TAB_AND_DIALOG;
	outTabFrameTopLeft.y = VSP_TAB_AND_DIALOG;
	outTabFrameWidthHeight.width = inPanelContainerWidth - INTEGER_DOUBLED(HSP_TAB_AND_DIALOG);
	outTabFrameWidthHeight.height = inPanelContainerHeight - INTEGER_DOUBLED(VSP_TAB_AND_DIALOG);
}// CalculateTabFrame (4 arguments)


/*!
When a dialog box needs to manipulate a panel,
its code should invoke this routine.  The code
responsible for customizing the specified panel
should have already used the method
Panel_SetContainerView() to set the super-view
(which is most likely a user pane) that embeds
every other view in the panel.

The dialog box can then move the view provided
by this routine, instantly relocating every view
in the panel, or perform other operations, such
as changing the visibility or active state of an
entire panel.

(3.0)
*/
void
Panel_GetContainerView	(Panel_Ref		inRef,
						 HIViewRef&		outView)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		outView = ptr->customizerWritable.container;
	}
}// GetContainerView


/*!
Returns a one or two line string that describes a
little about the panel (for instance, the kinds of
things you can do with it).  This may be used in
user interface elements in paragraph form.

See also Panel_GetName(), which acts as a very short
description (title) of the panel.

The returned string is NOT retained.

(3.1)
*/
void
Panel_GetDescription	(Panel_Ref		inRef,
						 CFStringRef&	outDescription)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		outDescription = ptr->customizerWritable.description.returnCFStringRef();
	}
}// GetDescription


/*!
Returns a very short label string that can be used to
describe a panel.  Panels should always specify this
label so that dialogs can use it to represent a set of
panels.  For example, labels may be used for text in
buttons, menus, lists or tabs that select amongst all
active panels.  Do not assume how your panel’s label
will be displayed in a user interface.

The returned string is NOT retained.

(3.1)
*/
void
Panel_GetName	(Panel_Ref		inRef,
				 CFStringRef&	outName)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		outName = ptr->customizerWritable.name.returnCFStringRef();
	}
}// GetName


/*!
Returns the user-specified size for the panel.  You
generally restore the panel to this size (if allowed
by the ideal size constraint) as part of panel display.

If none has been set, the width and height will be zero.

(3.1)
*/
void
Panel_GetPreferredSize	(Panel_Ref	inRef,
						 HISize&	outSize)
{
	outSize = CGSizeMake(0, 0);
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		outSize = ptr->utilizerWritable.preferredSize;
	}
}// GetPreferredSize


/*!
For a tab frame that fills an entire panel edge-to-edge
with tabs pointing upward, this routine returns the
offsets from the frame to reach the interior bounds.

Offsets will always be positive; the bottom-right offset
must therefore be subtracted from the bottom and right
edges of the boundary.

(3.0)
*/
void
Panel_GetTabPaneInsets	(Point*		outTabPaneTopLeft,
						 Point*		outTabPaneBottomRight)
{
	if ((nullptr != outTabPaneTopLeft) && (nullptr != outTabPaneBottomRight))
	{
		SetPt(outTabPaneTopLeft, HSP_TABPANE_AND_TAB, TAB_HT_BIG + VSP_TABPANE_AND_TAB);
		SetPt(outTabPaneBottomRight, HSP_TABPANE_AND_TAB, VSP_TABPANE_AND_TAB);
	}
}// GetTabPaneInsets


/*!
Use within a "Panel_ChangeProcPtr" callback; sends the received
message to another panel.  This may be useful if one panel acts
as a container/supervisor for other panels.

(3.1)
*/
SInt32
Panel_PropagateMessage		(Panel_Ref		inRef,
							 Panel_Message	inMessage,
							 void*			inDataPtr)
{
	PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
	
	
	return panelChanged(ptr, inMessage, inDataPtr);
}// PropagateMessage


/*!
Returns a pointer to auxiliary data you previously set
with Panel_SetImplementation().

Auxiliary data is reserved for use by panel customizing
code.  You generally use the panel descriptor to help
identify what the auxiliary data may be.  See also
Panel_ReturnKind().

(3.0)
*/
void*
Panel_ReturnImplementation	(Panel_Ref	inRef)
{
	void*	result = nullptr;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = ptr->customizerWritable.auxiliaryDataPtr;
	}
	
	return result;
}// ReturnImplementation


/*!
Helps identify a panel.

(3.0)
*/
Panel_Kind
Panel_ReturnKind	(Panel_Ref	inRef)
{
	Panel_Kind		result = kPanel_InvalidKind;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = ptr->customizerWritable.descriptor.returnCFStringRef();
	}
	
	return result;
}// ReturnKind


/*!
Returns the window that a panel is in.  This
property is set once, automatically, when a
dialog box fires the "kPanel_MessageCreateViews"
message.

(3.0)
*/
HIWindowRef
Panel_ReturnOwningWindow	(Panel_Ref	inRef)
{
	HIWindowRef		result = nullptr;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = ptr->utilizerWritable.owningWindow;
	}
	
	return result;
}// ReturnOwningWindow


/*!
Returns the Mac OS command ID (four character code)
for the command that should cause this panel to be
displayed.

This is an attribute only; the Panel module does not
handle this command in any way.

(3.1)
*/
UInt32
Panel_ReturnShowCommandID	(Panel_Ref	inRef)
{
	UInt32		result = 0;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = ptr->customizerWritable.showCommandID;
	}
	
	return result;
}// ReturnShowCommandID


/*!
Sends the "kPanel_MessageCreateViews" to the
specified panel’s handler, providing the handler
the given window.

See "Panel.h" for more information on this event.

(3.1)
*/
void
Panel_SendMessageCreateViews	(Panel_Ref		inRef,
								 HIWindowRef	inOwningWindow)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageCreateViews, &inOwningWindow);
	}
}// SendMessageCreateViews


/*!
Sends the "kPanel_MessageFocusFirst" to the specified
panel’s handler.

See "Panel.h" for more information on this event.

(4.0)
*/
void
Panel_SendMessageFocusFirst		(Panel_Ref		inRef)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageFocusFirst, nullptr/* context */);
	}
}// SendMessageFocusFirst


/*!
Sends the "kPanel_MessageFocusGained" to the
specified panel’s handler, providing the handler
the given view.

See "Panel.h" for more information on this event.

(3.0)
*/
void
Panel_SendMessageFocusGained	(Panel_Ref		inRef,
								 HIViewRef		inViewGainingKeyboardFocus)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageFocusGained, &inViewGainingKeyboardFocus);
	}
}// SendMessageFocusGained


/*!
Sends the "kPanel_MessageFocusLost" to the
specified panel’s handler, providing the handler
the given view.

See "Panel.h" for more information on this event.

(3.0)
*/
void
Panel_SendMessageFocusLost	(Panel_Ref		inRef,
							 HIViewRef		inViewLosingKeyboardFocus)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageFocusLost, &inViewLosingKeyboardFocus);
	}
}// SendMessageFocusLost


/*!
Sends the "kPanel_MessageGetEditType" to the
specified panel’s handler, and returns the panel’s
response.

See "Panel.h" for more information on this event.

(3.1)
*/
SInt32
Panel_SendMessageGetEditType	(Panel_Ref	inRef)
{
	SInt32	result = 0L;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = panelChanged(ptr, kPanel_MessageGetEditType, nullptr/* message data */);
	}
	
	return result;
}// SendMessageGetEditType


/*!
Sends the "kPanel_MessageGetGrowBoxLook" to the
specified panel’s handler, and returns the panel’s
response.

See "Panel.h" for more information on this event.

(3.1)
*/
SInt32
Panel_SendMessageGetGrowBoxLook		(Panel_Ref	inRef)
{
	SInt32	result = 0L;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = panelChanged(ptr, kPanel_MessageGetGrowBoxLook, nullptr/* message data */);
	}
	
	return result;
}// SendMessageGetGrowBoxLook


/*!
Sends the "kPanel_MessageGetHelpKeyPhrase" to the
specified panel’s handler, and returns the panel’s
response.

See "Panel.h" for more information on this event.

(3.1)
*/
SInt32
Panel_SendMessageGetHelpKeyPhrase	(Panel_Ref	inRef)
{
	SInt32	result = 0L;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = panelChanged(ptr, kPanel_MessageGetHelpKeyPhrase, nullptr/* message data */);
	}
	
	return result;
}// SendMessageGetHelpKeyPhrase


/*!
Sends the "kPanel_MessageGetIdealSize" to the
specified panel’s handler, and returns the panel’s
response.

The data is only valid if the response is
"kPanel_ResponseSizeProvided".

See also Panel_SetPreferredSize().

See "Panel.h" for more information on this event.

(3.1)
*/
SInt32
Panel_SendMessageGetIdealSize	(Panel_Ref	inRef,
								 HISize&	outData)
{
	SInt32	result = 0L;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = panelChanged(ptr, kPanel_MessageGetIdealSize, &outData/* message data */);
	}
	
	return result;
}// SendMessageGetIdealSize


/*!
Sends the "kPanel_MessageNewAppearanceTheme" to
the specified panel’s handler.

See "Panel.h" for more information on this event.

(3.0)
*/
void
Panel_SendMessageNewAppearanceTheme		(Panel_Ref	inRef)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageNewAppearanceTheme, nullptr/* message data */);
	}
}// SendMessageNewAppearanceTheme


/*!
Sends the "kPanel_MessageNewDataSet" to the
specified panel’s handler, providing the handler
the given data set information.

See "Panel.h" for more information on this event.

(3.1)
*/
void
Panel_SendMessageNewDataSet		(Panel_Ref							inRef,
								 Panel_DataSetTransition const&		inNewDataSet)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker				ptr(gPanelPtrLocks(), inRef);
		// a copy is made because the data is small and is not
		// “supposed” to be changed by the callback; this ensures
		// any accidental changes only affect the copy below
		Panel_DataSetTransition		mutableCopy = inNewDataSet;
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageNewDataSet, &mutableCopy);
	}
}// SendMessageNewDataSet


/*!
Sends the "kPanel_MessageNewVisibility" to the
specified panel’s handler, providing the handler
the given true/false value.

See "Panel.h" for more information on this event.

(3.0)
*/
void
Panel_SendMessageNewVisibility	(Panel_Ref	inRef,
								 Boolean	inIsNowVisible)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(SInt32)panelChanged(ptr, kPanel_MessageNewVisibility, &inIsNowVisible);
	}
}// SendMessageNewVisibility


/*!
Conveniently sets up any Appearance control that
can take standard content types such as icons,
pictures, etc. so it uses the icon defined for this
panel (if any).  You can use this, for example, to
set the icon for a bevel button.

(3.0)
*/
OSStatus
Panel_SetButtonIcon		(HIViewRef		inView,
						 Panel_Ref		inRef)
{
	OSStatus	result = noErr;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = IconManager_SetButtonIcon(inView, ptr->customizerWritable.icon);
	}
	return result;
}// SetButtonIcon


/*!
Sets the super-view (usually a user pane) that
directly or indirectly embeds every view in
the specified panel.

Dialog boxes that use Panel objects generally
require custom panels to have a container view!
Without one, it is not possible for a dialog to
include the views of a panel, much less
manipulate them.  A dialog uses the method
Panel_GetContainerView() to acquire the view
that is specified here.

(3.0)
*/
void
Panel_SetContainerView	(Panel_Ref		inRef,
						 HIViewRef		inView)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->customizerWritable.container = inView;
	}
}// SetContainerView


/*!
Specifies a long (multi-sentence) description string
for the panel.  This description might be displayed
in user interface elements.

The Core Foundation string is retained.

(3.1)
*/
void
Panel_SetDescription	(Panel_Ref		inRef,
						 CFStringRef	inDescription)
{
	if ((nullptr != inDescription) && (nullptr != inRef))
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->customizerWritable.description.setCFTypeRef(inDescription);
	}
}// SetDescription


/*!
Uses an Icon Services icon set to describe a panel.
An Icon Services reference will not take effect
unless the user has at least OS 8.5.  If OS 8.5 or
later is not in use, an attempt is made to find an
equivalent icon suite, which may fail.

See also Panel_SetIconRefFromBundleFile() and
Panel_SetIconSuite().

(3.0)
*/
void
Panel_SetIconRef	(Panel_Ref	inRef,
					 SInt16		inIconServicesVolumeNumber,
					 OSType		inIconServicesCreator,
					 OSType		inIconServicesDescription)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(OSStatus)IconManager_MakeIconRef(ptr->customizerWritable.icon,
											inIconServicesVolumeNumber,
											inIconServicesCreator,
											inIconServicesDescription);
	}
}// SetIconRef


/*!
Uses an Icon Services icon set to describe a panel,
but based on a flat file in the Mac OS X bundle.

See also Panel_SetIconRef() and Panel_SetIconSuite().

(3.0)
*/
void
Panel_SetIconRefFromBundleFile	(Panel_Ref		inRef,
								 CFStringRef	inFileNameWithoutExtension,
								 OSType			inIconServicesCreator,
								 OSType			inIconServicesDescription)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(OSStatus)IconManager_MakeIconRefFromBundleFile(ptr->customizerWritable.icon,
														inFileNameWithoutExtension,
														inIconServicesCreator,
														inIconServicesDescription);
	}
}// SetIconRefFromBundleFile


/*!
Uses a traditional icon suite to describe a panel.
An Icon Services reference will automatically take
effect if OS 8.5 or later is in use and an equivalent
icon can be found.  If no equivalent can be found,
the icon suite resource is used.

See also Panel_SetIconRefFromBundleFile() and
Panel_SetIconRef().

(3.0)
*/
void
Panel_SetIconSuite	(Panel_Ref			inRef,
					 SInt16				inSuiteResourceID,
					 IconSelectorValue	inWhichIcons)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		(OSStatus)IconManager_MakeIconSuite(ptr->customizerWritable.icon,
											inSuiteResourceID, inWhichIcons);
	}
}// SetIconSuite


/*!
Associates arbitrary data with a panel.  You
generally also call Panel_SetKind(), to help
you identify what this auxiliary data is.
This access is only allowed by panel customizing
code, not dialogs.

To acquire this data later, use this method
Panel_ReturnImplementation().

(3.0)
*/
void
Panel_SetImplementation		(Panel_Ref	inRef,
							 void*		inAuxiliaryDataPtr)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->customizerWritable.auxiliaryDataPtr = inAuxiliaryDataPtr;
	}
}// SetImplementation


/*!
Changes the descriptor of a dialog panel, which in
effect identifies this panel’s type.  Your descriptor
should not be "kPanel_InvalidKind".

Descriptors can be useful in identifying auxiliary
data: see also the method Panel_SetImplementation().

(3.0)
*/
void
Panel_SetKind	(Panel_Ref		inRef,
				 Panel_Kind		inDescriptor)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->customizerWritable.descriptor.setCFTypeRef(inDescriptor);
	}
}// SetKind


/*!
Specifies a label string that can be used to describe
a panel.  Generally, even if panel implementations do
not use labels themselves, they should specify a label
that dialogs can use as desired to represent a set of
panels.  For example, labels may be used for text in
buttons, menus, lists or tabs that select amongst
active panels.  Since you should not assume how your
panel will be displayed in a user interface, provide a
label whether you need it or not.

See also Panel_SetIconSuite(), which lets you specify
a set of icons for your panel that is often paired
with a label in dialog boxes.

The Core Foundation string is retained.

(3.1)
*/
void
Panel_SetName	(Panel_Ref		inRef,
				 CFStringRef	inName)
{
	if ((nullptr != inName) && (nullptr != inRef))
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->customizerWritable.name.setCFTypeRef(inName);
	}
}// SetName


/*!
Sets the dimensions for the panel that the user wants.

Dialogs should update this whenever a user event
changes the panel size (however, you should constrain
this according to the ideal or minimum size).  You
might also update this size as part of initialization,
after reading a preferences file for instance.

This size should not be smaller than the minimum size,
but this routine will not check the size for you.

(3.1)
*/
void
Panel_SetPreferredSize	(Panel_Ref		inRef,
						 HISize const&	inSize)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->utilizerWritable.preferredSize = inSize;
	}
}// SetPreferredSize


/*!
Specifies the Mac OS command ID (four character code)
for the command that should cause this panel to be
displayed.

This is an attribute only; the Panel module does not
handle this command in any way.

(3.1)
*/
void
Panel_SetShowCommandID	(Panel_Ref	inRef,
						 UInt32		inCommandID)
{
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		ptr->customizerWritable.showCommandID = inCommandID;
	}
}// SetShowCommandID


/*!
Conveniently sets the label and icon of a standard
HIToolbar item to use the icon and label of the given
panel, and sets the command ID to that returned by
Panel_ReturnShowCommandID().

(3.1)
*/
OSStatus
Panel_SetToolbarItemIconAndLabel	(HIToolbarItemRef	inItem,
									 Panel_Ref			inRef)
{
	OSStatus	result = noErr;
	
	
	if (nullptr != inRef)
	{
		PanelAutoLocker		ptr(gPanelPtrLocks(), inRef);
		
		
		result = HIToolbarItemSetCommandID(inItem, Panel_ReturnShowCommandID(inRef));
		if (noErr == result)
		{
			result = HIToolbarItemSetLabel(inItem, ptr->customizerWritable.name.returnCFStringRef());
			if (noErr == result)
			{
				result = IconManager_SetToolbarItemIcon(inItem, ptr->customizerWritable.icon);
			}
		}
	}
	return result;
}// SetToolbarItemIconAndLabel


#pragma mark Internal Methods

/*!
When a dialog box performs some kind of operation on a
panel, such as changing its size or visible state, this
routine should always be invoked to notify the panel that
a change has occurred.  Some messages generate defined
result codes, and others do not.  See "Panel.h" to see
which messages have results.

You specify the routine that will get this notification
when you create a panel with Panel_New().

(3.0)
*/
static SInt32
panelChanged	(PanelPtr		inPtr,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32	result = 0L;
	
	
	if (nullptr != inPtr)
	{
		// automatically save this property, for convenience
		if (inMessage == kPanel_MessageCreateViews)
		{
			inPtr->utilizerWritable.owningWindow = *(REINTERPRET_CAST(inDataPtr, HIWindowRef*));
		}
		
		if (nullptr != inPtr->customizerWritable.changedProc)
		{
			// notify the panel of the change
			result = Panel_InvokeChangeProc(inPtr->customizerWritable.changedProc,
											inPtr->selfRef, inMessage, inDataPtr);
		}
	}
	
	return result;
}// panelChanged

// BELOW IS REQUIRED NEWLINE TO END FILE
