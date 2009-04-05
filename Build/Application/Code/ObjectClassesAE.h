/*!	\file ObjectClassesAE.h
	\brief Lists all of the Apple Event object classes, and
	defines the (internal) token classes.
	
	See "Terminology.h" for constants describing MacTelnet’s
	classes, all of which begin with "cMy".
	
	IMPORTANT:	Remember, a structure only needs to contain the
				data that is required in order to *find out* the
				information its OSL object representation
				“contains”.  For example, the data structure for
				the OSL window class only contains a Mac OS
				window pointer, because all of the properties of
				the scriptable "window" class (visibility, close
				box, etc.) can be determined using that pointer.
				
				By following this simple rule, you safeguard
				against having to update data in more than one
				place!!!  AppleScript support is supposed to
				*supplement* the application, not burden it!
				NEVER store any “real” data in an OSL object.
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

#ifndef __OBJECTCLASSESAE__
#define __OBJECTCLASSESAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "ConnectionData.h"
#include "MacroManager.h"



#pragma mark Helpers

#define INHERITANCE(x)		x		// used for clarifying intent, nothing more

#pragma mark Constants

typedef UInt32 ObjectClassesAE_TokenFlags;
enum
{
	kObjectClassesAE_TokenFlagsIsObject = (1 << 0),				//!< token represents an Apple Event class instance
	kObjectClassesAE_TokenFlagsIsProperty = (1 << 1),			//!< token represents a property of an Apple Event class
	kObjectClassesAE_TokenFlagsDisposeDataPointer = (1 << 30),	//!< data is dynamic pointer that needs disposing
	kObjectClassesAE_TokenFlagsDisposeDataHandle = (1 << 31)	//!< data is dynamic handle that needs disposing
};

#pragma mark Types

/*!
The data structure used as the token data for
an AppleScript "application" class instance.
*/
struct ObjectClassesAE_Application
{
	ProcessSerialNumber		process;
};

/*!
The data structure used as the token data for
an AppleScript "application preferences" class
instance.
*/
struct ObjectClassesAE_ApplicationPreferences
{
	// (does not need any data)
	int		x;
};

/*!
The data structure used as the token data for
an AppleScript "connection" class instance.
*/
struct ObjectClassesAE_Connection
{
	ConnectionDataPtr	connectionDataPtr; // temporary, deprecated
	SessionRef			session;
};

/*!
The data structure used as the token data for
an AppleScript "dialog reply" class instance.
*/
struct ObjectClassesAE_DialogReply
{
	Str31			buttonName;
	Boolean			gaveUp;
};

/*!
The data structure used as the token data for
an AppleScript "format" class instance.
*/
struct ObjectClassesAE_Format
{
	WindowRef				window;
	//TerminalView_ColorIndex	foregroundScreenColorIndex;
	//TerminalView_ColorIndex	backgroundScreenColorIndex;
};

/*!
The data structure used as the token data for
an AppleScript "macro set" class instance.
*/
struct ObjectClassesAE_MacroSet
{
	// the MacTelnet macro set index (from 1 to MACRO_SET_COUNT)
	UInt16			number;
};

/*!
The data structure used as the token data for
an AppleScript "proxy server" class instance.
*/
struct ObjectClassesAE_ProxyServerConfiguration
{
	UInt8			method;
};

/*!
The data structure used as the token data for
an AppleScript "text" class instance.
*/
struct ObjectClassesAE_Text
{
	WindowRef		window;
	Handle			text;
	SInt32			offset;
	UInt32			length;
};

/*!
The data structure used as the token data for
an AppleScript "window" class instance.
*/
struct ObjectClassesAE_Window
{
	WindowRef		ref;
};

/*!
The data structure used as the token data for an
AppleScript "clipboard window" class instance.
*/
struct ObjectClassesAE_ClipboardWindow
{
	// WARNING: property accessors will upcast this structure to its superclass structure type
	INHERITANCE(ObjectClassesAE_Window		windowClass);
};

/*!
The data structure used as the token data for an
AppleScript "terminal window" class instance.
*/
struct ObjectClassesAE_TerminalWindow
{
	// WARNING: property accessors will upcast this structure to its superclass structure type
	INHERITANCE(ObjectClassesAE_Window  windowClass);
	TerminalWindowRef					ref;
};

/*!
The data structure used as the token data for an
AppleScript "session window" class instance.
*/
struct ObjectClassesAE_SessionWindow
{
	// WARNING: property accessors will upcast this structure to its superclass structure type
	INHERITANCE(ObjectClassesAE_TerminalWindow	terminalWindowClass);
	SessionRef									session;
};

/*!
The data structure used as the token data for
an AppleScript "word" class instance.
*/
struct ObjectClassesAE_Word
{
	ObjectClassesAE_Text		contents;
};

/*!
A basic structure tying a token’s data structure
with its Apple Event descriptor type.
*/
struct ObjectClassesAE_Object
{
	DescType		eventClass;		//!< A "cMy..." constant (see "Terminology.h" for more details).  This value
									//!<   determines what part of the "data" union should be used.
	
