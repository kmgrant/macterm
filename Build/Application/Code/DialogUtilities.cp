/*###############################################################

	DialogUtilities.cp
	
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

// standard-C includes
#include <cctype>
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <Cursors.h>
#include <Embedding.h>
#include <FileSelectionDialogs.h>
#include <HIViewWrap.h>
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "GeneralResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "ContextualMenuBuilder.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "GenericThreads.h"
#include "MenuBar.h"
#include "Preferences.h"
#include "UIStrings.h"



#pragma mark Types
namespace {

/*!
This structure is used for DeviceLoopClipAndDrawControl().
*/
struct My_DeviceLoopControlInfo
{
	DialogUtilities_DrawViewProcPtr		proc;
	ControlRef							control;
	SInt16								controlPartCode;
};
typedef struct My_DeviceLoopControlInfo const*		My_DeviceLoopControlInfoConstPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

pascal void		clipAndDrawControlDeviceLoop	(short, short, GDHandle, long);
void			useBestHelpButtonIcon			(ControlRef);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a set of controls using a dummy NIB as a basis;
in other words, the controls are really intended for a
different window, with perhaps a completely different
control hierarchy, but it is desirable to use Interface
Builder to do the layout anyway.  (For instance, this
may be useful when laying out tab panes or other things
that Interface Builder does not otherwise do well.)

For convenience, the size of the NIB window is returned.
You can use this, for instance, to resize your destination
region if it is not large enough.

The destination window must also be the owner of the
given destination parent control (pass the root control
of the window if you have no other more specific parent
pane to use).

The new controls are appended to the given list of
controls.

This function is necessary because Apple didn’t make
this easy, soon enough.  Specifically, not only is there
no CreateControlsFromNib() routine akin to the APIs for
creating windows and menus, but the OS does not allow
existing controls to be moved between windows on all OS
versions!  (Another motivator for this function is to
avoid having a user pane parent in a NIB - annoying
because of the way Interface Builder renders it as a
shaded rectangle that gets in the way of everything.)

(3.1)
*/
OSStatus
DialogUtilities_CreateControlsBasedOnWindowNIB	(CFStringRef					inNIBFileBasename,
												 CFStringRef					inPanelNameInNIB,
												 WindowRef						inDestinationWindow,
												 ControlRef						inDestinationParent,
												 std::vector< ControlRef >&		inoutControlArray,
												 Rect&							outIdealBounds)
{
	OSStatus	result = noErr;
	
	
	if ((nullptr == inNIBFileBasename) ||
		(nullptr == inPanelNameInNIB) ||
		(nullptr == inDestinationWindow) ||
		(nullptr == inDestinationParent))
	{
		result = memPCErr;
	}
	else if (GetControlOwner(inDestinationParent) != inDestinationWindow)
	{
		result = paramErr;
	}
	else
	{
		NIBWindow	dummyWindow(AppResources_ReturnBundleForNIBs(), inNIBFileBasename, inPanelNameInNIB);
		
		
		dummyWindow << NIBLoader_AssertWindowExists;
		if (false == dummyWindow.exists())
		{
			result = resNotFound;
		}
		else
		{
			HIViewRef	contentView = nullptr;
			
			
			// determine the size of the NIB window (it should be created
			// at its ideal size with no wasted edge space)
			assert_noerr(GetWindowBounds(dummyWindow, kWindowContentRgn, &outIdealBounds));
			
			// get the root view, in order to count subviews
			result = HIViewFindByID(HIViewGetRoot(dummyWindow), kHIViewWindowContentID, &contentView);
			if (noErr == result)
			{
				HIViewRef		templateView = HIViewGetLastSubview(contentView);
				
				
			#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
				{
					CFIndex const	kControlCount = HIViewCountSubviews(contentView);
					
					
					// first set the vector size appropriately
					inoutControlArray.resize(inoutControlArray.size() + kControlCount);
				}
			#endif
				
				// find all the views in the window, and use their characteristics
				// to create duplicate views; this is done “backwards” (last to
				// first) so that the view order is preserved, e.g. with respect
				// to the tab ordering specified in Interface Builder
				for (; ((noErr == result) && (nullptr != templateView));
						templateView = HIViewGetPreviousView(templateView))
				{
					HIViewRef	newView = nullptr;
					
					
					result = DialogUtilities_DuplicateControl(templateView, inDestinationWindow, newView);
					if (noErr == result)
					{
						assert(nullptr != newView);
						assert_noerr(HIViewAddSubview(inDestinationParent, newView));
						inoutControlArray.push_back(newView);
						assert(false == inoutControlArray.empty());
					}
				}
			}
		}
	}
	
	return result;
}// CreateControlsBasedOnWindowNIB


/*!
Disposes a set of controls created with the
DialogUtilities_CreateControlsBasedOnWindowNIB()
routine.  It is not usually necessary to call
this routine, but is advisable since certain
controls (namely, pop-up menus) have extra data
allocated in order to create them in the first
place; that extra data is freed here.

(3.1)
*/
void
DialogUtilities_DisposeControlsBasedOnWindowNIB		(std::vector< ControlRef > const&		inControlArray)
{
	std::vector< ControlRef >::const_iterator	controlIter;
	
	
	for (controlIter = inControlArray.begin(); controlIter != inControlArray.end(); ++controlIter)
	{
		DialogUtilities_DisposeDuplicateControl(*controlIter);
	}
}// DisposeControlsBasedOnWindowNIB


/*!
To deactivate the root control of the frontmost
window and to change the cursor to the standard
watch, call this method.  Usually, you do this
immediately prior to displaying a dialog box on
the screen.

This routine automatically finds the “real”
front window, deactivating all floating windows
as well.

(3.0)
*/
void
DeactivateFrontmostWindow ()
{
	Embedding_DeactivateFrontmostWindow();
}// DeactivateFrontmostWindow


/*!
To display a standard Save File dialog box and
create a file containing a dump of the control
embedding hierarchy for the specified window,
use this method.

(3.0)
*/
void
DebugSelectControlHierarchyDumpFile		(WindowRef		inForWindow)
{
	FSSpec		file;
	OSStatus	error = noErr;
	Str255		prompt,
				title,
				fileDefaultName;
	Boolean		good = false;
	
	
	PLstrcpy(prompt, "\pCreate a file to show the window’s control hierarchy.");
	PLstrcpy(title, "\pCreate Control Hierarchy Dump File");
	PLstrcpy(fileDefaultName, "\phierarchy-dump.txt");
	
	{
		NavReplyRecord	reply;
		
		
		Alert_ReportOSStatus(error = FileSelectionDialogs_PutFile
										(prompt, title, fileDefaultName,
											AppResources_ReturnCreatorCode(), 'TEXT',
											kPreferences_NavPrefKeyGenericSaveFile,
											kNavDefaultNavDlogOptions | kNavDontAddTranslateItems,
											EventLoop_HandleNavigationUpdate, &reply, &file));
		good = ((error == noErr) && (reply.validRecord));
	}
	
	if (good) error = DumpControlHierarchy(inForWindow, &file);
	else error = userCanceledErr;
	
	if (error != noErr) Sound_StandardAlert();
}// DebugSelectControlHierarchyDumpFile


