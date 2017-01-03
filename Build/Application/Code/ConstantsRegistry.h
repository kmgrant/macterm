/*!	\file ConstantsRegistry.h
	\brief Collection of enumerations that must be unique
	across the application.
	
	This file contains collections of constants that must be
	unique within their set.  With all constants defined in one
	place, it is easy to see at a glance if there are any
	values that are not unique!
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#include <UniversalDefines.h>

#ifndef __CONSTANTSREGISTRY__
#define __CONSTANTSREGISTRY__

// library includes
#ifdef __OBJC__
#	include <CocoaFuture.objc++.h>
#endif
#include <FlagManager.h>
#include <ListenerModel.h>



//!\name HIView-Related Constants
//@{

/*!
Control kinds that use AppResources_ReturnCreatorCode()
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
	// use AppResources_ReturnCreatorCode() to set the creator code of these properties
	kConstantsRegistry_ControlPropertyTypeBackgroundColor			= 'BkCl',	//!< data: CGDeviceColor
	kConstantsRegistry_ControlPropertyTypeShowDragHighlight			= 'Drag',	//!< data: Boolean (if true, draw highlight; if false; erase)
	kConstantsRegistry_ControlPropertyTypeTerminalBackgroundData	= 'TrmB',	//!< data: MyTerminalBackgroundPtr (internal to TerminalView.cp)
	kConstantsRegistry_ControlPropertyTypeTerminalViewRef			= 'TrmV',	//!< data: TerminalViewRef
	kConstantsRegistry_ControlPropertyTypeTerminalWindowRef			= 'TrmW',	//!< data: TerminalWindowRef
};

/*!
Used in Cocoa with NSError, to specify a domain for all
application-generated errors.  The enumeration that follows
is for error codes.

For example:
	[NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
		code:kConstantsRegistry_NSErrorBadUserID . . .]
*/
CFStringRef const kConstantsRegistry_NSErrorDomainAppDefault				= CFSTR("net.macterm.errors");
enum
{
	kConstantsRegistry_NSErrorBadPortNumber		= 1,
	kConstantsRegistry_NSErrorBadUserID			= 2,
	kConstantsRegistry_NSErrorBadNumber			= 3, // generic
	kConstantsRegistry_NSErrorBadArray			= 4, // generic
	kConstantsRegistry_NSErrorResourceNotFound	= 5, // generic
};

/*!
Panel descriptors, which help to identify panels when given
only an opaque reference to one.  See "Panel.h".

The order here is not important, though each panel needs a
unique value.

Note that these strings are also used as HIObject IDs for
toolbar items.
*/
CFStringRef const kConstantsRegistry_PrefPanelDescriptorGeneral				= CFSTR("net.macterm.prefpanels.general");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorSessions			= CFSTR("net.macterm.prefpanels.sessions");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorSessionDataFlow		= CFSTR("net.macterm.prefpanels.sessions.dataflow");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorSessionKeyboard		= CFSTR("net.macterm.prefpanels.sessions.keyboard");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorSessionGraphics		= CFSTR("net.macterm.prefpanels.sessions.graphics");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorSessionResource		= CFSTR("net.macterm.prefpanels.sessions.resource");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorMacros				= CFSTR("net.macterm.prefpanels.macros");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTerminals			= CFSTR("net.macterm.prefpanels.terminals");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTerminalsEmulation	= CFSTR("net.macterm.prefpanels.terminals.emulation");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTerminalsHacks		= CFSTR("net.macterm.prefpanels.terminals.hacks");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTerminalsOptions	= CFSTR("net.macterm.prefpanels.terminals.options");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTerminalsScreen		= CFSTR("net.macterm.prefpanels.terminals.screen");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorFormats				= CFSTR("net.macterm.prefpanels.formats");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorFormatsANSI			= CFSTR("net.macterm.prefpanels.formats.ansicolors");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorFormatsNormal		= CFSTR("net.macterm.prefpanels.formats.normal");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorKiosk				= CFSTR("net.macterm.prefpanels.kiosk");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorScripts				= CFSTR("net.macterm.prefpanels.scripts");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorTranslations		= CFSTR("net.macterm.prefpanels.translations");
CFStringRef const kConstantsRegistry_PrefPanelDescriptorWorkspaces			= CFSTR("net.macterm.prefpanels.workspaces");

