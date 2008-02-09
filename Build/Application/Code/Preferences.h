/*!	\file Preferences.h
	\brief Interfaces to access and modify user preferences,
	or be notified when they are changed.
	
	Preferences in MacTelnet 3.0 are now accessed through a
	layer of indirection, in part to reduce the number of code
	modules that have access to the “guts” of preferences data
	structures, and in part to allow this module to notify
	interested parties when settings are changed.
	
	Also new in version 3.0 is the notion of a “preference
	context”.  A context allows settings to be saved in very
	specific places, but retrieved through an automatic scan
	of a chain of possible locations for a given setting.  For
	example, you start with the frontmost window, and if no
	window-specific preference is available, an associated
	workspace file could be searched, and finally consulting
	the global defaults from the application preferences file.
	A window only has to ask for a setting, it does not have
	to know where the setting is!
*/
/*###############################################################

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

#ifndef __PREFERENCES__
#define __PREFERENCES__

// standard-C++ includes
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include "ListenerModel.h"

// MacTelnet includes
#include "Commands.h"



#pragma mark Constants

/*!
Most APIs in this module return a code of this type.
*/
typedef SInt32 Preferences_Result;
enum
{
	kPreferences_ResultOK = 0,								//!< no error
	kPreferences_ResultNotInitialized = -1,					//!< Preferences_Init() has not been called at all, or not successfully
	kPreferences_ResultUnknownTagOrClass = -2,				//!< specified preference tag or class tag isn’t valid
	kPreferences_ResultUnknownName = -3,					//!< specified name string doesn’t match any existing preferences data
	kPreferences_ResultInsufficientBufferSpace = -4,		//!< memory space provided isn’t large enough to hold data on disk
	kPreferences_ResultBadVersionDataNotAvailable = -5,		//!< preferences file does not contain this information in any form
	kPreferences_ResultBadVersionDataNewer = -6,			//!< preferences file contains more information than necessary
	kPreferences_ResultBadVersionDataOlder = -7,			//!< preferences file does not contain all necessary information
	kPreferences_ResultCannotCreateContext = -8,			//!< something prevents a requested context from being created
	kPreferences_ResultDescriptorNotUnique = -9,			//!< given descriptor was used in a previous call for the same purpose
	kPreferences_ResultInvalidContextReference = -10,		//!< a given "Preferences_ContextRef" cannot be resolved properly
	kPreferences_ResultNoMoreGeneralContext = -11,			//!< the global context was given
	kPreferences_ResultOneOrMoreNamesNotAvailable = -12,	//!< when returning a list of names, at least one was not retrievable
	kPreferences_ResultGenericFailure = -13					//!< if some unknown problem occurred
};

/*!
Generic ID number for an alias stored as preferences
on disk.  Using a simple ID you can create, save and
retrieve alias records easily.  This is the ONLY way
you should ever save file preferences to disk - they
are more flexible than regular pathnames.
*/
typedef SInt16 Preferences_AliasID;
enum
{
	kPreferences_InvalidAliasID = 0
};

/*!
Defines the subset of all possible preference tags
that are expected as input to an API in this module.
Check documentation on the tags to see what class
they belong to.
*/
typedef FourCharCode Preferences_Class;
enum
{
	// All of the four-character codes for classes are listed
	// alphabetically by code, and must be unique. 
	kPreferences_ClassGeneral	= FOUR_CHAR_CODE('appl'),
	kPreferences_ClassFormat	= FOUR_CHAR_CODE('text'),	//!< use with Preferences_NewContext()
	kPreferences_ClassMacroSet	= FOUR_CHAR_CODE('mcro'),	//!< use with Preferences_NewContext()
	kPreferences_ClassSession	= FOUR_CHAR_CODE('sess'),	//!< use with Preferences_NewContext()
	kPreferences_ClassTerminal	= FOUR_CHAR_CODE('term'),	//!< use with Preferences_NewContext()
	kPreferences_ClassWindow	= FOUR_CHAR_CODE('wind')	//!< use only Preferences_SetWindowArrangementData() and
															//!  Preferences_ArrangeWindow() with this class
};