/*!
If you want to draw something using a DeviceLoop()
drawing procedure, use this method.  The graphics
port is automatically clipped to the specified
region, preserving and restoring any previous
clipping regions.  DeviceLoop() is called using
the given drawing procedure, “user data”, and flags.

IMPORTANT:	On output, the specified clip region
			will be changed to the intersection of
			the region on input with the visible
			region of the current port.

(3.0)
*/
void
DeviceLoopClipAndDraw	(RgnHandle				inoutNewClipRegion,
						 DeviceLoopDrawingUPP	inProc,
						 long					inDeviceLoopUserData,
						 DeviceLoopFlags		inDeviceLoopFlags)
{
	RgnHandle		oldClipRgn = Memory_NewRegion();
	
	
	// get the arrangement of this control
	if ((oldClipRgn != nullptr) && (inoutNewClipRegion != nullptr))
	{
		CGrafPtr	currentPort = nullptr;
		GDHandle	currentDevice = nullptr;
		Boolean		useDeviceLoop = true;
		
		
		// get a reference to the current graphics port
		GetGWorld(&currentPort, &currentDevice);
		
		// for better performance on Mac OS X, lock the bits of a port
		// before performing a series of QuickDraw operations in it,
		// and make the intended drawing area part of the dirty region
		(OSStatus)LockPortBits(currentPort);
		(OSStatus)QDAddRegionToDirtyRegion(currentPort, inoutNewClipRegion);
		
		// on Mac OS X, DeviceLoop() is unnecessary for buffered ports
		useDeviceLoop = !QDIsPortBuffered(currentPort);
		
		// first determine the exact area in which to draw (the intersection
		// of the control region and the graphics port’s visible region)
		GetClip(oldClipRgn);
		
		// now clip drawing in the port to the exact area where drawing should occur
		SetClip(inoutNewClipRegion);
		
		if (useDeviceLoop)
		{
			// call DeviceLoop() to make sure the window looks right no matter how many monitors it may cross
			DeviceLoop(inoutNewClipRegion, inProc, inDeviceLoopUserData, inDeviceLoopFlags);
		}
		else
		{
			// instead of using DeviceLoop(), invoke the drawing procedure using the port pixel map values
			SInt16			colorDepth = 0;
			PixMapHandle	portPixelMap = GetPortPixMap(currentPort);
			GDHandle		device = GetGDevice();
			
			
			colorDepth = (**portPixelMap).pixelSize;
			InvokeDeviceLoopDrawingUPP(colorDepth, TestDeviceAttribute(device, gdDevType),
										device, inDeviceLoopUserData, inProc);
		}
		
		// undo the lock done earlier in this block
		(OSStatus)UnlockPortBits(currentPort);
		
		SetClip(oldClipRgn);
	}
	if (oldClipRgn != nullptr) Memory_DisposeRegion(&oldClipRgn);
}// DeviceLoopClipAndDraw


/*!
If you want to draw a user pane control using a
DeviceLoop() drawing procedure, use this method.

The graphics port is automatically clipped to
the control’s rectangle (or its even more specific
structure region, if an appropriate API from the
Carbon Control Manager is available).  If you are
drawing a control that is capable of acquiring
user focus, you should pass "true" for the
"inExtendClipBoundaryForFocusRing" flag, so that
the clipping area is extended slightly outside
the control boundaries to allow for a focus ring.

The DeviceLoop() routine is called using the given
drawing procedure, passing a “user data” of type
"ControlInfoConstPtr", and flags set to zero.  In
other words, by using this routine you gain the
ability to automatically set up the clipping
properly, but you have no control over the
DeviceLoop() flags and your drawing procedure
*must* interpret its user data as being of type
"ControlInfoConstPtr".

(3.0)
*/
void
DeviceLoopClipAndDrawControl	(ControlRef							inControl,
								 SInt16								inControlPartCode,
								 DialogUtilities_DrawViewProcPtr	inProc,
								 Boolean							inExtendClipBoundaryForFocusRing)
{
	RgnHandle	newClipRgn = Memory_NewRegion();
	
	
	if (newClipRgn != nullptr)
	{
		DeviceLoopDrawingUPP		upp = nullptr;
		My_DeviceLoopControlInfo	controlInfo;
		OSStatus					error = noErr;
		
		
		// first determine the exact area in which to draw (the intersection
		// of the control region and the graphics port’s visible region)
		error = GetControlRegion(inControl, kControlStructureMetaPart, newClipRgn);
		if (error != noErr)
		{
			Rect		controlBounds;
			
			
			error = noErr;
			GetControlBounds(inControl, &controlBounds);
			SetRectRgn(newClipRgn, controlBounds.left, controlBounds.top,
						controlBounds.right, controlBounds.bottom);
		}
		
		InsetRgn(newClipRgn, (inExtendClipBoundaryForFocusRing) ? -4 : -1,
					(inExtendClipBoundaryForFocusRing) ? -4 : -1);
		controlInfo.proc = inProc;
		controlInfo.control = inControl;
		controlInfo.controlPartCode = inControlPartCode;
		upp = NewDeviceLoopDrawingUPP(clipAndDrawControlDeviceLoop);
		DeviceLoopClipAndDraw(newClipRgn, upp, REINTERPRET_CAST(&controlInfo, long), 0L/* flags */);
		DisposeDeviceLoopDrawingUPP(upp), upp = nullptr;
		Memory_DisposeRegion(&newClipRgn);
	}
}// DeviceLoopClipAndDrawControl


/*!
If DialogUtilities_DuplicateControl() was used to
copy a control, this routine can be used to free the
copy.  Note that in most cases this is not required,
but is advisable anyway in case the implementation
changes.  Currently, only pop-up menu controls must
be disposed with this routine, so their copied menus
can be removed from the menu list and disposed of.

(3.1)
*/
void
DialogUtilities_DisposeDuplicateControl		(ControlRef		inDuplicatedControl)
{
	ControlKind		controlInfo;
	
	
	// find out what it is, and create a duplicate that matches
	(OSStatus)GetControlKind(inDuplicatedControl, &controlInfo);
	if (kControlKindSignatureApple == controlInfo.signature)
	{
		// undo the allocation steps done for each type of control
		// in the DialogUtilities_DuplicateControl() routine
		switch (controlInfo.kind)
		{
		case kControlKindPopupButton:
			{
				OSStatus	error = noErr;
				MenuRef		allocatedMenu = nullptr;
				Size		actualSize = 0;
				
				
				error = GetControlData(inDuplicatedControl, kControlEntireControl, kControlPopupButtonMenuRefTag,
											sizeof(allocatedMenu), &allocatedMenu, &actualSize);
				if (noErr == error)
				{
					DeleteMenu(GetMenuID(allocatedMenu));
					DisposeMenu(allocatedMenu);
				}
			}
			break;
		
		default:
			// currently, no other control kinds require cleanup
			break;
		}
	}
	else
	{
		// ???
	}
}// DisposeDuplicateControl


