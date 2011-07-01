/*###############################################################

	TerminalFile.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <Console.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <TerminalFile.h>

// application includes
#include "TerminalView.h"



#pragma mark Constants

#define kWindowSettingsKey				CFSTR("WindowSettings")					// 10.1, 10.2, 10.3

#define kAutoFocusKey					CFSTR("AutoFocus")						// 10.1, 10.2, 10.3
#define kAutowrapKey					CFSTR("Autowrap")						// 10.1, 10.2, 10.3
#define kBackgroundImagePathKey			CFSTR("BackgroundImagePath")			// 10.3
#define kBackwrapKey					CFSTR("Backwrap")						// 10.2, 10.3
#define kBellKey						CFSTR("Bell")							// 10.1, 10.2, 10.3
#define kBlinkCursorKey					CFSTR("BlinkCursor")					// 10.1, 10.2, 10.3
#define kBlinkTextKey					CFSTR("BlinkText")						// 10.3
#define kCleanCommandsKey				CFSTR("CleanCommands")					// 10.2, 10.3
#define kColumnsKey						CFSTR("Columns")						// 10.1, 10.2, 10.3
#define kCursorShapeKey					CFSTR("CursorShape")					// 10.1, 10.2, 10.3
#define kCustomTitleKey					CFSTR("CustomTitle")					// 10.1, 10.2, 10.3
#define kDeleteKeySendsBackspaceKey		CFSTR("DeleteKeySendsBackspace")		// 10.2, 10.3
#define kDisableAnsiColorsKey			CFSTR("DisableAnsiColors")				// 10.1, 10.2, 10.3
#define kDoubleBoldKey					CFSTR("DoubleBold")						// 10.1, 10.2, 10.3
#define kDoubleColumnsForDoubleWideKey	CFSTR("DoubleColumnsForDoubleWide")		// 10.2, 10.3
#define kDoubleWideCharsKey				CFSTR("DoubleWideChars")				// 10.2, 10.3
#define kEnableDragCopyKey				CFSTR("EnableDragCopy")					// 10.3
#define kExecutionStringKey				CFSTR("ExecutionString")				// 10.1, 10.2, 10.3
#define kFontAntialiasingKey			CFSTR("FontAntialiasing")				// 10.2, 10.3
#define kFontHeightSpacingKey			CFSTR("FontHeightSpacing")				// 10.1, 10.2, 10.3
#define kFontWidthSpacingKey			CFSTR("FontWidthSpacing")				// 10.1, 10.2, 10.3
#define kIsMiniaturizedKey				CFSTR("IsMiniaturized")					// 10.1, 10.2, 10.3
#define kKeyBindingsKey					CFSTR("KeyBindings")					// 10.3
#define kKeypadKey						CFSTR("Keypad")							// 10.1
#define kMacTermFunctionKeysKey			CFSTR("MacTermFunctionKeys")			// 10.2
#define kMetaKey						CFSTR("Meta")							// 10.1, 10.2, 10.3
#define kNSFixedPitchFontKey			CFSTR("NSFixedPitchFont")				// 10.1, 10.2, 10.3
#define kNSFixedPitchFontSizeKey		CFSTR("NSFixedPitchFontSize")			// 10.1, 10.2, 10.3
#define kOptionClickToMoveCursorKey		CFSTR("OptionClickToMoveCursor")		// 10.3
#define kPadBottomKey					CFSTR("PadBottom")						// 10.3
#define kPadLeftKey						CFSTR("PadLeft")						// 10.3
#define kPadRightKey					CFSTR("PadRight")						// 10.3
#define kPadTopKey						CFSTR("PadTop")							// 10.3
#define kRewrapOnResizeKey				CFSTR("RewrapOnResize")					// 10.3
#define kRowsKey						CFSTR("Rows")							// 10.1, 10.2, 10.3
#define kSaveLinesKey					CFSTR("SaveLines")						// 10.1, 10.2, 10.3
#define kScrollRegionCompatKey			CFSTR("ScrollRegionCompat")				// 10.2, 10.3
#define kScrollRowsKey					CFSTR("ScrollRows")						// 10.2, 10.3
#define kScrollbackKey					CFSTR("Scrollback")						// 10.1, 10.2, 10.3
#define kScrollbarKey					CFSTR("Scrollbar")						// 10.3
#define kShellKey						CFSTR("Shell")							// 10.1, 10.2, 10.3
#define kShellExitActionKey				CFSTR("ShellExitAction")				// 10.1, 10.2, 10.3
#define kSourceDotLoginKey				CFSTR("SourceDotLogin")					// 10.1
#define kStrictEmulationKey				CFSTR("StrictEmulation")				// 10.1, 10.2, 10.3
#define kStringEncodingKey				CFSTR("StringEncoding")					// 10.2, 10.3
#define kTermCapStringKey				CFSTR("TermCapString")					// 10.3
#define kTerminalOpaquenessKey			CFSTR("TerminalOpaqueness")				// 10.1, 10.2, 10.3
#define kTextColorsKey					CFSTR("TextColors")						// 10.1, 10.2, 10.3
#define kTitleBitsKey					CFSTR("TitleBits")						// 10.1, 10.2, 10.3
#define kTranslateKey					CFSTR("Translate")						// 10.1, 10.2, 10.3
#define kUseCtrlVEscapesKey				CFSTR("UseCtrlVEscapes")				// 10.2, 10.3
#define kVisualBellKey					CFSTR("VisualBell")						// 10.2, 10.3
#define kWinLocULYKey					CFSTR("WinLocULY")						// 10.1, 10.2, 10.3
#define kWinLocXKey						CFSTR("WinLocX")						// 10.1, 10.2, 10.3
#define kWinLocYKey						CFSTR("WinLocY")						// 10.1, 10.2, 10.3
#define kWindowCloseActionKey			CFSTR("WindowCloseAction")				// 10.2, 10.3

#pragma mark Types

struct My_TerminalFile
{
	CFPropertyListRef	propertyList;
};
typedef My_TerminalFile const*	My_TerminalFileConstPtr;
typedef My_TerminalFile*		My_TerminalFilePtr;

typedef MemoryBlockPtrLocker< TerminalFileRef, My_TerminalFile >	My_TerminalFilePtrLocker;
typedef LockAcquireRelease< TerminalFileRef, My_TerminalFile >		My_TerminalFileAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_TerminalFilePtrLocker&	gTerminalFilePtrLocks ()	{ static My_TerminalFilePtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static TerminalFile_Result	getBooleanValue			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getColorArrayValues		(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getDictionaryValue		(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getFloat32Value			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getFloat64Value			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getSInt16Value			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getSInt32Value			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getStringArrayValues	(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getStringValue			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getUInt16Value			(CFDictionaryRef, CFStringRef, void*);
static TerminalFile_Result	getUInt32Value			(CFDictionaryRef, CFStringRef, void*);
static CFStringRef			rgbCopyDescription		(void const*);
static Boolean				rgbEqual				(void const*, void const*);



#pragma mark Public Methods

/*!
This method loads a property list file and stores it in a 
private structure.

(3.0)
*/
OSStatus
TerminalFile_NewFromFile	(CFURLRef			inFileURL,
							 TerminalFileRef*	outTermFile)
{
	OSStatus	result = noErr;
	
	
	if (outTermFile == nullptr) result = memPCErr;
	else
	{
		CFDataRef	fileData = nullptr;
		SInt32		errorCode = 0;
		
		
		*outTermFile = nullptr;
		
		if (!CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, inFileURL, &fileData, 
														nullptr, nullptr, &errorCode))
		{
			// NOTE: Technically, the error code returned here is not an OSStatus.
			//       If negative, it is an Apple error, and if positive it is scheme-specific.
			result = errorCode;
		}
		else
		{
			*outTermFile = REINTERPRET_CAST(CFAllocatorAllocate(kCFAllocatorDefault, sizeof(My_TerminalFile), 
																0/* hint flags */), TerminalFileRef);
			
			if (*outTermFile == nullptr)
			{
				result = memFullErr;
			}
			else
			{
				My_TerminalFileAutoLocker	ptr(gTerminalFilePtrLocks(), *outTermFile);
				CFStringRef					errorString = nullptr;
				
				
				ptr->propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, fileData,
																	kCFPropertyListImmutable, &errorString);
				
				if (errorString != nullptr)
				{
					CFRelease(errorString), errorString = nullptr;
					result = kTerminalFile_ResultInvalidFileType;
				}
			}
		}
		
		if (fileData != nullptr)
		{
			CFRelease(fileData), fileData = nullptr;
		}
	}
	
	return result;
}// NewFromFile


