/*!	\file DialogAdjust.h
	\brief Completely handles all the “dirty work” of resizing
	windows or dialog boxes.
	
	Just begin either a control or dialog item adjustment,
	assemble a list of adjustments to make to the various
	controls in a window or dialog box respectively, and end
	the adjustment to have your changes take effect.
	
	The Dialog Adjust module assumes you are making adjustments
	*as if* the user were expanding the size of a window both
	horizontally and vertically; under this assumption, Dialog
	Adjust *automatically changes* the order of adjustment
	depending on what the actual delta-X and delta-Y values are.
	This smoothes out window resizing in the event that there is
	not enough memory to use an offscreen buffer.  But if there
	*is* enough memory, an offscreen buffer is automatically
	created and your dialog adjustments are dumped to the screen
	in a single, fluid motion.
*/
/*###############################################################

	Interface Library 1.3
	© 1998-2006 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
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

#ifndef __DIALOGADJUST__
#define __DIALOGADJUST__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef SInt32 DialogItemAdjustment;
enum
{
	// do not use these first constants, they’re only used to define other constants
	kDialogItemAdjustmentResize = (1 << 0),		// control dimension is changing
	kDialogItemAdjustmentMove = (1 << 1),		// control position is changing
	kDialogItemAdjustmentVerticalAxis = (1 << 2),	// vertical change (horizontal by default)
	// use the constants below
	kDialogItemAdjustmentResizeH = kDialogItemAdjustmentResize,
	kDialogItemAdjustmentResizeV = kDialogItemAdjustmentResize | kDialogItemAdjustmentVerticalAxis,
	kDialogItemAdjustmentMoveH = kDialogItemAdjustmentMove,
	kDialogItemAdjustmentMoveV = kDialogItemAdjustmentMove | kDialogItemAdjustmentVerticalAxis,
	// number of possible values
	kDialogItemAdjustmentCount = 4
};



#pragma mark Public Methods

//!\name Item Adjustment
//@{

// WARNING - YOU CANNOT COMBINE DialogAdjust_AddDialogItem() CALLS WITHIN THE SAME BEGIN-END BRACKET AS THIS CALL
void
	DialogAdjust_AddControl					(DialogItemAdjustment			inAdjustment,
											 ControlRef						inControlToAdjust,
											 SInt32							inAmount);

// WARNING - YOU CANNOT COMBINE DialogAdjust_AddControlAdjustment() CALLS WITHIN THE SAME BEGIN-END BRACKET AS THIS CALL
void
	DialogAdjust_AddDialogItem				(DialogItemAdjustment			inAdjustment,
											 DialogItemIndex				inItemIndex,
											 SInt32							inAmount);

// CONTROL ADJUSTMENT OPERATIONS ARE NOT YET NESTABLE
void
	DialogAdjust_BeginControlAdjustment		(WindowRef						inWindow);

// CONTROL ADJUSTMENT OPERATIONS ARE NOT YET NESTABLE
void
	DialogAdjust_BeginDialogItemAdjustment	(DialogRef						inDialog);

void
	DialogAdjust_EndAdjustment				(SInt32							inDialogDeltaSizeX,
											 SInt32							inDialogDeltaSizeY);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