/*!
Creates a new control based on an existing one; the new
control may be in a different window than the original.
This is useful for many purposes, but was designed to
make it easier to create windows in Interface Builder
whose contents can be easily put in other places.

You should call DialogUtilities_DisposeDuplicateControl()
when you are finished with a control duplicate, to ensure
any extra data created on behalf of the control is freed.
Currently, this only applies to pop-up menus, where a
copy of the menu is created and inserted into the menu list.

IMPORTANT:	This routine is currently implemented “as
			needed”, so it does not support all possible
			controls or arrangements.  It should be easy
			to add support for extra stuff when that is
			necessary.

(3.1)
*/
OSStatus
DialogUtilities_DuplicateControl	(ControlRef		inTemplateControl,
									 WindowRef		inDestinationWindow,
									 ControlRef&	outNewControl)
{
	OSStatus	result = noErr;
	
	
	outNewControl = nullptr; // initially...
	
	if (nullptr == inTemplateControl) result = memPCErr;
	else
	{
		ControlKind		templateControlInfo;
		Rect			templateBounds;
		HIRect			templateFrame;
		Boolean			kindBased = true;
		
		
		// copy the control’s boundaries (pretty much always needed)
		GetControlBounds(inTemplateControl, &templateBounds);
		HIViewGetFrame(inTemplateControl, &templateFrame);
		
		// find out what it is, and create a duplicate that matches
		result = GetControlKind(inTemplateControl, &templateControlInfo);
		kindBased = (noErr == result);
		if (HIObjectIsOfClass(REINTERPRET_CAST(inTemplateControl, HIObjectRef), kHIImageViewClassID))
		{
			kindBased = false;
			result = HIImageViewCreate(nullptr, &outNewControl);
			if (noErr == result)
			{
				(OSStatus)HIImageViewSetOpaque(outNewControl, HIImageViewIsOpaque(inTemplateControl));
				(OSStatus)HIImageViewSetAlpha(outNewControl, HIImageViewGetAlpha(inTemplateControl));
				(OSStatus)HIImageViewSetScaleToFit(outNewControl, HIImageViewGetScaleToFit(inTemplateControl));
				
				(OSStatus)HIViewSetFrame(outNewControl, &templateFrame);
				
				(OSStatus)HIViewSetVisible(outNewControl, true);
			}
		}
		
		if (kindBased)
		{
			if (kControlKindSignatureApple == templateControlInfo.signature)
			{
				switch (templateControlInfo.kind)
				{
				case kControlKindBevelButton:
					{
						CFStringRef		titleCFString = nullptr;
						
						
						result = CopyControlTitleAsCFString(inTemplateControl, &titleCFString);
						if (noErr == result)
						{
							ControlButtonContentInfo	contentInfo;
							ControlBevelThickness		bevelThickness = kControlBevelButtonNormalBevel;
							ControlBevelButtonBehavior	buttonBehavior = kControlBehaviorPushbutton;
							ControlButtonTextAlignment	textAlignment = kControlBevelButtonAlignTextCenter;
							ThemeButtonKind				buttonKind = kThemeMediumBevelButton;
							Size						actualSize = 0;
							
							
							// TEMPORARY - duplicating buttons with menus is not yet supported
							// ignore result because the data may not be present in the original
							(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlIconContentTag,
														sizeof(bevelThickness), &bevelThickness, &actualSize);
							bzero(&contentInfo, sizeof(contentInfo));
							// ignore result because the data may not be present in the original
							(OSStatus)GetBevelButtonContentInfo(inTemplateControl, &contentInfo);
							
							result = CreateBevelButtonControl(inDestinationWindow, &templateBounds, titleCFString,
																bevelThickness, buttonBehavior, &contentInfo,
																0/* menu ID */, 0/* menu behavior */,
																kControlBevelButtonMenuOnBottom, &outNewControl);
							CFRelease(titleCFString), titleCFString = nullptr;
							
							// copy alignment
							if (noErr == GetControlData(inTemplateControl, kControlEntireControl, kControlBevelButtonTextAlignTag,
														sizeof(textAlignment), &textAlignment, &actualSize))
							{
								(OSStatus)SetControlData(outNewControl, kControlEntireControl, kControlBevelButtonTextAlignTag,
															sizeof(textAlignment), &textAlignment);
							}
							
							// copy font style
							{
								ControlFontStyleRec		styleInfo;
								
								
								bzero(&styleInfo, sizeof(styleInfo));
								if (noErr == GetControlData(inTemplateControl, kControlEntireControl, kControlFontStyleTag,
															sizeof(styleInfo), &styleInfo, &actualSize))
								{
									(OSStatus)SetControlData(outNewControl, kControlEntireControl, kControlFontStyleTag,
																sizeof(styleInfo), &styleInfo);
								}
							}
							
							// copy the button kind too
							if (noErr == GetControlData(inTemplateControl, kControlEntireControl, kControlBevelButtonKindTag,
														sizeof(buttonKind), &buttonKind, &actualSize))
							{
								(OSStatus)SetControlData(outNewControl, kControlEntireControl, kControlBevelButtonKindTag,
															sizeof(buttonKind), &buttonKind);
							}
						}
					}
					break;
				
				case kControlKindChasingArrows:
					{
						result = CreateChasingArrowsControl(inDestinationWindow, &templateBounds, &outNewControl);
					}
					break;
				
				case kControlKindCheckBox:
					{
						CFStringRef		titleCFString = nullptr;
						
						
						result = CopyControlTitleAsCFString(inTemplateControl, &titleCFString);
						if (noErr == result)
						{
							result = CreateCheckBoxControl(inDestinationWindow, &templateBounds,
															titleCFString, GetControl32BitValue(inTemplateControl),
															true/* auto-toggle; Carbon Events are assumed */,
															&outNewControl);
							CFRelease(titleCFString), titleCFString = nullptr;
						}
					}
					break;
				
				case kControlKindEditUnicodeText:
					{
						CFStringRef		textCFString = nullptr;
						
						
						GetControlTextAsCFString(inTemplateControl, textCFString);
						if (nullptr == textCFString) textCFString = CFSTR("");
						
						{
							ControlFontStyleRec		styleInfo;
							Size					actualSize = 0;
							
							
							bzero(&styleInfo, sizeof(styleInfo));
							// ignore result because the data may not be present in the original
							(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlFontStyleTag,
														sizeof(styleInfo), &styleInfo, &actualSize);
							result = CreateEditUnicodeTextControl(inDestinationWindow, &templateBounds, textCFString,
																	false/* is password */, &styleInfo, &outNewControl);
						}
					}
					break;
				
				case kControlKindIcon:
					{
						ControlButtonContentInfo	contentInfo;
						Size						actualSize = 0;
						
						
						bzero(&contentInfo, sizeof(contentInfo));
						// ignore result because the data may not be present in the original
						(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlIconContentTag,
													sizeof(contentInfo), &contentInfo, &actualSize);
						result = CreateIconControl(inDestinationWindow, &templateBounds, &contentInfo,
													true/* tracking; assumed to never be necessary */, &outNewControl);
					}
					break;
				
				case kControlKindLittleArrows:
					{
						SInt32		incrementValue = 1L;
						Size		actualSize = 0L;
						
						
						// ignore result because the data may not be present in the original
						(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlLittleArrowsIncrementValueTag,
													sizeof(incrementValue), &incrementValue, &actualSize);
						result = CreateLittleArrowsControl(inDestinationWindow, &templateBounds,
															GetControl32BitValue(inTemplateControl),
															GetControl32BitMinimum(inTemplateControl),
															GetControl32BitMaximum(inTemplateControl),
															incrementValue,
															&outNewControl);
					}
					break;
				
				case kControlKindPopupButton:
					{
						//MenuRef		duplicateMenu = MenuBar_NewMenuWithArbitraryID();
						
						
						//if (menu == nullptr) result = memFullErr; // arbitrary
						//else
						{
							MenuRef		originalMenu = nullptr;
							Size		actualSize = 0;
							
							
							result = GetControlData(inTemplateControl, kControlEntireControl, kControlPopupButtonMenuRefTag,
													sizeof(originalMenu), &originalMenu, &actualSize);
							if (noErr == result)
							{
								OSStatus	error = noErr;
								Boolean		isSegmentedViewPlaceholder = false;
								
								
								// Although segmented views are available in Mac OS X 10.3,
								// Interface Builder does not support them on 10.3.  To
								// work around this extreme annoyance, pop-up menus are
								// used as placeholders (to contain all relevant details,
								// like the size, the option titles and their command IDs),
								// and a property is set to identify the menu as a segmented
								// view.  These menus are scrapped, and real segmented views
								// are created instead.  All calling code should use
								// segmented view APIs when dealing with the “duplicate”.
								error =  GetControlProperty(inTemplateControl, AppResources_ReturnCreatorCode(),
															'Segm', sizeof(isSegmentedViewPlaceholder), nullptr/* actual size */,
															&isSegmentedViewPlaceholder);
								if ((noErr == error) && (isSegmentedViewPlaceholder))
								{
									// this is actually supposed to be a segmented view;
									// use the menu items as the base for all segments,
									// and make the first item initially selected
									result = HISegmentedViewCreate(&templateFrame, &outNewControl);
									if (noErr == result)
									{
										UInt32				segmentCount = CountMenuItems(originalMenu);
										HISegmentBehavior	segmentBehavior = kHISegmentBehaviorRadio;
										OptionBits			segmentAttributes = 0L;
										SInt32				segmentValue = 0;
										UInt32				segmentCommand = 0;
										CFStringRef			segmentLabelCFString = nullptr;
										// The content width per segment appears to exclude the padding in the
										// buttons, and there is no apparent way to automatically determine
										// what the padding is.  Therefore, a HARD-CODED APPROXIMATION (TEMPORARY)
										// is used, that should be replaced by a future API call, if an API
										// becomes available.  This adjustment prevents the segmented view from
										// having boundaries that exceed the original frame.
										float				segmentWidth = (templateFrame.size.width - 9 * segmentCount) /
																			segmentCount;
										Boolean				segmentEnabled = true;
										
										
										(OSStatus)HISegmentedViewSetSegmentCount(outNewControl, segmentCount);
										for (UInt32 i = 1; i <= segmentCount; ++i)
										{
											error = HISegmentedViewSetSegmentBehavior(outNewControl, i, segmentBehavior);
											assert_noerr(error);
											
											if (1 == i)
											{
												error = HISegmentedViewSetSegmentValue(outNewControl, i,
																						kControlRadioButtonCheckedValue);
												assert_noerr(error);
											}
											
											error = GetMenuItemCommandID(originalMenu, i, &segmentCommand);
											assert_noerr(error);
											error = HISegmentedViewSetSegmentCommand(outNewControl, i, segmentCommand);
											assert_noerr(error);
											
											if (noErr == CopyMenuItemTextAsCFString(originalMenu, i, &segmentLabelCFString))
											{
												error = HISegmentedViewSetSegmentLabel(outNewControl, i, segmentLabelCFString);
												assert_noerr(error);
												CFRelease(segmentLabelCFString), segmentLabelCFString = nullptr;
											}
											
											error = HISegmentedViewSetSegmentContentWidth(outNewControl, i, false/* auto-calculate */,
																							segmentWidth);
											assert_noerr(error);
											
											error = HISegmentedViewSetSegmentEnabled(outNewControl, i, segmentEnabled);
											assert_noerr(error);
										}
										(OSStatus)HIViewSetVisible(outNewControl, true);
									}
								}
								else
								{
									MenuRef		duplicateMenu = nullptr;
									
									
									// not a segmented view, but a pop-up menu after all
									result = DuplicateMenu(originalMenu, &duplicateMenu);
									if (noErr == result)
									{
										SetMenuID(duplicateMenu, MenuBar_ReturnUniqueMenuID());
										InsertMenu(duplicateMenu, kInsertHierarchicalMenu);
										result = CreatePopupButtonControl(inDestinationWindow, &templateBounds, CFSTR(""),
																			GetMenuID(duplicateMenu), true/* variable width */,
																			0/* title width */,
																			teFlushDefault/* title justification */, normal/* title style */,
																			&outNewControl);
									}
								}
							}
						}
					}
					break;
				
				case kControlKindPushButton:
					{
						CFStringRef		titleCFString = nullptr;
						
						
						result = CopyControlTitleAsCFString(inTemplateControl, &titleCFString);
						if (noErr == result)
						{
							result = CreatePushButtonControl(inDestinationWindow, &templateBounds,
																titleCFString, &outNewControl);
							CFRelease(titleCFString), titleCFString = nullptr;
						}
					}
					break;
				
				case kControlKindRadioButton:
					{
						CFStringRef		titleCFString = nullptr;
						
						
						result = CopyControlTitleAsCFString(inTemplateControl, &titleCFString);
						if (noErr == result)
						{
							result = CreateRadioButtonControl(inDestinationWindow, &templateBounds,
																titleCFString, GetControl32BitValue(inTemplateControl),
																true/* auto-toggle; Carbon Events are assumed */,
																&outNewControl);
							CFRelease(titleCFString), titleCFString = nullptr;
						}
					}
					break;
				
				case kControlKindRoundButton:
					{
						ControlButtonContentInfo	contentInfo;
						ControlRoundButtonSize		roundButtonSize = kControlRoundButtonNormalSize;
						Size						actualSize = 0;
						
						
						// ignore result because the data may not be present in the original
						(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlRoundButtonSizeTag,
													sizeof(roundButtonSize), &roundButtonSize, &actualSize);
						bzero(&contentInfo, sizeof(contentInfo));
						// ignore result because the data may not be present in the original
						(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlRoundButtonContentTag,
													sizeof(contentInfo), &contentInfo, &actualSize);
						
						result = CreateRoundButtonControl(inDestinationWindow, &templateBounds, roundButtonSize,
															&contentInfo, &outNewControl);
					}
					break;
				
				case kControlKindSeparator:
					{
						result = CreateSeparatorControl(inDestinationWindow, &templateBounds, &outNewControl);
					}
					break;
				
				case kControlKindSlider:
					{
						UInt32		numberOfTickMarks = 0;
						
						
						// use a property to read tick marks, if available (since there is no
						// API for reading this automatically)
						(OSStatus)GetControlProperty(inTemplateControl, AppResources_ReturnCreatorCode(),
														'Tick', sizeof(numberOfTickMarks), nullptr/* actual size */,
														&numberOfTickMarks);
						
						// NOTE: There are no control data tags for sliders, so it is difficult
						// to faithfully replicate them!!!
						result = CreateSliderControl(inDestinationWindow, &templateBounds,
														GetControl32BitValue(inTemplateControl),
														GetControl32BitMinimum(inTemplateControl),
														GetControl32BitMaximum(inTemplateControl),
														kControlSliderPointsDownOrRight, numberOfTickMarks,
														true/* live tracking */, nullptr/* action routine */,
														&outNewControl);
					}
					break;
				
				case kControlKindStaticText:
					{
						CFStringRef		textCFString = nullptr;
						
						
						GetControlTextAsCFString(inTemplateControl, textCFString);
						if (nullptr == textCFString) textCFString = CFSTR("");
						
						{
							ControlFontStyleRec		styleInfo;
							Size					actualSize = 0;
							
							
							bzero(&styleInfo, sizeof(styleInfo));
							// ignore result because the data may not be present in the original
							(OSStatus)GetControlData(inTemplateControl, kControlEntireControl, kControlFontStyleTag,
														sizeof(styleInfo), &styleInfo, &actualSize);
							result = CreateStaticTextControl(inDestinationWindow, &templateBounds,
																textCFString, &styleInfo, &outNewControl);
						}
					}
					break;
				
				case kControlKindUserPane:
					{
						UInt32		features = 0L;
						
						
						result = GetControlFeatures(inTemplateControl, &features);
						if (noErr == result)
						{
							result = CreateUserPaneControl(inDestinationWindow, &templateBounds,
															features, &outNewControl);
						}
					}
					break;
				
				case kHISegmentedViewKind:
					{
						result = HISegmentedViewCreate(&templateFrame, &outNewControl);
						if (noErr == result)
						{
							UInt32				segmentCount = HISegmentedViewGetSegmentCount(inTemplateControl);
							HISegmentBehavior	segmentBehavior = kHISegmentBehaviorRadio;
							OptionBits			segmentAttributes = 0L;
							SInt32				segmentValue = 0;
							UInt32				segmentCommand = 0;
							CFStringRef			segmentLabelCFString = nullptr;
							float				segmentWidth = 0.0;
							Boolean				segmentWidthAutoCalculated = false;
							Boolean				segmentEnabled = false;
							
							
							(OSStatus)HISegmentedViewSetSegmentCount(outNewControl, segmentCount);
							for (UInt32 i = 1; i <= segmentCount; ++i)
							{
								segmentBehavior = HISegmentedViewGetSegmentBehavior(inTemplateControl, i);
								(OSStatus)HISegmentedViewSetSegmentBehavior(outNewControl, i, segmentBehavior);
								
								segmentAttributes = HISegmentedViewGetSegmentAttributes(inTemplateControl, i);
								(OSStatus)HISegmentedViewChangeSegmentAttributes(outNewControl, i, segmentAttributes/* set */,
																					0L/* clear */);
								
								segmentValue = HISegmentedViewGetSegmentValue(inTemplateControl, i);
								(OSStatus)HISegmentedViewSetSegmentValue(outNewControl, i, segmentValue);
								
								segmentCommand = HISegmentedViewGetSegmentCommand(inTemplateControl, i);
								(OSStatus)HISegmentedViewSetSegmentCommand(outNewControl, i, segmentCommand);
								
								if (noErr == HISegmentedViewCopySegmentLabel(inTemplateControl, i, &segmentLabelCFString))
								{
									(OSStatus)HISegmentedViewSetSegmentLabel(outNewControl, i, segmentLabelCFString);
									CFRelease(segmentLabelCFString), segmentLabelCFString = nullptr;
								}
								
								segmentWidth = HISegmentedViewGetSegmentContentWidth(inTemplateControl, i, &segmentWidthAutoCalculated);
								(OSStatus)HISegmentedViewSetSegmentContentWidth(outNewControl, i, segmentWidthAutoCalculated, segmentWidth);
								
								segmentEnabled = HISegmentedViewIsSegmentEnabled(inTemplateControl, i);
								(OSStatus)HISegmentedViewSetSegmentEnabled(outNewControl, i, segmentEnabled);
							}
							(OSStatus)HIViewSetVisible(outNewControl, true);
						}
					}
					break;
				
				default:
					// not all types are supported (add more if needed)
					Console_Warning(Console_WriteValueFourChars, "do not know how to duplicate interface element of type",
									templateControlInfo.kind);
					result = unimpErr;
					break;
				}
			}
			else
			{
				// ???
				result = paramErr;
			}
		}
		
		// if successful, copy other common attributes (again, arbitrary,
		// and if you depend on other things, you need to add support for
		// them here)
		if ((noErr == result) && (nullptr != outNewControl))
		{
			// if a view has an ID, copy that to the new view
			{
				HIViewID	controlID;
				
				
				if (noErr == GetControlID(inTemplateControl, &controlID))
				{
					result = SetControlID(outNewControl, &controlID);
				}
			}
			
			// if a view has an associated command, copy that to the new view
			{
				UInt32		commandID = 0;
				
				
				if (noErr == GetControlCommandID(inTemplateControl, &commandID))
				{
					result = SetControlCommandID(outNewControl, commandID);
				}
			}
			
			// if a view has layout information, copy that to the new view
			{
				HILayoutInfo	layoutInfo;
				
				
				bzero(&layoutInfo, sizeof(layoutInfo));
				layoutInfo.version = kHILayoutInfoVersionZero;
				if (noErr == HIViewGetLayoutInfo(inTemplateControl, &layoutInfo))
				{
					result = HIViewSetLayoutInfo(outNewControl, &layoutInfo);
				}
			}
			
			// copy other attributes
			SetControl32BitMaximum(outNewControl, GetControl32BitMaximum(inTemplateControl));
			SetControl32BitMinimum(outNewControl, GetControl32BitMinimum(inTemplateControl));
			SetControl32BitValue(outNewControl, GetControl32BitValue(inTemplateControl));
			
			// if a control has an unusual size, make sure the duplicate is also resized;
			// note that not all controls have this tag, so ignore it for those cases
			{
				ControlSize		controlSize = kControlSizeNormal;
				Size			actualSize = 0;
				
				
				if (noErr == GetControlData(inTemplateControl, kControlEntireControl, kControlSizeTag,
											sizeof(controlSize), &controlSize, &actualSize))
				{
					// not all views support size, apparently, so ignore this result
					(OSStatus)SetControlData(outNewControl, kControlEntireControl, kControlSizeTag,
												sizeof(controlSize), &controlSize);
				}
			}
		}
	}
	
	return result;
}// DuplicateControl