/*!
All tags from the same preference class must have
unique values.  The tags are grouped by class.
When you call Preferences_GetData...() methods,
make sure the storage space you provide is large
enough to hold the data type indicated below for
the tag you specify.  Similarly, with the
Preferences_SetData...() methods, the data pointer
you provide should refer to data of the type that
the tag “expects”.  In each case, the data *points*
to storage of the type indicated.
*/
typedef FourCharCode Preferences_Tag;

/*!
Tags for use with kPreferences_ClassFormat.
*/
enum
{
	kPreferences_TagFontName							= FOUR_CHAR_CODE('font'),	//!< data: "Str255"
	kPreferences_TagFontSize							= FOUR_CHAR_CODE('fsiz'),	//!< data: "SInt16"
	kPreferences_TagTerminalColorMatteBackground		= FOUR_CHAR_CODE('mtte'),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorBlinkingForeground		= FOUR_CHAR_CODE('c*fg'),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorBlinkingBackground		= FOUR_CHAR_CODE('c*bg'),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorBoldForeground			= FOUR_CHAR_CODE('cbfg'),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorBoldBackground			= FOUR_CHAR_CODE('cbbg'),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorNormalForeground		= FOUR_CHAR_CODE('cnfg'),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorNormalBackground		= FOUR_CHAR_CODE('cnbg'),	//!< data: "RGBColor"
	// NOTE: These match menu command IDs for convenience in color menus.
	kPreferences_TagTerminalColorANSIBlack				= FOUR_CHAR_CODE(kCommandColorBlack),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIRed				= FOUR_CHAR_CODE(kCommandColorRed),					//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIGreen				= FOUR_CHAR_CODE(kCommandColorGreen),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIYellow				= FOUR_CHAR_CODE(kCommandColorYellow),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlue				= FOUR_CHAR_CODE(kCommandColorBlue),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIMagenta			= FOUR_CHAR_CODE(kCommandColorMagenta),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSICyan				= FOUR_CHAR_CODE(kCommandColorCyan),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIWhite				= FOUR_CHAR_CODE(kCommandColorWhite),				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlackBold			= FOUR_CHAR_CODE(kCommandColorBlackEmphasized),		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIRedBold			= FOUR_CHAR_CODE(kCommandColorRedEmphasized),		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIGreenBold			= FOUR_CHAR_CODE(kCommandColorGreenEmphasized),		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIYellowBold			= FOUR_CHAR_CODE(kCommandColorYellowEmphasized),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlueBold			= FOUR_CHAR_CODE(kCommandColorBlueEmphasized),		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIMagentaBold		= FOUR_CHAR_CODE(kCommandColorMagentaEmphasized),	//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSICyanBold			= FOUR_CHAR_CODE(kCommandColorCyanEmphasized),		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIWhiteBold			= FOUR_CHAR_CODE(kCommandColorWhiteEmphasized)		//!< data: "RGBColor"
};