/*!
This method releases all of the memory used by the private 
terminal file structure.

(3.0)
*/
void
TerminalFile_Dispose	(TerminalFileRef*	inoutTermFilePtr)
{
	if (gTerminalFilePtrLocks().isLocked(*inoutTermFilePtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked terminal file; outstanding locks",
						gTerminalFilePtrLocks().returnLockCount(*inoutTermFilePtr));
	}
	else
	{
		{
			My_TerminalFileAutoLocker	ptr(gTerminalFilePtrLocks(), *inoutTermFilePtr);
			
			
			// dispose of all members
			CFRelease(ptr->propertyList), ptr->propertyList = nullptr;
		}
		
		// now dispose of the structure itself
		CFAllocatorDeallocate(kCFAllocatorDefault, *inoutTermFilePtr), *inoutTermFilePtr = nullptr;
	}
}// Dispose


/*!
This method gets the data for the attribute tags of the settings 
type at the provided index.  Behavior is undefined if inTermFile 
is not a valid terminal file reference, if inSettingsType is not 
a valid settings type or not contained in inTermFile, if 
inSettingsIndex is out of range, if any tag in inTagArray is not 
a valid tag, if any value pointer in outValueArray is nullptr (the 
pointer’s value can be nullptr but not the pointer itself), or if 
inAttributeCount is out of range.

(3.0)
*/
TerminalFile_Result
TerminalFile_GetAttributes	(TerminalFileRef					inTermFile,
							 TerminalFile_SettingsType			inSettingsType,
							 CFIndex							inSettingsIndex,
							 ItemCount							inAttributeCount,
							 TerminalFile_AttributeTag const	inTagArray[],
							 TerminalFile_AttributeValuePtr		outValueArray[])
{
	My_TerminalFileAutoLocker	ptr(gTerminalFilePtrLocks(), inTermFile);
	TerminalFile_Result			result = kTerminalFile_ResultOK;
	CFRetainRelease				mainDictionary(ptr->propertyList);
	CFDictionaryRef				windowSettings = nullptr;
	CFArrayRef					windowSettingsArray = nullptr;
	
	
	switch (inSettingsType)
	{
	case kTerminalFile_SettingsTypeWindow:
		{
			ItemCount		currentItem = 0;
			
			
			windowSettingsArray = CFUtilities_ArrayCast
									(CFDictionaryGetValue(mainDictionary.returnCFDictionaryRef(), kWindowSettingsKey));
			windowSettings = CFUtilities_DictionaryCast(CFArrayGetValueAtIndex(windowSettingsArray, inSettingsIndex));
			
			// Right now we are reading each tag in inTags one at a time and 
			// then essentially doing a linear search through the list of possible 
			// tags.  That is certainly not the most efficient method and definitely 
			// something we will want to improve on in a future version.
			for (currentItem = 0; ((currentItem < inAttributeCount) && (result == kTerminalFile_ResultOK));
					++currentItem)
			{
				TerminalFile_AttributeValuePtr&		value = outValueArray[currentItem];
				UInt16*								cursorShape = nullptr;
				CFStringEncoding					encoding = kCFStringEncodingMacRoman;
				short								bottom = 0,
													vertical = 0;
				
				
				switch (inTagArray[currentItem])
				{
				case kTerminalFile_AttributeTagWindowScrollOnInput:
					result = getBooleanValue(windowSettings, kAutoFocusKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowAutowrap:
					result = getBooleanValue(windowSettings, kAutowrapKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowBackgroundImage:
					result = getStringValue(windowSettings, kBackgroundImagePathKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowBackwrap:
					result = getBooleanValue(windowSettings, kBackwrapKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowAudibleBell:
					result = getBooleanValue(windowSettings, kBellKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowBlinkCursor:
					result = getBooleanValue(windowSettings, kBlinkCursorKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowBlinkingText:
					result = getBooleanValue(windowSettings, kBlinkTextKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowCleanCommandList:
					result = getStringArrayValues(windowSettings, kCleanCommandsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowColumnCount:
					result = getUInt32Value(windowSettings, kColumnsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowCursorShape:
					result = getUInt16Value(windowSettings, kCursorShapeKey, value);
					cursorShape = (UInt16*)value;
					switch (*cursorShape)
					{
					case 0:
						*cursorShape = kTerminalView_CursorTypeBlock;
						break;
					
					case 1:
						*cursorShape = kTerminalView_CursorTypeUnderscore;
						break;
					
					case 2:
						*cursorShape = kTerminalView_CursorTypeVerticalLine;
						break;
					
					default:
						*cursorShape = kTerminalView_CursorTypeBlock;
						break;
					}
					break;
				
				case kTerminalFile_AttributeTagWindowCustomTitle:
					result = getStringValue(windowSettings, kCustomTitleKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowDeleteSendsBS:
					result = getBooleanValue(windowSettings, kDeleteKeySendsBackspaceKey, 
												value);
					break;
				
				case kTerminalFile_AttributeTagWindowDisableColors:
					result = getBooleanValue(windowSettings, kDisableAnsiColorsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowDoubleBold:
					result = getBooleanValue(windowSettings, kDoubleBoldKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowDColumnsDWide:
					result = getBooleanValue(windowSettings, 
												kDoubleColumnsForDoubleWideKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowDoubleWideChars:
					result = getBooleanValue(windowSettings, kDoubleWideCharsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowDragCopy:
					result = getBooleanValue(windowSettings, kEnableDragCopyKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowExecutionString:
					result = getStringValue(windowSettings, kExecutionStringKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowFontAntialiased:
					result = getBooleanValue(windowSettings, kFontAntialiasingKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowFontSpacingV:
					result = getFloat32Value(windowSettings, kFontHeightSpacingKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowFontSpacingH:
					result = getFloat32Value(windowSettings, kFontWidthSpacingKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowIsMinimized:
					result = getBooleanValue(windowSettings, kIsMiniaturizedKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowKeyMappings:
					result = getDictionaryValue(windowSettings, kKeyBindingsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowKeypad:
					result = getBooleanValue(windowSettings, kKeypadKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowMacTermFunctionKeys:
					result = getBooleanValue(windowSettings, kMacTermFunctionKeysKey, 
												value);
					break;
				
				case kTerminalFile_AttributeTagWindowMetaKeyMapping:
					result = getSInt32Value(windowSettings, kMetaKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowFontName:
					result = getStringValue(windowSettings, kNSFixedPitchFontKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowFontSize:
					result = getFloat32Value(windowSettings, kNSFixedPitchFontSizeKey, 
												value);
					break;
				
				case kTerminalFile_AttributeTagWindowOptClickMoveCursor:
					result = getBooleanValue(windowSettings, kOptionClickToMoveCursorKey, 
												value);
					break;
				
				case kTerminalFile_AttributeTagWindowPadding:
					result = getSInt16Value(windowSettings, kPadBottomKey, &bottom);
					if (result == kTerminalFile_ResultOK)
					{
						short	left;
						
						
						result = getSInt16Value(windowSettings, kPadLeftKey, &left);
						if (result == kTerminalFile_ResultOK)
						{
							short	right;
							
							
							result = getSInt16Value(windowSettings, kPadRightKey, &right);
							if (result == kTerminalFile_ResultOK)
							{
								short	top;
								
								
								result = getSInt16Value(windowSettings, kPadTopKey, &top);
								if (result == kTerminalFile_ResultOK)
								{
									SetRect((Rect*)value, left, top, right, bottom);
								}
							}
						}
					}
					break;
				
				case kTerminalFile_AttributeTagWindowRewrapOnResize:
					result = getBooleanValue(windowSettings, kRewrapOnResizeKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowRowCount:
					result = getUInt32Value(windowSettings, kRowsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowSaveLines:
					result = getSInt32Value(windowSettings, kSaveLinesKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowScrollRgnCompatible:
					result = getBooleanValue(windowSettings, kScrollRegionCompatKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowScrollbackRows:
					result = getUInt32Value(windowSettings, kScrollRowsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowScrollbackEnabled:
					result = getBooleanValue(windowSettings, kScrollbackKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowScrollbar:
					result = getBooleanValue(windowSettings, kScrollbarKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowUserShell:
					result = getStringValue(windowSettings, kShellKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowShellExitAction:
					result = getUInt32Value(windowSettings, kShellExitActionKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowSourceDotLogin:
					result = getBooleanValue(windowSettings, kSourceDotLoginKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowStrictEmulation:
					result = getBooleanValue(windowSettings, kStrictEmulationKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowTextEncoding:
					result = getUInt32Value(windowSettings, kStringEncodingKey, value);
					encoding = CFStringConvertNSStringEncodingToEncoding(*(UInt32*)value);
					*(CFStringEncoding*)value = encoding;
					break;
				
				case kTerminalFile_AttributeTagWindowTerminalType:
					result = getStringValue(windowSettings, kTermCapStringKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowOpaqueness:
					result = getFloat64Value(windowSettings, kTerminalOpaquenessKey, value);
					
					break;
				
				case kTerminalFile_AttributeTagWindowTextColors:
					result = getColorArrayValues(windowSettings, kTextColorsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowTitleBits:
					result = getUInt32Value(windowSettings, kTitleBitsKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowTranslateOnPaste:
					result = getBooleanValue(windowSettings, kTranslateKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowEscape8BitCharsWithCtrlV:
					result = getBooleanValue(windowSettings, kUseCtrlVEscapesKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowVisualBell:
					result = getBooleanValue(windowSettings, kVisualBellKey, value);
					break;
			
				case kTerminalFile_AttributeTagWindowLocation:
					result = getSInt16Value(windowSettings, kWinLocULYKey, &vertical);
					if (result == kTerminalFile_ResultOK)
					{
						short	horizontal;
						
						
						result = getSInt16Value(windowSettings, kWinLocXKey, &horizontal);
						if (result == kTerminalFile_ResultOK)
						{
							RgnHandle	deskRegion;
							Rect		bounds;
							
							
							deskRegion = NewRgn();
							DMGetDeskRegion(&deskRegion);
							
							GetRegionBounds(deskRegion, &bounds);
							DisposeRgn(deskRegion);
							
							SetPt((Point*)value, horizontal, bounds.bottom - vertical);
						}
					}
					break;
				
				case kTerminalFile_AttributeTagWindowLocationY:
					result = getSInt16Value(windowSettings, kWinLocYKey, value);
					break;
				
				case kTerminalFile_AttributeTagWindowCloseAction:
					result = getUInt32Value(windowSettings, kWindowCloseActionKey, value);
					break;
				
				default:
					// ???
					break;
				}
			}
		}
		break;
	}
	
	return result;
}// GetAttributes


/*!
This method gets the count of the settings for the specified
settings type.  For example, if you have two sets of window
settings in your terminal file, this function will return 2.
If the settings type isn’t in the file at all or the given
settings type is invalid, 0 is returned.

(3.0)
*/
CFIndex
TerminalFile_ReturnSettingsCount	(TerminalFileRef			inTermFile,
									 TerminalFile_SettingsType	inSettingsType)
{
	My_TerminalFileAutoLocker	ptr(gTerminalFilePtrLocks(), inTermFile);
	CFDictionaryRef				mainDictionary = nullptr;
	CFStringRef					settingsKey = nullptr;
	CFArrayRef					settingsArray = nullptr;
	CFIndex						result = 0;
	
	
	mainDictionary = CFUtilities_DictionaryCast(ptr->propertyList);
	CFRetain(mainDictionary);
	
	// get the appropriate settings key
	switch (inSettingsType)
	{
	case kTerminalFile_SettingsTypeWindow:
		settingsKey = kWindowSettingsKey;
		break;
	
	default:
		// ???
		break;
	}
	
	if (settingsKey != nullptr)
	{
		settingsArray = CFUtilities_ArrayCast(CFDictionaryGetValue(mainDictionary, settingsKey));
		if (settingsArray != nullptr)
		{
			result = CFArrayGetCount(settingsArray);
		}
	}
	
	CFRelease(mainDictionary), mainDictionary = nullptr;
	
	return result;
}// ReturnSettingsCount


/*!
This method gets the version of Terminal used to create the 
Terminal file.

(3.0)
*/
TerminalFile_Version
TerminalFile_ReturnVersion		(TerminalFileRef	inTermFile)
{
	My_TerminalFileAutoLocker	ptr(gTerminalFilePtrLocks(), inTermFile);
	CFDictionaryRef				mainDictionary = nullptr;
	CFDictionaryRef				windowSettings = nullptr;
	CFArrayRef					settingsArray = nullptr;
	TerminalFile_Version		result = kTerminalFile_VersionUnknown;
	
	
	mainDictionary = CFUtilities_DictionaryCast(ptr->propertyList);
	CFRetain(mainDictionary);
	
	settingsArray = CFUtilities_ArrayCast(CFDictionaryGetValue(mainDictionary, kWindowSettingsKey));
	windowSettings = CFUtilities_DictionaryCast(CFArrayGetValueAtIndex(settingsArray, 0));
	
	if (CFDictionaryContainsKey(windowSettings, kAutoFocusKey))
	{
		if (!CFDictionaryContainsKey(windowSettings, kBackwrapKey))
		{
			result = kTerminalFile_Version1_1;
		}
		else
		{
			if (!CFDictionaryContainsKey(windowSettings, kBackgroundImagePathKey))
			{
				result = kTerminalFile_Version1_3;
			}
			else
			{
				result = kTerminalFile_Version1_4;
			}
		}
	}
	
	CFRelease(mainDictionary), mainDictionary = nullptr;
	
	return result;
}// ReturnVersion


#pragma mark Internal Methods

/*!
Returns the value of the specified boolean key.  Results are 
undefined if the value for the given key is not a CFBoolean, 
CFSTR("YES") (case insensitive), or CFSTR("NO") (case 
insensitive).

The "outValue" is expected to really be of type "Boolean*".

(3.0)
*/
static TerminalFile_Result
getBooleanValue		(CFDictionaryRef	inDictionary,
					 CFStringRef		inKey,
					 void*				outValue)
						
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFBoolean or a CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFDictionaryGetValue(inDictionary, inKey);
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID			type;
		CFStringRef const	stringValue = CFUtilities_StringCast(value);
		Boolean* const		outBooleanValue = REINTERPRET_CAST(outValue, Boolean*);
		
		
		type = CFGetTypeID(value);
		
		if (type == CFBooleanGetTypeID())
		{
			*outBooleanValue = CFBooleanGetValue(CFUtilities_BooleanCast(value));
		}
		else if (type == CFStringGetTypeID())
		{
			if (CFStringCompare(stringValue, CFSTR("YES"), kCFCompareCaseInsensitive) == 
				kCFCompareEqualTo)
			{
				*outBooleanValue = true;
			}
			else if (CFStringCompare(stringValue, CFSTR("NO"), kCFCompareCaseInsensitive) == 
						kCFCompareEqualTo)
			{
				*outBooleanValue = false;
			}
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getBooleanValue


/*!
Returns the values of the specified RGB color array key.  We are 
assuming that the RGBColor array is contained in a CFString 
value consisting of 8 RGB triples expressed as 32-bit floating 
point numbers between 0.000 and 1.000 separated by spaces, if it 
is not then the results of this method are undefined.

The "outValue" is expected to really be of type "CFArrayRef*".

(3.0)
*/
static TerminalFile_Result
getColorArrayValues		(CFDictionaryRef	inDictionary,
						 CFStringRef		inKey,
						 void*				outValues)
{
	enum
	{
		kRGBColorCount = 8
	};
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFStringRef				values = nullptr;
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	values = CFUtilities_StringCast(CFDictionaryGetValue(inDictionary, CFUtilities_StringCast(inKey)));
	if (values == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFArrayRef		stringArray = nullptr;
		
		
		stringArray = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, values, CFSTR(" "));
		if (stringArray == nullptr)
		{
			result = kTerminalFile_ResultMemAllocErr;
		}
		else
		{
			RGBColor*			colorPtrArray[kRGBColorCount];
			CFIndex				colorIndex = 0;
			CFStringRef			redString = nullptr;
			CFStringRef			greenString = nullptr;
			CFStringRef			blueString = nullptr;
			Float64				calculation = 0.0; // ensure there is enough space to compute a fraction of RGBCOLOR_INTENSITY_MAX
			Float32				red = 0.0;
			Float32				green = 0.0;
			Float32				blue = 0.0;
			CFArrayCallBacks	rgbColorCallBacks;
			
			
			for (colorIndex = 0; colorIndex < kRGBColorCount; ++colorIndex)
			{
				colorPtrArray[colorIndex] = REINTERPRET_CAST(CFAllocatorAllocate(kCFAllocatorDefault, sizeof(RGBColor),
																					0/* hints */), RGBColor*);
				if (colorPtrArray[colorIndex] != nullptr)
				{
					redString = CFUtilities_StringCast(CFArrayGetValueAtIndex(stringArray, INTEGER_TRIPLED(colorIndex)));
					greenString = CFUtilities_StringCast(CFArrayGetValueAtIndex(stringArray, INTEGER_TRIPLED(colorIndex) + 1));
					blueString = CFUtilities_StringCast(CFArrayGetValueAtIndex(stringArray, INTEGER_TRIPLED(colorIndex) + 2));
					
					red = STATIC_CAST(CFStringGetDoubleValue(redString), Float32);
					green = STATIC_CAST(CFStringGetDoubleValue(greenString), Float32);
					blue = STATIC_CAST(CFStringGetDoubleValue(blueString), Float32);
					
					calculation = red * RGBCOLOR_INTENSITY_MAX;
					colorPtrArray[colorIndex]->red = STATIC_CAST(calculation, UInt16);
					calculation = green * RGBCOLOR_INTENSITY_MAX;
					colorPtrArray[colorIndex]->green = STATIC_CAST(calculation, UInt16);
					calculation = blue * RGBCOLOR_INTENSITY_MAX;
					colorPtrArray[colorIndex]->blue = STATIC_CAST(calculation, UInt16);
				}
				else
				{
					// flag errors, but still try to return as many colors as possible
					result = kTerminalFile_ResultMemAllocErr;
				}
			}
			
			bzero(&rgbColorCallBacks, sizeof(rgbColorCallBacks));
			rgbColorCallBacks.version = 0;
			rgbColorCallBacks.retain = nullptr;
			rgbColorCallBacks.release = nullptr;
			rgbColorCallBacks.copyDescription = rgbCopyDescription;
			rgbColorCallBacks.equal = rgbEqual;
			
			{
				// this copy seems necessary to avoid a GCC compiler warning
				RGBColor const*		colorConstArray[kRGBColorCount] =
									{
										colorPtrArray[0],
										colorPtrArray[1],
										colorPtrArray[2],
										colorPtrArray[3],
										colorPtrArray[4],
										colorPtrArray[5],
										colorPtrArray[6],
										colorPtrArray[7]
									};
				
				
				*(REINTERPRET_CAST(outValues, CFArrayRef*)) = CFArrayCreate(kCFAllocatorDefault, REINTERPRET_CAST(colorConstArray, void const**),
																			kRGBColorCount, &rgbColorCallBacks);
			}
			
			// free allocated colors; check for nullptr entries because this
			// could indicate a failure to allocate some, but not all, colors
			for (colorIndex = 0; colorIndex < kRGBColorCount; ++colorIndex)
			{
				if (colorPtrArray[colorIndex] != nullptr)
				{
					CFAllocatorDeallocate(kCFAllocatorDefault, colorPtrArray[colorIndex]);
				}
			}
			
			CFRelease(stringArray);
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getColorArrayValues


/*!
Returns the value of the specified dictionary key.  Results are 
undefined if the value for the given key is not a CFDictionary.

The "outValue" is expected to really be of type "CFDictionaryRef*".

(3.0)
*/
static TerminalFile_Result
getDictionaryValue	(CFDictionaryRef	inDictionary,
					 CFStringRef		inKey,
					 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFDictionaryRef			value;
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFUtilities_DictionaryCast(CFDictionaryGetValue(inDictionary, inKey));
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFRetain(value);
		*(REINTERPRET_CAST(outValue, CFDictionaryRef*)) = value;
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getDictionaryValue


/*!
Returns the value of the specified 32-bit IEEE floating point 
key.  Results are undefined if the value for the given key is 
not a CFNumber or a real number represented by a CFString.

The "outValue" is expected to really be of type "Float32*".

(3.0)
*/
static TerminalFile_Result
getFloat32Value		(CFDictionaryRef	inDictionary,
					 CFStringRef		inKey,
					 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFNumber or CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFUtilities_PropertyListCast(CFDictionaryGetValue(inDictionary, inKey));
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID	type;
		
		
		type = CFGetTypeID(value);
		
		if (type == CFNumberGetTypeID())
		{
			Boolean		success;
			
			
			success = CFNumberGetValue(CFUtilities_NumberCast(value), kCFNumberFloat32Type, outValue);
			if (!success)
				result = kTerminalFile_ResultNumberConversionErr;
		}
		else if (type == CFStringGetTypeID())
		{
			*(REINTERPRET_CAST(outValue, Float32*)) = (Float32)CFStringGetDoubleValue(CFUtilities_StringCast(value));
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getFloat32Value


/*!
Returns the value of the specified 64-bit IEEE floating point 
key.  Results are undefined if the value for the given key is 
not a CFNumber or a real number represented by a CFString.

The "outValue" is expected to really be of type "Float64*".

(3.0)
*/
static TerminalFile_Result
getFloat64Value		(CFDictionaryRef	inDictionary,
					 CFStringRef		inKey,
					 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFNumber or CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFUtilities_PropertyListCast(CFDictionaryGetValue(inDictionary, inKey));
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID	type;
		
		
		type = CFGetTypeID(value);
		
		if (type == CFNumberGetTypeID())
		{
			Boolean		success;
			
			
			success = CFNumberGetValue(CFUtilities_NumberCast(value), kCFNumberFloat64Type, outValue);
			if (!success)
				result = kTerminalFile_ResultNumberConversionErr;
		}
		else if (type == CFStringGetTypeID())
		{
			*(REINTERPRET_CAST(outValue, Float64*)) = CFStringGetDoubleValue(CFUtilities_StringCast(value));
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getFloat64Value


/*!
Returns the value of the specified 16-bit signed integer key.  
Results are undefined if the value for the given key is not a 
CFNumber or a signed integer represented by a CFString.

The "outValue" is expected to really be of type "SInt16*".

(3.0)
*/
static TerminalFile_Result
getSInt16Value	(CFDictionaryRef	inDictionary,
				 CFStringRef		inKey,
				 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFNumber or a CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFDictionaryGetValue(inDictionary, inKey);
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID	type;
		
		
		type = CFGetTypeID(value);
		
		if (type == CFNumberGetTypeID())
		{
			Boolean		success;
			
			
			success = CFNumberGetValue(CFUtilities_NumberCast(value), kCFNumberSInt16Type, outValue);
			if (!success)
				result = kTerminalFile_ResultNumberConversionErr;
		}
		else if (type == CFStringGetTypeID())
		{
			*(REINTERPRET_CAST(outValue, SInt16*)) = (SInt16)CFStringGetIntValue(CFUtilities_StringCast(value));
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getSInt16Value


/*!
Returns the value of the specified 32-bit signed integer key.  
Results are undefined if the value for the given key is not a 
CFNumber or a signed integer represented by a CFString.

The "outValue" is expected to really be of type "SInt32*".

(3.0)
*/
static TerminalFile_Result
getSInt32Value	(CFDictionaryRef	inDictionary,
				 CFStringRef		inKey,
				 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFNumber or a CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFUtilities_PropertyListCast(CFDictionaryGetValue(inDictionary, inKey));
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID	type;
		
		
		type = CFGetTypeID(value);
		
		if (type == CFNumberGetTypeID())
		{
			Boolean		success;
			
			
			success = CFNumberGetValue(CFUtilities_NumberCast(value), kCFNumberSInt32Type, outValue);
			if (!success)
				result = kTerminalFile_ResultNumberConversionErr;
		}
		else if (type == CFStringGetTypeID())
		{
			*(REINTERPRET_CAST(outValue, SInt32*)) = CFStringGetIntValue(CFUtilities_StringCast(value));
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getSInt32Value


/*!
Returns the values of the specified string array key.  We are 
assuming that the string array is contained in a CFString value 
and separated by semicolons, if it is not then the results of 
this method are undefined.

The "outValue" is expected to really be of type "CFArrayRef*".

(3.0)
*/
static TerminalFile_Result
getStringArrayValues	(CFDictionaryRef	inDictionary,
						 CFStringRef		inKey, 
						 void*				outValues)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFStringRef				values = nullptr;
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	values = CFUtilities_StringCast(CFDictionaryGetValue(inDictionary, inKey));
	
	if (values == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		*(REINTERPRET_CAST(outValues, CFArrayRef*)) = CFStringCreateArrayBySeparatingStrings
														(kCFAllocatorDefault, values, CFSTR(";"));
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getStringArrayValues


/*!
Returns the value of the specified string key.  Results are 
undefined if the value for the given key is not a CFString.

The "outValue" is expected to really be of type "CFStringRef*".

(3.0)
*/
static TerminalFile_Result
getStringValue	(CFDictionaryRef	inDictionary,
				 CFStringRef		inKey,
				 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFStringRef				value;
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFUtilities_StringCast(CFDictionaryGetValue(inDictionary, inKey));
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFRetain(value);
		*(REINTERPRET_CAST(outValue, CFStringRef*)) = value;
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getStringValue


/*!
Returns the value of the specified 16-bit unsigned integer key.  
Results are undefined if the value for the given key is not a 
CFNumber or an unsigned integer represented by a CFString.

The "outValue" is expected to really be of type "UInt16*".

(3.0)
*/
static TerminalFile_Result
getUInt16Value	(CFDictionaryRef	inDictionary,
				 CFStringRef		inKey,
				 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFNumber or a CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFDictionaryGetValue(inDictionary, inKey);
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID	type;
		
		
		type = CFGetTypeID(value);
		
		if (type == CFNumberGetTypeID())
		{
			Boolean		success;
			
			
			success = CFNumberGetValue(CFUtilities_NumberCast(value), kCFNumberSInt16Type, outValue);
			if (!success)
				result = kTerminalFile_ResultNumberConversionErr;
			else				// cast the signed value returned from the CFNumber 
				*(REINTERPRET_CAST(outValue, UInt16*)) = (UInt16)*(SInt16*)outValue;		// to an unsigned value
		}
		else if (type == CFStringGetTypeID())
		{
			*(REINTERPRET_CAST(outValue, UInt16*)) = (UInt16)CFStringGetIntValue(CFUtilities_StringCast(value));
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getUInt16Value


/*!
Returns the value of the specified 32-bit unsigned integer key.  
Results are undefined if the value for the given key is not a 
CFNumber or an unsigned integer represented by a CFString.

The "outValue" is expected to really be of type "UInt32*".

(3.0)
*/
static TerminalFile_Result
getUInt32Value	(CFDictionaryRef	inDictionary,
				 CFStringRef		inKey,
				 void*				outValue)
{
	TerminalFile_Result		result = kTerminalFile_ResultOK;
	CFPropertyListRef		value;	// this could be a CFNumber or a CFString
	
	
	CFRetain(inDictionary);
	CFRetain(inKey);
	
	value = CFDictionaryGetValue(inDictionary, inKey);
	if (value == nullptr)
	{
		result = kTerminalFile_ResultTagNotFound;
	}
	else
	{
		CFTypeID	type;
		
		
		type = CFGetTypeID(value);
		
		if (type == CFNumberGetTypeID())
		{
			Boolean		success;
			
			
			success = CFNumberGetValue(CFUtilities_NumberCast(value), kCFNumberSInt32Type, outValue);
			if (!success)
				result = kTerminalFile_ResultNumberConversionErr;
			else				// cast the signed value returned from the CFNumber
				*(REINTERPRET_CAST(outValue, UInt32*)) = (UInt32)*(SInt32*)outValue;		// to an unsigned value
		}
		else if (type == CFStringGetTypeID())
		{
			*(REINTERPRET_CAST(outValue, UInt32*)) = (UInt32)CFStringGetIntValue(CFUtilities_StringCast(value));
		}
	}
	
	CFRelease(inDictionary);
	CFRelease(inKey);
	
	return result;
}// getUInt32Value


/*!
Creates a CFString describing the value of an RGBColor in the 
format {R, G, B}.

(3.0)
*/
static CFStringRef
rgbCopyDescription	(void const*	inValue)
{
	CFStringRef			result = nullptr;
	RGBColor const*		colorPtr = REINTERPRET_CAST(inValue, RGBColor const*);
	
	
	result = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */, CFSTR("{%i, %i, %i}"), 
										colorPtr->red, colorPtr->green, colorPtr->blue);
	
	return result;
}// rgbCopyDescription


/*!
Checks to see if two RGBColors are equal.

(3.0)
*/
static Boolean
rgbEqual	(void const*	inColor1Ptr,
			 void const*	inColor2Ptr)
{
	RGBColor const* const		kColor1Ptr = REINTERPRET_CAST(inColor1Ptr, RGBColor const*);
	RGBColor const* const		kColor2Ptr = REINTERPRET_CAST(inColor2Ptr, RGBColor const*);
	Boolean						result = true;
	
	
	if (kColor1Ptr->red != kColor2Ptr->red)
	{
		result = false;
	}
	else if (kColor1Ptr->green != kColor2Ptr->green)
	{
		result = false;
	}
	else if (kColor1Ptr->blue != kColor2Ptr->blue)
	{
		result = false;
	}
	
	return result;
}// rgbEqual

// BELOW IS REQUIRED NEWLINE TO END FILE
