/*!	\file DialogAdjust.h
	\brief Completely handles all the “dirty work” of resizing
	windows or dialog boxes.
*/
/*###############################################################

	Interface Library 2.4
	© 1998-2011 by Kevin Grant
	
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

#include <DialogAdjust.h>
#include <UniversalDefines.h>

// standard-C++ includes
#include <iterator>
#include <list>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Cursors.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>



#pragma mark Types
namespace {

struct AdjustmentData
{
	// It is important to correctly rearrange items
	// belonging to an embedding hierarchy.  Therefore,
	// dialog adjustments are first *proposed* by
	// using DialogAdjust_BeginAdjustment() followed by
	// one or more DialogAdjust_Addxxx() calls, where
	// each adjustment is described by an instance of
	// AdjustmentData.
	//
	// A logical implementation order is determined by a
	// call to DialogAdjust_EndAdjustment(), which will
	// also perform all adjustments in an efficient way.
	union
	{
		ControlRef			control;
		DialogItemIndex		itemIndex;
	} adjusted;
	SInt32			amount;
};
typedef struct AdjustmentData	AdjustmentData;
typedef AdjustmentData*			AdjustmentDataPtr;

struct AdjustmentDelta
{
	// This allows both delta-X and delta-Y to be
	// passed to the offscreen graphics routine
	// without “wasting” both "data1" and "data2".
	// See the adjustControlsOperation() method.
	SInt32		deltaX;
	SInt32		deltaY;
};
typedef struct AdjustmentDelta	AdjustmentDelta;
typedef AdjustmentDelta*		AdjustmentDeltaPtr;

typedef std::list< AdjustmentDataPtr >							AdjustmentDataList;
typedef std::map< DialogItemAdjustment, AdjustmentDataList >	AdjustmentTypeToListOfAdjustmentsMap;

} // anonymous namespace

#pragma mark Variables
namespace {

WindowRef								gPendingAdjustmentWindowRef = nullptr;
Boolean									gPendingAdjustmentListContainsDialogItems = true;
AdjustmentTypeToListOfAdjustmentsMap	gPendingAdjustmentsMap;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean		adjustControlsOperation		(ControlRef, SInt16, SInt16, GDHandle, SInt32, SInt32);
void		beginAdjustment				(WindowRef, Boolean);

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this routine after DialogAdjust_BeginControlAdjustment().
Use this method once for every control adjustment that must be
made.  If you must resize or move a control in two directions
(horizontal and vertical), you must call this routine twice to
register the operations separately - this is required for proper
handling of the changes.

Always invoke DialogAdjust_AddControl() in the order you would
use if the window were made *bigger* in *both* dimensions; that
is, adjust embedders before their subcontrols. The list of
proposed adjustments will then be invoked in a logical order by
a call to the DialogAdjust_EndAdjustment() routine.

If "inAmount" is 0, this routine automatically has no effect.
This allows you to write “stupid code” that installs change
requests for all controls, while still only registering the
changes that would actually have an effect.

IMPORTANT:	You cannot use this method if you have started any
			adjustments with DialogAdjust_BeginDialogItemAdjustment().
			Always bracket calls to this routine with calls to
			DialogAdjust_BeginControlAdjustment() and
			DialogAdjust_EndAdjustment().

(1.0)
*/
void
DialogAdjust_AddControl		(DialogItemAdjustment	inAdjustment,
							 ControlRef				inControlToAdjust,
							 SInt32					inAmount)
{
	Boolean		isError = false;
	
	
	if (inAdjustment < 0) isError = true;
	else if (inAmount != 0)
	{
		AdjustmentDataList&		itemList = gPendingAdjustmentsMap[inAdjustment];
		AdjustmentDataPtr		dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(AdjustmentData)), AdjustmentDataPtr);
		
		
		if (dataPtr == nullptr) isError = true;
		else
		{
			Boolean		invert = false;
			
			
			dataPtr->adjusted.control = inControlToAdjust;
			dataPtr->amount = inAmount;
			
			// resize controls inside-out to avoid cutoff drawing glitches
			// (however, order seems unimportant when moving controls)
			if (inAdjustment & kDialogItemAdjustmentResize) invert = (inAmount < 0);
			if (invert)
			{
				itemList.push_front(dataPtr);
				assert(!itemList.empty());
				assert(itemList.front() == dataPtr);
			}
			else
			{
				itemList.push_back(dataPtr);
				assert(!itemList.empty());
				assert(itemList.back() == dataPtr);
			}
		}
	}
	
	if (isError) Sound_StandardAlert();
}// AddControl


