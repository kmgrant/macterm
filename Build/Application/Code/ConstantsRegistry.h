/*!	\file ConstantsRegistry.h
	\brief Collection of enumerations that must be unique
	across the application.
	
	This file contains collections of constants that must be
	unique within their set.  With all constants defined in one
	place, it is easy to see at a glance if there are any
	values that are not unique!
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __CONSTANTSREGISTRY__
#define __CONSTANTSREGISTRY__

// library includes
#include <FlagManager.h>
#include <ListenerModel.h>
#include <WindowInfo.h>



//!\name HIView-Related Constants
//@{

/*!
Control kinds that use kConstantsRegistry_ApplicationCreatorSignature
(this application) as the signature.
*/
enum
{
	kConstantsRegistry_ControlKindTerminalView		= 'TrmV'	//!< see TerminalView.cp
};

/*!
This list contains all tags used with the SetControlProperty()
API; kept in one place so they are known to be unique.
*/
enum
{
	kConstantsRegistry_ControlPropertyCreator						= 'KevG',	//!< use as the creator code of all control properties
	kConstantsRegistry_ControlPropertyTypeAddressDialog				= 'ADlg',	//!< data: AddressDialog_Ref
	kConstantsRegistry_ControlPropertyTypeColorBoxData				= 'ClBx',	//!< data: MyColorBoxDataPtr (internal to ColorBox.cp)
	kConstantsRegistry_ControlPropertyTypeShowDragHighlight			= 'Drag',	//!< data: Boolean (if true, draw highlight; if false; erase)
	kConstantsRegistry_ControlPropertyTypeTerminalBackgroundData	= 'TrmB',	//!< data: MyTerminalBackgroundPtr (internal to TerminalView.cp)
	kConstantsRegistry_ControlPropertyTypeTerminalViewRef			= 'TrmV',	//!< data: TerminalViewRef
	kConstantsRegistry_ControlPropertyTypeTerminalWindowRef			= 'TrmW'	//!< data: TerminalWindowRef
};

/*!
Control kinds that use kConstantsRegistry_ApplicationCreatorSignature
(this application) as the signature.
*/
enum
{
	kConstantsRegistry_ControlDataTagTerminalBackgroundColor		= 'TrmB'	//!< data: MyTerminalBackgroundPtr (internal to TerminalBackground.cp)
};

/*!
Panel descriptors, which help to identify panels when given
only an opaque reference to one.  See "Panel.h".

The order here is not important, though each panel needs a
unique value.

Note that these strings are also used as HIObject IDs for
toolbar items.
*/
CFStringRef const kConstantsRegistry_PrefPanelDescriptorGeneral			= CFSTR("com.mactelnet.prefpanels.general");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorSessions		= CFSTR("com.mactelnet.prefpanels.sessions");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorMacros			= CFSTR("com.mactelnet.prefpanels.macros");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTerminals		= CFSTR("com.mactelnet.prefpanels.terminals");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorFormats			= CFSTR("com.mactelnet.prefpanels.formats");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorKiosk			= CFSTR("com.mactelnet.prefpanels.kiosk");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorScripts			= CFSTR("com.mactelnet.prefpanels.scripts");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTranslations	= CFSTR("com.mactelnet.prefpanels.translations");

