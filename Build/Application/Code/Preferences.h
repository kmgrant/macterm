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

#ifndef __PREFERENCES__
#define __PREFERENCES__

// standard-C++ includes
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <ListenerModel.h>

// MacTelnet includes
#include "Commands.h"
#include "QuillsPrefs.h"



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

See also Preferences_Index, which is interlaced
with a tag in certain circumstances.
*/
typedef FourCharCode Preferences_Tag;

/*!
Tags for use with Quills::Prefs::FORMAT.
*/
enum
{
	kPreferences_TagFontName							= 'font',	//!< data: "Str255"
	kPreferences_TagFontSize							= 'fsiz',	//!< data: "SInt16"
	kPreferences_TagFontCharacterWidthMultiplier		= 'cwid',	//!< data: "Float32"
	// NOTE: These match menu command IDs for convenience in color boxes.
	kPreferences_TagTerminalColorMatteBackground		= kCommandColorMatteBackground,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorBlinkingForeground		= kCommandColorBlinkingForeground,	//!< data: "RGBColor"
	kPreferences_TagTerminalColorBlinkingBackground		= kCommandColorBlinkingBackground,	//!< data: "RGBColor"
	kPreferences_TagTerminalColorBoldForeground			= kCommandColorBoldForeground,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorBoldBackground			= kCommandColorBoldBackground,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorNormalForeground		= kCommandColorNormalForeground,	//!< data: "RGBColor"
	kPreferences_TagTerminalColorNormalBackground		= kCommandColorNormalBackground,	//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlack				= kCommandColorBlack,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIRed				= kCommandColorRed,					//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIGreen				= kCommandColorGreen,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIYellow				= kCommandColorYellow,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlue				= kCommandColorBlue,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIMagenta			= kCommandColorMagenta,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSICyan				= kCommandColorCyan,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIWhite				= kCommandColorWhite,				//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlackBold			= kCommandColorBlackEmphasized,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIRedBold			= kCommandColorRedEmphasized,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIGreenBold			= kCommandColorGreenEmphasized,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIYellowBold			= kCommandColorYellowEmphasized,	//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIBlueBold			= kCommandColorBlueEmphasized,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIMagentaBold		= kCommandColorMagentaEmphasized,	//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSICyanBold			= kCommandColorCyanEmphasized,		//!< data: "RGBColor"
	kPreferences_TagTerminalColorANSIWhiteBold			= kCommandColorWhiteEmphasized,		//!< data: "RGBColor"
	kPreferences_TagTerminalMarginLeft					= 'mgnl',	//!< data: "Float32", multiplies against font “m” width (even for vertical margins)
	kPreferences_TagTerminalMarginRight					= 'mgnr',	//!< data: "Float32"
	kPreferences_TagTerminalMarginTop					= 'mgnt',	//!< data: "Float32"
	kPreferences_TagTerminalMarginBottom				= 'mgnb',	//!< data: "Float32"
	kPreferences_TagTerminalPaddingLeft					= 'padl',	//!< data: "Float32", multiplies against font “m” width (even for vertical paddings)
	kPreferences_TagTerminalPaddingRight				= 'padr',	//!< data: "Float32"
	kPreferences_TagTerminalPaddingTop					= 'padt',	//!< data: "Float32"
	kPreferences_TagTerminalPaddingBottom				= 'padb'	//!< data: "Float32"
};