/*!
Call this routine after DialogAdjust_BeginDialogItemAdjustment().
Use this method once for every dialog item adjustment that must
be made.  If you must resize or move an item in two directions
(horizontal and vertical), you must call this routine twice to
register the operations separately - this is required for proper
handling of the changes.

Always invoke DialogAdjust_AddDialogItem() in the order you would
use if the dialog were made *bigger* in *both* dimensions; that
is, adjust embedders before their subcontrols. The list of
proposed adjustments will then be invoked in a logical order by
a call to the DialogAdjust_EndAdjustment() routine.

If "inAmount" is 0, this routine automatically has no effect.
This allows you to write “stupid code” that installs change
requests for all controls, while still only registering the
changes that would actually have an effect.

IMPORTANT:	You cannot use this method if you have started any
			adjustments with DialogAdjust_BeginControlAdjustment().
			Always bracket calls to this routine with calls to
			DialogAdjust_BeginDialogItemAdjustment() and
			DialogAdjust_EndAdjustment().

(1.0)
*/
void
DialogAdjust_AddDialogItem	(DialogItemAdjustment	inAdjustment,
							 DialogItemIndex		inItemIndex,
							 SInt32					inAmount)
{
	Boolean		isError = false;
	
	
	if (inAdjustment < 0) isError = true;
	else if (inAmount != 0)
	{
		AdjustmentDataList&		itemList = gPendingAdjustmentsMap[inAdjustment];
		AdjustmentDataPtr		dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(AdjustmentData)), AdjustmentDataPtr);
		
		
		if (dataPtr == nullptr) isError = true;
		else
		{
			Boolean		invert = false;
			
			
			dataPtr->adjusted.itemIndex = inItemIndex;
			dataPtr->amount = inAmount;
			
			// resize controls inside-out to avoid cutoff drawing glitches
			// (however, order seems unimportant when moving controls)
			if (inAdjustment & kDialogItemAdjustmentResize) invert = (inAmount < 0);
			if (invert)
			{
				itemList.push_front(dataPtr);
				assert(!itemList.empty());
				assert(itemList.front() == dataPtr);
			}
			else
			{
				itemList.push_back(dataPtr);
				assert(!itemList.empty());
				assert(itemList.back() == dataPtr);
			}
		}
	}
	
	if (isError) Sound_StandardAlert();
}// AddDialogItem


/*!
Call this routine to construct a new list of adjustments
for a particular window.  Once created, use one or more
calls to DialogAdjust_AddControl() to make a list of
proposed control adjustments.  When finished, call the
DialogAdjust_EndAdjustment() routine.

(1.0)
*/
void
DialogAdjust_BeginControlAdjustment		(WindowRef		inWindow)
{
	beginAdjustment(inWindow, false);
}// BeginControlAdjustment


/*!
Call this routine to construct a new list of adjustments
for a particular dialog box.  Once created, use one or
more calls to DialogAdjust_AddDialogItem() to make a list
of proposed item adjustments.  When finished, call the
DialogAdjust_EndAdjustment() routine.

(1.0)
*/
void
DialogAdjust_BeginDialogItemAdjustment		(DialogRef		inDialog)
{
	beginAdjustment(GetDialogWindow(inDialog), true);
}// BeginDialogItemAdjustment


/*!
Call this routine to effect changes to a dialog box or
a window that you listed using other item adjustment
routines from this module.

If the dialog is being made horizontally smaller, then
the list of "kDialogItemAdjustmentResizeH" operations is
traversed backwards; otherwise, it is traversed in the
order the adjustments were specified.

If the dialog is being made vertically smaller, then the
list of "kDialogItemAdjustmentResizeV" operations is
traversed backwards; otherwise, it is traversed in the
order the adjustments were specified.

(1.0)
*/
void
DialogAdjust_EndAdjustment		(SInt32		inDialogDeltaSizeX,
								 SInt32		inDialogDeltaSizeY)
{
	AdjustmentDelta		delta;
	
	
	delta.deltaX = inDialogDeltaSizeX;
	delta.deltaY = inDialogDeltaSizeY;
	
	if (Embedding_WindowUsesCompositingMode(gPendingAdjustmentWindowRef))
	{
		HIViewRef	root = nullptr;
		OSStatus	error = HIViewFindByID(HIViewGetRoot(gPendingAdjustmentWindowRef)/* start where */,
											kHIViewWindowContentID/* find what */, &root);
		
		
		assert(error == noErr);
		(Boolean)adjustControlsOperation(root/* use root */, 0/* unused depth */, 0/* unused device flags */,
											nullptr/* unused target device */, (SInt32)&delta/* data 1 */, 0L/* unused data 2 */);
	}
	else
	{
		Embedding_OffscreenControlOperation(gPendingAdjustmentWindowRef, nullptr/* use root */, adjustControlsOperation/* proc. */,
											(SInt32)&delta/* data 1 */, 0L/* data 2 (reserved) */);
	}
}// EndAdjustment