/*!
DEPRECATED.  Do not use.  All current invocations
are in old-style dialogs that are being rewritten
anyway.
*/
Boolean
FlashButtonControl	(ControlRef		inControl,
					 Boolean		inIsDefaultButton,
					 Boolean		inIsCancelButton)
{
	Boolean			result = false;
	
	
	if (IsControlActive(inControl))
	{
		// play sound when highlighting button
		if (inIsDefaultButton) PlayThemeSound(kThemeSoundDefaultButtonPress);
		else if (inIsCancelButton) PlayThemeSound(kThemeSoundCancelButtonPress);
		else PlayThemeSound(kThemeSoundButtonPress);
		
		// under Carbon, it is necessary to hand-hold QuickDraw to get
		// the damned highlighting to actually show up immediately
		HiliteControl(inControl, kControlButtonPart);
		//EventLoop_HandlePendingUpdates();
		GenericThreads_DelayMinimumTicks(8);
		HiliteControl(inControl, kControlNoPart);
		//EventLoop_HandlePendingUpdates();
		
		// play sound when unhighlighting button
		if (inIsDefaultButton) PlayThemeSound(kThemeSoundDefaultButtonRelease);
		else if (inIsCancelButton) PlayThemeSound(kThemeSoundCancelButtonRelease);
		else PlayThemeSound(kThemeSoundButtonRelease);
		
		result = true;
	}
	return result;
}// FlashButtonControl