/*!
Tags for use with Quills::Prefs::GENERAL.
*/
enum
{
	kPreferences_TagBellSound							= 'bsnd',	//!< data: "CFStringRef" ("off", or the basename of sound file in a Sounds library folder)
	kPreferences_TagCaptureFileCreator					= 'cpfc',	//!< data: "OSType"
	kPreferences_TagCopySelectedText					= 'cpsl',	//!< data: "Boolean"
	kPreferences_TagCopyTableThreshold					= 'ctth',	//!< data: "UInt16", the number of spaces per tab
	kPreferences_TagCursorBlinks						= 'curf',	//!< data: "Boolean"
	kPreferences_TagCursorMovesPriorToDrops				= 'curm',	//!< data: "Boolean"
	kPreferences_TagDontAutoClose						= 'wdga',	//!< data: "Boolean"
	kPreferences_TagDontAutoNewOnApplicationReopen		= 'nonu',	//!< data: "Boolean"
	kPreferences_TagDontDimBackgroundScreens			= 'wddb',	//!< data: "Boolean"
	kPreferences_TagFocusFollowsMouse					= 'fcfm',	//!< data: "Boolean"
	kPreferences_TagInfoWindowColumnOrdering			= 'sico',	//!< data: "CFArrayRef" (of CFStrings)
	kPreferences_TagHeadersCollapsed					= 'hdcl',	//!< data: "Boolean"
	kPreferences_TagKioskAllowsForceQuit				= 'kafq',	//!< data: "Boolean"
	kPreferences_TagKioskShowsMenuBar					= 'kmnb',	//!< data: "Boolean"
	kPreferences_TagKioskShowsOffSwitch					= 'koff',	//!< data: "Boolean"
	kPreferences_TagKioskShowsScrollBar					= 'kscr',	//!< data: "Boolean"
	kPreferences_TagKioskShowsWindowFrame				= 'kwnf',	//!< data: "Boolean"
	kPreferences_TagKioskUsesSuperfluousEffects			= 'kewl',	//!< data: "Boolean"
	kPreferences_TagMacrosMenuVisible					= 'mmnu',	//!< data: "Boolean"
	kPreferences_TagMapBackquote						= 'map`',	//!< data: "Boolean"
	kPreferences_TagMenuItemKeys						= 'mkey',	//!< data: "Boolean"
	kPreferences_TagNewCommandShortcutEffect			= 'new?',	//!< data: "UInt32", a "kCommand..." constant
	kPreferences_TagNotification						= 'noti',	//!< data: "SInt16", a "kAlert_Notify..." constant
	kPreferences_TagNotifyOfBeeps						= 'bnot',	//!< data: "Boolean"
	kPreferences_TagPureInverse							= 'pinv',	//!< data: "Boolean"
	kPreferences_TagRandomTerminalFormats				= 'rfmt',	//!< data: "Boolean"
	kPreferences_TagTerminalCursorType					= 'curs',	//!< data: "TerminalView_CursorType"
	kPreferences_TagTerminalResizeAffectsFontSize		= 'rszf',	//!< data: "Boolean"
	kPreferences_TagTerminalShowMarginAtColumn			= 'smar',	//!< data: "UInt16"; 0 turns off, 1 is first column, etc.
	kPreferences_TagVisualBell							= 'visb',	//!< data: "Boolean"
	kPreferences_TagWasClipboardShowing					= 'wvcl',	//!< data: "Boolean"
	kPreferences_TagWasCommandLineShowing				= 'wvcm',	//!< data: "Boolean"
	kPreferences_TagWasControlKeypadShowing				= 'wvck',	//!< data: "Boolean"
	kPreferences_TagWasFunctionKeypadShowing			= 'wvfk',	//!< data: "Boolean"
	kPreferences_TagWasSessionInfoShowing				= 'wvsi',	//!< data: "Boolean"
	kPreferences_TagWasVT220KeypadShowing				= 'wvvk',	//!< data: "Boolean"
	kPreferences_TagWindowStackingOrigin				= 'wino',	//!< data: "Point"
	kPreferences_TagWindowTabPreferredEdge				= 'tedg'	//!< data: "OptionBits", a "kWindowEdge..." constant
};

