/*!	\file MenuResources.h
	\brief Constants describing menu resources, including menu
	bars, menu item command IDs and indices of menus in the main
	menu array.
	
	NOTE:	On Mac OS X, virtually all resources are being
			converted into NIBs as soon as possible.
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

#ifndef __MENURESOURCES__
#define __MENURESOURCES__

/*
*******************************************
	MENUS
*******************************************
*/

// main menus
// These MUST agree with "MainMenuCocoa.xib".  In the Carbon days,
// these were menu IDs.  But for Cocoa, they are the "tag" values
// on each of the menus in the main menu.  So you could ask NSApp
// for "mainMenu", and then use "itemWithTag" with an ID below to
// find an NSMenuItem for the title, whose "submenu" is the NSMenu
// containing all of the items.
#define	kMenuIDApplication			512
#define	kMenuIDFile					513
#define	kMenuIDEdit					514
#define	kMenuIDView					515
#define	kMenuIDTerminal				516
#define	kMenuIDKeys					517
#define	kMenuIDWindow				518
#define	kMenuItemIDPrecedingWindowList		123
#define	kMenuIDDebug				519
#define	kMenuIDScripts				520
#define	kMenuIDContextual			521
#define	kMenuIDMacros				522
// 525 is the Help menu template

#define kMenuIDFirstShared			800

// menus for pop-up buttons and menu bevel buttons
#define kPopUpMenuIDFormatType			160
#define kPopUpMenuIDFont				161
#define kPopUpMenuIDSize				162

// menus created internally by MacTelnet
#define kMenuIDNewSessionsProxies				600
#define kMenuIDNewSessionsTerminals				601
#define kMenuIDPrefsConfigurationsListDummy		602
#define kMenuIDSessionEditorTerminals			603
#define kMenuIDSessionEditorTranslationTables	604
#define kMenuIDTerminalEditorFonts				605
#define kMenuIDCharacterSets					606
#define kMenuIDFormatDialogCharacterSets		607

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