/*!
Used with HIObject routines (such as HIObjectCreate()) to
implement custom user interface elements.
*/
CFStringRef const kConstantsRegistry_HIObjectClassIDTerminalBackgroundView	= CFSTR("net.macterm.terminal.background");
CFStringRef const kConstantsRegistry_HIObjectClassIDTerminalTextView		= CFSTR("net.macterm.terminal.text");
// IMPORTANT: Since old toolbar item identifiers might be referenced by user preferences,
// these cannot migrate to a new domain name until the associated preferences can be
// properly converted.  Of course, a transition to Cocoa-based toolbars has the same
// issue, so maybe it is unavoidable.
CFStringRef const kConstantsRegistry_HIToolbarIDPreferences					= CFSTR("com.mactelnet.toolbar.preferences");
CFStringRef const kConstantsRegistry_HIToolbarIDSessionInfo					= CFSTR("com.mactelnet.toolbar.sessioninfo");
CFStringRef const kConstantsRegistry_HIToolbarIDTerminal					= CFSTR("com.mactelnet.toolbar.terminal");
CFStringRef const kConstantsRegistry_HIToolbarIDTerminalFloater				= CFSTR("com.mactelnet.toolbar.terminalfloater");
CFStringRef const kConstantsRegistry_HIToolbarItemIDCollections				= CFSTR("com.mactelnet.toolbaritem.collections");
CFStringRef const kConstantsRegistry_HIToolbarItemIDCustomize				= CFSTR("com.mactelnet.toolbaritem.customize");
CFStringRef const kConstantsRegistry_HIToolbarItemIDFullScreen				= CFSTR("com.mactelnet.toolbaritem.fullscreen");
CFStringRef const kConstantsRegistry_HIToolbarItemIDHideWindow				= CFSTR("com.mactelnet.toolbaritem.hidewindow");
CFStringRef const kConstantsRegistry_HIToolbarItemIDNewDefaultSession		= CFSTR("com.mactelnet.toolbaritem.newsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDNewLoginSession			= CFSTR("com.mactelnet.toolbaritem.newloginsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDNewShellSession			= CFSTR("com.mactelnet.toolbaritem.newshellsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDPrint					= CFSTR("com.mactelnet.toolbaritem.print");
CFStringRef const kConstantsRegistry_HIToolbarItemIDRestartSession			= CFSTR("com.mactelnet.toolbaritem.restartsession");
CFStringRef const kConstantsRegistry_HIToolbarItemIDScrollLock				= CFSTR("com.mactelnet.toolbaritem.scrolllock");
CFStringRef const kConstantsRegistry_HIToolbarItemIDStackWindows			= CFSTR("com.mactelnet.toolbaritem.stackwindows");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalBell			= CFSTR("com.mactelnet.toolbaritem.terminalbell");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED1			= CFSTR("com.mactelnet.toolbaritem.terminalled1");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED2			= CFSTR("com.mactelnet.toolbaritem.terminalled2");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED3			= CFSTR("com.mactelnet.toolbaritem.terminalled3");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalLED4			= CFSTR("com.mactelnet.toolbaritem.terminalled4");
CFStringRef const kConstantsRegistry_HIToolbarItemIDTerminalSearch			= CFSTR("com.mactelnet.toolbaritem.terminalsearch");

/*!
Used when registering icons with Icon Services.
These must all be unique within the application.
*/
enum
{
	// use AppResources_ReturnCreatorCode() to set the creator of these icons
	kConstantsRegistry_IconServicesIconApplication				= 'AppI',
	kConstantsRegistry_IconServicesIconContextMenu				= 'CtxM',
	kConstantsRegistry_IconServicesIconFileTypeMacroSet			= 'PMcS',
	kConstantsRegistry_IconServicesIconItemAdd					= 'AddI',
	kConstantsRegistry_IconServicesIconItemRemove				= 'DelI',
	kConstantsRegistry_IconServicesIconKeypadArrowDown			= 'KPAD',
	kConstantsRegistry_IconServicesIconKeypadArrowLeft			= 'KPAL',
	kConstantsRegistry_IconServicesIconKeypadArrowRight			= 'KPAR',
	kConstantsRegistry_IconServicesIconKeypadArrowUp			= 'KPAU',
	kConstantsRegistry_IconServicesIconKeypadDelete				= 'KPDl',
	kConstantsRegistry_IconServicesIconKeypadEnter				= 'KPEn',
	kConstantsRegistry_IconServicesIconKeypadFind				= 'KPFn',
	kConstantsRegistry_IconServicesIconKeypadInsert				= 'KPIn',
	kConstantsRegistry_IconServicesIconKeypadPageDown			= 'KPPD',
	kConstantsRegistry_IconServicesIconKeypadPageUp				= 'KPPU',
	kConstantsRegistry_IconServicesIconKeypadSelect				= 'KPSl',
	kConstantsRegistry_IconServicesIconMenuTitleScripts			= 'ScpM',
	kConstantsRegistry_IconServicesIconPreferenceCollection		= 'PrfC',
	kConstantsRegistry_IconServicesIconPrefPanelFormats			= 'PFmt',
	kConstantsRegistry_IconServicesIconPrefPanelFullScreen		= 'PKio',
	kConstantsRegistry_IconServicesIconPrefPanelGeneral			= 'PGen',
	kConstantsRegistry_IconServicesIconPrefPanelMacros			= 'PMcr',
	kConstantsRegistry_IconServicesIconPrefPanelScripts			= 'PScr',
	kConstantsRegistry_IconServicesIconPrefPanelSessions		= 'PSsn',
	kConstantsRegistry_IconServicesIconPrefPanelTerminals		= 'PTrm',
	kConstantsRegistry_IconServicesIconPrefPanelTranslations	= 'PXlt',
	kConstantsRegistry_IconServicesIconPrefPanelWorkspaces		= 'PWsp',
	kConstantsRegistry_IconServicesIconSessionStatusActive		= 'Actv',
	kConstantsRegistry_IconServicesIconSessionStatusDead		= 'Dead',
	kConstantsRegistry_IconServicesIconToolbarItemBellOff		= 'BelO',
	kConstantsRegistry_IconServicesIconToolbarItemBellOn		= 'BelI',
	kConstantsRegistry_IconServicesIconToolbarItemCustomize		= 'CTlb',
	kConstantsRegistry_IconServicesIconToolbarItemFullScreen	= 'Kios',
	kConstantsRegistry_IconServicesIconToolbarItemHideWindow	= 'Hide',
	kConstantsRegistry_IconServicesIconToolbarItemKillSession	= 'Kill',
	kConstantsRegistry_IconServicesIconToolbarItemLEDOff		= 'LEDO',
	kConstantsRegistry_IconServicesIconToolbarItemLEDOn			= 'LEDI',
	kConstantsRegistry_IconServicesIconToolbarItemPrint			= 'Prnt',
	kConstantsRegistry_IconServicesIconToolbarItemRestartSession= 'RSsn',
	kConstantsRegistry_IconServicesIconToolbarItemScrollLockOff	= 'XON ',
	kConstantsRegistry_IconServicesIconToolbarItemScrollLockOn	= 'XOFF',
	kConstantsRegistry_IconServicesIconToolbarItemStackWindows	= 'StkW'
};

//@}

#ifdef __OBJC__
//!\name Touch Bar Identifiers
//@{

extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDApplicationMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDInfoWindowMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDPrefsWindowMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDTerminalWindowMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDVectorWindowMain;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDFind;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDFullScreen;

//@}
#endif

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
abstractly, “listeners” for events, and then notify the
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
	kConstantsRegistry_ListenerModelDescriptorNetwork							= 'Netw',
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
	kFlagSuspended					= (kFlagManager_FirstValidFlag + 13),	//!< is this application’s process in the background?
	kFlagUserOverrideAutoNew		= (kFlagManager_FirstValidFlag + 14)	//!< skip auto-new-window on application launch?
};

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