/*!
Tags for use with Quills::Prefs::MACRO_SET.

IMPORTANT:	These are indexed tags, so calls to APIs must
			use Preferences_ReturnTagVariantForIndex()
			when defining the tag parameter.
*/
enum
{
	// indexed tags must have a zero byte to have space for tag variants;
	// see also Preferences_ReturnTagVariantForIndex()
	kPreferences_TagIndexedMacroAction					= 'mca\0',	//!< data: a "kMacroManager_Action…" constant
	kPreferences_TagIndexedMacroContents				= 'mtx\0',	//!< data: "CFStringRef"
	kPreferences_TagIndexedMacroKey						= 'mck\0',	//!< data: "MacroManager_KeyID"
	kPreferences_TagIndexedMacroKeyModifiers			= 'mmo\0',	//!< data: "UInt32", 0 or a bitwise-OR with any of: cmdKey, shiftKey, controlKey, optionKey
	kPreferences_TagIndexedMacroName					= 'mna\0'	//!< data: "CFStringRef"
};

/*!
Tags for use with Quills::Prefs::SESSION.
*/
enum
{
	kPreferences_TagAssociatedFormatFavorite			= 'frmt',	//!< data: "CFStringRef" (a Quills::Prefs::FORMAT context name)
	kPreferences_TagAssociatedTerminalFavorite			= 'term',	//!< data: "CFStringRef" (a Quills::Prefs::TERMINAL context name)
	kPreferences_TagAssociatedTranslationFavorite		= 'xlat',	//!< data: "CFStringRef" (a Quills::Prefs::TRANSLATION context name)
	kPreferences_TagAutoCaptureToFile					= 'capt',	//!< data: "Boolean"
	kPreferences_TagCaptureFileAlias					= 'cfil',	//!< data: "Preferences_AliasID"
	kPreferences_TagCommandLine							= 'cmdl',	//!< data: "CFArrayRef" (of CFStrings)
	kPreferences_TagDataReadBufferSize					= 'rdbf',	//!< data: "SInt16"
	kPreferences_TagIdleAfterInactivityInSeconds		= 'idle',	//!< data: "UInt16"
	kPreferences_TagKeepAlivePeriodInMinutes			= 'kfqm',	//!< data: "UInt16"
	kPreferences_TagKeyInterruptProcess					= 'kint',	//!< data: "char" (actual non-printable ASCII control character)
	kPreferences_TagKeyResumeOutput						= 'kres',	//!< data: "char" (actual non-printable ASCII control character)
	kPreferences_TagKeySuspendOutput					= 'ksus',	//!< data: "char" (actual non-printable ASCII control character)
	kPreferences_TagLineModeEnabled						= 'linm',	//!< data: "Boolean"
	kPreferences_TagLocalEchoEnabled					= 'echo',	//!< data: "Boolean"
	kPreferences_TagLocalEchoHalfDuplex					= 'lehd',	//!< data: "Boolean"
	kPreferences_TagMapCarriageReturnToCRNull			= 'crnl',	//!< data: "Boolean"
	kPreferences_TagMapDeleteToBackspace				= 'delb',	//!< data: "Boolean"
	kPreferences_TagPasteBlockSize						= 'pblk',	//!< data: "SInt16"
	kPreferences_TagPasteMethod							= 'pstm',	//!< data: a "kClipboard_PasteMethod…" constant
	kPreferences_TagScrollDelay							= 'scrd',	//!< data: "EventTime" in MILLISECONDS
	kPreferences_TagServerHost							= 'host',	//!< data: "CFStringRef" (domain name or IP address)
	kPreferences_TagServerPort							= 'port',	//!< data: "SInt16"
	kPreferences_TagServerProtocol						= 'prcl',	//!< data: "Session_Protocol"
	kPreferences_TagServerUserID						= 'user',	//!< data: "CFStringRef"
	kPreferences_TagTektronixMode						= 'tekm',	//!< data: a "kVectorInterpreter_Mode…" constant
	kPreferences_TagTektronixPAGEClearsScreen			= 'tkpc'	//!< data: "Boolean"
};