/*!
To convert the text in a text field or static
text control into a number, use this convenient
method.

(3.0)
*/
void
GetControlNumericalText		(ControlRef		inControl,
							 SInt32*		outNumberPtr)
{
	if (outNumberPtr != nullptr)
	{
		Str255		string;
		
		
		GetControlText(inControl, string);
		StringToNum(string, outNumberPtr);
	}
}// GetControlNumericalText


/*!
To get a static text or editable text control’s
text contents as a four-character code, use this
method.

(3.0)
*/
void
GetControlOSTypeText	(ControlRef		inControl,
						 OSType*		outTypePtr)
{
	if (outTypePtr != nullptr)
	{
		Str15	typeString;
		
		
		PLstrcpy(typeString, EMPTY_PSTRING);
		GetControlText(inControl, typeString);
		typeString[0] = 4;
		BlockMoveData(&typeString[1], outTypePtr, sizeof(OSType));
	}
}// GetControlOSTypeText


/*!
To get a copy of the text in a text field control,
use this method.

(3.0)
*/
void
GetControlText	(ControlRef			inControl,
				 Str255				outText)
{
	if (outText != nullptr)
	{
		Size	actualSize = 0;
		
		
		// obtain the data in the generic buffer
		(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlEditTextTextTag,
									255 * sizeof(UInt8), (Ptr)(1 + outText), &actualSize);
		((Ptr)(outText))[0] = (char)actualSize; // mush in the length byte
	}
}// GetControlText


/*!
Returns the specified control’s text as a CFString.

(3.0)
*/
void
GetControlTextAsCFString	(ControlRef		inControl,
							 CFStringRef&	outCFString)
{
	Size	actualSize = 0;
	
	
	outCFString = nullptr; // in case of error
	(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlEditTextCFStringTag,
								sizeof(outCFString), &outCFString, &actualSize);
}// GetControlTextAsCFString