/*!
Used with HIObject routines (such as HIObjectCreate()) to
implement custom user interface elements.
*/
CFStringRef const kConstantsRegistry_HIObjectClassIDTerminalBackgroundView	= CFSTR("com.mactelnet.terminal.background");
CFStringRef const kConstantsRegistry_HIObjectClassIDTerminalTextView		= CFSTR("com.mactelnet.terminal.text");
CFStringRef const kConstantsRegistry_HIToolbarIDPreferences					= CFSTR("com.mactelnet.toolbar.preferences");
CFStringRef const kConstantsRegistry_HIToolbarIDSessionInfo					= CFSTR("com.mactelnet.toolbar.sessioninfo");
CFStringRef const kConstantsRegistry_HIToolbarIDTerminal					= CFSTR("com.mactelnet.toolbar.terminal");
CFStringRef const kConstantsRegistry_HIToolbarIDTerminalFloater				= CFSTR("com.mactelnet.toolbar.terminalfloater");
CFStringRef const kConstantsRegistry_HIToolbarItemIDCollections				= CFSTR("com.mactelnet.toolbaritem.collections");
CFStringRef const kConstantsRegistry_HIToolbarItemIDFullScreen				= CFSTR("com.mactelnet.toolbaritem.fullscreen");
CFStringRef const kConstantsRegistry_HIToolbarItemIDHideWindow				= CFSTR("com.mactelnet.toolbaritem.hidewindow");
CFStringRef const kConstantsRegistry_HIToolbarItemIDNewDefaultSession		= CFSTR("com.mactelnet.toolbaritem.newsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDNewLoginSession			= CFSTR("com.mactelnet.toolbaritem.newloginsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDNewShellSession			= CFSTR("com.mactelnet.toolbaritem.newshellsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDScrollLock				= CFSTR("com.mactelnet.toolbaritem.scrolllock");
CFStringRef const kConstantsRegistry_HIToolbarItemIDStackWindows			= CFSTR("com.mactelnet.toolbaritem.stackwindows");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalBell			= CFSTR("com.mactelnet.toolbaritem.terminalbell");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED1			= CFSTR("com.mactelnet.toolbaritem.terminalled1");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED2			= CFSTR("com.mactelnet.toolbaritem.terminalled2");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED3			= CFSTR("com.mactelnet.toolbaritem.terminalled3");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED4			= CFSTR("com.mactelnet.toolbaritem.terminalled4");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalSearch			= CFSTR("com.mactelnet.toolbaritem.terminalsearch");

/*!
Used when registering icons with Icon Services under the
creator "kConstantsRegistry_ApplicationCreatorSignature".
These must all be unique within the application.
*/
enum
{
	kConstantsRegistry_IconServicesIconApplication				= FOUR_CHAR_CODE('AppI'),
	kConstantsRegistry_IconServicesIconFileTypeMacroSet			= FOUR_CHAR_CODE('PMcS'),
	kConstantsRegistry_IconServicesIconItemAdd					= FOUR_CHAR_CODE('AddI'),
	kConstantsRegistry_IconServicesIconItemRemove				= FOUR_CHAR_CODE('DelI'),
	kConstantsRegistry_IconServicesIconKeypadArrowDown			= FOUR_CHAR_CODE('KPAD'),
	kConstantsRegistry_IconServicesIconKeypadArrowLeft			= FOUR_CHAR_CODE('KPAL'),
	kConstantsRegistry_IconServicesIconKeypadArrowRight			= FOUR_CHAR_CODE('KPAR'),
	kConstantsRegistry_IconServicesIconKeypadArrowUp			= FOUR_CHAR_CODE('KPAU'),
	kConstantsRegistry_IconServicesIconKeypadDelete				= FOUR_CHAR_CODE('KPDl'),
	kConstantsRegistry_IconServicesIconKeypadEnter				= FOUR_CHAR_CODE('KPEn'),
	kConstantsRegistry_IconServicesIconKeypadFind				= FOUR_CHAR_CODE('KPFn'),
	kConstantsRegistry_IconServicesIconKeypadInsert				= FOUR_CHAR_CODE('KPIn'),
	kConstantsRegistry_IconServicesIconKeypadPageDown			= FOUR_CHAR_CODE('KPPD'),
	kConstantsRegistry_IconServicesIconKeypadPageUp				= FOUR_CHAR_CODE('KPPU'),
	kConstantsRegistry_IconServicesIconKeypadSelect				= FOUR_CHAR_CODE('KPSl'),
	kConstantsRegistry_IconServicesIconMenuTitleScripts			= FOUR_CHAR_CODE('ScpM'),
	kConstantsRegistry_IconServicesIconPreferenceCollection		= FOUR_CHAR_CODE('PrfC'),
	kConstantsRegistry_IconServicesIconPrefPanelFormats			= FOUR_CHAR_CODE('PFmt'),
	kConstantsRegistry_IconServicesIconPrefPanelFullScreen		= FOUR_CHAR_CODE('PKio'),
	kConstantsRegistry_IconServicesIconPrefPanelGeneral			= FOUR_CHAR_CODE('PGen'),
	kConstantsRegistry_IconServicesIconPrefPanelMacros			= FOUR_CHAR_CODE('PMcr'),
	kConstantsRegistry_IconServicesIconPrefPanelScripts			= FOUR_CHAR_CODE('PScr'),
	kConstantsRegistry_IconServicesIconPrefPanelSessions		= FOUR_CHAR_CODE('PSsn'),
	kConstantsRegistry_IconServicesIconPrefPanelTerminals		= FOUR_CHAR_CODE('PTrm'),
	kConstantsRegistry_IconServicesIconSessionStatusActive		= FOUR_CHAR_CODE('Actv'),
	kConstantsRegistry_IconServicesIconSessionStatusDead		= FOUR_CHAR_CODE('Dead'),
	kConstantsRegistry_IconServicesIconToolbarItemBellOff		= FOUR_CHAR_CODE('BelO'),
	kConstantsRegistry_IconServicesIconToolbarItemBellOn		= FOUR_CHAR_CODE('BelI'),
	kConstantsRegistry_IconServicesIconToolbarItemFullScreen	= FOUR_CHAR_CODE('Kios'),
	kConstantsRegistry_IconServicesIconToolbarItemHideWindow	= FOUR_CHAR_CODE('Hide'),
	kConstantsRegistry_IconServicesIconToolbarItemLEDOff		= FOUR_CHAR_CODE('LEDO'),
	kConstantsRegistry_IconServicesIconToolbarItemLEDOn			= FOUR_CHAR_CODE('LEDI'),
	kConstantsRegistry_IconServicesIconToolbarItemScrollLockOff	= FOUR_CHAR_CODE('XON '),
	kConstantsRegistry_IconServicesIconToolbarItemScrollLockOn	= FOUR_CHAR_CODE('XOFF'),
	kConstantsRegistry_IconServicesIconToolbarItemStackWindows	= FOUR_CHAR_CODE('StkW')
};

//@}

//!\name Window-Related Constants
//@{

/*!
This list contains all tags used with the SetWindowProperty()
API; kept in one place so they are known to be unique.
*/
enum
{
	kConstantsRegistry_WindowPropertyCreator			= 'KevG',	//!< use as the creator code of all window properties
	kConstantsRegistry_WindowPropertyTypeWindowInfoRef	= 'WInf'	//!< data: WindowInfo_Ref
};

/*!
MacTelnet window descriptors, used with Window Info.
These are all to be considered of the type
"WindowInfoDescriptor".  A descriptor should be unique
to a class of MacTelnet windows - often, only a single
MacTelnet window will have a particular descriptor
(but most terminals, and all sheets of the same kind,
share descriptors).

You will find that certain code modules switch on
window descriptors to determine what to do with an
unknown window.  Therefore, every time a new kind of
window is added to MacTelnet, a unique descriptor for
it should appear in the list below, and search source
files (e.g. for "kConstantsRegistry_WindowDescriptor")
to find switches that should perhaps be augmented to
include a case for your new descriptor.

IMPORTANT:	Use of descriptors has been reduced quite
			a bit since Carbon Events came along.  It
			is likely they will disappear eventually.
*/
enum
{
	// terminals
	kConstantsRegistry_WindowDescriptorAnyTerminal			= FOUR_CHAR_CODE('Term'),
	kConstantsRegistry_WindowDescriptorDebuggingConsole		= FOUR_CHAR_CODE('cerr'),
	// floating windows
	kConstantsRegistry_WindowDescriptorCommandLine			= FOUR_CHAR_CODE('CmdL'),
	kConstantsRegistry_WindowDescriptorKeypadWindoid		= FOUR_CHAR_CODE('Keyp'),
	kConstantsRegistry_WindowDescriptorFunctionWindoid		= FOUR_CHAR_CODE('FKey'),
	kConstantsRegistry_WindowDescriptorControlWindoid		= FOUR_CHAR_CODE('CKey'),
	// modeless dialogs and windows
	kConstantsRegistry_WindowDescriptorPreferences			= FOUR_CHAR_CODE('Pref'),
	kConstantsRegistry_WindowDescriptorConnectionStatus		= FOUR_CHAR_CODE('Stat'),
	kConstantsRegistry_WindowDescriptorClipboard			= FOUR_CHAR_CODE('Clip'),
	kConstantsRegistry_WindowDescriptorStandardAboutBox		= FOUR_CHAR_CODE('Info'),
	kConstantsRegistry_WindowDescriptorCreditsBox			= FOUR_CHAR_CODE('Cred'),
	// modal dialogs and sheets
	kConstantsRegistry_WindowDescriptorFind					= FOUR_CHAR_CODE('Find'),
	kConstantsRegistry_WindowDescriptorFormat				= FOUR_CHAR_CODE('Font'),
	kConstantsRegistry_WindowDescriptorMacroSetup			= FOUR_CHAR_CODE('Mcro'),
	kConstantsRegistry_WindowDescriptorNameNewFavorite		= FOUR_CHAR_CODE('DupF'),
	kConstantsRegistry_WindowDescriptorNewSessions			= FOUR_CHAR_CODE('NewS'),
	kConstantsRegistry_WindowDescriptorPasswordEntry		= FOUR_CHAR_CODE('Pswd'),
	kConstantsRegistry_WindowDescriptorScreenSize			= FOUR_CHAR_CODE('Size'),
	kConstantsRegistry_WindowDescriptorSessionEditor		= FOUR_CHAR_CODE('SFav'),
	kConstantsRegistry_WindowDescriptorSetWindowTitle		= FOUR_CHAR_CODE('WTtl'),
	kConstantsRegistry_WindowDescriptorTerminalEditor		= FOUR_CHAR_CODE('TFav')
};

/*!
Possible return values from GetWindowKind() on MacTelnet windows.
*/
enum
{
	WIN_CONSOLE		= kApplicationWindowKind + 2,			//!< console window
	WIN_CNXN		= kApplicationWindowKind + 4,			//!< remote shells
	WIN_SHELL		= kApplicationWindowKind + 5,			//!< local shells
	WIN_TEK			= kApplicationWindowKind + 6,			//!< TEK windows
	WIN_ICRG		= kApplicationWindowKind + 7			//!< Interactive Color Raster Graphics windows
};

//@}

//!\name Undoable-Operation Context Identifiers
//@{

/*!
Undoable operation context IDs, used with Undoable objects.
These are all to be considered of the type
"UndoableContextIdentifier", defined in "Undoables.h".
*/
enum
{
	kUndoableContextIdentifierTerminalDimensionChanges		= 1,
	kUndoableContextIdentifierTerminalFormatChanges			= 2,
	kUndoableContextIdentifierTerminalFontSizeChanges		= 3,
	kUndoableContextIdentifierTerminalFullScreenChanges		= 4
};

//@}

//!\name Unique Identifiers for Registries of Public Events
//@{

/*!
Listener Model descriptors, identifying objects that manage,
abstractly, ÒlistenersÓ for events, and then notify the
listeners when events actually take place.  See "ListenerModel.h".

The main benefit behind these identifiers is for cases where the
same listener responds to events from multiple models and needs
to know which model an event came from.
*/
enum
{
	kConstantsRegistry_ListenerModelDescriptorCommandExecution					= 'CmdX',
	kConstantsRegistry_ListenerModelDescriptorCommandModification				= 'CmdM',
	kConstantsRegistry_ListenerModelDescriptorCommandPostExecution				= 'CmdF',
	kConstantsRegistry_ListenerModelDescriptorEventTargetForControl				= 'cetg',
	kConstantsRegistry_ListenerModelDescriptorEventTargetForWindow				= 'wetg',
	kConstantsRegistry_ListenerModelDescriptorEventTargetGlobal					= 'getg',
	kConstantsRegistry_ListenerModelDescriptorMacroChanges						= 'Mcro',
	kConstantsRegistry_ListenerModelDescriptorPreferences						= 'Pref',
	kConstantsRegistry_ListenerModelDescriptorPreferencePanels					= 'PPnl',
	kConstantsRegistry_ListenerModelDescriptorSessionChanges					= 'Sssn',
	kConstantsRegistry_ListenerModelDescriptorSessionFactoryChanges				= 'SsnF',
	kConstantsRegistry_ListenerModelDescriptorSessionFactoryAnySessionChanges   = 'Ssn*',
	kConstantsRegistry_ListenerModelDescriptorTerminalChanges					= 'Term',
	kConstantsRegistry_ListenerModelDescriptorTerminalSpeakerChanges			= 'Spkr',
	kConstantsRegistry_ListenerModelDescriptorTerminalViewChanges				= 'View',
	kConstantsRegistry_ListenerModelDescriptorTerminalWindowChanges				= 'TWin',
	kConstantsRegistry_ListenerModelDescriptorToolbarChanges					= 'TBar',
	kConstantsRegistry_ListenerModelDescriptorToolbarItemChanges				= 'Tool'
};

//@}

//!\name Special Global Flags
//@{

/*!
Pass one of these to FlagManager_Test() (they are generally
initialized at startup time).
*/
enum
{
	// All of these must be unique.  Note that, thanks to the excellent design
	// of the Flag Manager, there is no real limit as to the number of flags
	// you can define here.  There is also no efficiency concern - it is not
	// necessary to use multiples of 16 or 32 flags, for example.
	kFlagAppleScriptRecording		= (kFlagManager_FirstValidFlag + 0),	//!< is the user recording events into a script?
	kFlagInitializationComplete		= (kFlagManager_FirstValidFlag + 1),	//!< has the application completely finished launching?
	kFlagKioskMode					= (kFlagManager_FirstValidFlag + 2),	//!< is the application in Full Screen mode?
	kFlagOS10_0API					= (kFlagManager_FirstValidFlag + 3),	//!< is Mac OS 10.0 or later in use?
	kFlagOS10_1API					= (kFlagManager_FirstValidFlag + 4),	//!< is Mac OS 10.1 or later in use?
	kFlagOS10_2API					= (kFlagManager_FirstValidFlag + 5),	//!< is Mac OS 10.2 or later in use?
	kFlagOS10_3API					= (kFlagManager_FirstValidFlag + 6),	//!< is Mac OS 10.3 or later in use?
	kFlagOS10_4API					= (kFlagManager_FirstValidFlag + 7),	//!< is Mac OS 10.4 or later in use?
	kFlagOS10_5API					= (kFlagManager_FirstValidFlag + 8),	//!< is Mac OS 10.5 or later in use?
	kFlagOS10_6API					= (kFlagManager_FirstValidFlag + 9),	//!< is Mac OS 10.6 or later in use?
	kFlagQuickTime					= (kFlagManager_FirstValidFlag + 10),	//!< is QuickTime (any version) installed?
	kFlagSuspended					= (kFlagManager_FirstValidFlag + 11),	//!< is MacTelnetÕs process in the background?
	kFlagThreadManager				= (kFlagManager_FirstValidFlag + 12),	//!< is the Thread Manager (any version) installed?
	kFlagUserOverrideAutoNew		= (kFlagManager_FirstValidFlag + 13)	//!< skip auto-new-window on application launch?
};

//@}

//!\name Miscellaneous Constants
//@{

enum
{
	kConstantsRegistry_ApplicationCreatorSignature = FOUR_CHAR_CODE('KevG')
};

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