/*!
Tags for use with kPreferences_ClassGeneral.
*/
enum
{
	kPreferences_TagArrangeWindowsUsingTabs				= FOUR_CHAR_CODE('tabs'),	//!< data: "Boolean"
	kPreferences_TagCaptureFileCreator					= FOUR_CHAR_CODE('cpfc'),	//!< data: "OSType"
	kPreferences_TagCopySelectedText					= FOUR_CHAR_CODE('cpsl'),	//!< data: "Boolean"
	kPreferences_TagCopyTableThreshold					= FOUR_CHAR_CODE('ctth'),	//!< data: "UInt16", the number of spaces per tab
	kPreferences_TagCursorBlinks						= FOUR_CHAR_CODE('curf'),	//!< data: "Boolean"
	kPreferences_TagCursorMovesPriorToDrops				= FOUR_CHAR_CODE('curm'),	//!< data: "Boolean"
	kPreferences_TagDontAutoClose						= FOUR_CHAR_CODE('wdga'),	//!< data: "Boolean"
	kPreferences_TagDontAutoNewOnApplicationReopen		= FOUR_CHAR_CODE('nonu'),	//!< data: "Boolean"
	kPreferences_TagDontDimBackgroundScreens			= FOUR_CHAR_CODE('wddb'),	//!< data: "Boolean"
	kPreferences_TagDynamicResizing						= FOUR_CHAR_CODE('dynr'),	//!< data: "Boolean"
	kPreferences_TagFocusFollowsMouse					= FOUR_CHAR_CODE('fcfm'),	//!< data: "Boolean"
	kPreferences_TagInfoWindowColumnOrdering			= FOUR_CHAR_CODE('sico'),	//!< data: "CFArrayRef" (of CFStrings)
	kPreferences_TagHeadersCollapsed					= FOUR_CHAR_CODE('hdcl'),	//!< data: "Boolean"
	kPreferences_TagKioskAllowsForceQuit				= FOUR_CHAR_CODE('kafq'),	//!< data: "Boolean"
	kPreferences_TagKioskShowsMenuBar					= FOUR_CHAR_CODE('kmnb'),	//!< data: "Boolean"
	kPreferences_TagKioskShowsOffSwitch					= FOUR_CHAR_CODE('koff'),	//!< data: "Boolean"
	kPreferences_TagKioskShowsScrollBar					= FOUR_CHAR_CODE('kscr'),	//!< data: "Boolean"
	kPreferences_TagKioskUsesSuperfluousEffects			= FOUR_CHAR_CODE('kewl'),	//!< data: "Boolean"
	kPreferences_TagMapBackquote						= FOUR_CHAR_CODE('map`'),	//!< data: "Boolean"
	kPreferences_TagMarginBell							= FOUR_CHAR_CODE('marb'),	//!< data: "Boolean"
	kPreferences_TagMenuItemKeys						= FOUR_CHAR_CODE('mkey'),	//!< data: "Boolean"
	kPreferences_TagNewCommandShortcutEffect			= FOUR_CHAR_CODE('new?'),	//!< data: "UInt32", a "kCommand..." constant
	kPreferences_TagNotification						= FOUR_CHAR_CODE('noti'),	//!< data: "SInt16", a "kAlert_Notify..." constant
	kPreferences_TagNotifyOfBeeps						= FOUR_CHAR_CODE('bnot'),	//!< data: "Boolean"
	kPreferences_TagPureInverse							= FOUR_CHAR_CODE('pinv'),	//!< data: "Boolean"
	kPreferences_TagSimplifiedUserInterface				= FOUR_CHAR_CODE('simp'),	//!< data: "Boolean"
	kPreferences_TagTerminalCursorType					= FOUR_CHAR_CODE('curs'),	//!< data: "TerminalView_CursorType"
	kPreferences_TagTerminalResizeAffectsFontSize		= FOUR_CHAR_CODE('rszf'),	//!< data: "Boolean"
	kPreferences_TagVisualBell							= FOUR_CHAR_CODE('visb'),	//!< data: "Boolean"
	kPreferences_TagWasClipboardShowing					= FOUR_CHAR_CODE('wvcl'),	//!< data: "Boolean"
	kPreferences_TagWasCommandLineShowing				= FOUR_CHAR_CODE('wvcm'),	//!< data: "Boolean"
	kPreferences_TagWasControlKeypadShowing				= FOUR_CHAR_CODE('wvck'),	//!< data: "Boolean"
	kPreferences_TagWasFunctionKeypadShowing			= FOUR_CHAR_CODE('wvfk'),	//!< data: "Boolean"
	kPreferences_TagWasSessionInfoShowing				= FOUR_CHAR_CODE('wvsi'),	//!< data: "Boolean"
	kPreferences_TagWasVT220KeypadShowing				= FOUR_CHAR_CODE('wvvk'),	//!< data: "Boolean"
	kPreferences_TagWindowStackingOrigin				= FOUR_CHAR_CODE('wino')	//!< data: "Point"
};

/*!
Tags for use with kPreferences_ClassMacroSet.
*/
enum
{
	kPreferences_TagMacrosMenuVisible					= FOUR_CHAR_CODE('mmnu')	//!< data: "Boolean"
};