/*!
To create a new dialog from a 'DLOG', 'DITL'
and 'dlgx' resource having the specified ID,
use this method.  The dialog is returned, and
if a non-nullptr reference to a Window Info
object is provided, a new Window Info object
is automatically created.  Dispose of the
Window Info object when you are finished
with the dialog by invoking the method
WindowInfo_Dispose().

(3.0)
*/
void
GetNewDialogWithWindowInfo		(SInt16				inDialogResourceID,
								 DialogRef*			outDialog,
								 WindowInfo_Ref*	outWindowInfoRefPtr)
{
	if (outDialog != nullptr)
	{
		Cursors_DeferredUseWatch(30); // if it takes more than half a second to initialize, show the watch cursor
		
		*outDialog = GetNewDialog(inDialogResourceID, nullptr, (WindowRef)-1);
		if (outWindowInfoRefPtr != nullptr) *outWindowInfoRefPtr = WindowInfo_New();
		if (*outDialog != nullptr)
		{
			// MacTelnet has a thread that takes care of this, so don’t let the Dialog Manager do it
			SetDialogTracksCursor(*outDialog, false);
		}
	}
}// GetNewDialogWithWindowInfo


/*!
This is a standard ControlKeyFilterUPP that will
prevent any characters invalid in an Internet host name
from being typed into a text field.

(3.1)
*/
pascal ControlKeyFilterResult
HostNameLimiter		(ControlRef			inControl,
					 SInt16*			inKeyCode,
					 SInt16*			inCharCode,
					 EventModifiers*	UNUSED_ARGUMENT(inModifiers))
{
	ControlKeyFilterResult  result = kControlKeyFilterPassKey;
	
	
	// make sure that any arrow or delete key press is allowed
	if ((*inKeyCode != 0x3B/* left arrow */) &&
		(*inKeyCode != 0x7B/* left arrow */) &&
		(*inKeyCode != 0x3E/* up arrow */) &&
		(*inKeyCode != 0x7E/* up arrow */) &&
		(*inKeyCode != 0x3C/* right arrow */) &&
		(*inKeyCode != 0x7C/* right arrow */) &&
		(*inKeyCode != 0x3D/* down arrow */) &&
		(*inKeyCode != 0x7D/* down arrow */) &&
		(*inKeyCode != 0x33/* delete */) &&
		(*inKeyCode != 0x75/* del */))
	{
		// the rules of RFC-952 are followed here, as well as the
		// extra provision in RFC-1123 that the first character
		// may be a letter or a number, and the addition of IPv6
		// where a colon may be the delimiter of an IP address
		CFStringRef		textCFString = nullptr;
		
		
		GetControlTextAsCFString(inControl, textCFString);
		if (0 == CFStringGetLength(textCFString))
		{
			// first character; MUST be a letter or number
			unless (std::isdigit(*inCharCode) || std::isalpha(*inCharCode))
			{
				result = kControlKeyFilterBlockKey;
			}
		}
		else
		{
			// all other characters have more leniency
			unless (std::isdigit(*inCharCode) || std::isalpha(*inCharCode) || ('.' == *inCharCode) ||
					('-' == *inCharCode) || (':' == *inCharCode)/* IPv6 */)
			{
				result = kControlKeyFilterBlockKey;
			}
		}
	}
	
	return result;
}// HostNameLimiter


/*!
Returns a key filter UPP for HostNameLimiter().

(3.1)
*/
ControlKeyFilterUPP
HostNameLimiterKeyFilterUPP ()
{
	static ControlKeyFilterUPP		result = NewControlKeyFilterUPP(HostNameLimiter);
	
	
	return result;
}// HostNameLimiterKeyFilterUPP


/*!
This is a standard ControlKeyFilterUPP that will
prevent any characters invalid in an Internet host name
from being typed into a text field.

(3.1)
*/
pascal ControlKeyFilterResult
NoSpaceLimiter		(ControlRef			UNUSED_ARGUMENT(inControl),
					 SInt16*			inKeyCode,
					 SInt16*			inCharCode,
					 EventModifiers*	UNUSED_ARGUMENT(inModifiers))
{
	ControlKeyFilterResult  result = kControlKeyFilterPassKey;
	
	
	// make sure that any arrow or delete key press is allowed
	if ((*inKeyCode != 0x3B/* left arrow */) &&
		(*inKeyCode != 0x7B/* left arrow */) &&
		(*inKeyCode != 0x3E/* up arrow */) &&
		(*inKeyCode != 0x7E/* up arrow */) &&
		(*inKeyCode != 0x3C/* right arrow */) &&
		(*inKeyCode != 0x7C/* right arrow */) &&
		(*inKeyCode != 0x3D/* down arrow */) &&
		(*inKeyCode != 0x7D/* down arrow */) &&
		(*inKeyCode != 0x33/* delete */) &&
		(*inKeyCode != 0x75/* del */))
	{
		// whitespace prohibited, everything else is okay
		if (std::isspace(*inCharCode))
		{
			result = kControlKeyFilterBlockKey;
		}
	}
	
	return result;
}// NoSpaceLimiter


/*!
Returns a key filter UPP for NoSpaceLimiter().

(3.1)
*/
ControlKeyFilterUPP
NoSpaceLimiterKeyFilterUPP ()
{
	static ControlKeyFilterUPP		result = NewControlKeyFilterUPP(NoSpaceLimiter);
	
	
	return result;
}// NoSpaceLimiterKeyFilterUPP


/*!
This is a standard ControlKeyFilterUPP that will
prevent any non-digit characters from being typed
into a text field.

If there is a text selection in the specified
field, or if a special key (such as an arrow key
or the delete key) is pressed, this method will
not block it.

(3.0)
*/
pascal ControlKeyFilterResult
NumericalLimiter		(ControlRef			UNUSED_ARGUMENT(inControl),
						 SInt16*			inKeyCode,
						 SInt16*			inCharCode,
						 EventModifiers*	inModifiers)
{
	ControlKeyFilterResult  result = kControlKeyFilterPassKey;
	
	
	if (!(*inModifiers & cmdKey))
	{
		// make sure that any arrow or delete key press is allowed
		if ((*inKeyCode != 0x3B/* left arrow */) &&
			(*inKeyCode != 0x7B/* left arrow */) &&
			(*inKeyCode != 0x3E/* up arrow */) &&
			(*inKeyCode != 0x7E/* up arrow */) &&
			(*inKeyCode != 0x3C/* right arrow */) &&
			(*inKeyCode != 0x7C/* right arrow */) &&
			(*inKeyCode != 0x3D/* down arrow */) &&
			(*inKeyCode != 0x7D/* down arrow */) &&
			(*inKeyCode != 0x33/* delete */) &&
			(*inKeyCode != 0x75/* del */))
		{
			// if a character key was pressed, block it when there are four or more characters in the field already
			unless (CPP_STD::isdigit(*inCharCode)) result = kControlKeyFilterBlockKey;
		}
	}
	else
	{
		// command key is down - scan for a Paste operation
		if ((*inCharCode == 'V') || (*inCharCode == 'v'))
		{
			// pasting isn’t allowed because there is no easy way to filter out non-digits
			result = kControlKeyFilterBlockKey;
		}
	}
	return result;
}// NumericalLimiter


/*!
Returns a key filter UPP for NumericalLimiter().

(3.1)
*/
ControlKeyFilterUPP
NumericalLimiterKeyFilterUPP ()
{
	static ControlKeyFilterUPP		result = NewControlKeyFilterUPP(NumericalLimiter);
	
	
	return result;
}// NumericalLimiterKeyFilterUPP