	// The following union contains an entry for every single
	// Apple Event object class supported by MacTelnet.  Each
	// object class defines data uniquely distinguishing an
	// object from others of its class.
	//
	// Since MacTelnet implements inheritance by making the
	// first part of a subclass’ data structure a copy of the
	// data structure of its parent class, it is possible to
	// “upcast” a more refined version of a class simply by
	// referencing the parent class in the union "data".  For
	// example, if "eventClass" is cMyTerminalWindow, you can
	// safely refer to either "terminalWindow" or "window" in
	// the union below, because the first element of the
	// ObjectClassesAE_TerminalWindow structure is in fact a
	// ObjectClassesAE_Window structure.  This allows the window
	// property accessor to function without any typecasting,
	// and allows the terminal window property accessor to
	// invoke the accessor of the window class for any property
	// it does not recognize.  This also means that it is not
	// really possible for a class to have more than one
	// *immediate* superclass (so no multiple inheritance), and
	// we all know multiple inheritance is usually evil anyway,
	// so everybody should be happy!
	//
	// The only obvious disadvantage of this approach is that
	// some tokens would be slightly larger (memory-wise) than
	// they need to be, due to the union being sized as large
	// as the biggest structure within it.  This is a small
	// price to pay for the drastic simplification of the code
	// required to handle these objects!
	union
	{
		ObjectClassesAE_Application					application;
		ObjectClassesAE_ApplicationPreferences		applicationPreferences;
		ObjectClassesAE_ClipboardWindow				clipboardWindow;
		ObjectClassesAE_Connection					connection;
		ObjectClassesAE_DialogReply					dialogReply;
		ObjectClassesAE_Format						format;
		ObjectClassesAE_MacroSet					macroSet;
		ObjectClassesAE_ProxyServerConfiguration	proxyServer;
		ObjectClassesAE_SessionWindow				sessionWindow;
		ObjectClassesAE_TerminalWindow				terminalWindow;
		ObjectClassesAE_Text						text;
		ObjectClassesAE_Window						window;
		ObjectClassesAE_Word						word;
		// incomplete
	} data;
};

/*!
The data structure used as the token data for
an AppleScript class property.
*/
struct ObjectClassesAE_Property
{
	DescType				dataType;   //!< a "p..." or "pMy..." constant (see "Terminology.h" for details)
	ObjectClassesAE_Object	container;  //!< the object that contains this property
};

/*!
The data structure that ALL tokens use, when AppleScript
object specifiers are resolved to the "cMyInternalToken"
type.
*/
struct ObjectClassesAE_Token
{
	ObjectClassesAE_TokenFlags	flags;			//!< information about this token
	
	void*						genericPointer;	//!< can be a pointer or a handle; if not nullptr, set the appropriate flag -
												//!  "kObjectClassesAE_TokenFlagsDisposeDataPointer" or
												//!  "kObjectClassesAE_TokenFlagsDisposeDataHandle" - and AEDisposeToken() will then
												//!  dispose of the data for you, using the appropriate Memory Manager routine
												//!  (thanks to the MacTelnet-defined token disposal callback in "InstallAE.cp")
	
	// a token is either an object or a property
	union
	{
		ObjectClassesAE_Object		object;
		ObjectClassesAE_Property	property;
	} as;
};

typedef ObjectClassesAE_Application*				ObjectClassesAE_ApplicationPtr;
typedef ObjectClassesAE_ApplicationPreferences*		ObjectClassesAE_ApplicationPreferencesPtr;
typedef ObjectClassesAE_ClipboardWindow*			ObjectClassesAE_ClipboardWindowPtr;
typedef ObjectClassesAE_Connection*					ObjectClassesAE_ConnectionPtr;
typedef ObjectClassesAE_DialogReply*				ObjectClassesAE_DialogReplyPtr;
typedef ObjectClassesAE_Format*						ObjectClassesAE_FormatPtr;
typedef ObjectClassesAE_MacroSet*					ObjectClassesAE_MacroSetPtr;
typedef ObjectClassesAE_ProxyServerConfiguration*	ObjectClassesAE_ProxyServerConfigurationPtr;
typedef ObjectClassesAE_SessionWindow*				ObjectClassesAE_SessionWindowPtr;
typedef ObjectClassesAE_TerminalWindow*				ObjectClassesAE_TerminalWindowPtr;
typedef ObjectClassesAE_Text*						ObjectClassesAE_TextPtr;
typedef ObjectClassesAE_Window*						ObjectClassesAE_WindowPtr;
typedef ObjectClassesAE_Word*						ObjectClassesAE_WordPtr;
typedef ObjectClassesAE_Object*						ObjectClassesAE_ObjectPtr;
typedef ObjectClassesAE_Property*					ObjectClassesAE_PropertyPtr;
typedef ObjectClassesAE_Token*						ObjectClassesAE_TokenPtr;

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