#pragma mark Internal Methods
namespace {

/*!
This method, of standard OffscreenOperationProcPtr
form, performs the specified adjustment of controls
or dialog items in the given window.  This allows,
memory permitting, the entire arrangement to take
place in an offscreen graphics world, which would
greatly smooth the updating of windows.

The value of "inData1" is a pointer to a structure
of type "AdjustmentDelta", specifying the change
in width and height of the window.  The value of
"inData2" is reserved.

If the operation cannot be completed, "false" is
returned; otherwise, "true" is returned.

(1.0)
*/
Boolean
adjustControlsOperation		(ControlRef		inSpecificControlOrRoot,
							 SInt16			UNUSED_ARGUMENT(inColorDepth),
							 SInt16			UNUSED_ARGUMENT(inDeviceFlags),
							 GDHandle		UNUSED_ARGUMENT(inTargetDevice),
							 SInt32			inData1,
							 SInt32			UNUSED_ARGUMENT(inData2))
{
	OSStatus			error = noErr;
	AdjustmentDeltaPtr	deltaPtr = REINTERPRET_CAST(inData1, AdjustmentDeltaPtr);
	Boolean				adjustingDialogItems = gPendingAdjustmentListContainsDialogItems; // as opposed to regular controls
	Boolean				result = true;
	
	
	if (inSpecificControlOrRoot == nullptr) error = memPCErr;
	if (deltaPtr == nullptr) error = memPCErr;
	if (error == noErr)
	{
		typedef std::vector< DialogItemAdjustment >					AdjustmentExecutionOrderList;
		
		AdjustmentDataPtr											dataPtr = nullptr;
		WindowRef													window = GetControlOwner(inSpecificControlOrRoot);
		Boolean														wasControlVisible = false;
		AdjustmentExecutionOrderList								orderedListOfAdjustments(kDialogItemAdjustmentCount);
		AdjustmentExecutionOrderList::const_iterator				orderedListOfAdjustmentsIterator;
		std::back_insert_iterator< AdjustmentExecutionOrderList >	nextAdjustment(orderedListOfAdjustments);
		
		
		// hide the root, so moving controls, etc. can’t be “seen”
		//(OSStatus)SetControlVisibility(inSpecificControlOrRoot, false/* visibility */, false/* draw */);
		
		// Decide which operations should be performed first, based on what is
		// likely to cause the least graphics glitches if buffering doesn’t work;
		// rule of thumb is “resize before move if shrinking a dimension, move
		// before resize if expanding a dimension”.
		//
		// NOTE: Probably, all of these conditionals can be skipped on Mac OS X.
		if (deltaPtr->deltaX < 0)
		{
			if (deltaPtr->deltaY < 0)
			{
				*nextAdjustment++ = kDialogItemAdjustmentResizeH;
				*nextAdjustment++ = kDialogItemAdjustmentResizeV;
				*nextAdjustment++ = kDialogItemAdjustmentMoveH;
				*nextAdjustment++ = kDialogItemAdjustmentMoveV;
			}
			else // (deltaPtr->deltaY >= 0))
			{
				*nextAdjustment++ = kDialogItemAdjustmentResizeH;
				*nextAdjustment++ = kDialogItemAdjustmentMoveH;
				*nextAdjustment++ = kDialogItemAdjustmentMoveV;
				*nextAdjustment++ = kDialogItemAdjustmentResizeV;
			}
		}
		else // (deltaPtr->deltaX >= 0)
		{
			if (deltaPtr->deltaY < 0)
			{
				*nextAdjustment++ = kDialogItemAdjustmentMoveH;
				*nextAdjustment++ = kDialogItemAdjustmentResizeH;
				*nextAdjustment++ = kDialogItemAdjustmentResizeV;
				*nextAdjustment++ = kDialogItemAdjustmentMoveV;
			}
			else // (deltaPtr->deltaY >= 0)
			{
				*nextAdjustment++ = kDialogItemAdjustmentMoveH;
				*nextAdjustment++ = kDialogItemAdjustmentMoveV;
				*nextAdjustment++ = kDialogItemAdjustmentResizeH;
				*nextAdjustment++ = kDialogItemAdjustmentResizeV;
			}
		}
		
		// Decide how to perform the operations, and then do them!
		// As soon as an operation is performed, it is destroyed,
		// because there is no need to keep the information in
		// memory any longer.
		for (orderedListOfAdjustmentsIterator = orderedListOfAdjustments.begin();
				orderedListOfAdjustmentsIterator != orderedListOfAdjustments.end();
				++orderedListOfAdjustmentsIterator)
		{
			DialogItemAdjustment			currentAdjustment = *orderedListOfAdjustmentsIterator;
			AdjustmentDataList&				currentList = gPendingAdjustmentsMap[*orderedListOfAdjustmentsIterator];
			AdjustmentDataList::iterator	adjustmentDataIterator;
			
			
			// Now perform the operations.  As soon as an operation
			// is performed, it is destroyed, because the information
			// is not used for anything else.  When all operations
			// are performed, therefore, the list itself is disposed.
			for (adjustmentDataIterator = currentList.begin(); adjustmentDataIterator != currentList.end();
					++adjustmentDataIterator)
			{
				dataPtr = *adjustmentDataIterator;
				if (dataPtr != nullptr)
				{
					ControlRef	control = nullptr;
					Rect		controlRect;
					
					
					// perform the operation...
					if (adjustingDialogItems)
					{
						(OSStatus)GetDialogItemAsControl(GetDialogFromWindow(window), dataPtr->adjusted.itemIndex, &control);
					}
					else
					{
						control = dataPtr->adjusted.control;
					}
					
					wasControlVisible = IsControlVisible(control);
					GetControlBounds(control, &controlRect);
					SetControlVisibility(control, false/* visible */, false/* draw */);
					
					if (currentAdjustment & kDialogItemAdjustmentResize)
					{
						SInt16		sizeX = 0;
						SInt16		sizeY = 0;
						
						
						if (currentAdjustment & kDialogItemAdjustmentVerticalAxis) controlRect.bottom += dataPtr->amount;
						else controlRect.right += dataPtr->amount;
						
						sizeX = controlRect.right - controlRect.left;
						sizeY = controlRect.bottom - controlRect.top;
						
						if (adjustingDialogItems)
						{
							SizeDialogItem(GetDialogFromWindow(window), dataPtr->adjusted.itemIndex, sizeX, sizeY);
						}
						else
						{
							SizeControl(control, sizeX, sizeY);
						}
					}
					else if (currentAdjustment & kDialogItemAdjustmentMove)
					{
						SInt16		positionX = 0;
						SInt16		positionY = 0;
						
						
						if (currentAdjustment & kDialogItemAdjustmentVerticalAxis)
						{
							positionX = controlRect.left;
							positionY = controlRect.top + dataPtr->amount;
						}
						else
						{
							positionX = controlRect.left + dataPtr->amount;
							positionY = controlRect.top;
						}
						
						if (adjustingDialogItems)
						{
							MoveDialogItem(GetDialogFromWindow(window), dataPtr->adjusted.itemIndex, positionX, positionY);
						}
						else
						{
							MoveControl(control, positionX, positionY);
						}
					}
					if (wasControlVisible) SetControlVisibility(control, true/* visible */, true/* draw */);
					
					// ...and now it’s no longer needed, so get rid of it
					Memory_DisposePtr(REINTERPRET_CAST(&dataPtr, Ptr*));
				}
			}
			currentList.clear();
		}
		gPendingAdjustmentWindowRef = nullptr;
		
		// restore the visible state of the root
		//(OSStatus)SetControlVisibility(inSpecificControlOrRoot, true/* visibility */, true/* draw */);
		
		Cursors_UseArrow();
	}
	else result = false;
	
	return result;
}// adjustControlsOperation


/*!
Call this routine to construct a new list of adjustments
for a particular dialog box or window.  This method is
invoked by the two public list-creation methods.  The
"inIsDialogBox" flag determines what kinds of adjustments
are legal to add to the resultant list.

(1.0)
*/
void
beginAdjustment		(WindowRef	inWindow,
					 Boolean	inIsDialogBox)
{
	gPendingAdjustmentWindowRef = inWindow;
	gPendingAdjustmentListContainsDialogItems = inIsDialogBox;
}// beginAdjustment

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