/*!
This is a standard ControlKeyFilterUPP that will
prevent more than 4 characters from being typed
into an OSType text field.

If there is a text selection in the specified
field, or if a special key (such as an arrow key
or the delete key) is pressed, this method will
not block it.

(3.0)
*/
pascal ControlKeyFilterResult
OSTypeLengthLimiter		(ControlRef			inControl,
						 SInt16*			inKeyCode,
						 SInt16*			inCharCode,
						 EventModifiers*	inModifiers)
{
	Size							actualSize = 0L;
	ControlKeyFilterResult			result = kControlKeyFilterPassKey;
	ControlEditTextSelectionRec		selectionRangeInfo;
	
	
	// determine if text is selected - if so, don’t block any keys
	selectionRangeInfo.selStart = selectionRangeInfo.selEnd = 0;
	(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlEditTextSelectionTag,
								sizeof(selectionRangeInfo), &selectionRangeInfo, &actualSize);
	if (selectionRangeInfo.selStart == selectionRangeInfo.selEnd)
	{
		Str255		text;
		
		
		// determine the text currently in the control - if it is 4 characters long, block the key
		GetControlText(inControl, text);
		
		if (!(*inModifiers & cmdKey))
		{
			// make sure that any arrow or delete key press is allowed
			if ((*inKeyCode != 0x3B/* left arrow */) &&
				(*inKeyCode != 0x7B/* left arrow */) &&
				(*inKeyCode != 0x3E/* up arrow */) &&
				(*inKeyCode != 0x7E/* up arrow */) &&
				(*inKeyCode != 0x3C/* right arrow */) &&
				(*inKeyCode != 0x7C/* right arrow */) &&
				(*inKeyCode != 0x3D/* down arrow */) &&
				(*inKeyCode != 0x7D/* down arrow */) &&
				(*inKeyCode != 0x33/* delete */) &&
				(*inKeyCode != 0x75/* del */))
			{
				// if a character key was pressed, block it when there are four or more characters in the field already
				if (PLstrlen(text) >= 4) result = kControlKeyFilterBlockKey;
			}
		}
		else
		{
			// command key is down - scan for a Paste operation
			if ((*inCharCode == 'V') || (*inCharCode == 'v'))
			{
				// check the size of the TextEdit scrap
				if ((TEGetScrapLength() + PLstrlen(text) * sizeof(UInt8)) > (4 * sizeof(UInt8)))
				{
					// pasting isn’t allowed because adding the scrap would make the field too big
					result = kControlKeyFilterBlockKey;
				}
			}
		}
	}
	return result;
}// OSTypeLengthLimiter


/*!
Returns a key filter UPP for OSTypeLengthLimiter().

(3.1)
*/
ControlKeyFilterUPP
OSTypeLengthLimiterKeyFilterUPP ()
{
	static ControlKeyFilterUPP		result = NewControlKeyFilterUPP(OSTypeLengthLimiter);
	
	
	return result;
}// OSTypeLengthLimiterKeyFilterUPP


/*!
Removes the interior of a region so that
it ends up describing only its outline.

(2.6)
*/
void
OutlineRegion	(RgnHandle		inoutRegion)
{
	RgnHandle	tempRgn = Memory_NewRegion();
	
	
	if (tempRgn != nullptr)
	{
		CopyRgn(inoutRegion, tempRgn);
		InsetRgn(tempRgn, 1, 1);
		DiffRgn(inoutRegion, tempRgn, inoutRegion);
		Memory_DisposeRegion(&tempRgn);
	}
}// OutlineRegion


/*!
To activate the root control of the frontmost
window and to change the cursor to the standard
arrow, call this method.  Usually, you do this
immediately after removing a dialog box from the
screen, especially if you previously used the
DeactivateFrontmostWindow() method.

(3.0)
*/
void
RestoreFrontmostWindow ()
{
	Embedding_RestoreFrontmostWindow();
}// RestoreFrontmostWindow


/*!
To fill in a text field or static text control
with a number, use this convenient method.
The specified signed number is first converted
to a string, and then the resultant string is
passed to SetControlText().

Use DrawOneControl() to update the control to
reflect the change.

(3.0)
*/
void
SetControlNumericalText		(ControlRef		inControl,
							 SInt32			inNumber)
{
	Str255		string;
	
	
	NumToString(inNumber, string);
	SetControlText(inControl, string);
}// SetControlNumericalText


/*!
To set a static text or editable text control’s
text contents to represent the specified four-
character code, use this method.

Use DrawOneControl() to update the control to
reflect the change.

(3.0)
*/
void
SetControlOSTypeText	(ControlRef		inControl,
						 OSType			inType)
{
	Str15		typeString;
	
	
	PLstrcpy(typeString, EMPTY_PSTRING);
	typeString[0] = 4;
	BlockMoveData(&inType, &typeString[1], sizeof(OSType));
	(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextTextTag,
								PLstrlen(typeString) * sizeof(UInt8),
								typeString + 1/* length byte is skipped */);
}// SetControlOSTypeText


/*!
This method uses the SetControlData() API to
change the text of an editable text control.

Use DrawOneControl() to update the control
to reflect the change.

(3.0)
*/
void
SetControlText		(ControlRef			inControl,
					 ConstStringPtr		inText)
{
	Ptr		stringBuffer = nullptr;
	
	
	// create a disposable copy of the given text
	stringBuffer = Memory_NewPtr(PLstrlen(inText) * sizeof(UInt8)); // we skip the length byte!
	BlockMoveData(inText + 1, stringBuffer, GetPtrSize(stringBuffer)); // buffer has no length byte
	
	// set the control text to be the copy that was just created
	(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextTextTag,
								GetPtrSize(stringBuffer), stringBuffer);
	
	// destroy the copy
	Memory_DisposePtr(&stringBuffer);
}// SetControlText


/*!
This method uses the SetControlData() API to change
the text of an editable text control using the
contents of a Core Foundation string.

If you pass a nullptr string, the Toolbox will not
accept this, so this routine converts it to an
empty string first.

Use DrawOneControl() to update the control to reflect
the change.

(3.0)
*/
void
SetControlTextWithCFString		(ControlRef		inControl,
								 CFStringRef	inText)
{
	if (inText != nullptr)
	{
		// set the control text
		(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextCFStringTag,
									sizeof(inText), &inText);
	}
	else
	{
		CFStringRef		emptyString = CFSTR("");
		
		
		// set the control text to an empty string
		(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextCFStringTag,
									sizeof(emptyString), &emptyString);
	}
}// SetControlTextWithCFString


/*!
Sets the current keyboard focus to a specific view.

(3.1)
*/
OSStatus
DialogUtilities_SetKeyboardFocus	(HIViewRef		inView)
{
	OSStatus	result = noErr;
	
	
	result = SetKeyboardFocus(HIViewGetWindow(inView), inView, kControlFocusNoPart);
	if (noErr == result)
	{
		result = SetKeyboardFocus(HIViewGetWindow(inView), inView, kControlFocusNextPart);
	}
	return result;
}// SetKeyboardFocus


/*!
Finds the menu attached to the specified view, determines
the first item in it matching the given command ID, and
updates the control value to match.

(3.1)
*/
OSStatus
DialogUtilities_SetPopUpItemByCommand	(HIViewRef		inPopUpMenuView,
										 UInt32			inCommandID)
{
	OSStatus	result = errUnknownControl;
	MenuRef		menu = GetControlPopupMenuRef(inPopUpMenuView);
	
	
	if (nullptr != menu)
	{
		MenuItemIndex	itemIndex = 0;
		
		
		result = GetIndMenuItemWithCommandID(menu, inCommandID, 1/* which match to return */,
												&menu, &itemIndex);
		if (noErr == result)
		{
			SetControl32BitValue(inPopUpMenuView, itemIndex);
			result = noErr;
		}
	}
	return result;
}// SetPopUpItemByCommand