/*!
Tags for use with kPreferences_ClassSession.
*/
enum
{
	kPreferences_TagAssociatedTerminalFavorite			= FOUR_CHAR_CODE('term'),	//!< data: "CFStringRef" (a kPreferences_ClassTerminal context name)
	kPreferences_TagAutoCaptureToFile					= FOUR_CHAR_CODE('capt'),	//!< data: "Boolean"
	kPreferences_TagCaptureFileAlias					= FOUR_CHAR_CODE('cfil'),	//!< data: "Preferences_AliasID"
	kPreferences_TagCommandLine							= FOUR_CHAR_CODE('cmdl'),	//!< data: "CFArrayRef" (of CFStrings)
	kPreferences_TagDataReadBufferSize					= FOUR_CHAR_CODE('rdbf'),	//!< data: "SInt16"
	kPreferences_TagKeyInterruptProcess					= FOUR_CHAR_CODE('kint'),	//!< data: "char" (actual non-printable ASCII control character)
	kPreferences_TagKeyResumeOutput						= FOUR_CHAR_CODE('kres'),	//!< data: "char" (actual non-printable ASCII control character)
	kPreferences_TagKeySuspendOutput					= FOUR_CHAR_CODE('ksus'),	//!< data: "char" (actual non-printable ASCII control character)
	kPreferences_TagLineModeEnabled						= FOUR_CHAR_CODE('linm'),	//!< data: "Boolean"
	kPreferences_TagLocalEchoEnabled					= FOUR_CHAR_CODE('echo'),	//!< data: "Boolean"
	kPreferences_TagLocalEchoHalfDuplex					= FOUR_CHAR_CODE('lehd'),	//!< data: "Boolean"
	kPreferences_TagMapCarriageReturnToCRNull			= FOUR_CHAR_CODE('crnl'),	//!< data: "Boolean"
	kPreferences_TagMapDeleteToBackspace				= FOUR_CHAR_CODE('delb'),	//!< data: "Boolean"
	kPreferences_TagPasteBlockSize						= FOUR_CHAR_CODE('pblk'),	//!< data: "SInt16"
	kPreferences_TagPasteMethod							= FOUR_CHAR_CODE('pstm'),	//!< data: a "kClipboard_PasteMethod…" constant
	kPreferences_TagServerHost							= FOUR_CHAR_CODE('host'),	//!< data: "CFStringRef" (domain name or IP address)
	kPreferences_TagServerPort							= FOUR_CHAR_CODE('port'),	//!< data: "SInt16"
	kPreferences_TagServerProtocol						= FOUR_CHAR_CODE('prcl'),	//!< data: "Session_Protocol"
	kPreferences_TagTektronixMode						= FOUR_CHAR_CODE('tekm'),	//!< data: a "kTektronixMode…" constant (see tekdefs.h)
	kPreferences_TagTektronixPAGEClearsScreen			= FOUR_CHAR_CODE('tkpc'),	//!< data: "Boolean"
	kPreferences_TagTextEncoding						= FOUR_CHAR_CODE('tenc'),	//!< data: "TextEncoding"
	kPreferences_TagTextTranslationTable				= FOUR_CHAR_CODE('xlat'),	//!< data: "CFStringRef" (a translation table name)
};