/*!
Tags for use with Quills::Prefs::TERMINAL.

Some are terminal-specific tweaks; anything starting with
"kPreferences_TagVT..." or "kPreferences_TagXTerm...".
These should also have localized names in
"UIStrings_PrefsWindow.h".
*/
enum
{
	kPreferences_TagDataReceiveDoNotStripHighBit		= '8bit',	//!< data: "Boolean"
	kPreferences_TagEMACSMetaKey						= 'meta',	//!< data: "Session_EMACSMetaKey"
	kPreferences_TagMapArrowsForEMACS					= 'mapE',	//!< data: "Boolean"
	kPreferences_TagMapKeypadTopRowForVT220				= 'mapK',	//!< data: "Boolean"
	kPreferences_TagPageKeysControlLocalTerminal		= 'pgtm',	//!< data: "Boolean"
	kPreferences_TagTerminalAnswerBackMessage			= 'ansb',	//!< data: "CFStringRef"
	kPreferences_TagTerminalClearSavesLines				= 'clsv',	//!< data: "Boolean"
	kPreferences_TagTerminalEmulatorType				= 'emul',	//!< data: "Terminal_Emulator", a "kTerminal_Emulator..." constant
	kPreferences_TagTerminalLineWrap					= 'wrap',	//!< data: "Boolean"
	kPreferences_TagTerminalScreenColumns				= 'scol',	//!< data: "UInt16"
	kPreferences_TagTerminalScreenRows					= 'srow',	//!< data: "UInt16"
	kPreferences_TagTerminalScreenScrollbackRows		= 'scrb',	//!< data: "UInt16"
	kPreferences_TagTerminalScreenScrollbackType		= 'scrt',	//!< data: "Terminal_ScrollbackType"
	kPreferences_TagVT100FixLineWrappingBug				= 'vlwr',	//!< data: "Boolean"
	kPreferences_TagXTermColorEnabled					= 'xtcl',	//!< data: "Boolean"
	kPreferences_TagXTerm256ColorsEnabled				= 'x256',	//!< data: "Boolean"
	kPreferences_TagXTermGraphicsEnabled				= 'xtgr',	//!< data: "Boolean"
	kPreferences_TagXTermWindowAlterationEnabled		= 'xtwn'	//!< data: "Boolean"
};

/*!
Tags for use with Quills::Prefs::TRANSLATION.
*/
enum
{
	kPreferences_TagBackupFontName						= 'bfnt',	//!< data: "Str255"
	kPreferences_TagTextEncodingIANAName				= 'iana',	//!< data: "CFStringRef"
	kPreferences_TagTextEncodingID						= 'encd'	//!< data: "CFStringEncoding"
};