/*!
Finds the menu attached to the specified view, determines
the first item in it matching the given text, and updates
the control value to match.

\retval noErr
if the value was set successfully

\retval errUnknownControl
if the view is invalid or it has no menu

\retval controlPropertyNotFoundErr
if the specified text was not found in any menu item, and
the fallback selection was (successfully) applied instead

(4.0)
*/
OSStatus
DialogUtilities_SetPopUpItemByText	(HIViewRef		inPopUpMenuView,
									 CFStringRef	inText,
									 MenuItemIndex	inFallbackSelection)
{
	OSStatus	result = errUnknownControl;
	MenuRef		menu = GetControlPopupMenuRef(inPopUpMenuView);
	
	
	if (nullptr != menu)
	{
		MenuItemIndex	itemIndex = MenuBar_ReturnMenuItemIndexByItemText(menu, inText);
		
		
		if (0 == itemIndex)
		{
			SetControl32BitValue(inPopUpMenuView, inFallbackSelection);
			result = controlPropertyNotFoundErr;
		}
		else
		{
			SetControl32BitValue(inPopUpMenuView, itemIndex);
			result = noErr;
		}
	}
	return result;
}// SetPopUpItemByText


/*!
Call this routine on the Help button control of every window.
This routine checks to see if all necessary components for
contextual help are available (disabling the button if help
is not available), and changes the icon of the button to be a
high-quality icon if possible.  On pre-Panther systems, it
also enforces the size of the help button so that the icon is
properly centered, as 10.3-created NIB files shrink the size
of the help button slightly.  Finally, on Tiger and beyond, an
appropriate accessibility description is given to the button.

(3.1)
*/
HIViewWrap&
DialogUtilities_SetUpHelpButton		(HIViewWrap&	inoutView)
{
	useBestHelpButtonIcon(inoutView);
	unless (FlagManager_Test(kFlagOS10_3API))
	{
		// on Panther, Interface Builder will create a nice
		// “purple help button”, but it is sized badly for
		// older Mac OS X systems; so on these older systems
		// this routine fixes the button size so the icon is
		// nicely centered
		SizeControl(inoutView, 20, 20);
	}
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	if (FlagManager_Test(kFlagOS10_4API))
	{
		// set accessibility title
		CFStringRef		accessibilityDescCFString = nullptr;
		
		
		if (UIStrings_Copy(kUIStrings_ButtonHelpAccessibilityDesc, accessibilityDescCFString).ok())
		{
			HIViewRef const		kViewRef = inoutView;
			HIObjectRef const	kViewObjectRef = REINTERPRET_CAST(kViewRef, HIObjectRef);
			OSStatus			error = noErr;
			
			
			error = HIObjectSetAuxiliaryAccessibilityAttribute
					(kViewObjectRef, 0/* sub-component identifier */,
						kAXDescriptionAttribute, accessibilityDescCFString);
			CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
		}
	}
#endif
	return inoutView;
}// SetUpHelpButton


/*!
Specifies the Window menu (and Dock menu) text
for a window to be something other than its
title.  Use this for windows that have no title,
or windows whose titles are inappropriate when
listed in a menu.

This method saves you from having to construct
a CFString manually when interfacing with the
Mac OS.  Carbon only.

(3.0)
*/
OSStatus
SetWindowAlternateTitleWithPString	(WindowRef			inWindow,
									 ConstStringPtr		inText)
{
	OSStatus		result = noErr;
	CFStringRef		alternateWindowTitle = nullptr;
	
	
	alternateWindowTitle = CFStringCreateWithPascalString(kCFAllocatorDefault, inText,
															CFStringGetSystemEncoding());
	if (alternateWindowTitle == nullptr) result = memFullErr;
	else
	{
		result = SetWindowAlternateTitle(inWindow, alternateWindowTitle);
		CFRelease(alternateWindowTitle);
	}
	return result;
}// SetWindowAlternateTitleWithPString


/*!
To change the font of the current graphics port
to be the font with the specified name, use this
method.

This routine is present for assisting Carbon
compatibility: Apple does not recommend using
GetFNum(), so the fewer places font IDs are used,
the better.

(3.0)
*/
void
TextFontByName		(ConstStringPtr		inFontName)
{
	SInt16	fontID = FMGetFontFamilyFromName(inFontName);
	
	
	if (fontID != kInvalidFontFamily)
	{
		TextFont(fontID);
	}
}// TextFontByName


/*!
This is a standard ControlKeyFilterUPP that will
prevent any characters invalid in a Unix command line
from being typed into a text field.

(3.1)
*/
pascal ControlKeyFilterResult
UnixCommandLineLimiter	(ControlRef			UNUSED_ARGUMENT(inControl),
						 SInt16*			inKeyCode,
						 SInt16*			UNUSED_ARGUMENT(inCharCode),
						 EventModifiers*	UNUSED_ARGUMENT(inModifiers))
{
	ControlKeyFilterResult  result = kControlKeyFilterPassKey;
	
	
	// make sure that any arrow or delete key press is allowed
	if ((*inKeyCode != 0x3B/* left arrow */) &&
		(*inKeyCode != 0x7B/* left arrow */) &&
		(*inKeyCode != 0x3E/* up arrow */) &&
		(*inKeyCode != 0x7E/* up arrow */) &&
		(*inKeyCode != 0x3C/* right arrow */) &&
		(*inKeyCode != 0x7C/* right arrow */) &&
		(*inKeyCode != 0x3D/* down arrow */) &&
		(*inKeyCode != 0x7D/* down arrow */) &&
		(*inKeyCode != 0x33/* delete */) &&
		(*inKeyCode != 0x75/* del */))
	{
		; // UNIMPLEMENTED
	}
	
	return result;
}// UnixCommandLineLimiter


/*!
Returns a key filter UPP for UnixCommandLineLimiter().

(3.1)
*/
ControlKeyFilterUPP
UnixCommandLineLimiterKeyFilterUPP ()
{
	static ControlKeyFilterUPP		result = NewControlKeyFilterUPP(UnixCommandLineLimiter);
	
	
	return result;
}// UnixCommandLineLimiterKeyFilterUPP


#pragma mark Internal Methods
namespace {

/*!
The “real” DeviceLoopDrawingUPP that is called by
the OS from DeviceLoopClipAndDrawControl().  This
stand-in routine performs type-casting correctly
and re-invokes a user-defined C function that then
does not have to worry about type-casting errors.

(3.0)
*/
pascal void
clipAndDrawControlDeviceLoop	(short		inColorDepth,
								 short		inDeviceFlags,
								 GDHandle	inTargetDevice,
								 long		inControlInfoConstPtr)
{
	My_DeviceLoopControlInfoConstPtr	controlInfoPtr = REINTERPRET_CAST(inControlInfoConstPtr,
																			My_DeviceLoopControlInfoConstPtr);
	
	
	DialogUtilities_InvokeDrawViewProc(controlInfoPtr->proc, inColorDepth, inDeviceFlags, inTargetDevice,
										controlInfoPtr->control, controlInfoPtr->controlPartCode);
}// clipAndDrawControlDeviceLoop


/*!
To automatically use the best (32-bit) Help button icon
available for the specified Help bevel button control,
use this method.  If Icon Services is not available, a
system icon resource is automatically used.

(3.0)
*/
void
useBestHelpButtonIcon	(ControlRef		inControl)
{
	IconManagerIconRef		icon = IconManager_NewIcon();
	
	
	IconManager_MakeIconSuite(icon, kHelpIconResource, kSelectorAllSmallData);
	(OSStatus)IconManager_SetButtonIcon(inControl, icon);
	IconManager_DisposeIcon(&icon);
}// useBestHelpButtonIcon

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