/*!
Tags for use with kPreferences_ClassTerminal.
*/
enum
{
	kPreferences_TagANSIColorsEnabled					= FOUR_CHAR_CODE('ansc'),	//!< data: "Boolean"
	kPreferences_TagDataReceiveDoNotStripHighBit		= FOUR_CHAR_CODE('8bit'),	//!< data: "Boolean"
	kPreferences_TagEMACSMetaKey						= FOUR_CHAR_CODE('meta'),	//!< data: "Session_EMACSMetaKey"
	kPreferences_TagMapArrowsForEMACS					= FOUR_CHAR_CODE('mapE'),	//!< data: "Boolean"
	kPreferences_TagMapKeypadTopRowForVT220				= FOUR_CHAR_CODE('mapK'),	//!< data: "Boolean"
	kPreferences_TagPageKeysControlLocalTerminal		= FOUR_CHAR_CODE('pgtm'),	//!< data: "Boolean"
	kPreferences_TagTerminalAnswerBackMessage			= FOUR_CHAR_CODE('ansb'),	//!< data: "CFStringRef"
	kPreferences_TagTerminalClearSavesLines				= FOUR_CHAR_CODE('clsv'),	//!< data: "Boolean"
	kPreferences_TagTerminalEmulatorType				= FOUR_CHAR_CODE('emul'),	//!< data: "Terminal_Emulator", a "kTerminal_Emulator..." constant
	kPreferences_TagTerminalLineWrap					= FOUR_CHAR_CODE('wrap'),	//!< data: "Boolean"
	kPreferences_TagTerminalPaddingLeft					= FOUR_CHAR_CODE('padl'),	//!< data: "Float32"
	kPreferences_TagTerminalPaddingRight				= FOUR_CHAR_CODE('padr'),	//!< data: "Float32"
	kPreferences_TagTerminalPaddingTop					= FOUR_CHAR_CODE('padt'),	//!< data: "Float32"
	kPreferences_TagTerminalPaddingBottom				= FOUR_CHAR_CODE('padb'),	//!< data: "Float32"
	kPreferences_TagTerminalScreenColumns				= FOUR_CHAR_CODE('scol'),	//!< data: "UInt16"
	kPreferences_TagTerminalScreenRows					= FOUR_CHAR_CODE('srow'),	//!< data: "UInt16"
	kPreferences_TagTerminalScreenScrollbackRows		= FOUR_CHAR_CODE('scrb'),	//!< data: "UInt16"
	kPreferences_TagTerminalScrollDelay					= FOUR_CHAR_CODE('scrd'),	//!< data: "EventTime" in MILLISECONDS
	kPreferences_TagXTermSequencesEnabled				= FOUR_CHAR_CODE('xtrm')	//!< data: "Boolean"
};

/*!
Tags for use with the special window preference APIs.
*/
enum
{
	kPreferences_WindowTagCommandLine					= FOUR_CHAR_CODE('Wcml'),
	kPreferences_WindowTagControlKeypad					= FOUR_CHAR_CODE('Wctk'),
	kPreferences_WindowTagFunctionKeypad				= FOUR_CHAR_CODE('Wfnk'),
	kPreferences_WindowTagMacroSetup					= FOUR_CHAR_CODE('Wmcs'),
	kPreferences_WindowTagPreferences					= FOUR_CHAR_CODE('Wprf'),
	kPreferences_WindowTagSessionInfo					= FOUR_CHAR_CODE('Winf'),
	kPreferences_WindowTagVT220Keypad					= FOUR_CHAR_CODE('Wvtk')
};

/*!
Navigation Services preference keys.  The idea here is to
define and use a unique key any time that may be helpful
to the user (e.g. when choosing an application, the user
would likely want to return to the Applications folder,
but wouldn’t want to see Applications as a default save
location for macro sets).
*/
enum
{
	kPreferences_NavPrefKeyGenericOpenFile		= 1,	//!< preference key for Open dialogs that don’t have a more specific key in the list
	kPreferences_NavPrefKeyGenericSaveFile		= 2,	//!< preference key for Save dialogs that don’t have a more specific key in the list
	kPreferences_NavPrefKeyGenericChooseFolder	= 3,	//!< preference key for Choose Folder dialogs
	kPreferences_NavPrefKeyChooseTextEditor		= 4,	//!< preference key for any Choose dialog used to locate a text editing application
	kPreferences_NavPrefKeyMacroStuff			= 5		//!< preference key for an Open or Save dialog that handles macros
};