/*!
Tags for use with Quills::Prefs::WORKSPACE.

IMPORTANT:	Some are indexed tags, so calls to APIs must
			use Preferences_ReturnTagVariantForIndex()
			when defining those tag parameters.
*/
enum
{
	kPreferences_TagArrangeWindowsUsingTabs				= 'tabs',	//!< data: "Boolean"
	// indexed tags must have a zero byte to have space for tag variants;
	// see also Preferences_ReturnTagVariantForIndex()
	kPreferences_TagIndexedWindowFrameBounds			= 'wfb\0',	//!< data: "HIRect"
	kPreferences_TagIndexedWindowScreenBounds			= 'wsb\0'	//!< data: "HIRect"
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
Lists the kinds of global changes that can trigger notification
when they occur.  Each of these is considered to be of type
"Preferences_Change".  Use Preferences_StartMonitoring() to
establish callbacks.

These are NOT used for context-specific callbacks; see
Preferences_ContextStartMonitoring().
*/
enum
{
	kPreferences_ChangeContextName			= ('CNam'),		//!< the given context has been renamed; this may be important
															//!  for updating user interface elements; UNDER EVALUATION: now
															//!  that context-specific callbacks are possible, this event may
															//!  be converted to that set
	kPreferences_ChangeNumberOfContexts		= ('SvCC')		//!< the number of collections (regardless of class) or their order
															//!  has changed; this may be important for updating user interfaces
};
/*!
Lists the kinds of changes that only trigger notification when
they occur for specific contexts.  Each of these is considered
to be of type "Preferences_Change".  You can establish callbacks
with Preferences_ContextStartMonitoring().

See also Preferences_StartMonitoring().
*/
enum
{
	kPreferences_ChangeContextBatchMode		= ('CMny')		//!< several settings were changed all at once; simply respond
															//!  as if everything you care about has changed
};
typedef Preferences_Tag		Preferences_Change;

#pragma mark Types

#include "PreferencesContextRef.typedef.h"

/*!
A zero-based preferences index is added to the tag value to
generate a unique tag that can be hashed.  So, a tag must have
enough unused bits to allow this arithmetic (and other tags
must not use values similar to those of indexed tags).

Always use Preferences_ReturnTagVariantForIndex() to produce a
valid tag out of a base tag and an index.
*/
typedef UInt8		Preferences_Index;

/*!
The context passed to the listeners of global preference changes.
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

//!\name Module Tests
//@{

void
	Preferences_RunTests					();

//@}

//!\name Creating, Retaining and Releasing Preferences Contexts
//@{

Preferences_ContextRef
	Preferences_NewContext					(Quills::Prefs::Class				inClass);

Preferences_ContextRef
	Preferences_NewContextFromFavorites		(Quills::Prefs::Class				inClass,
											 CFStringRef						inNameOrNullToAutoGenerateUniqueName = nullptr,
											 CFStringRef						inDomainNameIfInitializingOrNull = nullptr);

Preferences_ContextRef
	Preferences_NewContextFromXMLData		(Quills::Prefs::Class				inClass,
											 CFDataRef							inData);

Preferences_ContextRef
	Preferences_NewContextFromXMLFileRef	(Quills::Prefs::Class				inClass,
											 FSRef const&						inFile);

Preferences_ContextRef
	Preferences_NewContextFromXMLFileURL	(Quills::Prefs::Class				inClass,
											 CFURLRef							inFileURL);

Preferences_ContextRef
	Preferences_NewCloneContext				(Preferences_ContextRef				inBaseContext,
											 Boolean							inForceDetach = false);

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
											 Quills::Prefs::Class				inClass = Quills::Prefs::GENERAL);

Preferences_Result
	Preferences_GetFactoryDefaultsContext	(Preferences_ContextRef*			outContextPtr);

//@}

//!\name User Interface Utilities
//@{

Preferences_Result
	Preferences_ContextGetName						(Preferences_ContextRef				inContext,
													 CFStringRef&						outNewName);

Preferences_Result
	Preferences_ContextRename						(Preferences_ContextRef				inContext,
													 CFStringRef						inNewName);

Preferences_Result
	Preferences_ContextRepositionRelativeToContext	(Preferences_ContextRef				inContext,
													 Preferences_ContextRef				inReferenceContext,
													 Boolean							inInsertBefore = false);

Preferences_Result
	Preferences_ContextRepositionRelativeToSelf		(Preferences_ContextRef				inContext,
													 SInt32								inDelta);

Preferences_Result
	Preferences_CreateContextNameArray				(Quills::Prefs::Class				inClass,
													 CFArrayRef&						outNewArrayOfNewCFStrings);

Preferences_Result
	Preferences_CreateUniqueContextName				(Quills::Prefs::Class				inClass,
													 CFStringRef&						outNewName,
													 CFStringRef						inBaseNameOrNull = nullptr);

Boolean
	Preferences_GetContextsInClass					(Quills::Prefs::Class				inClass,
													 std::vector< Preferences_ContextRef >&);

Preferences_Result
	Preferences_InsertContextNamesInMenu			(Quills::Prefs::Class				inClass,
													 MenuRef							inoutMenuRef,
													 MenuItemIndex						inAfterItemIndex,
													 UInt32								inInitialIndent,
													 UInt32								inCommandID,
													 MenuItemIndex&						outHowManyItemsAdded);

//@}

//!\name Accessing Contextual Preferences
//@{

Preferences_Result
	Preferences_ContextCopy					(Preferences_ContextRef				inBaseContext,
											 Preferences_ContextRef				inDestinationContext);

Preferences_Result
	Preferences_ContextDeleteData			(Preferences_ContextRef				inContext,
											 Preferences_Tag					inDataPreferenceTag);

Preferences_Result
	Preferences_ContextDeleteSaved			(Preferences_ContextRef				inContext);

Preferences_Result
	Preferences_ContextGetData				(Preferences_ContextRef				inStartingContext,
											 Preferences_Tag					inDataPreferenceTag,
											 size_t								inDataStorageSize,
											 void*								outDataStorage,
											 Boolean							inSearchDefaults = false,
											 size_t*							outActualSizePtrOrNull = nullptr,
											 Boolean*							outIsDefaultOrNull = nullptr);

Quills::Prefs::Class
	Preferences_ContextReturnClass			(Preferences_ContextRef				inContext);

Preferences_Result
	Preferences_ContextSave					(Preferences_ContextRef				inContext);

Preferences_Result
	Preferences_ContextSaveAsXMLData		(Preferences_ContextRef				inContext,
											 CFDataRef&							outData);

Preferences_Result
	Preferences_ContextSaveAsXMLFileRef		(Preferences_ContextRef				inContext,
											 FSRef const&						inFile);

Preferences_Result
	Preferences_ContextSaveAsXMLFileURL		(Preferences_ContextRef				inContext,
											 CFURLRef							inURL);

Preferences_Result
	Preferences_ContextSetData				(Preferences_ContextRef				inContext,
											 Preferences_Tag					inDataPreferenceTag,
											 size_t								inDataSize,
											 void const*						inDataPtr);

/*!
For a tag produced by Preferences_ReturnTagFromVariant(),
returns the base tag (without any index).  This is useful
in things like "switch" statements, to catch any tag of
a certain type.

(4.0)
*/
inline Preferences_Tag
	Preferences_ReturnTagFromVariant		(Preferences_Tag					inIndexedTag)
	{
		return (inIndexedTag & 0xFFFFFF00);
	}

/*!
For a tag produced by Preferences_ReturnTagFromVariant(),
returns the index only.

(4.0)
*/
inline Preferences_Index
	Preferences_ReturnTagIndex				(Preferences_Tag					inIndexedTag)
	{
		return (inIndexedTag & 0x000000FF);
	}

/*!
Generate a tag that combines a base tag and index.
This is only used by preferences whose tag constants
follow the "kPreferences_TagIndexed…" convention.
Decode later with Preferences_ReturnTagFromVariant()
and Preferences_ReturnTagIndex().

(4.0)
*/
inline Preferences_Tag
	Preferences_ReturnTagVariantForIndex	(Preferences_Tag					inIndexedTag,
											 Preferences_Index					inOneBasedIndex)
	{
		assert(inOneBasedIndex >= 1);
		return (inIndexedTag + inOneBasedIndex);
	}

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
	Preferences_ContextStartMonitoring		(Preferences_ContextRef				inContext,
											 ListenerModel_ListenerRef			inListener,
											 Preferences_Change					inForWhatChange,
											 Boolean							inNotifyOfInitialValue = false);

Preferences_Result
	Preferences_ContextStopMonitoring		(Preferences_ContextRef				inContext,
											 ListenerModel_ListenerRef			inListener,
											 Preferences_Change					inForWhatChange);

Preferences_Result
	Preferences_StartMonitoring				(ListenerModel_ListenerRef			inListener,
											 Preferences_Change					inForWhatChange,
											 Boolean							inNotifyOfInitialValue = false);

Preferences_Result
	Preferences_StopMonitoring				(ListenerModel_ListenerRef			inListener,
											 Preferences_Change					inForWhatChange);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