/*!
Boundary elements for use with the special window
preference APIs; they specify which components of
a saved window rectangle are to be restored.
*/
typedef UInt16 Preferences_WindowBoundaryElements;
enum
{
	kPreferences_WindowBoundaryElementLocationH			= (1 << 0),		//!< specifies that the saved window left edge should be used
	kPreferences_WindowBoundaryElementLocationV			= (1 << 1),		//!< specifies that the saved window top edge should be used
	kPreferences_WindowBoundaryElementWidth				= (1 << 2),		//!< specifies that the saved window width should be used
	kPreferences_WindowBoundaryElementHeight			= (1 << 3),		//!< specifies that the saved window height should be used
	kPreferences_WindowBoundaryLocation					= kPreferences_WindowBoundaryElementLocationH |
															kPreferences_WindowBoundaryElementLocationV,
	kPreferences_WindowBoundarySize						= kPreferences_WindowBoundaryElementWidth |
															kPreferences_WindowBoundaryElementHeight,
	kPreferences_WindowBoundaryAllElements				= kPreferences_WindowBoundaryLocation |
															kPreferences_WindowBoundarySize
};

/*!
Lists the kinds of preferences that can trigger notification
when they are changed.  Use Preferences_ListenForChanges()
to establish callbacks to receive notifications.

The event context passed to the listener is a pointer to a
data structure of type "Preferences_ChangeContext".

Some changes are specific events.  However, most change codes
directly match preferences tags, allowing you to monitor
changes to those preferences.  For efficiency, most changes
do NOT trigger events: only the following tags are supported
with notifiers:
- kPreferences_TagArrangeWindowsUsingTabs
- kPreferences_TagCursorBlinks
- kPreferences_TagDontDimBackgroundScreens
- kPreferences_TagFocusFollowsMouse
- kPreferences_TagMacrosMenuVisible
- kPreferences_TagMapBackquote
- kPreferences_TagMenuItemKeys
- kPreferences_TagNewCommandShortcutEffect
- kPreferences_TagPureInverse
- kPreferences_TagSimplifiedUserInterface
- kPreferences_TagTerminalCursorType
- kPreferences_TagTerminalResizeAffectsFontSize
- kPreferences_TagTerminalScrollDelay
*/
enum
{
	kPreferences_ChangeContextName			= ('CNam'),		//!< the given context has been renamed; this may be important
															//!  for updating user interface elements
	kPreferences_ChangeNumberOfContexts		= ('SvCC')		//!< the number of collections (regardless of class) has changed;
															//!  this may be important for updating user interface elements
};
typedef Preferences_Tag		Preferences_Change;

#pragma mark Types

/*!
A context defines where to start looking for preferences data, in
what might be a string of possible contexts in a directed graph.

When setting data, however, a context is absolute; every context
has a specific place on disk where its data is stored, so setting
a preference in a specific context states exactly where the data
should go.
*/
typedef struct Preferences_OpaqueContext*	Preferences_ContextRef;

/*!
The context passed to the listeners of
preference changes.
*/
struct Preferences_ChangeContext
{
	Preferences_ContextRef		contextRef;		//!< if nullptr, the preference is global; otherwise, it occurred in this context
	Boolean						firstCall;		//!< whether or not this is the first time the preference notification has occurred
												//!  (if so, the value of the preference reflects its initial value)
};



#pragma mark Public Methods

//!\name Initialization
//@{

Preferences_Result
	Preferences_Init						();

void
	Preferences_Done						();

Preferences_Result
	Preferences_CreateOrFindFiles			();

//@}

//!\name Creating, Retaining and Releasing Preferences Contexts
//@{

Preferences_ContextRef
	Preferences_NewContext					(Preferences_Class					inClass,
											 CFStringRef						inNameOrNullToAutoGenerateUniqueName = nullptr);

Preferences_ContextRef
	Preferences_NewCloneContext				(Preferences_ContextRef				inBaseContext);

// IMPLICITLY DONE WHEN A CONTEXT IS CREATED
void
	Preferences_RetainContext				(Preferences_ContextRef				inContext);

void
	Preferences_ReleaseContext				(Preferences_ContextRef*			inoutContextPtr);

//@}

//!\name Using Existing Contexts (No Dispose Necessary)
//@{

Preferences_Result
	Preferences_GetDefaultContext			(Preferences_ContextRef*			outContextPtr,
											 Preferences_Class					inClass = kPreferences_ClassGeneral);

//@}

//!\name User Interface Utilities
//@{

Preferences_Result
	Preferences_ContextGetName				(Preferences_ContextRef				inContext,
											 CFStringRef&						outNewName);

Preferences_Result
	Preferences_ContextRename				(Preferences_ContextRef				inContext,
											 CFStringRef						inNewName);

Preferences_Result
	Preferences_CreateContextNameArray		(Preferences_Class					inClass,
											 CFArrayRef&						outNewArrayOfNewCFStrings);

Boolean
	Preferences_GetContextsInClass			(Preferences_Class					inClass,
											 std::vector< Preferences_ContextRef >&);

Preferences_Result
	Preferences_InsertContextNamesInMenu	(Preferences_Class					inClass,
											 MenuRef							inoutMenuRef,
											 MenuItemIndex						inAfterItemIndex,
											 UInt32								inInitialIndent,
											 MenuItemIndex&						outHowManyItemsAdded);

//@}

//!\name Accessing Contextual Preferences
//@{

Preferences_Result
	Preferences_ContextDeleteSaved			(Preferences_ContextRef				inContext);

Preferences_Result
	Preferences_ContextGetData				(Preferences_ContextRef				inStartingContext,
											 Preferences_Tag					inDataPreferenceTag,
											 size_t								inDataStorageSize,
											 void*								outDataStorage,
											 size_t*							outActualSizePtrOrNull = nullptr);

Preferences_Result
	Preferences_ContextSave					(Preferences_ContextRef				inContext);

Preferences_Result
	Preferences_ContextSetData				(Preferences_ContextRef				inContext,
											 Preferences_Tag					inDataPreferenceTag,
											 size_t								inDataSize,
											 void const*						inDataPtr);

//@}

//!\name Global Context APIs (Preferences Window Use Only)
//@{

Preferences_Result
	Preferences_Save						();

// DEPRECATED
Preferences_Result
	Preferences_GetData						(Preferences_Tag					inDataPreferenceTag,
											 size_t								inDataStorageSize,
											 void*								outDataStorage,
											 size_t*							outActualSizePtrOrNull = nullptr);

// DEPRECATED
Preferences_Result
	Preferences_SetData						(Preferences_Tag					inDataPreferenceTag,
											 size_t								inDataSize,
											 void const*						inDataPtr);

//@}

//!\name Window Size and Position Management - Deprecated
//@{

Preferences_Result
	Preferences_ArrangeWindow				(WindowRef							inWindow,
											 Preferences_Tag					inWindowPreferenceTag,
											 Point*								inoutMinimumSizeFinalSizePtr,
											 Preferences_WindowBoundaryElements	inBoundaryElementsToRestore =
											 										kPreferences_WindowBoundaryAllElements);

void
	Preferences_SetWindowArrangementData	(WindowRef							inWindow,
											 Preferences_Tag					inWindowPreferenceTag);

//@}

//!\name Alias Management
//@{

Preferences_AliasID
	Preferences_NewAlias					(FSSpec const*						inFileSpecificationPtr);

Preferences_AliasID
	Preferences_NewSavedAlias				(Preferences_AliasID				inAliasID);

void
	Preferences_DisposeAlias				(Preferences_AliasID				inAliasID);

void
	Preferences_AliasChange					(Preferences_AliasID				inAliasID,
											 FSSpec const*						inNewAliasFileSpecificationPtr);

void
	Preferences_AliasDeleteSaved			(Preferences_AliasID				inAliasID);

Boolean
	Preferences_AliasIsStored				(Preferences_AliasID				inAliasID);

Boolean
	Preferences_AliasParse					(Preferences_AliasID				inAliasID,
											 FSSpec*							outAliasFileSpecificationPtr);

void
	Preferences_AliasSave					(Preferences_AliasID				inAliasID,
											 ConstStringPtr						inName);

//@}

//!\name Receiving Notification of Changes
//@{

Preferences_Result
	Preferences_ListenForChanges			(ListenerModel_ListenerRef			inListener,
											 Preferences_Change					inForWhatChange,
											 Boolean							inNotifyOfInitialValue = false);

Preferences_Result
	Preferences_StopListeningForChanges		(ListenerModel_ListenerRef			inListener,
											 Preferences_Change					inForWhatChange);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
