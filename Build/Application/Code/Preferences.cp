/*###############################################################

	Preferences.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
		© 2001-2004 by Ian Anderson.
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

// standard-C++ includes
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

// pseudo-standard-C++ includes
#if __MWERKS__
#   include <hash_fun>
#   include <hash_map>
#   define hash_map_namespace Metrowerks
#elif (__GNUC__ > 3)
#   include <ext/hash_map>
#   define hash_map_namespace __gnu_cxx
#elif (__GNUC__ == 3)
#   include <ext/stl_hash_fun.h>
#   include <ext/hash_map>
#   define hash_map_namespace __gnu_cxx
#elif (__GNUC__ < 3)
#   include <stl_hash_fun.h>
#   include <hash_map>
#   define hash_map_namespace
#else
#   include <hash_fun>
#   include <hash_map>
#   define hash_map_namespace
#endif

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CFKeyValueInterface.h>
#include <CFUtilities.h>
#include <CocoaUserDefaults.h>
#include <Console.h>
#include <ListenerModel.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlockReferenceLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <WindowInfo.h>

// resource includes
#include "ApplicationVersion.h"
#include "GeneralResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Clipboard.h"
#include "Commands.h"
#include "Folder.h"
#include "MacroManager.h"
#include "NetEvents.h"
#include "Preferences.h"
#include "Session.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "UIStrings.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

CFStringEncoding const		kMy_SavedNameEncoding = kCFStringEncodingUnicode;
CFStringRef const			kMy_PreferencesSubDomainFormats = CFSTR("com.mactelnet.MacTelnet.formats");
CFStringRef const			kMy_PreferencesSubDomainMacros = CFSTR("com.mactelnet.MacTelnet.macros");
CFStringRef const			kMy_PreferencesSubDomainSessions = CFSTR("com.mactelnet.MacTelnet.sessions");
CFStringRef const			kMy_PreferencesSubDomainTerminals = CFSTR("com.mactelnet.MacTelnet.terminals");
CFStringRef const			kMy_PreferencesSubDomainTranslations = CFSTR("com.mactelnet.MacTelnet.translations");

} // anonymous namespace

#pragma mark Functors Used in Type Definitions
namespace {

/*!
This is a functor that determines if a pair of
Core Foundation strings is identical.

Model of STL Binary Predicate.

(1.0)
*/
#pragma mark isCFStringPairEqual
class isCFStringPairEqual:
public std::binary_function< CFStringRef/* argument 1 */, CFStringRef/* argument 2 */, bool/* return */ >
{
public:
	bool
	operator()	(CFStringRef	inString1,
				 CFStringRef	inString2)
	const
	{
		return (kCFCompareEqualTo == CFStringCompare(inString1, inString2, 0/* compare options */));
	}

protected:

private:
};

/*!
This is a functor that determines the hash code of
a Core Foundation string.

Model of STL Unary Function.

(1.0)
*/
#pragma mark returnCFStringHashCode
class returnCFStringHashCode:
public std::unary_function< CFStringRef/* argument */, size_t/* return */ >
{
public:
	size_t
	operator()	(CFStringRef	inString)
	const
	{
		return STATIC_CAST(CFHash(inString), size_t);
	}

protected:

private:
};

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Information about a file on disk.  This is a typical
way to store information about files within preferences,
since files can still be found even if the user renames
them or moves them, etc.
*/
struct My_AliasInfo
{
	AliasHandle				alias;
	Preferences_AliasID		resourceID;
	Boolean					wasChanged;
	Boolean					isResource;
};
typedef My_AliasInfo const*		My_AliasInfoConstPtr;
typedef My_AliasInfo*			My_AliasInfoPtr;

/*!
Keeps track of all aliases in memory.
*/
typedef std::vector< My_AliasInfoPtr >	My_AliasInfoList;

/*!
Provides uniform access to context information no
matter how it is really stored.

Monitors are automatically supported at this level,
so changing any value will trigger the appropriate
change notification.
*/
class My_ContextInterface
{
public:
	virtual
	~My_ContextInterface ();
	
	Preferences_ContextRef		selfRef;	//!< convenient, redundant self-reference
	
	//! inserts array value into dictionary
	void
	addArray	(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 CFArrayRef			inValue)
	{
		_implementorPtr->addArray(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! inserts data value into dictionary
	void
	addData		(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 CFDataRef			inValue)
	{
		_implementorPtr->addData(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! inserts true/false value into dictionary
	void
	addFlag		(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 Boolean			inValue)
	{
		_implementorPtr->addFlag(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! inserts floating-point value into dictionary
	void
	addFloat	(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 Float32			inValue)
	{
		_implementorPtr->addFloat(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! inserts short integer value into dictionary
	void
	addInteger	(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 SInt16				inValue)
	{
		_implementorPtr->addInteger(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! manages callbacks that are invoked as changes are made
	Boolean
	addListener		(ListenerModel_ListenerRef,
					 Preferences_Change,
					 Boolean);
	
	//! inserts short integer value into dictionary
	void
	addLong		(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 SInt32				inValue)
	{
		_implementorPtr->addLong(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! inserts string value into dictionary
	void
	addString	(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 CFStringRef		inValue)
	{
		_implementorPtr->addString(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! inserts arbitrary value into dictionary
	void
	addValue	(Preferences_Tag	inTag,
				 CFStringRef		inKey,
				 CFPropertyListRef	inValue)
	{
		_implementorPtr->addValue(inKey, inValue);
		notifyListeners(inTag);
	}
	
	//! removes an arbitrary value from the dictionary
	virtual void
	deleteValue		(CFStringRef	inKey)
	{
		_implementorPtr->deleteValue(inKey);
	}
	
	//! delete this key-value set from application preferences
	virtual Preferences_Result
	destroy () NO_METHOD_IMPL = 0;
	
	//! invokes callbacks for an event; usually done automatically
	void
	notifyListeners	(Preferences_Tag);
	
	//! manages callbacks that are invoked as changes are made
	Boolean
	removeListener	(ListenerModel_ListenerRef,
					 Preferences_Change);
	
	//! alter the name under which this is saved; useful in UI elements
	virtual Preferences_Result
	rename	(CFStringRef) NO_METHOD_IMPL = 0;
	
	//! retrieves an array value from the dictionary (use only if the value really is an array!)
	virtual CFArrayRef
	returnArrayCopy		(CFStringRef	inKey) const
	{
		return _implementorPtr->returnArrayCopy(inKey);
	}
	
	//! the category of this context
	inline Preferences_Class
	returnClass () const;
	
	//! retrieves a true or false value from the dictionary (use only if the value really is a Boolean!)
	virtual Boolean
	returnFlag		(CFStringRef	inKey) const
	{
		return _implementorPtr->returnFlag(inKey);
	}
	
	//! retrieves a short integer value from the dictionary (use only if the value really is a number!)
	virtual SInt16
	returnInteger	(CFStringRef	inKey) const
	{
		return _implementorPtr->returnInteger(inKey);
	}
	
	//! retrieves a floating-point value from the dictionary (use only if the value really is a number!)
	virtual Float32
	returnFloat		(CFStringRef	inKey) const
	{
		return _implementorPtr->returnFloat(inKey);
	}
	
	//! creates an array of CFStringRef values for each key used in this context
	CFArrayRef
	returnKeyListCopy () const
	{
		return _implementorPtr->returnKeyListCopy();
	}
	
	//! retrieves a long integer value from the dictionary (use only if the value really is a number!)
	virtual SInt32
	returnLong		(CFStringRef	inKey) const
	{
		return _implementorPtr->returnLong(inKey);
	}
	
	//! the name under which this is saved; useful in UI elements
	virtual CFStringRef
	returnName () const NO_METHOD_IMPL = 0;
	
	//! retrieves a string value from the dictionary (use only if the value really is a string!)
	virtual CFStringRef
	returnStringCopy	(CFStringRef	inKey) const
	{
		return _implementorPtr->returnStringCopy(inKey);
	}
	
	//! retrieves an arbitrary value from the dictionary
	virtual CFPropertyListRef
	returnValueCopy		(CFStringRef	inKey) const
	{
		return _implementorPtr->returnValueCopy(inKey);
	}
	
	//! save changes to this key-value set in the application preferences
	virtual Preferences_Result
	save () NO_METHOD_IMPL = 0;
	
	//! test routine
	static Boolean
	unitTest	(My_ContextInterface*);

protected:
	My_ContextInterface	(Preferences_Class);
	
	void
	setImplementor	(CFKeyValueInterface*);

private:
	Preferences_Class		_preferencesClass;	//!< hint as to what keys are likely to be present
	ListenerModel_Ref		_listenerModel;		//!< if monitors are used, handles change notifications
	CFKeyValueInterface*	_implementorPtr;	//!< how settings are saved (e.g. application preferences, an in-memory dictionary...)
};
typedef My_ContextInterface const*	My_ContextInterfaceConstPtr;
typedef My_ContextInterface*		My_ContextInterfacePtr;

typedef MemoryBlockPtrLocker< Preferences_ContextRef, My_ContextInterface >			My_ContextPtrLocker;
typedef LockAcquireRelease< Preferences_ContextRef, My_ContextInterface >			My_ContextAutoLocker;
typedef MemoryBlockReferenceLocker< Preferences_ContextRef, My_ContextInterface >	My_ContextReferenceLocker;

/*!
A context specifically for storing in a CFDictionary.
*/
class My_ContextCFDictionary:
public My_ContextInterface
{
public:
	My_ContextCFDictionary	(Preferences_Class, CFDictionaryRef);
	
	My_ContextCFDictionary	(Preferences_Class, CFMutableDictionaryRef = nullptr);
	
	//!\name New Methods In This Class
	//@{
	
	//! test routine
	static Boolean
	unitTest ();
	
	//@}
	
	//!\name My_ContextInterface Methods
	//@{
	
	//! has no effect because this context never saves anything to disk
	Preferences_Result
	destroy ();
	
	//! not allowed for this type of context
	Preferences_Result
	rename	(CFStringRef);
	
	//! always an empty string
	CFStringRef
	returnName () const;
	
	//! not allowed for this type of context
	Preferences_Result
	save ();
	
	//@}

protected:
	CFMutableDictionaryRef
	createDictionary ();

private:
	CFKeyValueDictionary	_dictionary;	//!< handles key value lookups
};
typedef My_ContextCFDictionary const*	My_ContextCFDictionaryConstPtr;
typedef My_ContextCFDictionary*			My_ContextCFDictionaryPtr;

/*!
A context specifically for storing defaults.  It
doesn’t actually manage a dictionary, it uses the
Core Foundation Preferences APIs instead; though,
the consistency of this API compared to that of
other contexts is useful.

See also My_ContextFavorite, which manages
collections of preferences in sub-domains.
*/
class My_ContextDefault:
public My_ContextInterface
{
public:
	My_ContextDefault	(Preferences_Class);
	
	//!\name New Methods In This Class
	//@{
	
	//! test routine
	static Boolean
	unitTest ();
	
	//@}
	
	//!\name My_ContextInterface Methods
	//@{
	
	//! remove values for all keys in this dictionary from application preferences
	Preferences_Result
	destroy ();
	
	//! has no effect; a default context cannot be renamed
	Preferences_Result
	rename	(CFStringRef);
	
	//! the name under which this is saved; useful in UI elements
	CFStringRef
	returnName () const;
	
	//! synchronize application’s Core Foundation preferences
	Preferences_Result
	save ();
	
	//@}

protected:

private:
	CFRetainRelease			_contextName;	//!< CFStringRef; a display name for this context
	CFKeyValuePreferences	_dictionary;	//!< handles key value lookups
};
typedef My_ContextDefault const*	My_ContextDefaultConstPtr;
typedef My_ContextDefault*			My_ContextDefaultPtr;

/*!
A context that automatically chooses a sub-domain in which
to store and retrieve values via CFPreferences.

See also My_ContextDefault, which manages core CFPreferences
in the root application domain.
*/
class My_ContextFavorite:
public My_ContextInterface
{
public:
	My_ContextFavorite	(Preferences_Class, CFStringRef, CFStringRef = nullptr);
	
	//!\name New Methods In This Class
	//@{
	
	//! returns the common prefix for any domain of a collection in the given class
	static CFStringRef
	returnClassDomainNamePrefix	(Preferences_Class);
	
	//! rearrange this context relative to another context
	Preferences_Result
	shift	(My_ContextFavorite*, Boolean = false);
	
	//! test routine
	static Boolean
	unitTest	(Preferences_Class, CFStringRef);
	
	//@}
	
	//!\name My_ContextInterface Methods
	//@{
	
	//! remove values for all keys in this dictionary from application preferences
	Preferences_Result
	destroy ();
	
	//! has no effect; a default context cannot be renamed
	Preferences_Result
	rename	(CFStringRef);
	
	//! the name under which this is saved; useful in UI elements
	CFStringRef
	returnName () const;
	
	//! synchronize application’s Core Foundation preferences
	Preferences_Result
	save ();
	
	//@}

protected:
	CFStringRef
	createDomainName	(Preferences_Class, CFStringRef);

private:
	CFRetainRelease			_contextName;	//!< CFStringRef; a display name for this context
	CFRetainRelease			_domainName;	//!< CFStringRef; an arbitrary but class-based sub-domain for settings
	CFKeyValuePreferences	_dictionary;	//!< handles key value lookups in the chosen sub-domain
};
typedef My_ContextFavorite const*	My_ContextFavoriteConstPtr;
typedef My_ContextFavorite*			My_ContextFavoritePtr;

/*!
Keeps track of all named contexts in memory.
*/
typedef std::vector< My_ContextFavoritePtr >	My_FavoriteContextList;

/*!
This class can be used to define new preferences, and
automatically cache them in multiple hash tables for
efficient lookups by various attributes (such as tag).
*/
class My_PreferenceDefinition
{
public:
	static void
	create		(Preferences_Tag, CFStringRef, FourCharCode, size_t,
				 Preferences_Class, My_PreferenceDefinition** = nullptr);
	
	static void
	createFlag	(Preferences_Tag, CFStringRef, Preferences_Class,
				 My_PreferenceDefinition** = nullptr);
	
	static void
	createIndexed	(Preferences_Tag, Preferences_Index, CFStringRef, FourCharCode,
					 size_t, Preferences_Class);
	
	static void
	createRGBColor	(Preferences_Tag, CFStringRef, Preferences_Class,
					 My_PreferenceDefinition** = nullptr);
	
	static My_PreferenceDefinition*
	findByKeyName	(CFStringRef);
	
	static My_PreferenceDefinition*
	findByTag	(Preferences_Tag);
	
	Preferences_Tag		tag;						//!< tag that describes this setting
	CFRetainRelease		keyName;					//!< key used to store this in XML or CFPreferences
	FourCharCode		keyValueType;				//!< property list type of key (e.g. typeCFArrayRef)
	size_t				nonDictionaryValueSize;		//!< bytes required for data buffers that read/write this value
	Preferences_Class	preferenceClass;			//!< the class of context that this tag is generally used in
	
protected:
	My_PreferenceDefinition		(Preferences_Tag, CFStringRef, FourCharCode, size_t, Preferences_Class);

private:
	typedef std::map< Preferences_Tag, class My_PreferenceDefinition* >		DefinitionPtrByTag;
	typedef hash_map_namespace::hash_map
			<
				CFStringRef,						// key type - name string
				class My_PreferenceDefinition*,		// value type - preference setting data pointer
				returnCFStringHashCode,				// hash code generator
				isCFStringPairEqual					// key comparator
			>	DefinitionPtrByKeyName;
	
	static DefinitionPtrByKeyName	_definitionsByKeyName;
	static DefinitionPtrByTag		_definitionsByTag;
};
My_PreferenceDefinition::DefinitionPtrByKeyName		My_PreferenceDefinition::_definitionsByKeyName;
My_PreferenceDefinition::DefinitionPtrByTag			My_PreferenceDefinition::_definitionsByTag;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Preferences_Result		assertInitialized						();
void					changeNotify							(Preferences_Change, Preferences_ContextRef = nullptr,
																 Boolean = false);
Preferences_Result		contextGetData							(My_ContextInterfacePtr, Preferences_Class, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					convertCFArrayToRGBColor				(CFArrayRef, RGBColor*);
Boolean					convertRGBColorToCFArray				(RGBColor const*, CFArrayRef&);
Preferences_Result		copyClassDomainCFArray					(Preferences_Class, CFArrayRef&);
Preferences_Result		createAllPreferencesContextsFromDisk	();
CFDictionaryRef			createDefaultPrefDictionary				();
CFStringRef				createKeyAtIndex						(CFStringRef, UInt32);
void					deleteAliasData							(My_AliasInfoPtr*);
void					deleteAllAliasNodes						();
My_AliasInfoPtr			findAlias								(Preferences_AliasID);
Boolean					findAliasOnDisk							(Preferences_AliasID, AliasHandle*);
CFIndex					findDomainIndexInArray					(CFArrayRef, CFStringRef);
CFStringRef				findDomainUserSpecifiedName				(CFStringRef);
Boolean					getDefaultContext						(Preferences_Class, My_ContextInterfacePtr&);
Boolean					getFactoryDefaultsContext				(My_ContextInterfacePtr&);
Preferences_Result		getFormatPreference						(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getGeneralPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getListOfContexts						(Preferences_Class, My_FavoriteContextList*&);
Preferences_Result		getMacroPreference						(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getNamedContext							(Preferences_Class, CFStringRef,
																 My_ContextFavoritePtr&);
Preferences_Result		getPreferenceDataInfo					(Preferences_Tag, CFStringRef&,
																 FourCharCode&, size_t&, Preferences_Class&);
Preferences_Result		getSessionPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getTerminalPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getTranslationPreference				(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
OSStatus				mergeInDefaultPreferences				();
Preferences_Result		overwriteClassDomainCFArray				(Preferences_Class, CFArrayRef);
void					readMacTelnetCoordPreference			(CFStringRef, SInt16&, SInt16&);
void					readMacTelnetArrayPreference			(CFStringRef, CFArrayRef&);
OSStatus				readPreferencesDictionary				(CFDictionaryRef, Boolean);
OSStatus				readPreferencesDictionaryInContext		(My_ContextInterfacePtr, CFDictionaryRef, Boolean);
OSStatus				setAliasChanged							(My_AliasInfoPtr);
Preferences_Result		setFormatPreference						(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setGeneralPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setMacroPreference						(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Boolean					setMacTelnetCoordPreference				(CFStringRef, SInt16, SInt16);
void					setMacTelnetPreference					(CFStringRef, CFPropertyListRef);
Preferences_Result		setSessionPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setTerminalPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setTranslationPreference				(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
CFStringRef				virtualKeyCreateName					(UInt16);
Boolean					virtualKeyParseName						(CFStringRef, UInt16&, Boolean&);
Boolean					unitTest000_Begin						();
Boolean					unitTest001_Begin						();
Boolean					unitTest002_Begin						();
Boolean					unitTest003_Begin						();

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_Ref			gPreferenceEventListenerModel = nullptr;
Boolean						gHaveRunConverter = false;
Boolean						gInitialized = false;
My_AliasInfoList&			gAliasList ()	{ static My_AliasInfoList x; return x; }
My_ContextPtrLocker&		gMyContextPtrLocks ()	{ static My_ContextPtrLocker x; return x; }
My_ContextReferenceLocker&	gMyContextRefLocks ()	{ static My_ContextReferenceLocker x; return x; }
My_ContextInterface&		gFactoryDefaultsContext ()	{ static My_ContextCFDictionary x(kPreferences_ClassFactoryDefaults, createDefaultPrefDictionary()); return x; }
My_ContextInterface&		gGeneralDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassGeneral); return x; }
My_ContextInterface&		gFormatDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassFormat); return x; }
My_FavoriteContextList&		gFormatNamedContexts ()		{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gMacroSetDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassMacroSet); return x; }
My_FavoriteContextList&		gMacroSetNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gSessionDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassSession); return x; }
My_FavoriteContextList&		gSessionNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gTerminalDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassTerminal); return x; }
My_FavoriteContextList&		gTerminalNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gTranslationDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassTranslation); return x; }
My_FavoriteContextList&		gTranslationNamedContexts ()	{ static My_FavoriteContextList x; return x; }

} // anonymous namespace

#pragma mark Functors
namespace {

/*!
Returns true only if the specified Alias Info record
has a resource ID matching the given ID.

Model of STL Predicate.

(3.0)
*/
class aliasInfoResourceIDMatches:
public std::unary_function< My_AliasInfoPtr/* argument */, bool/* return */ >
{
public:
	aliasInfoResourceIDMatches	(SInt16	inResourceID)
	: _matchID(inResourceID) {}
	
	bool
	operator ()	(My_AliasInfoPtr	inAliasInfoPtr)
	{
		return (inAliasInfoPtr->resourceID == _matchID);
	}

protected:

private:
	SInt16		_matchID;
};

/*!
Returns true only if the specified context has a name
equal to the given name.

Model of STL Predicate.

(3.1)
*/
class contextNameEqualTo:
public std::unary_function< My_ContextInterfacePtr/* argument */, bool/* return */ >
{
public:
	contextNameEqualTo	(CFStringRef	inName)
	: _name(inName) {}
	
	bool
	operator ()	(My_ContextInterfacePtr		inContextPtr)
	{
		return (kCFCompareEqualTo == CFStringCompare
										(inContextPtr->returnName(), _name.returnCFStringRef(), 0/* options */));
	}

protected:

private:
	CFRetainRelease		_name;
};

/*!
Calls save() on a context.

(3.1)
*/
class contextSave:
public std::unary_function< My_ContextInterfacePtr/* argument */, void/* return */ >
{
public:
	contextSave () {}
	
	void
	operator ()	(My_ContextInterfacePtr		inContextPtr)
	{
		inContextPtr->save();
	}

protected:

private:
};

} // anonymous namespace


#pragma mark Public Methods

/*!
Initializes this module, launching the MacTelnet
Preferences Converter application if necessary.

WARNING:	This is called automatically as needed;
			therefore, its implementation should
			not invoke any Preferences module routine
			that would recurse back into this code.

\retval kPreferences_ResultOK
if the module was initialized completely

\retval kPreferences_ResultNotInitialized
if any initialization problems occurred

(3.0)
*/
Preferences_Result
Preferences_Init ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// If MacTelnetPrefsConverter runs and succeeds, it will write a
	// preferences version value to MacTelnet’s core preferences;
	// if that value is nonexistent or is not what MacTelnet expects,
	// MacTelnetPrefsConverter will be launched to update the user’s
	// preferences to the new format.  Basically, this code ensures
	// the converter is run whenever it is appropriate to do so, and
	// not otherwise.
	unless (gHaveRunConverter)
	{
		CFDictionaryRef		defaultPrefDictionary = createDefaultPrefDictionary();
		SInt16				currentPrefsVersion = 2; // only a default...
		
		
		// The "prefs-version" key in DefaultPreferences.plist defines the
		// version of preferences shipped with this version of MacTelnet.
		if (nullptr != defaultPrefDictionary)
		{
			CFNumberRef		numberRef = CFUtilities_NumberCast(CFDictionaryGetValue(defaultPrefDictionary,
																					CFSTR("prefs-version")));
			
			
			if (nullptr != numberRef)
			{
				Boolean		gotNumber = false;
				
				
				gotNumber = CFNumberGetValue(numberRef, kCFNumberSInt16Type, &currentPrefsVersion);
				assert(gotNumber);
			}
			CFRelease(defaultPrefDictionary), defaultPrefDictionary = nullptr;
		}
		
		// Whenever the preferences for this version of MacTelnet are
		// newer than what is saved for the user, run the converter.
		{
			SInt16 const	kCurrentPrefsVersion = currentPrefsVersion;
			CFIndex			diskVersion = 0;
			Boolean			existsAndIsValidFormat = false;
			
			
			// The preference key used here MUST match what MacTelnetPrefsConverter
			// uses to write the value, for obvious reasons.  Similarly, the
			// application domain must match what MacTelnetPrefsConverter uses.
			diskVersion = CFPreferencesGetAppIntegerValue
							(CFSTR("prefs-version"), kCFPreferencesCurrentApplication,
								&existsAndIsValidFormat);
			if ((!existsAndIsValidFormat) || (kCurrentPrefsVersion > diskVersion))
			{
				AppResources_Result		launchResult = noErr;
				
				
				// launch the converter and wait for it to complete
				launchResult = AppResources_LaunchPreferencesConverter();
				gHaveRunConverter = true;
				if (launchResult == noErr)
				{
				#if 0
					// for error checking purposes, could synchronize preferences and read
					// back the version value; it should be up to date if all went well
					// (but note, MacTelnetPrefsConverter should already inform the user
					// if any problems occur)
					CFIndex		updatedVersion = 0;
					
					
					(Boolean)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
					updatedVersion = CFPreferencesGetAppIntegerValue
										(CFSTR("prefs-version"), kCFPreferencesCurrentApplication,
											&existsAndIsValidFormat);
					assert(kCurrentPrefsVersion >= updatedVersion);
				#endif
				}
				else
				{
					// unable to run MacTelnetPrefsConverter!
					Console_Warning(Console_WriteValue, "error launching preferences converter", launchResult);
				}
			}
		}
	}
	
	// Create definitions for all possible settings; these are used for
	// efficient access later, and to guarantee no duplication.
	// IMPORTANT: The data types used here are documented in Preferences.h,
	//            and relied upon in other methods.  They are also used in
	//            the PrefsConverter.  Check for consistency!
	My_PreferenceDefinition::createFlag(kPreferences_TagArrangeWindowsUsingTabs,
										CFSTR("terminal-use-tabs"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedFormatFavorite,
									CFSTR("format-favorite"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedTerminalFavorite,
									CFSTR("terminal-favorite"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedTranslationFavorite,
									CFSTR("translation-favorite"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagAutoCaptureToFile,
										CFSTR("terminal-capture-auto-start"), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagBackupFontName,
									CFSTR("terminal-backup-font-family"), typeCFStringRef,
									sizeof(Str255), kPreferences_ClassTranslation);
	My_PreferenceDefinition::create(kPreferences_TagBellSound,
									CFSTR("terminal-when-bell-sound-basename"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagCaptureFileAlias,
									CFSTR("terminal-capture-file-alias-id"), typeNetEvents_CFNumberRef,
									sizeof(Preferences_AliasID), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagCaptureFileCreator,
									CFSTR("terminal-capture-file-creator-code"), typeCFStringRef,
									sizeof(OSType), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagCommandLine,
									CFSTR("command-line-token-strings"), typeCFArrayRef,
									sizeof(CFArrayRef), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagCopySelectedText,
										CFSTR("terminal-auto-copy-on-select"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagCopyTableThreshold,
									CFSTR("spaces-per-tab"), typeNetEvents_CFNumberRef,
									sizeof(UInt16), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagCursorBlinks,
										CFSTR("terminal-cursor-blinking"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagCursorMovesPriorToDrops,
										CFSTR("terminal-cursor-auto-move-on-drop"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagDataReceiveDoNotStripHighBit,
										CFSTR("data-receive-do-not-strip-high-bit"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagDataReadBufferSize,
									CFSTR("data-receive-buffer-size-bytes"), typeNetEvents_CFNumberRef,
									sizeof(SInt16), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagDontAutoClose,
										CFSTR("no-auto-close"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagDontAutoNewOnApplicationReopen,
										CFSTR("no-auto-new"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagEMACSMetaKey,
									CFSTR("key-map-emacs-meta"), typeCFStringRef,
									sizeof(Session_EMACSMetaKey), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagFocusFollowsMouse,
										CFSTR("terminal-focus-follows-mouse"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagFontCharacterWidthMultiplier,
									CFSTR("terminal-font-width-multiplier"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagFontName,
									CFSTR("terminal-font-family"), typeCFStringRef,
									sizeof(Str255), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagFontSize,
									CFSTR("terminal-font-size-points"), typeNetEvents_CFNumberRef,
									sizeof(SInt16), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagIdleAfterInactivityInSeconds,
									CFSTR("data-receive-idle-seconds"), typeNetEvents_CFNumberRef,
									sizeof(UInt16), kPreferences_ClassSession);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroAction, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-action"), typeCFStringRef,
											sizeof(MacroManager_Action), kPreferences_ClassMacroSet);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroContents, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-contents-string"), typeCFStringRef,
											sizeof(CFStringRef), kPreferences_ClassMacroSet);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroKey, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-key"), typeCFStringRef,
											sizeof(MacroManager_KeyID), kPreferences_ClassMacroSet);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroKeyModifiers, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-modifiers"), typeCFArrayRef,
											sizeof(UInt32), kPreferences_ClassMacroSet);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroName, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-name-string"), typeCFStringRef,
											sizeof(CFStringRef), kPreferences_ClassMacroSet);
	My_PreferenceDefinition::create(kPreferences_TagKeepAlivePeriodInMinutes,
									CFSTR("data-send-keepalive-period-minutes"), typeNetEvents_CFNumberRef,
									sizeof(UInt16), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagKeyInterruptProcess,
									CFSTR("command-key-interrupt-process"), typeCFStringRef,
									sizeof(char), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagKeyResumeOutput,
									CFSTR("command-key-resume-output"), typeCFStringRef,
									sizeof(char), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagKeySuspendOutput,
									CFSTR("command-key-suspend-output"), typeCFStringRef,
									sizeof(char), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskAllowsForceQuit,
										CFSTR("kiosk-force-quit-enabled"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskShowsMenuBar,
										CFSTR("kiosk-menu-bar-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskShowsOffSwitch,
										CFSTR("kiosk-off-switch-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskShowsScrollBar,
										CFSTR("kiosk-scroll-bar-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskUsesSuperfluousEffects,
										CFSTR("kiosk-effects-enabled"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagHeadersCollapsed,
										CFSTR("window-terminal-toolbar-invisible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagInfoWindowColumnOrdering,
									CFSTR("window-sessioninfo-column-order"), typeCFArrayRef,
									sizeof(CFArrayRef), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagLineModeEnabled,
										CFSTR("line-mode-enabled"), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagLocalEchoEnabled,
										CFSTR("data-send-local-echo-enabled"), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagLocalEchoHalfDuplex,
										CFSTR("data-send-local-echo-half-duplex"), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagMacrosMenuVisible,
										CFSTR("menu-macros-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagMapArrowsForEMACS,
									CFSTR("command-key-emacs-move-down"), typeCFStringRef,
									sizeof(Boolean), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagMapBackquote,
									CFSTR("key-map-backquote"), typeCFStringRef/* keystroke string, e.g. blank "" or escape "\e" */,
									sizeof(Boolean), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagMapCarriageReturnToCRNull,
									CFSTR("key-map-new-line"), typeCFStringRef,
									sizeof(Boolean), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagMapDeleteToBackspace,
									CFSTR("key-map-delete"), typeCFStringRef,
									sizeof(Boolean), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagMapKeypadTopRowForVT220,
									CFSTR("command-key-vt220-pf1")/* TEMPORARY - one of several key names used */, typeCFStringRef,
									sizeof(Boolean), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagMarginBell,
									CFSTR("terminal-when-cursor-near-right-margin"), typeCFStringRef/* "bell", "ignore" */,
									sizeof(Boolean), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagMenuItemKeys,
										CFSTR("menu-key-equivalents"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagNewCommandShortcutEffect,
									CFSTR("new-means"), typeCFStringRef/* "shell", "dialog", "default" */,
									sizeof(UInt32), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagNotification,
									CFSTR("when-alert-in-background"), typeCFStringRef/* "alert", "animate", "badge", "ignore" */,
									sizeof(SInt16), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagNotifyOfBeeps,
									CFSTR("terminal-when-bell-in-background"), typeCFStringRef/* "notify", "ignore" */,
									sizeof(Boolean), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagPageKeysControlLocalTerminal,
									CFSTR("command-key-terminal-end")/* TEMPORARY - one of several key names used */, typeCFStringRef,
									sizeof(Boolean), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagPasteBlockSize,
									CFSTR("data-send-paste-block-size-bytes"), typeNetEvents_CFNumberRef,
									sizeof(SInt16), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagPasteMethod,
									CFSTR("data-send-paste-method"), typeCFStringRef,
									sizeof(Clipboard_PasteMethod), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagPureInverse,
										CFSTR("terminal-inverse-selections"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagRandomTerminalFormats,
										CFSTR("terminal-format-random"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagServerHost,
									CFSTR("server-host"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagServerPort,
									CFSTR("server-port"), typeNetEvents_CFNumberRef,
									sizeof(SInt16), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagServerProtocol,
									CFSTR("server-protocol"), typeCFStringRef,
									sizeof(Session_Protocol), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagServerUserID,
									CFSTR("server-user-id"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagSimplifiedUserInterface,
										CFSTR("menu-command-set-simplified"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagTektronixMode,
									CFSTR("tek-mode"), typeCFStringRef,
									sizeof(VectorInterpreter_Mode), kPreferences_ClassSession);
	My_PreferenceDefinition::createFlag(kPreferences_TagTektronixPAGEClearsScreen,
										CFSTR("tek-page-clears-screen"), kPreferences_ClassSession);
	My_PreferenceDefinition::create(kPreferences_TagTerminalAnswerBackMessage,
									CFSTR("terminal-emulator-answerback"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagTerminalClearSavesLines,
										CFSTR("terminal-clear-saves-lines"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlack,
											CFSTR("terminal-color-ansi-black-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIRed,
											CFSTR("terminal-color-ansi-red-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIGreen,
											CFSTR("terminal-color-ansi-green-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIYellow,
											CFSTR("terminal-color-ansi-yellow-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlue,
											CFSTR("terminal-color-ansi-blue-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIMagenta,
											CFSTR("terminal-color-ansi-magenta-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSICyan,
											CFSTR("terminal-color-ansi-cyan-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIWhite,
											CFSTR("terminal-color-ansi-white-normal-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlackBold,
											CFSTR("terminal-color-ansi-black-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIRedBold,
											CFSTR("terminal-color-ansi-red-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIGreenBold,
											CFSTR("terminal-color-ansi-green-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIYellowBold,
											CFSTR("terminal-color-ansi-yellow-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlueBold,
											CFSTR("terminal-color-ansi-blue-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIMagentaBold,
											CFSTR("terminal-color-ansi-magenta-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSICyanBold,
											CFSTR("terminal-color-ansi-cyan-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIWhiteBold,
											CFSTR("terminal-color-ansi-white-bold-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBlinkingForeground,
											CFSTR("terminal-color-blinking-foreground-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBlinkingBackground,
											CFSTR("terminal-color-blinking-background-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBoldForeground,
											CFSTR("terminal-color-bold-foreground-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBoldBackground,
											CFSTR("terminal-color-bold-background-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorMatteBackground,
											CFSTR("terminal-color-matte-background-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorNormalForeground,
											CFSTR("terminal-color-normal-foreground-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorNormalBackground,
											CFSTR("terminal-color-normal-background-rgb"), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalCursorType,
									CFSTR("terminal-cursor-shape"),
									typeCFStringRef/* "block", "underline", "thick underline", "vertical bar", "thick vertical bar" */,
									sizeof(TerminalView_CursorType), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagTerminalEmulatorType,
									CFSTR("terminal-emulator-type"), typeCFStringRef,
									sizeof(Terminal_Emulator), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagTerminalLineWrap,
										CFSTR("terminal-line-wrap"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginLeft,
									CFSTR("terminal-margin-left-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginRight,
									CFSTR("terminal-margin-right-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginTop,
									CFSTR("terminal-margin-top-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginBottom,
									CFSTR("terminal-margin-bottom-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingLeft,
									CFSTR("terminal-padding-left-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingRight,
									CFSTR("terminal-padding-right-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingTop,
									CFSTR("terminal-padding-top-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingBottom,
									CFSTR("terminal-padding-bottom-em"), typeNetEvents_CFNumberRef,
									sizeof(Float32), kPreferences_ClassFormat);
	My_PreferenceDefinition::create(kPreferences_TagTerminalResizeAffectsFontSize,
									CFSTR("terminal-resize-affects"), typeCFStringRef/* "screen" or "font" */,
									sizeof(Boolean), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenColumns,
									CFSTR("terminal-screen-dimensions-columns"), typeNetEvents_CFNumberRef,
									sizeof(UInt16), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenRows,
									CFSTR("terminal-screen-dimensions-rows"), typeNetEvents_CFNumberRef,
									sizeof(UInt16), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenScrollbackRows,
									CFSTR("terminal-scrollback-size-lines"), typeNetEvents_CFNumberRef,
									sizeof(UInt16), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenScrollbackType,
									CFSTR("terminal-scrollback-type"), typeCFStringRef,
									sizeof(Terminal_ScrollbackType), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScrollDelay,
									CFSTR("terminal-scroll-delay-milliseconds"), typeNetEvents_CFNumberRef,
									sizeof(EventTime), kPreferences_ClassTerminal);
	My_PreferenceDefinition::create(kPreferences_TagTextEncodingIANAName,
									CFSTR("terminal-text-encoding-name"), typeCFStringRef,
									sizeof(CFStringRef), kPreferences_ClassTranslation);
	My_PreferenceDefinition::create(kPreferences_TagTextEncodingID,
									CFSTR("terminal-text-encoding-id"), typeNetEvents_CFNumberRef,
									sizeof(CFStringEncoding), kPreferences_ClassTranslation);
	My_PreferenceDefinition::create(kPreferences_TagVisualBell,
									CFSTR("terminal-when-bell"), typeCFStringRef/* "visual" or "audio+visual" */,
									sizeof(Boolean), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasClipboardShowing,
										CFSTR("window-clipboard-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasCommandLineShowing,
										CFSTR("window-commandline-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasControlKeypadShowing,
										CFSTR("window-controlkeys-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasFunctionKeypadShowing,
										CFSTR("window-functionkeys-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasSessionInfoShowing,
										CFSTR("window-sessioninfo-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasVT220KeypadShowing,
										CFSTR("window-vt220keys-visible"), kPreferences_ClassGeneral);
	My_PreferenceDefinition::create(kPreferences_TagWindowStackingOrigin,
									CFSTR("window-terminal-position-pixels"), typeCFArrayRef/* 2 CFNumberRefs, pixels from top-left */,
									sizeof(Point), kPreferences_ClassGeneral);
	My_PreferenceDefinition::createFlag(kPreferences_TagVT100FixLineWrappingBug,
										CFSTR("terminal-emulator-vt100-fix-line-wrapping-bug"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTerm256ColorsEnabled,
										CFSTR("terminal-emulator-xterm-enable-color-256"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermColorEnabled,
										CFSTR("terminal-emulator-xterm-enable-color"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermGraphicsEnabled,
										CFSTR("terminal-emulator-xterm-enable-graphics"), kPreferences_ClassTerminal);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermWindowAlterationEnabled,
										CFSTR("terminal-emulator-xterm-enable-window-alteration-sequences"), kPreferences_ClassTerminal);
	
	// to ensure that the rest of the application can depend on its
	// known keys being defined, ALWAYS merge in default values for
	// any keys that may not already be defined (by the data on disk,
	// by the Preferences Converter, etc.)
	// NOTE: the Cocoa NSUserDefaults class supports the concept of
	// a registered domain, which *would* avoid the need to actually
	// replicate defaults; instead, the defaults are simply referred
	// to as needed; a better future solution would be to register
	// all defaults instead of copying them into user preferences
	mergeInDefaultPreferences();
	
	gPreferenceEventListenerModel = ListenerModel_New(kListenerModel_StyleStandard,
														kConstantsRegistry_ListenerModelDescriptorPreferences);
	if (nullptr == gPreferenceEventListenerModel) result = kPreferences_ResultNotInitialized;
	else
	{
		// create ALL preferences contexts based on available data on disk;
		// these are retained in memory so that they may be used on demand
		// by things like user interface elements and the Preferences window
		result = createAllPreferencesContextsFromDisk();
		
		// success!
		gInitialized = true;
	}
	
	return result;
}// Init


/*!
Destroys the global preference structures.

(3.0)
*/
void
Preferences_Done ()
{
	if (gInitialized)
	{
		// dispose of the listener model
		ListenerModel_Dispose(&gPreferenceEventListenerModel);
		
		// clean up the list of alias references
		deleteAllAliasNodes();
		
		gInitialized = false;
	}
}// Done


/*!
A unit test for this module.  This should always
be run before a release, after any substantial
changes are made, or if you suspect bugs!  It
should also be EXPANDED as new functionality is
proposed (ideally, a test is written before the
functionality is added).

(3.1)
*/
void
Preferences_RunTests ()
{
	UInt16		totalTests = 0;
	UInt16		failedTests = 0;
	
	
	++totalTests; if (false == unitTest000_Begin()) ++failedTests;
	++totalTests; if (false == unitTest001_Begin()) ++failedTests;
	++totalTests; if (false == unitTest002_Begin()) ++failedTests;
	++totalTests; if (false == unitTest003_Begin()) ++failedTests;
	
	Console_WriteUnitTestReport("Preferences", failedTests, totalTests);
}// RunTests


/*!
Creates a new alias in memory.  You can set up “pending
alias preferences” this way, changing the alias as many
times as you need to, but only writing it to the preferences
file if necessary.  This prevents unused alias resources
from appearing in the preferences file.

NOTE:	Always call Preferences_AliasIsStored() before
		creating a new alias.  If you want to update an
		existing alias, you need to create it using
		Preferences_NewSavedAlias().  Otherwise, if you
		use Preferences_NewAlias(), and then save the
		alias to disk, it will never replace any existing
		alias.

(3.0)
*/
Preferences_AliasID
Preferences_NewAlias	(FSSpec const*		inFileSpecificationPtr)
{
	Preferences_AliasID		result = kPreferences_InvalidAliasID;
	My_AliasInfoPtr			dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(My_AliasInfo)), My_AliasInfoPtr);
	
	
	if ((nullptr != dataPtr) && (nullptr != inFileSpecificationPtr))
	{
		SInt16		oldResFile = CurResFile();
		
		
		AppResources_UseResFile(kAppResources_FileIDPreferences);
		gAliasList().push_back(dataPtr);
		dataPtr->alias = nullptr;
		if (NewAlias(nullptr/* base */, inFileSpecificationPtr, &dataPtr->alias) == noErr)
		{
			dataPtr->resourceID = result = Unique1ID(rAliasType);
			dataPtr->wasChanged = false;
			dataPtr->isResource = false;
		}
		UseResFile(oldResFile);
	}
	return result;
}// NewAlias


/*!
Creates a new preferences context that copies the data of the
specified original context.  The new context is automatically
retained, so you need to invoke Preferences_ReleaseContext()
when finished.  If any problems occur, nullptr is returned.

IMPORTANT:	Any context originally read from preferences, such
			as defaults or user favorites, *will* be tied to
			disk, causing its clone to also be tied to disk by
			default (albeit under a different name).  You can
			force the clone to be anonymous and in memory only
			by setting "inForceDetach" to true.

WARNING:	Currently, cloning a Default context is naïve,
			because it holds global application preferences.
			The resulting copy technically contains all necessary
			keys, but will also have many other irrelevant keys
			that will never be used.  (A future implementation
			should copy only the keys expected for the class of
			the original context.)  It may be better to use a
			few Preferences_ContextGetData() calls on your
			desired source of defaults, and then manually call
			Preferences_ContextSetData() on a new context to
			extract only the keys you want.

The special "kPreferences_ChangeContextBatchMode" event is
triggered by cloning, INSTEAD OF causing notifications for each
key individually.

Note that although the clone is given an arbitrary unique name,
this can be changed later with Preferences_ContextRename().

Contexts that were already temporary or otherwise detached,
remain detached (unnamed) when cloned, regardless of the value
of "inForceDetach".  Still, it is a good idea to explicitly
specify the value you want for this argument.

See also Preferences_ContextCopy().

(3.1)
*/
Preferences_ContextRef
Preferences_NewCloneContext		(Preferences_ContextRef		inBaseContext,
								 Boolean					inForceDetach)
{
	My_ContextAutoLocker		basePtr(gMyContextPtrLocks(), inBaseContext);
	Preferences_Class			baseClass = kPreferences_ClassGeneral;
	Preferences_ContextRef		result = nullptr;
	Boolean						isDetached = (0 == CFStringGetLength(basePtr->returnName()));
	
	
	baseClass = basePtr->returnClass();
	
	if ((isDetached) || (inForceDetach))
	{
		result = Preferences_NewContext(baseClass);
	}
	else
	{
		CFStringRef			nameCFString = nullptr;
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		// INCOMPLETE: Base the new name on the name of the original.
		// scan the list of all contexts for the given class and find a unique name
		prefsResult = Preferences_CreateUniqueContextName(baseClass, nameCFString/* new name */,
															basePtr->returnName()/* base name */);
		if (kPreferences_ResultOK == prefsResult)
		{
			result = Preferences_NewContextFromFavorites(baseClass, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
	}
	
	if (nullptr != result)
	{
		My_ContextAutoLocker	resultPtr(gMyContextPtrLocks(), result);
		CFRetainRelease			sourceKeys(basePtr->returnKeyListCopy(), true/* is retained */);
		CFArrayRef				sourceKeysCFArray = sourceKeys.returnCFArrayRef();
		
		
		if (nullptr != sourceKeysCFArray)
		{
			CFIndex const	kNumberOfKeys = CFArrayGetCount(sourceKeysCFArray);
			
			
			for (CFIndex i = 0; i < kNumberOfKeys; ++i)
			{
				CFStringRef const	kKeyCFStringRef = CFUtilities_StringCast
														(CFArrayGetValueAtIndex(sourceKeysCFArray, i));
				CFRetainRelease		keyValueCFType(basePtr->returnValueCopy(kKeyCFStringRef),
													true/* is retained */);
				
				
				resultPtr->addValue(0/* do not notify */, kKeyCFStringRef, keyValueCFType.returnCFTypeRef());
			}
			
			// it is not easy to tell what the tags are of the changed keys, so send
			// a single event informing listeners that many things changed at once
			resultPtr->notifyListeners(kPreferences_ChangeContextBatchMode);
		}
	}
	return result;
}// NewCloneContext


/*!
Creates a new preferences context in the given class that
is isolated, not part of the ordinary list of contexts
stored in user preferences.  The reference is automatically
retained, but you need to invoke Preferences_ReleaseContext()
when finished.

Since the context is isolated it does not need a name, it is
not registered in the list of known contexts, and creating it
does not trigger any notifications (for example, there is no
kPreferences_ChangeNumberOfContexts event).

If any problems occur, nullptr is returned; otherwise, a
reference to the new context is returned.

(3.1)
*/
Preferences_ContextRef
Preferences_NewContext	(Preferences_Class		inClass)
{
	Preferences_ContextRef		result = nullptr;
	
	
	try
	{
		My_ContextInterfacePtr		contextPtr = nullptr;
		My_ContextCFDictionaryPtr	newDictionary = new My_ContextCFDictionary(inClass);
		
		
		contextPtr = newDictionary;
		
		if (nullptr != contextPtr)
		{
			result = REINTERPRET_CAST(contextPtr, Preferences_ContextRef);
			Preferences_RetainContext(result);
		}
	}
	catch (std::exception const&	inException)
	{
		Console_WriteLine(inException.what());
		result = nullptr;
	}
	return result;
}// NewContext


/*!
Creates a new preferences context according to the given
specifications, or returns an existing one that matches.
The reference is automatically retained, but you need to
invoke Preferences_ReleaseContext() when finished.

The domain name should be "nullptr" unless this is being
called as part of the initialization loop (as that is the
only time it is not possible to automatically determine
the proper domain).  Only the Preferences module should
set the domain.

If any problems occur, nullptr is returned; otherwise,
a reference to the new context is returned.

If you need a standalone copy based on an existing user
collection, first use this routine to acquire the collection
you want, and then use Preferences_NewCloneContext().  Keep
in mind that Preferences_NewCloneContext() allows two kinds
of copies: in-memory only (temporary), or named copies that
are synchronized to disk.

Contexts are used in order to make changes to settings
(and save changes) within the constraints of a single
named dictionary of a particular class.  To the user,
this is a collection such as a Session Favorite.

IMPORTANT:	Contexts can be renamed.  Make sure you
			are not relying on names for long periods
			of time, otherwise your name may be out
			of date and any attempt to access its
			context will end up creating a new blank.

WARNING:	Despite the name, the 2nd parameter does
			not (yet) automatically generate a unique
			value when nullptr is used.

(3.1)
*/
Preferences_ContextRef
Preferences_NewContextFromFavorites		(Preferences_Class		inClass,
										 CFStringRef			inNameOrNullToAutoGenerateUniqueName,
										 CFStringRef			inDomainNameIfInitializingOrNull)
{
	Preferences_ContextRef		result = nullptr;
	Boolean						releaseName = false;
	
	
	// when nullptr is given, scan the list of all contexts for the
	// given class and find a unique name
	if (nullptr == inNameOrNullToAutoGenerateUniqueName)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_CreateUniqueContextName
						(inClass, inNameOrNullToAutoGenerateUniqueName/* new name */);
		assert(kPreferences_ResultOK == prefsResult);
		releaseName = true;
	}
	
	try
	{
		My_ContextFavoritePtr	contextPtr = nullptr;
		
		
		if (false == getNamedContext(inClass, inNameOrNullToAutoGenerateUniqueName, contextPtr))
		{
			My_FavoriteContextList*		listPtr = nullptr;
			
			
			if (getListOfContexts(inClass, listPtr))
			{
				My_ContextFavoritePtr	newDictionary = new My_ContextFavorite
															(inClass, inNameOrNullToAutoGenerateUniqueName,
																inDomainNameIfInitializingOrNull);
				
				
				contextPtr = newDictionary;
				listPtr->push_back(newDictionary);
				assert(true == getNamedContext(inClass, inNameOrNullToAutoGenerateUniqueName, contextPtr));
				changeNotify(kPreferences_ChangeNumberOfContexts);
			}
		}
		
		if (nullptr != contextPtr)
		{
			result = REINTERPRET_CAST(contextPtr, Preferences_ContextRef);
			Preferences_RetainContext(result);
		}
	}
	catch (std::exception const&	inException)
	{
		Console_WriteLine(inException.what());
		result = nullptr;
	}
	
	if (releaseName)
	{
		CFRelease(inNameOrNullToAutoGenerateUniqueName), inNameOrNullToAutoGenerateUniqueName = nullptr;
	}
	
	return result;
}// NewContextFromFavorites


/*!
Creates a new alias in memory based on an alias already
present in the preferences file.  You generally save an
alias ID in a preferences structure in order to refer to
a file, and then use that ID and this method to work with
the alias in memory.  To make changes to an alias in the
preferences file, you must first use this routine to read
it into memory, and then use the Preferences_AliasChange()
and Preferences_AliasSave() methods.

NOTE:	Always call Preferences_AliasIsStored() before
		creating a new alias with this routine.  If an
		alias with the ID of "inAliasID" exists, it *will*
		be overwritten if you subsequently call
		Preferences_AliasSave() on your new alias.  If
		you want a new alias that will not conflict with
		saved aliases, use Preferences_NewAlias().

WARNING:	Never call Preferences_NewSavedAlias() more
			than once with the same alias ID, unless you
			always call Preferences_AliasDispose() to
			destroy prior instances with the same ID.  If
			you create more than one alias with the same
			ID, a memory leak will ensue and you will only
			be able to access data for the first alias
			with the repeated ID.

(3.0)
*/
Preferences_AliasID
Preferences_NewSavedAlias	(Preferences_AliasID	inAliasID)
{
	AliasHandle				alias = nullptr;
	Preferences_AliasID		result = kPreferences_InvalidAliasID;
	
	
	// look for an alias resource matching the given ID
	if (findAliasOnDisk(inAliasID, &alias))
	{
		FSSpec		aliasFile;
		Boolean		isError = false,
					recordChanged = false;
		
		
		if (ResolveAlias(nullptr/* base */, alias, &aliasFile, &recordChanged) != noErr)
		{
			// most likely, the alias stored on disk no longer points to a valid file
			isError = true;
		}
		else
		{
			My_AliasInfoPtr		dataPtr = findAlias(Preferences_NewAlias(&aliasFile));
			
			
			if (nullptr == dataPtr)
			{
				// somehow, the alias was found earlier but is not there anymore!
				isError = true;
			}
			else
			{
				// set the alias ID to the specified ID, to make sure the stored alias
				// is overwritten when the alias is saved
				dataPtr->resourceID = result = inAliasID; // changing this field allows the alias to be stored under the given ID
				dataPtr->alias = alias; // set the handle to be a resource handle (see Preferences_AliasSave())
				dataPtr->isResource = true;
				if (recordChanged) ChangedResource(REINTERPRET_CAST(alias, Handle));
				Alert_ReportOSStatus(ResError());
			}
		}
		
		if (isError)
		{
			// bail; findAliasOnDisk() loads a resource into memory,
			// so that resource must be released
			ReleaseResource(REINTERPRET_CAST(alias, Handle)), alias = nullptr;
		}
	}
	return result;
}// NewSavedAlias


/*!
Destroys the in-memory version of an alias, not affecting
its on-disk version.  To write the contents of an alias in
memory to the preferences file first, use
Preferences_AliasSave(), and then call this method to dispose
of the memory.  After calling this routine, the specified
alias ID is invalid.

(3.0)
*/
void
Preferences_DisposeAlias	(Preferences_AliasID	inAliasID)
{
	My_AliasInfoList::iterator		aliasInfoIterator;
	
	
	// look for the given alias in the linked list
	aliasInfoIterator = find_if(gAliasList().begin(), gAliasList().end(), aliasInfoResourceIDMatches(inAliasID));
	if (aliasInfoIterator != gAliasList().end())
	{
		My_AliasInfoPtr		dataPtr = *aliasInfoIterator;
		
		
		deleteAliasData(&dataPtr);
		gAliasList().erase(aliasInfoIterator);
	}
}// DisposeAlias


/*!
Adds an additional lock on the specified reference.
This indicates that you are using the context for
some reason, so attempts by anyone else to delete
the model with Preferences_ReleaseContext() will
fail until you release your lock (and everyone else
releases locks they may have).

(3.1)
*/
void
Preferences_RetainContext	(Preferences_ContextRef		inRef)
{
	gMyContextRefLocks().acquireLock(inRef);
}// RetainContext


/*!
Releases one lock on the specified context created with
Preferences_NewContextFromFavorites() or similar routines,
and deletes the context *if* no other locks remain.  Your
copy of the reference is set to nullptr.

(3.1)
*/
void
Preferences_ReleaseContext	(Preferences_ContextRef*	inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		gMyContextRefLocks().releaseLock(*inoutRefPtr);
		unless (gMyContextRefLocks().isLocked(*inoutRefPtr))
		{
			My_ContextInterfacePtr		ptr = REINTERPRET_CAST(*inoutRefPtr, My_ContextInterfacePtr);
			My_FavoriteContextList*		listPtr = nullptr;
			
			
			if (getListOfContexts(ptr->returnClass(), listPtr))
			{
				// delete occurrences of the released context in its context list
				listPtr->erase(std::remove(listPtr->begin(), listPtr->end(), ptr),
								listPtr->end());
				assert(listPtr->end() == std::find(listPtr->begin(), listPtr->end(), ptr));
			}
			delete ptr;
		}
		*inoutRefPtr = nullptr;
	}
}// ReleaseContext


/*!
Modifies an alias in memory.  Your changes are not
saved to the preferences file unless you call
Preferences_AliasSave().

(3.0)
*/
void
Preferences_AliasChange		(Preferences_AliasID	inAliasID,
							 FSSpec const*			inNewAliasFileSpecificationPtr)
{
	My_AliasInfoPtr		dataPtr = findAlias(inAliasID);
	
	
	if (nullptr != dataPtr)
	{
		Boolean		wasChanged = false;
		
		
		if (UpdateAlias(nullptr/* base */, inNewAliasFileSpecificationPtr, dataPtr->alias, &wasChanged) == noErr)
		{
			(OSStatus)setAliasChanged(dataPtr);
		}
	}
}// AliasChange


/*!
Removes an alias from the preferences file.  This
does not affect the version in memory (the alias
remains valid) - to destroy the memory alias, use
the method Preferences_AliasDispose().

(3.0)
*/
void
Preferences_AliasDeleteSaved	(Preferences_AliasID	inAliasID)
{
	My_AliasInfoPtr		dataPtr = findAlias(inAliasID);
	
	
	if (nullptr != dataPtr)
	{
		Handle		aliasAsHandle = REINTERPRET_CAST(dataPtr->alias, Handle);
		Handle		temporaryHandle = Memory_NewHandle(GetHandleSize(aliasAsHandle));
		
		
		if (nullptr != temporaryHandle)
		{
			// copy the alias data, because RemoveResource() creates an unreliable handle
			BlockMoveData(*aliasAsHandle, *temporaryHandle, GetHandleSize(temporaryHandle));
			
			// remove the alias resource, and replace the internal handle with a valid one (the copy)
			RemoveResource(aliasAsHandle);
			AppResources_UpdateResFile(kAppResources_FileIDPreferences);
			dataPtr->alias = REINTERPRET_CAST(temporaryHandle, AliasHandle);
		}
		dataPtr->isResource = false; // signifies that the data in memory no longer represents a resource
		
		Alert_ReportOSStatus(ResError());
	}
}// AliasDeleteSaved


/*!
Determines if an alias with the specified ID is in the
preferences file.

(3.0)
*/
Boolean
Preferences_AliasIsStored	(Preferences_AliasID	inAliasID)
{
	Boolean			result = false;
	AliasHandle		unused = nullptr;
	
	
	result = findAliasOnDisk(inAliasID, &unused);
	ReleaseResource(REINTERPRET_CAST(unused, Handle)), unused = nullptr;
	return result;
}// AliasIsStored


/*!
Finds the file for an alias.  If the file does not exist,
the result is "false"; otherwise, the result is "true".

(3.0)
*/
Boolean
Preferences_AliasParse	(Preferences_AliasID	inAliasID,
						 FSSpec*				outAliasFileSpecificationPtr)
{
	Boolean		result = false;
	
	
	if (nullptr != outAliasFileSpecificationPtr)
	{
		My_AliasInfoPtr		dataPtr = findAlias(inAliasID);
		
		
		if (nullptr != dataPtr)
		{
			Boolean		wasChanged = false;
			
			
			if (ResolveAlias(nullptr/* base */, dataPtr->alias, outAliasFileSpecificationPtr, &wasChanged) == noErr)
			{
				// then the file was found
				result = true;
				if (wasChanged) (OSStatus)setAliasChanged(dataPtr);
			}
		}
	}
	return result;
}// AliasParse


/*!
Saves an alias to disk, but only if it has been modified.
This is the only way to physically write an alias record
to the preferences file.

The new alias resource will be given the specified name.
If you do not want to use a name, pass an empty string.

(3.0)
*/
void
Preferences_AliasSave	(Preferences_AliasID	inAliasID,
						 ConstStringPtr			inName)
{
	My_AliasInfoPtr		dataPtr = findAlias(inAliasID);
	
	
	if (nullptr != dataPtr)
	{
		// only write to disk if the data in memory actually changed at some point
		if (dataPtr->wasChanged)
		{
			SInt16		oldResFile = CurResFile();
			OSStatus	error = noErr;
			
			
			AppResources_UseResFile(kAppResources_FileIDPreferences);
			unless (Preferences_AliasIsStored(inAliasID))
			{
				// write a new alias record to the preferences file, using the unique resource ID number
				// NOTE: hmmm, AddResource() would convert the AliasHandle to a resource handle here, is that a good idea?
				AddResource(REINTERPRET_CAST(dataPtr->alias, Handle), rAliasType, dataPtr->resourceID, inName);
				error = ResError();
			}
			if (error == noErr) AppResources_UpdateResFile(kAppResources_FileIDPreferences);
			Alert_ReportOSStatus(error);
			UseResFile(oldResFile);
		}
	}
}// AliasSave


/*!
Copies all the key-value pairs from the source to the
destination (regardless of class), and does not remove
or change any other keys defined in the destination.

Unlike Preferences_NewCloneContext(), this routine does
not create any new contexts.  It might be used to apply
changes in a temporary context to a more permanent one.

\retval kPreferences_ResultOK
if the context was successfully copied

\retval kPreferences_ResultInvalidContextReference
if either of the specified contexts does not exist

\retval kPreferences_ResultOneOrMoreNamesNotAvailable
if there was a problem determining what data is in the source

(3.1)
*/
Preferences_Result
Preferences_ContextCopy		(Preferences_ContextRef		inBaseContext,
							 Preferences_ContextRef		inDestinationContext)
{
	My_ContextAutoLocker	basePtr(gMyContextPtrLocks(), inBaseContext);
	My_ContextAutoLocker	destPtr(gMyContextPtrLocks(), inDestinationContext);
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if ((nullptr == basePtr) || (nullptr == destPtr)) result = kPreferences_ResultInvalidContextReference;
	else
	{
		CFRetainRelease		sourceKeys(basePtr->returnKeyListCopy(), true/* is retained */);
		CFArrayRef			sourceKeysCFArray = sourceKeys.returnCFArrayRef();
		
		
		if (nullptr == sourceKeysCFArray) result = kPreferences_ResultOneOrMoreNamesNotAvailable;
		else
		{
			CFIndex const	kNumberOfKeys = CFArrayGetCount(sourceKeysCFArray);
			
			
			for (CFIndex i = 0; i < kNumberOfKeys; ++i)
			{
				CFStringRef const	kKeyCFStringRef = CFUtilities_StringCast
														(CFArrayGetValueAtIndex(sourceKeysCFArray, i));
				CFRetainRelease		keyValueCFType(basePtr->returnValueCopy(kKeyCFStringRef),
													true/* is retained */);
				
				
				destPtr->addValue(0/* do not notify */, kKeyCFStringRef, keyValueCFType.returnCFTypeRef());
			}
			
			// it is not easy to tell what the tags are of the changed keys, so send
			// a single event informing listeners that many things changed at once
			destPtr->notifyListeners(kPreferences_ChangeContextBatchMode);
		}
	}
	return result;
}// ContextCopy


/*!
Completely removes a preference corresponding to the specified
tag, making it “undefined”.  Subsequent queries that are set to
fall back on defaults, will do so because the context will no
longer have a value for the given setting.

If the tag has a "kPreferences_TagIndexed…" name, you should
call Preferences_ReturnTagVariantForIndex() to produce the
actual tag for the specific index you are interested in.

Note that any data removed via this routine will be permanently
removed from disk eventually (via Preferences_Done()).  You can
choose to save explicitly with Preferences_Save(), but that is
not strictly necessary unless you are paranoid that the
application will crash before Preferences_Done() is invoked.

\retval kPreferences_ResultOK
if the data is deleted properly

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultUnknownTagOrClass
if the specified tag is not understood

(4.0)
*/
Preferences_Result
Preferences_ContextDeleteData	(Preferences_ContextRef		inContext,
								 Preferences_Tag			inDataPreferenceTag)
{
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0;
	Preferences_Class		dataClass = kPreferences_ClassGeneral;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
		
		
		ptr->deleteValue(keyName);
	}
	return result;
}// ContextDeleteData


/*!
Deletes the specified context from in-memory structures;
this will become permanent the next time application
preferences are synchronized.

\retval kPreferences_ResultOK
if the persistent store is successfully deleted

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultOneOrMoreNamesNotAvailable
if an unexpected error occurred while searching for names

(3.0)
*/
Preferences_Result
Preferences_ContextDeleteSaved	(Preferences_ContextRef		inContext)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		result = ptr->destroy();
		if (kPreferences_ResultOK == result)
		{
			// now do a release without a retain, which will allow this
			// context to disappear from the internal list (see also
			// Preferences_ContextSave(), which should have done a retain
			// without a release in the first place)
			Preferences_ContextRef		deletedContext = inContext;
			
			
			Preferences_ReleaseContext(&deletedContext);
			changeNotify(kPreferences_ChangeNumberOfContexts);
		}
	}
	return result;
}// ContextDeleteSaved


/*!
Returns preference data corresponding to the specified tag,
starting with the specified context.

If the tag has a "kPreferences_TagIndexed…" name, you should
call Preferences_ReturnTagVariantForIndex() to produce the
actual tag for the specific index you are interested in.

If the data is not available in the specified context and
"inSearchDefaults" is true, then other sources of defaults
will be checked before returning an error.  This allows you
to retrieve sensible default values when a specific context
does not contain what you need.  If you also define the
result "outIsDefaultOrNull", you can find out whether or not
the returned data was read from the defaults.

\retval kPreferences_ResultOK
if data is acquired without errors; "outActualSizePtr" then
points to the exact number of bytes used by the data

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultUnknownTagOrClass
if the specified tag is not understood

\retval kPreferences_ResultInsufficientBufferSpace
if you provide a buffer too small to store the data

\retval kPreferences_ResultBadVersionDataNotAvailable
if the file on disk does not contain the requested data (it
may be an older version); the output data will be garbage and
the size pointed to by "outActualSizePtr" is set to 0

(3.1)
*/
Preferences_Result
Preferences_ContextGetData	(Preferences_ContextRef		inContext,
							 Preferences_Tag			inDataPreferenceTag,
							 size_t						inDataStorageSize,
							 void*						outDataStorage,
							 Boolean					inSearchDefaults,
							 size_t*					outActualSizePtrOrNull,
							 Boolean*					outIsDefaultOrNull)
{
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0;
	Boolean					isDefault = false;
	Preferences_Class		dataClass = kPreferences_ClassGeneral;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
		
		
		if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
		else
		{
			result = contextGetData(ptr, dataClass, inDataPreferenceTag, inDataStorageSize,
									outDataStorage, outActualSizePtrOrNull);
			if ((kPreferences_ResultOK != result) && (inSearchDefaults))
			{
				// not available...try another context
				Preferences_ContextRef		alternateContext = nullptr;
				
				
				isDefault = true;
				result = Preferences_GetDefaultContext(&alternateContext, ptr->returnClass());
				if (kPreferences_ResultOK == result)
				{
					My_ContextAutoLocker	alternatePtr(gMyContextPtrLocks(), alternateContext);
					
					
					result = contextGetData(alternatePtr, dataClass, inDataPreferenceTag, inDataStorageSize,
											outDataStorage, outActualSizePtrOrNull);
					if (kPreferences_ResultOK != result)
					{
						// not available...try yet another context
						Preferences_ContextRef		rootContext = nullptr;
						
						
						result = Preferences_GetDefaultContext(&rootContext);
						if (kPreferences_ResultOK == result)
						{
							My_ContextAutoLocker	rootPtr(gMyContextPtrLocks(), rootContext);
							
							
							result = contextGetData(rootPtr, dataClass, inDataPreferenceTag, inDataStorageSize,
													outDataStorage, outActualSizePtrOrNull);
						}
					}
				}
			}
		}
	}
	
	if (nullptr != outIsDefaultOrNull) *outIsDefaultOrNull = isDefault;
	
	return result;
}// ContextGetData


/*!
Provides the name of a context (which is typically displayed
in user interface elements).

\retval kPreferences_ResultOK
if the context name was found

\retval kPreferences_ResultUnknownName
if the context name was not found

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

(3.1)
*/
Preferences_Result
Preferences_ContextGetName	(Preferences_ContextRef		inContext,
							 CFStringRef&				outName)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		outName = ptr->returnName();
		if (nullptr == outName) result = kPreferences_ResultUnknownName;
	}
	return result;
}// ContextGetName


/*!
Changes the name of a context (which is typically displayed
in user interface elements); this will become permanent the
next time application preferences are synchronized.

\retval kPreferences_ResultOK
if the context is successfully renamed

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

(3.1)
*/
Preferences_Result
Preferences_ContextRename	(Preferences_ContextRef		inContext,
							 CFStringRef				inNewName)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		result = ptr->rename(inNewName);
		
		if (kPreferences_ResultOK == result)
		{
			changeNotify(kPreferences_ChangeContextName, inContext);
		}
	}
	return result;
}// ContextRename


/*!
Changes the position of a context in the list of contexts
for its class; this will become permanent the next time
application preferences are synchronized.

The specified reference context must be in the same class
as the original (otherwise they would not be in the same
list).  If "inInsertBefore" is true, the first context is
moved to the position before the reference context; otherwise
it is moved to the position immediately after the reference
context.

This triggers the "kPreferences_ChangeNumberOfContexts"
event; while technically it is not changing the number of
contexts, handlers that would care about the size of the
list probably also care when it has been rearranged.

This call invalidates results you may have previously had
through calls like Preferences_InsertContextNamesInMenu()
and Preferences_CreateContextNameArray(); you may wish to
use those routines again to achieve the correct ordering.

\retval kPreferences_ResultOK
if the context is successfully moved

\retval kPreferences_ResultInvalidContextReference
if one of the specified contexts does not exist

\retval kPreferences_ResultGenericFailure
if other failures occur

(4.0)
*/
Preferences_Result
Preferences_ContextRepositionRelativeToContext	(Preferences_ContextRef		inContext,
												 Preferences_ContextRef		inReferenceContext,
												 Boolean					inInsertBefore)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	My_ContextAutoLocker	referencePtr(gMyContextPtrLocks(), inReferenceContext);
	
	
	if ((nullptr == ptr) || (nullptr == referencePtr)) result = kPreferences_ResultInvalidContextReference;
	else
	{
		// first change the in-memory version of the list
		My_ContextFavoritePtr		derivedPtr = STATIC_CAST(&*ptr, My_ContextFavoritePtr);
		My_ContextFavoritePtr		derivedReferencePtr = STATIC_CAST(&*referencePtr, My_ContextFavoritePtr);
		My_FavoriteContextList*		listPtr = nullptr;
		
		
		if (false == getListOfContexts(derivedPtr->returnClass(), listPtr)) result = kPreferences_ResultGenericFailure;
		else
		{
			My_FavoriteContextList::iterator		toMovedContextPtr = std::find_if
																		(listPtr->begin(), listPtr->end(),
																			std::bind2nd
																			(std::equal_to< My_ContextFavoritePtr >(),
																				derivedPtr));
			My_FavoriteContextList::iterator		toRefContextPtr = std::find_if
																		(listPtr->begin(), listPtr->end(),
																			std::bind2nd
																			(std::equal_to< My_ContextFavoritePtr >(),
																				derivedReferencePtr));
			
			
			if ((listPtr->end() == toMovedContextPtr) || (listPtr->end() == toRefContextPtr))
			{
				result = kPreferences_ResultInvalidContextReference;
			}
			else
			{
				listPtr->erase(toMovedContextPtr);
				toRefContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
												std::bind2nd(std::equal_to< My_ContextFavoritePtr >(), derivedReferencePtr));
				assert(listPtr->end() != toRefContextPtr);
				if (false == inInsertBefore) std::advance(toRefContextPtr, +1);
				listPtr->insert(toRefContextPtr, derivedPtr);
			}
		}
		
		// now change the saved version of the list
		result = derivedPtr->shift(derivedReferencePtr, inInsertBefore);
		
		changeNotify(kPreferences_ChangeNumberOfContexts);
	}
	return result;
}// ContextRepositionRelativeToContext


/*!
Changes the position of a context in the list of contexts
for its class; this will become permanent the next time
application preferences are synchronized.

The delta is +1 to move the item down the list by one, or
-1 to move it up the list by one.  If moving the context
would fall off the end of the list, this routine does
nothing.

Preferences_ContextRepositionRelativeToContext() is called
by this routine, so all of the side effects of that call apply.

\retval kPreferences_ResultOK
if the context is successfully moved

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist in its class list

\retval kPreferences_ResultGenericFailure
if other failures occur

(4.0)
*/
Preferences_Result
Preferences_ContextRepositionRelativeToSelf		(Preferences_ContextRef		inContext,
												 SInt32						inDelta)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else if (0 != inDelta)
	{
		My_ContextFavoritePtr		derivedPtr = STATIC_CAST(&*ptr, My_ContextFavoritePtr);
		My_FavoriteContextList*		listPtr = nullptr;
		
		
		if (false == getListOfContexts(derivedPtr->returnClass(), listPtr)) result = kPreferences_ResultGenericFailure;
		else
		{
			// find the position of the specified context in the list
			My_FavoriteContextList::iterator	toContextPtr = std::find_if
																(listPtr->begin(), listPtr->end(),
																	std::bind2nd
																	(std::equal_to< My_ContextFavoritePtr >(),
																		derivedPtr));
			
			
			if (listPtr->end() == toContextPtr) result = kPreferences_ResultInvalidContextReference;
			else
			{
				SInt32 const								kSizeAsInt = STATIC_CAST(listPtr->size(), SInt32);
				My_FavoriteContextList::size_type const		kOldItemIndex = std::distance(listPtr->begin(), toContextPtr);
				SInt32										newItemIndex = STATIC_CAST(kOldItemIndex, SInt32) + inDelta;
				
				
				if (newItemIndex < 0) newItemIndex = 0;
				if (newItemIndex >= kSizeAsInt) newItemIndex = kSizeAsInt - 1;
				
				std::advance(toContextPtr, newItemIndex - kOldItemIndex);
				
				result = Preferences_ContextRepositionRelativeToContext
							(inContext, (*toContextPtr)->selfRef, (inDelta < 0)/* insert before */);
			}
		}
	}
	return result;
}// ContextRepositionRelativeToSelf


/*!
Returns the class ID used to create the specified context.
"kPreferences_ClassGeneral" is returned if there is no way
to tell what the class is.

(4.0)
*/
Preferences_Class
Preferences_ContextReturnClass		(Preferences_ContextRef		inContext)
{
	Preferences_Class		result = kPreferences_ClassGeneral;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr != ptr)
	{
		result = ptr->returnClass();
	}
	return result;
}// ContextReturnClass


/*!
Saves any in-memory preferences data model changes to
disk, and updates the in-memory model with any new
settings on disk.  This has the side effect of saving
global preferences changes as well, but it will not
commit changes to other specific contexts.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultGenericFailure
if preferences could not be fully saved for any reason

(3.1)
*/
Preferences_Result
Preferences_ContextSave		(Preferences_ContextRef		inContext)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		result = ptr->save();
		if (kPreferences_ResultOK == result)
		{
			// now do a retain without a release, which will prevent this
			// context from disappearing from the internal list (see also
			// Preferences_ContextDeleteSaved(), which should undo this by
			// doing a release without a retain)
			Preferences_RetainContext(inContext);
		}
	}
	return result;
}// ContextSave


/*!
Sets a preference corresponding to the specified tag.

If the tag has a "kPreferences_TagIndexed…" name, you should
call Preferences_ReturnTagVariantForIndex() to produce the
actual tag for the specific index you are interested in.

Note that any data you set via this routine will be permanently
saved to disk eventually (via Preferences_Done()).  You can
choose to save explicitly with Preferences_Save(), but that is
not strictly necessary unless you are paranoid that the
application will crash before Preferences_Done() is invoked.

\retval kPreferences_ResultOK
if the data is set properly

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultUnknownTagOrClass
if the specified tag is not understood

(3.0)
*/
Preferences_Result
Preferences_ContextSetData	(Preferences_ContextRef		inContext,
							 Preferences_Tag			inDataPreferenceTag,
							 size_t						inDataSize,
							 void const*				inDataPtr)
{
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0;
	Preferences_Class		dataClass = kPreferences_ClassGeneral;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
		
		
		switch (dataClass)
		{
		case kPreferences_ClassFormat:
			result = setFormatPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassGeneral:
			result = setGeneralPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassMacroSet:
			result = setMacroPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassSession:
			result = setSessionPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassTerminal:
			result = setTerminalPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassTranslation:
			result = setTranslationPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		default:
			result = kPreferences_ResultUnknownTagOrClass;
			break;
		}
	}
	return result;
}// ContextSetData


/*!
Arranges for a callback to be invoked every time the specified
setting is directly changed in the given context.

Note that if a tag is indexed, you have to monitor each possible
index individually (call Preferences_ReturnTagVariantForIndex()
to generate such a tag, and if necessary in your callback use
Preferences_ReturnTagFromVariant()/Preferences_ReturnTagIndex()
to “decode” the base tag and index value).

It is also possible for a context to change in “batch mode”, in
which case the change is NOT sent to individual listeners but
is rather its own event: "kPreferences_ChangeContextBatchMode".
You MUST register for this event in addition to your regular tag
if you wish to detect batch-mode changes as well (e.g. when the
entire context is duplicated).  The appropriate response to a
batch mode event is to simply pretend every tag you care about
has changed at once.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultInvalidContextReference
if the given context is invalid

\retval kPreferences_ResultGenericFailure
if the listener could not be added

(3.1)
*/
Preferences_Result
Preferences_ContextStartMonitoring		(Preferences_ContextRef		inContext,
										 ListenerModel_ListenerRef	inListener,
										 Preferences_Change			inForWhatChange,
										 Boolean					inNotifyOfInitialValue)
{
	Preferences_Result		result = kPreferences_ResultGenericFailure;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		Boolean		addOK = ptr->addListener(inListener, inForWhatChange, inNotifyOfInitialValue);
		
		
		if (addOK) result = kPreferences_ResultOK;
	}
	return result;
}// ContextStartMonitoring


/*!
Arranges for a callback to no longer be invoked every time the
specified setting is changed in the given context.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultInvalidContextReference
if the given context is invalid

\retval kPreferences_ResultGenericFailure
if the listener could not be removed

(3.1)
*/
Preferences_Result
Preferences_ContextStopMonitoring	(Preferences_ContextRef		inContext,
									 ListenerModel_ListenerRef	inListener,
									 Preferences_Change			inForWhatChange)
{
	Preferences_Result		result = kPreferences_ResultGenericFailure;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		Boolean		removeOK = ptr->removeListener(inListener, inForWhatChange);
		
		
		if (removeOK) result = kPreferences_ResultOK;
	}
	return result;
}// ContextStopMonitoring


/*!
Provides a list of the names of valid contexts in the
specified preferences class; none of these names should
be used when calling Preferences_NewContextFromFavorites(),
otherwise the creation will fail.

The Core Foundation array is allocated, so you must invoke
CFRelease() on the array when finished with it (however,
it is not necessary to do this for the array’s elements).

If there is a problem creating any of the strings, a
non-success code may be returned, although the array
could still be valid.  If the output array is not nullptr,
it must be released regardless.

\retval kPreferences_ResultOK
if all context names were found and retrieved successfully

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is not valid

\retval kPreferences_ResultOneOrMoreNamesNotAvailable
if at least one of the context names could not be retrieved
(but, the rest of the array will be valid)

\retval kPreferences_ResultGenericFailure
if any other error occurs

(3.1)
*/
Preferences_Result
Preferences_CreateContextNameArray	(Preferences_Class		inClass,
									 CFArrayRef&			outNewArrayOfNewCFStrings)
{
	Preferences_Result			result = kPreferences_ResultGenericFailure;
	My_FavoriteContextList*		listPtr = nullptr;
	
	
	if (false == getListOfContexts(inClass, listPtr)) result = kPreferences_ResultOneOrMoreNamesNotAvailable;
	else
	{
		CFMutableArrayRef	newArray = CFArrayCreateMutable(kCFAllocatorDefault, listPtr->size(),
															&kCFTypeArrayCallBacks);
		
		
		if (nullptr != newArray)
		{
			My_FavoriteContextList::const_iterator		toContextPtr = listPtr->begin();
			
			
			result = kPreferences_ResultOK; // initially...
			for (CFIndex i = 0; toContextPtr != listPtr->end(); ++toContextPtr)
			{
				My_ContextFavoritePtr	contextPtr = *toContextPtr;
				CFStringRef				thisName = contextPtr->returnName();
				
				
				if (nullptr == thisName) result = kPreferences_ResultOneOrMoreNamesNotAvailable;
				else
				{
					CFRetain(thisName);
					CFArraySetValueAtIndex(newArray, i, thisName);
					++i;
				}
			}
			outNewArrayOfNewCFStrings = newArray;
		}
	}
	
	return result;
}// CreateContextNameArray


/*!
Creates a name for a context in the given class, that no
other context is currently using.  You may optionally give
a name that the new name should be “similar” to.  You must
call CFRelease() on the new string when finished.

Since this routine uses a list of contexts in memory, it
is able to generate a unique name even among recently
constructed contexts that have not yet been saved.

\retval kPreferences_ResultOK
if a name was created successfully

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is not valid

\retval kPreferences_ResultGenericFailure
if any other error occurs

(3.1)
*/
Preferences_Result
Preferences_CreateUniqueContextName		(Preferences_Class	inClass,
										 CFStringRef&		outNewName,
										 CFStringRef		inBaseNameOrNull)
{
	Preferences_Result			result = kPreferences_ResultGenericFailure;
	My_FavoriteContextList*		listPtr = nullptr;
	
	
	if (getListOfContexts(inClass, listPtr))
	{
		CFIndex const								kMaxTries = 100; // arbitrary
		CFStringRef const							kBaseName = (nullptr == inBaseNameOrNull)
																? CFSTR("untitled") // LOCALIZE THIS
																: inBaseNameOrNull;
		My_FavoriteContextList::const_iterator		toContextPtr = listPtr->end();
		CFMutableStringRef							nameTemplateCFString = CFStringCreateMutableCopy
																			(kCFAllocatorDefault,
																				0/* maximum length */,
																				kBaseName);
		
		
		// keep trying names until a unique one is found
		result = kPreferences_ResultGenericFailure; // initially...
		if (nullptr != nameTemplateCFString)
		{
			CFStringAppend(nameTemplateCFString, CFSTR(" %ld")); // LOCALIZE THIS
			
			for (CFIndex i = 2; ((i <= kMaxTries) && (kPreferences_ResultOK != result)); ++i)
			{
				CFStringRef		currentNameCFString = CFStringCreateWithFormat
														(kCFAllocatorDefault, nullptr/* options */,
															nameTemplateCFString, i/* substitution */);
				
				
				if (nullptr != currentNameCFString)
				{
					toContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
												contextNameEqualTo(currentNameCFString));
					if (listPtr->end() == toContextPtr)
					{
						// success!
						outNewName = CFStringCreateCopy(kCFAllocatorDefault, currentNameCFString);
						result = kPreferences_ResultOK;
					}
					CFRelease(currentNameCFString), currentNameCFString = nullptr;
				}
			}
			CFRelease(nameTemplateCFString), nameTemplateCFString = nullptr;
		}
	}
	
	return result;
}// CreateUniqueContextName


/*!
Ensures that all necessary preferences folders exist,
and reads some global default preferences.

\retval kPreferences_ResultOK
if no error occurred (currently the only possible return code)

(3.0)
*/
Preferences_Result
Preferences_CreateOrFindFiles ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = assertInitialized();
	if (result == kPreferences_ResultOK)
	{
		// create the MacTelnet Preferences folder and important sub-folders
		FSRef		unusedFolderRef;
		
		
		// create the MacTelnet Application Support folder (no effect if it already exists); the resultant FSSpec is ignored
		Folder_GetFSRef(kFolder_RefApplicationSupport, unusedFolderRef);
		
		// create the MacTelnet Preferences folder (no effect if it already exists); the resultant FSSpec is ignored
		Folder_GetFSRef(kFolder_RefPreferences, unusedFolderRef);
		
		// create the Recent Sessions folder (no effect if it already exists); the resultant FSSpec is ignored
		Folder_GetFSRef(kFolder_RefRecentSessions, unusedFolderRef);
		
		// create the Favorites folder (no effect if it already exists); the resultant FSSpec is ignored
		Folder_GetFSRef(kFolder_RefFavorites, unusedFolderRef);
		
		// create the Macro Sets Favorites folder (no effect if it already exists); the resultant FSSpec is ignored
		Folder_GetFSRef(kFolder_RefUserMacroFavorites, unusedFolderRef);
		
		// other setup
		{
			Str255		notificationMessage;
			UInt16		notificationPreferences = kAlert_NotifyDisplayDiamondMark;
			size_t		actualSize = 0L;
			
			
			unless (Preferences_GetData(kPreferences_TagNotification, sizeof(notificationPreferences),
										&notificationPreferences, &actualSize) ==
					kPreferences_ResultOK)
			{
				notificationPreferences = kAlert_NotifyDisplayDiamondMark; // assume default, if preference can’t be found
			}
			
			// the Interface Library Alert module is responsible for handling Notification Manager stuff...
			Alert_SetNotificationPreferences(notificationPreferences);
			// TEMPORARY: This needs a new localized string.  LOCALIZE THIS.
			//GetIndString(notificationMessage, rStringsNoteAlerts, siNotificationAlert);
			//Alert_SetNotificationMessage(notificationMessage);
		}
	}
	return result;
}// CreateOrFindFiles


/*!
Provides a list of all created contexts in the specified
preferences class.  The first context in the list is the
default context (see also Preferences_GetDefaultContext()),
so the list will always be at least one element long.

The contexts are not retained, so be sure to retain them
if you keep them indefinitely.

Returns true only if successful.  It may fail, for instance,
if the given class is invalid.  Even when failing, however,
the list may contain a partial set of valid contexts.

(3.1)
*/
Boolean
Preferences_GetContextsInClass	(Preferences_Class							inClass,
								 std::vector< Preferences_ContextRef >&		outContextsList)
{
	Boolean					result = false;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef	defaultContext = nullptr;
	
	
	prefsResult = Preferences_GetDefaultContext(&defaultContext, inClass);
	if (kPreferences_ResultOK == prefsResult)
	{
		My_FavoriteContextList*		listPtr = nullptr;
		
		
		outContextsList.push_back(defaultContext);
		assert(false == outContextsList.empty());
		assert(outContextsList.back() == defaultContext);
		
		if (getListOfContexts(inClass, listPtr))
		{
			My_FavoriteContextList::const_iterator		toContextPtr = listPtr->begin();
			
			
			for (; toContextPtr != listPtr->end(); ++toContextPtr)
			{
				My_ContextFavoritePtr	contextPtr = *toContextPtr;
				
				
				outContextsList.push_back(REINTERPRET_CAST(contextPtr, Preferences_ContextRef));
			}
			result = true;
		}
	}
	return result;
}// GetContextsInClass


/*!
Calls Preferences_ContextGetData() using the global
context for the class of data associated with the
given tag.

(3.1)
*/
Preferences_Result
Preferences_GetData		(Preferences_Tag	inDataPreferenceTag,
						 size_t				inDataStorageSize,
						 void*				outDataStorage,
						 size_t*			outActualSizePtrOrNull)
{
	Preferences_Result			result = kPreferences_ResultOK;
	Preferences_ContextRef		context = nullptr;
	CFStringRef					keyName = nullptr;
	FourCharCode				keyValueType = '----';
	size_t						actualSize = 0;
	Preferences_Class			dataClass = kPreferences_ClassGeneral;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		result = Preferences_GetDefaultContext(&context, dataClass);
		if (kPreferences_ResultOK == result)
		{
			result = Preferences_ContextGetData(context, inDataPreferenceTag, inDataStorageSize,
												outDataStorage, false/* search defaults too */, outActualSizePtrOrNull);
		}
	}
	return result;
}// GetData


/*!
Returns the default context for the specified class.
Preferences written to the "kPreferences_ClassGeneral"
default context are in fact global settings.

Despite the given class, the resulting context could
easily contain additional settings you did not expect.
It is not a good idea to do a blanket copy of a default
context; read only what you need.

WARNING:	Do not dispose of default contexts.

\retval kPreferences_ResultOK
if the requested context was found

\retval kPreferences_ResultInsufficientBufferSpace
if "outContextPtr" is not valid

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is not valid

(3.1)
*/
Preferences_Result
Preferences_GetDefaultContext	(Preferences_ContextRef*	outContextPtr,
								 Preferences_Class			inClass)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = assertInitialized();
	if (result == kPreferences_ResultOK)
	{
		if (nullptr == outContextPtr) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			My_ContextInterfacePtr		contextPtr = nullptr;
			
			
			if (false == getDefaultContext(inClass, contextPtr))
			{
				result = kPreferences_ResultUnknownTagOrClass;
			}
			else
			{
				*outContextPtr = REINTERPRET_CAST(contextPtr, Preferences_ContextRef);
			}
		}
	}
	return result;
}// GetDefaultContext


/*!
Returns the “factory defaults” context, which represents
core settings ("DefaultPreferences.plist") that have not
been changed by the user.  This can be used to reset
something, if you have no other reasonable value to set
it to.

Attempting to change the returned context has no effect
and generates a warning in debug mode.

WARNING:	Do not dispose of factory defaults.

\retval kPreferences_ResultOK
if the requested context was found

\retval kPreferences_ResultInsufficientBufferSpace
if "outContextPtr" is not valid

\retval kPreferences_ResultGenericFailure
if anything else fails

(3.1)
*/
Preferences_Result
Preferences_GetFactoryDefaultsContext	(Preferences_ContextRef*	outContextPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = assertInitialized();
	if (result == kPreferences_ResultOK)
	{
		if (nullptr == outContextPtr) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			My_ContextInterfacePtr		contextPtr = nullptr;
			
			
			if (false == getFactoryDefaultsContext(contextPtr))
			{
				result = kPreferences_ResultGenericFailure;
			}
			else
			{
				*outContextPtr = REINTERPRET_CAST(contextPtr, Preferences_ContextRef);
			}
		}
	}
	return result;
}// GetFactoryDefaultsContext


/*!
Adds to the specified menu a list of the names of valid
contexts in the specified preferences class.

If there is a problem adding anything to the menu, a
non-success code may be returned, although the menu may
still be partially changed.  The "outHowManyItemsAdded"
value is always valid and indicates how many items were
added regardless of errors.

\retval kPreferences_ResultOK
if all context names were found and added successfully

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is not valid

\retval kPreferences_ResultOneOrMoreNamesNotAvailable
if at least one of the context names could not be retrieved

(3.1)
*/
Preferences_Result
Preferences_InsertContextNamesInMenu	(Preferences_Class		inClass,
										 MenuRef				inoutMenuRef,
										 MenuItemIndex			inAfterItemIndex,
										 UInt32					inInitialIndent,
										 UInt32					inCommandID,
										 MenuItemIndex&			outHowManyItemsAdded)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFArrayRef				nameCFStringCFArray = nullptr;
	
	
	result = Preferences_CreateContextNameArray(inClass, nameCFStringCFArray);
	if (result == kPreferences_ResultOK)
	{
		// now re-populate the menu using resource information
		MenuItemIndex const		kMenuItemCount = CFArrayGetCount(nameCFStringCFArray);
		MenuItemIndex			i = 0;
		CFStringRef				nameCFString = nullptr;
		OSStatus				error = noErr;
		
		
		outHowManyItemsAdded = 0; // initially...
		for (i = 0; i < kMenuItemCount; ++i)
		{
			nameCFString = CFUtilities_StringCast(CFArrayGetValueAtIndex(nameCFStringCFArray, i));
			if (nullptr != nameCFString)
			{
				error = InsertMenuItemTextWithCFString(inoutMenuRef, nameCFString, inAfterItemIndex + outHowManyItemsAdded,
														kMenuItemAttrIgnoreMeta /* attributes */,
														inCommandID/* command ID */);
				if (noErr == error)
				{
					++outHowManyItemsAdded;
					(OSStatus)SetMenuItemIndent(inoutMenuRef, inAfterItemIndex + outHowManyItemsAdded, inInitialIndent);
				}
			}
		}
		
		CFRelease(nameCFStringCFArray), nameCFStringCFArray = nullptr;
	}
	
	return result;
}// InsertContextNamesInMenu


/*!
Saves any in-memory preferences data model changes to
disk, and updates the in-memory model with any new
settings on disk.  This includes all contexts that
have been created and not released, but also see
Preferences_ContextSave().

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if preferences could not be fully saved for any reason

(3.1)
*/
Preferences_Result
Preferences_Save ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// make sure all open context dictionaries are in preferences too
	std::for_each(gFormatNamedContexts().begin(), gFormatNamedContexts().end(), contextSave());
	std::for_each(gMacroSetNamedContexts().begin(), gMacroSetNamedContexts().end(), contextSave());
	std::for_each(gSessionNamedContexts().begin(), gSessionNamedContexts().end(), contextSave());
	std::for_each(gTerminalNamedContexts().begin(), gTerminalNamedContexts().end(), contextSave());
	std::for_each(gTranslationNamedContexts().begin(), gTranslationNamedContexts().end(), contextSave());
	
	if (false == CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication))
	{
		result = kPreferences_ResultGenericFailure;
	}
	return result;
}// Save


/*!
Calls Preferences_ContextSetData() using the global
context for the class of data associated with the
given tag.

(3.1)
*/
Preferences_Result
Preferences_SetData		(Preferences_Tag	inDataPreferenceTag,
						 size_t				inDataSize,
						 void const*		inDataPtr)
{
	Preferences_Result			result = kPreferences_ResultOK;
	Preferences_ContextRef		context = nullptr;
	CFStringRef					keyName = nullptr;
	FourCharCode				keyValueType = '----';
	size_t						actualSize = 0;
	Preferences_Class			dataClass = kPreferences_ClassGeneral;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		result = Preferences_GetDefaultContext(&context, dataClass);
		if (kPreferences_ResultOK == result)
		{
			result = Preferences_ContextSetData(context, inDataPreferenceTag, inDataSize, inDataPtr);
		}
	}
	return result;
}// SetData


/*!
Arranges for a callback to be invoked whenever a user preference
changes globally.  The event context passed to the listener is a
pointer to a data structure of type "Preferences_ChangeContext".
The listener context is currently reserved and set to "nullptr".

Some changes are specific events.  However, most change codes
directly match preferences tags, allowing you to monitor
changes to those preferences.  For efficiency, most changes
do NOT trigger events: see the code for this routine to find out
the subset of tags you can use.

(3.0)
*/
Preferences_Result
Preferences_StartMonitoring		(ListenerModel_ListenerRef	inListener,
								 Preferences_Change			inForWhatChange,
								 Boolean					inNotifyOfInitialValue)
{
	Preferences_Result		result = kPreferences_ResultBadVersionDataNotAvailable;
	
	
	switch (inForWhatChange)
	{
	// If you change the list below, also check the preference-setting
	// code to make sure that the tag values checked here REALLY DO
	// trigger callback invocations!  Also keep this in sync with
	// Preferences_StopMonitoring().
	case kPreferences_TagArrangeWindowsUsingTabs:
	case kPreferences_TagBellSound:
	case kPreferences_TagCursorBlinks:
	case kPreferences_TagDontDimBackgroundScreens:
	case kPreferences_TagFocusFollowsMouse:
	case kPreferences_TagMacrosMenuVisible:
	case kPreferences_TagMapBackquote:
	case kPreferences_TagMenuItemKeys:
	case kPreferences_TagNewCommandShortcutEffect:
	case kPreferences_TagPureInverse:
	case kPreferences_TagSimplifiedUserInterface:
	case kPreferences_TagTerminalCursorType:
	case kPreferences_TagTerminalResizeAffectsFontSize:
	case kPreferences_TagTerminalScrollDelay:
	case kPreferences_ChangeContextName:
	case kPreferences_ChangeNumberOfContexts:
		result = assertInitialized();
		if (result == kPreferences_ResultOK)
		{
			OSStatus	error = noErr;
			
			
			error = ListenerModel_AddListenerForEvent(gPreferenceEventListenerModel, inForWhatChange,
														inListener);
			if (error != noErr)
			{
				// should probably define a more specific error type here...
				result = kPreferences_ResultInsufficientBufferSpace;
			}
			
			// if requested, invoke the callback immediately so that
			// the receiver can react accordingly given the initial
			// value of a particular preference
			if (inNotifyOfInitialValue)
			{
				changeNotify(inForWhatChange, nullptr/* context */, true/* is initial value */);
			}
		}
		break;
	
	default:
		// unsupported tag for notifiers
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked when
the specified user preference changes.

\retval kPreferences_ResultOK
if no error occurs

\retval kPreferences_ResultUnknownTagOrClass
if the specified preference change is not supported
with notifiers; therefore, you will have to use a
polling solution

(3.0)
*/
Preferences_Result
Preferences_StopMonitoring	(ListenerModel_ListenerRef	inListener,
							 Preferences_Change			inForWhatChange)
{
	Preferences_Result		result = kPreferences_ResultBadVersionDataNotAvailable;
	
	
	switch (inForWhatChange)
	{
	// Keep this in sync with Preferences_StartMonitoring().
	case kPreferences_TagArrangeWindowsUsingTabs:
	case kPreferences_TagBellSound:
	case kPreferences_TagCursorBlinks:
	case kPreferences_TagDontDimBackgroundScreens:
	case kPreferences_TagFocusFollowsMouse:
	case kPreferences_TagMacrosMenuVisible:
	case kPreferences_TagMapBackquote:
	case kPreferences_TagMenuItemKeys:
	case kPreferences_TagNewCommandShortcutEffect:
	case kPreferences_TagPureInverse:
	case kPreferences_TagSimplifiedUserInterface:
	case kPreferences_TagTerminalCursorType:
	case kPreferences_TagTerminalResizeAffectsFontSize:
	case kPreferences_TagTerminalScrollDelay:
	case kPreferences_ChangeContextName:
	case kPreferences_ChangeNumberOfContexts:
		result = assertInitialized();
		if (result == kPreferences_ResultOK)
		{
			ListenerModel_RemoveListenerForEvent(gPreferenceEventListenerModel, inForWhatChange, inListener);
		}
		break;
	
	default:
		// unsupported tag for notifiers
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// StopMonitoring


#pragma mark Internal Methods
namespace {

/*!
Constructor.  Used only by subclasses.

(3.1)
*/
My_ContextInterface::
My_ContextInterface		(Preferences_Class		inClass)
:
selfRef(REINTERPRET_CAST(this, Preferences_ContextRef)),
_preferencesClass(inClass),
_listenerModel(nullptr/* constructed as needed */),
_implementorPtr(nullptr)
{
}// My_ContextInterface 1-argument constructor


/*!
Destructor.  Used only by subclasses.

(3.1)
*/
My_ContextInterface::
~My_ContextInterface ()
{
	if (nullptr != _listenerModel) ListenerModel_Dispose(&_listenerModel);
}// My_ContextInterface destructor


/*!
Arranges for a callback to be invoked every time the specified
setting in the context is changed.  Returns true only if
successful.

(3.1)
*/
Boolean
My_ContextInterface::
addListener		(ListenerModel_ListenerRef	inListener,
				 Preferences_Change			inForWhatChange,
				 Boolean					inNotifyOfInitialValue)
{
	Boolean		result = false;
	OSStatus	error = noErr;
	
	
	if (nullptr == _listenerModel)
	{
		_listenerModel = ListenerModel_New(kListenerModel_StyleStandard, 'PCtx');
		assert(nullptr != _listenerModel);
	}
	
	if (nullptr != _listenerModel)
	{
		error = ListenerModel_AddListenerForEvent(_listenerModel, inForWhatChange, inListener);
		if (noErr == error) result = true;
	}
	
	if ((result) && (inNotifyOfInitialValue))
	{
		this->notifyListeners(inForWhatChange);
	}
	
	return result;
}// My_ContextInterface::addListener


/*!
If there are any monitors on this context, notifies them all
that something has changed.

You can pass the special tag of 0 to indicate no notifications.
This is useful in some cases where code “always notifies” and
you want it to do nothing.

(3.1)
*/
void
My_ContextInterface::
notifyListeners		(Preferences_Tag	inWhatChanged)
{
	if ((nullptr != _listenerModel) && (0 != inWhatChanged))
	{
		ListenerModel_NotifyListenersOfEvent(_listenerModel, inWhatChanged, this->selfRef);
	}
}// My_ContextInterface::notifyListeners


/*!
Removes a previously-added listener.  Returns true only if
successful.

(3.1)
*/
Boolean
My_ContextInterface::
removeListener	(ListenerModel_ListenerRef	inListener,
				 Preferences_Change			inForWhatChange)
{
	Boolean		result = false;
	
	
	if (nullptr != _listenerModel)
	{
		ListenerModel_Result	modelResult = kListenerModel_ResultOK;
		
		
		modelResult = ListenerModel_RemoveListenerForEvent(_listenerModel, inForWhatChange, inListener);
		if (kListenerModel_ResultOK == modelResult) result = true;
	}
	return result;
}// My_ContextInterface::removeListener


/*!
Returns a constant indicating the category to which
this context belongs.  The category generally
indicates what preference keys should be used.

(3.1)
*/
Preferences_Class
My_ContextInterface::
returnClass ()
const
{
	return _preferencesClass;
}// My_ContextInterface::returnClass


/*!
Specifies the delegate instance supporting the
CFKeyValueInterface API that will be asked to perform
all functions.  You MUST set this before using any of
the other methods of this class!
*/
void
My_ContextInterface::
setImplementor	(CFKeyValueInterface*	inImplementorPtr)
{
	_implementorPtr = inImplementorPtr;
}// My_ContextInterface::setImplementor


/*!
Tests a subclass instance that claims to support this
interface properly.  Returns true only if successful.
Information on failures is printed to the console.

(3.1)
*/
Boolean
My_ContextInterface::
unitTest	(My_ContextInterface*	inTestObjectPtr)
{
	Boolean		result = true;
	
	
	// array values
	{
		CFStringRef			values[] = { CFSTR("__test_1__"), CFSTR("__test_2__") };
		CFRetainRelease		testArray(CFArrayCreate(kCFAllocatorDefault, REINTERPRET_CAST(values, void const**),
													sizeof(values) / sizeof(CFStringRef),
													&kCFTypeArrayCallBacks), true/* is retained */);
		CFRetainRelease		copiedArray;
		
		
		result &= Console_Assert("test array exists", testArray.exists());
		result &= Console_Assert("test array is the right size", 2 == CFArrayGetCount(testArray.returnCFArrayRef()));
		inTestObjectPtr->addArray(0/* do not notify */, CFSTR("__test_array_key__"), testArray.returnCFArrayRef());
		copiedArray = inTestObjectPtr->returnArrayCopy(CFSTR("__test_array_key__"));
		result &= Console_Assert("returned array exists", copiedArray.exists());
		result &= Console_Assert("returned array is the right size", 2 == CFArrayGetCount(copiedArray.returnCFArrayRef()));
	}
	
	// data values
	{
		char const* const	kData = "my test string";
		size_t const		kDataSize = (1 + std::strlen(kData)) * sizeof(char);
		CFRetainRelease		testData(CFDataCreate(kCFAllocatorDefault, REINTERPRET_CAST(kData, UInt8 const*),
													kDataSize), true/* is retained */);
		CFDataRef			testDataRef = REINTERPRET_CAST(testData.returnCFTypeRef(), CFDataRef);
		CFRetainRelease		copiedValue;
		
		
		result &= Console_Assert("test data exists", testData.exists());
		result &= Console_Assert("test data is the right size", STATIC_CAST(kDataSize, CFIndex) == CFDataGetLength(testDataRef));
		inTestObjectPtr->addData(0/* do not notify */, CFSTR("__test_data_key__"), REINTERPRET_CAST(testData.returnCFTypeRef(), CFDataRef));
		copiedValue = inTestObjectPtr->returnValueCopy(CFSTR("__test_data_key__"));
		result &= Console_Assert("returned data exists", copiedValue.exists());
		result &= Console_Assert("returned data is the right size",
									STATIC_CAST(kDataSize, CFIndex) == CFDataGetLength(REINTERPRET_CAST
																						(copiedValue.returnCFTypeRef(), CFDataRef)));
	}
	
	// flag (Boolean) values
	{
		Boolean const	kFlag1 = true;
		Boolean const	kFlag2 = true;
		
		
		inTestObjectPtr->addFlag(0/* do not notify */, CFSTR("__test_flag_key_1__"), kFlag1);
		result &= Console_Assert("returned flag is true", kFlag1 == inTestObjectPtr->returnFlag(CFSTR("__test_flag_key_1__")));
		inTestObjectPtr->addFlag(0/* do not notify */, CFSTR("__test_flag_key_2__"), kFlag2);
		result &= Console_Assert("returned flag is false", kFlag2 == inTestObjectPtr->returnFlag(CFSTR("__test_flag_key_2__")));
		result &= Console_Assert("nonexistent flag is false", false == inTestObjectPtr->returnFlag(CFSTR("flag does not exist")));
	}
	
	// floating-point values
	{
		Float32 const	kFloat1 = 0;
		Float32 const	kFloat2 = 1;
		Float32 const	kFloat3 = -36.4;
		Float32 const	kFloat4 = 5312.79195;
		Float32 const	kTolerance = 0.1; // for fuzzy equality
		Float32			returnedFloat = 0.0;
		
		
		inTestObjectPtr->addFloat(0/* do not notify */, CFSTR("__test_float_key_1__"), kFloat1);
		inTestObjectPtr->addFloat(0/* do not notify */, CFSTR("__test_float_key_2__"), kFloat2);
		inTestObjectPtr->addFloat(0/* do not notify */, CFSTR("__test_float_key_3__"), kFloat3);
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_1__"));
		result &= Console_Assert("returned float is close to zero",
									(returnedFloat > (kFloat1 - kTolerance)) && (returnedFloat < (kFloat1 + kTolerance)));
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_2__"));
		result &= Console_Assert("returned float is close to 1",
									(returnedFloat > (kFloat2 - kTolerance)) && (returnedFloat < (kFloat2 + kTolerance)));
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_3__"));
		result &= Console_Assert("returned float is close to -36.4",
									(returnedFloat > (kFloat3 - kTolerance)) && (returnedFloat < (kFloat3 + kTolerance)));
		inTestObjectPtr->addFloat(0/* do not notify */, CFSTR("__test_float_key_4__"), kFloat4);
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_4__"));
		result &= Console_Assert("returned float is close to 5312.79195",
									(returnedFloat > (kFloat4 - kTolerance)) && (returnedFloat < (kFloat4 + kTolerance)));
		result &= Console_Assert("nonexistent float is exactly zero", 0 == inTestObjectPtr->returnFloat(CFSTR("float does not exist")));
	}
	
	// integer values
	{
		SInt16 const	kInteger1 = 0;
		SInt16 const	kInteger2 = 1;
		SInt16 const	kInteger3 = -77;
		SInt16 const	kInteger4 = 16122;
		SInt16			returnedInteger = 0;
		
		
		inTestObjectPtr->addInteger(0/* do not notify */, CFSTR("__test_integer_key_1__"), kInteger1);
		inTestObjectPtr->addInteger(0/* do not notify */, CFSTR("__test_integer_key_2__"), kInteger2);
		inTestObjectPtr->addInteger(0/* do not notify */, CFSTR("__test_integer_key_3__"), kInteger3);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_int_key_1__"));
		result &= Console_Assert("returned integer is zero", returnedInteger == kInteger1);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_integer_key_2__"));
		result &= Console_Assert("returned integer is 1", returnedInteger == kInteger2);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_integer_key_3__"));
		result &= Console_Assert("returned integer is -77", returnedInteger == kInteger3);
		inTestObjectPtr->addInteger(0/* do not notify */, CFSTR("__test_integer_key_4__"), kInteger4);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_integer_key_4__"));
		result &= Console_Assert("returned integer is 16122", returnedInteger == kInteger4);
		result &= Console_Assert("nonexistent integer is exactly zero", 0 == inTestObjectPtr->returnInteger(CFSTR("integer does not exist")));
	}
	
	// long integer values
	{
		SInt32 const	kInteger1 = 0L;
		SInt32 const	kInteger2 = 1L;
		SInt32 const	kInteger3 = -9124152L;
		SInt32 const	kInteger4 = 161124507L;
		SInt32			returnedInteger = 0;
		
		
		inTestObjectPtr->addLong(0/* do not notify */, CFSTR("__test_long_key_1__"), kInteger1);
		inTestObjectPtr->addLong(0/* do not notify */, CFSTR("__test_long_key_2__"), kInteger2);
		inTestObjectPtr->addLong(0/* do not notify */, CFSTR("__test_long_key_3__"), kInteger3);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_int_key_1__"));
		result &= Console_Assert("returned long integer is zero", returnedInteger == kInteger1);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_long_key_2__"));
		result &= Console_Assert("returned long integer is 1", returnedInteger == kInteger2);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_long_key_3__"));
		result &= Console_Assert("returned long integer is -9124152", returnedInteger == kInteger3);
		inTestObjectPtr->addLong(0/* do not notify */, CFSTR("__test_long_key_4__"), kInteger4);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_long_key_4__"));
		result &= Console_Assert("returned long integer is 161124507", returnedInteger == kInteger4);
		result &= Console_Assert("nonexistent long integer is exactly zero", 0 == inTestObjectPtr->returnLong(CFSTR("long does not exist")));
	}
	
	// string values and deleted values
	{
		CFStringRef			kString1 = CFSTR("my first test string");
		CFStringRef			kString2 = CFSTR("This is a somewhat more interesting test string.  Hopefully it still works!");
		CFRetainRelease		copiedString;
		
		
		inTestObjectPtr->addString(0/* do not notify */, CFSTR("__test_string_key_1__"), kString1);
		copiedString = inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_1__"));
		result &= Console_Assert("returned string 1 exists", copiedString.exists());
		result &= Console_Assert("returned string 1 is correct",
									kCFCompareEqualTo == CFStringCompare(copiedString.returnCFStringRef(), kString1, 0/* options */));
		inTestObjectPtr->addString(0/* do not notify */, CFSTR("__test_string_key_2__"), kString2);
		copiedString = inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_2__"));
		result &= Console_Assert("returned string 2 exists", copiedString.exists());
		result &= Console_Assert("returned string 2 is correct",
									kCFCompareEqualTo == CFStringCompare(copiedString.returnCFStringRef(), kString2, 0/* options */));
		inTestObjectPtr->deleteValue(CFSTR("__test_string_key_2__"));
		copiedString = inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_2__"));
		result &= Console_Assert("string 2 has been deleted", false == copiedString.exists());
	}
	
	return result;
}// My_ContextInterface::unitTest


/*!
This special constructor enables truly read-only data
(such as factory defaults) to be handled like other
mutable contexts, which is convenient.  It simply
presents the interface of a mutable object, but it is
only valid to use methods that read the data.  Any
attempt to write data will throw an exception.

(3.1)
*/
My_ContextCFDictionary::
My_ContextCFDictionary	(Preferences_Class		inClass,
						 CFDictionaryRef		inDictionary)
:
My_ContextInterface(inClass),
_dictionary(inDictionary)
{
	if (nullptr == _dictionary.returnDictionary())
	{
		throw std::runtime_error("unable to set data dictionary");
	}
	
	setImplementor(&_dictionary);
}// My_ContextCFDictionary 2-argument immutable reference constructor


/*!
Constructor.  See Preferences_NewContext().

(3.1)
*/
My_ContextCFDictionary::
My_ContextCFDictionary	(Preferences_Class			inClass,
						 CFMutableDictionaryRef		inDictionaryOrNull)
:
My_ContextInterface(inClass),
_dictionary(inDictionaryOrNull)
{
	if (nullptr == _dictionary.returnDictionary())
	{
		CFRetainRelease		newDictionary(createDictionary(), true/* is retained */);
		
		
		_dictionary.setDictionary(newDictionary.returnCFMutableDictionaryRef());
	}
	
	if (nullptr == _dictionary.returnDictionary())
	{
		throw std::runtime_error("unable to construct data dictionary");
	}
	
	setImplementor(&_dictionary);
}// My_ContextCFDictionary 2-argument mutable reference constructor


/*!
Creates an empty, infinitely-expandable dictionary for
containing Core Foundation types.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if the dictionary could not be created for any reason

(3.1)
*/
CFMutableDictionaryRef
My_ContextCFDictionary::
createDictionary ()
{
	Preferences_Result			error = kPreferences_ResultOK;
	CFMutableDictionaryRef		result = nullptr;
	
	
	result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0/* capacity */,
										&kCFTypeDictionaryKeyCallBacks,
										&kCFTypeDictionaryValueCallBacks);
	if (nullptr == result) error = kPreferences_ResultGenericFailure;
	else error = kPreferences_ResultOK;
	
	return result;
}// My_ContextCFDictionary::createDictionary


/*!
Since this type of context does not save anything to disk,
there is never anything to destroy, so this has no effect.

\retval kPreferences_ResultOK
always

(3.1)
*/
Preferences_Result
My_ContextCFDictionary::
destroy ()
{
	return kPreferences_ResultOK;
}// My_ContextCFDictionary::destroy


/*!
Always fails because this type of context does not have
a name.

\retval kPreferences_ResultGenericFailure
always

(3.1)
*/
Preferences_Result
My_ContextCFDictionary::
rename	(CFStringRef	UNUSED_ARGUMENT(inNewName))
{
	return kPreferences_ResultGenericFailure;
}// My_ContextCFDictionary::rename


/*!
Returns an empty string (these contexts are unnamed).

(3.1)
*/
CFStringRef
My_ContextCFDictionary::
returnName ()
const
{
	return CFSTR("");
}// My_ContextCFDictionary::returnName


/*!
Returns an error because dictionary-based contexts are
maintained only in memory; there is nowhere to save them.

\retval kPreferences_ResultGenericFailure
always

(3.1)
*/
Preferences_Result
My_ContextCFDictionary::
save ()
{
	return kPreferences_ResultGenericFailure;
}// My_ContextCFDictionary::save


/*!
Tests this class.  Returns true only if successful.
Information on failures is printed to the console.

(3.1)
*/
Boolean
My_ContextCFDictionary::
unitTest ()
{
	Boolean						result = true;
	My_ContextCFDictionary*		testObjectPtr = new My_ContextCFDictionary(kPreferences_ClassGeneral);
	CFArrayRef					keyListCFArray = nullptr;
	
	
	result &= Console_Assert("class is set correctly",
								kPreferences_ClassGeneral == testObjectPtr->returnClass());
	result &= Console_Assert("name is set correctly",
								kCFCompareEqualTo == CFStringCompare
														(CFSTR(""), testObjectPtr->returnName(),
															0/* options */));
	
	result &= Console_Assert("rename correctly failed",
								kPreferences_ResultGenericFailure == testObjectPtr->rename(CFSTR("__test_rename__")));
	
	keyListCFArray = testObjectPtr->returnKeyListCopy();
	result &= Console_Assert("key list exists", nullptr != keyListCFArray);
	result &= Console_Assert("key list is initially empty",
								0 == CFArrayGetCount(keyListCFArray));
	
	result &= My_ContextInterface::unitTest(testObjectPtr);
	
	CFRelease(keyListCFArray), keyListCFArray = nullptr;
	delete testObjectPtr, testObjectPtr = nullptr;
	
	return result;
}// My_ContextCFDictionary::unitTest


/*!
Constructor.  See Preferences_GetDefaultContext().

(3.1)
*/
My_ContextDefault::
My_ContextDefault	(Preferences_Class	inClass)
:
My_ContextInterface(inClass),
_contextName(CFSTR("Default")/* TEMPORARY - LOCALIZE  THIS */),
_dictionary()
{
	setImplementor(&_dictionary);
}// My_ContextDefault 1-argument constructor


/*!
Has no effect, a default context cannot be destroyed.

\retval kPreferences_ResultGenericFailure
always

(3.1)
*/
Preferences_Result
My_ContextDefault::
destroy ()
{
	// NO-OP
	return kPreferences_ResultGenericFailure;
}// My_ContextDefault::destroy


/*!
Has no effect, a default context cannot be renamed.

\retval kPreferences_ResultGenericFailure
always

(3.1)
*/
Preferences_Result
My_ContextDefault::
rename	(CFStringRef	UNUSED_ARGUMENT(inNewName))
{
	// NO-OP
	return kPreferences_ResultGenericFailure;
}// My_ContextDefault::rename


/*!
Returns the name of this context.  This is often
used in user interface elements.  Note that a default
context always has the same name.

(3.1)
*/
CFStringRef
My_ContextDefault::
returnName ()
const
{
	return _contextName.returnCFStringRef();
}// My_ContextDefault::returnName


/*!
Saves any in-memory preferences data model changes to
disk, and updates the in-memory model with any new
settings on disk.  Has the side effect of synchronizing
any other modified application-wide preferences (but
will not save contextual preferences).

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if preferences could not be fully saved for any reason

(3.1)
*/
Preferences_Result
My_ContextDefault::
save ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (false == CFPreferencesAppSynchronize(_dictionary.returnTargetApplication()))
	{
		result = kPreferences_ResultGenericFailure;
	}
	return result;
}// My_ContextDefault::save


/*!
Tests this class.  Returns true only if successful.
Information on failures is printed to the console.

(3.1)
*/
Boolean
My_ContextDefault::
unitTest ()
{
	Boolean				result = true;
	My_ContextDefault*	testObjectPtr = new My_ContextDefault(kPreferences_ClassGeneral);
	CFArrayRef			keyListCFArray = nullptr;
	
	
	result &= Console_Assert("class is set correctly",
								kPreferences_ClassGeneral == testObjectPtr->returnClass());
	result &= Console_Assert("name is set correctly",
								kCFCompareEqualTo == CFStringCompare
														(CFSTR("Default"), testObjectPtr->returnName(),
															0/* options */));
	
	// a Default context is not allowed to be renamed
	testObjectPtr->rename(CFSTR("__test_renamed__"));
	result &= Console_Assert("rename correctly fails",
								kCFCompareEqualTo != CFStringCompare
														(CFSTR("__test_renamed__"), testObjectPtr->returnName(),
															0/* options */));
	
	keyListCFArray = testObjectPtr->returnKeyListCopy();
	result &= Console_Assert("key list exists", nullptr != keyListCFArray);
	result &= Console_Assert("key list is initially populated (based on CFPreferences)",
								0 != CFArrayGetCount(keyListCFArray));
	
	result &= My_ContextInterface::unitTest(testObjectPtr);
	
	CFRelease(keyListCFArray), keyListCFArray = nullptr;
	delete testObjectPtr, testObjectPtr = nullptr;
	
	return result;
}// My_ContextDefault::unitTest


/*!
Constructor.  See Preferences_NewContextFromFavorites().

(3.1)
*/
My_ContextFavorite::
My_ContextFavorite	(Preferences_Class	inClass,
					 CFStringRef		inFavoriteName,
					 CFStringRef		inDomainName)
:
My_ContextInterface(inClass),
_contextName(inFavoriteName),
_domainName((nullptr != inDomainName) ? inDomainName : createDomainName(inClass, inFavoriteName)),
_dictionary(_domainName.returnCFStringRef())
{
	setImplementor(&_dictionary);
}// My_ContextFavorite 3-argument constructor


/*!
Creates an appropriate preferences domain for the
given parameters.  The application preferences are
first consulted, and every domain saved for the
class is checked; if no existing domain uses the
given name, a new domain is created and application
preferences are updated to list the new domain.

(3.1)
*/
CFStringRef
My_ContextFavorite::
createDomainName	(Preferences_Class	inClass,
					 CFStringRef		inName)
{
	CFStringRef				result = nullptr;
	My_ContextFavoritePtr	favoritePtr = nullptr;
	
	
	// see if any domain in the application list contains a name key
	// matching the specified name
	if (getNamedContext(inClass, inName, favoritePtr))
	{
		// specified class/name combination already exists, use its domain
		result = favoritePtr->_domainName.returnCFStringRef();
		CFRetain(result); // “create”
	}
	else
	{
		// does not exist; find a unique domain
		My_FavoriteContextList*		favoritesListPtr = nullptr;
		
		
		if (getListOfContexts(inClass, favoritesListPtr))
		{
			// contexts already exist in this class; check them all, and create a
			// unique domain name of the form <application>.<class>.<id>
			CFMutableStringRef		possibleName = nullptr;
			CFIndex					possibleSuffix = 1 + favoritesListPtr->size();
			Boolean					nameInUse = true;
			
			
			while (nameInUse)
			{
				possibleName = CFStringCreateMutable(kCFAllocatorDefault, 0/* maximum length or 0 for no limit */);
				if (nullptr == possibleName) throw std::bad_alloc();
				CFStringAppend(possibleName, returnClassDomainNamePrefix(inClass));
				CFStringAppendCString(possibleName, ".", kCFStringEncodingUTF8);
				CFStringAppendFormat(possibleName, nullptr/* options */, CFSTR("%d"), possibleSuffix);
				result = possibleName;
				
				nameInUse = false; // initially...
				for (My_FavoriteContextList::const_iterator toContext = favoritesListPtr->begin();
						toContext != favoritesListPtr->end(); ++toContext)
				{
					CFStringRef const	kThisDomain = (*toContext)->_domainName.returnCFStringRef();
					
					
					if (kCFCompareEqualTo == CFStringCompare(kThisDomain, possibleName, kCFCompareBackwards))
					{
						nameInUse = true;
						CFRelease(possibleName), possibleName = nullptr;
						break;
					}
				}
				++possibleSuffix;
			}
		}
		else
		{
			// apparently no contexts at all in this class; easy, just use "1"!
			CFMutableStringRef		mutableResult = CFStringCreateMutable(kCFAllocatorDefault,
																			0/* maximum length or 0 for no limit */);
			
			
			if (nullptr == mutableResult) throw std::bad_alloc();
			CFStringAppend(mutableResult, returnClassDomainNamePrefix(inClass));
			CFStringAppendCString(mutableResult, ".1", kCFStringEncodingUTF8);
			result = mutableResult;
		}
	}
	return result;
}// createDomainName


/*!
Removes the entire dictionary represented by this
context from a list in application preferences, but
does NOT affect what is in memory.  Effects become
permanent (on disk) the next time preferences are
synchronized.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if preferences could not be removed for any reason

(3.1)
*/
Preferences_Result
My_ContextFavorite::
destroy ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFArrayRef				favoritesListCFArray = nullptr;
	CFStringRef const		kDomainName = _domainName.returnCFStringRef();
	
	
	// delete this domain from the application list of domains for the class
	(Preferences_Result)copyClassDomainCFArray(this->returnClass(), favoritesListCFArray);
	if (nullptr != favoritesListCFArray)
	{
		CFIndex		indexForName = findDomainIndexInArray(favoritesListCFArray, kDomainName);
		
		
		if (indexForName >= 0)
		{
			// then an entry with this name exists on disk; remove it from the list!
			CFRetainRelease		mutableFavoritesList(CFArrayCreateMutableCopy
														(kCFAllocatorDefault, CFArrayGetCount(favoritesListCFArray)/* capacity */,
															favoritesListCFArray), true/* is retained */);
			
			
			if (false == mutableFavoritesList.exists()) result = kPreferences_ResultGenericFailure;
			else
			{
				// delete this entry from the array
				CFArrayRemoveValueAtIndex(mutableFavoritesList.returnCFMutableArrayRef(), indexForName);
				result = kPreferences_ResultOK;
			}
			
			// update the preferences list
			overwriteClassDomainCFArray(this->returnClass(), mutableFavoritesList.returnCFMutableArrayRef());
		}
		CFRelease(favoritesListCFArray), favoritesListCFArray = nullptr;
	}
	
	// also destroy the property list for this collection
	CocoaUserDefaults_DeleteDomain(kDomainName);
	
	if (false == CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication))
	{
		result = kPreferences_ResultGenericFailure;
	}
	
	return result;
}// My_ContextFavorite::destroy


/*!
Changes the name of this context.  This is often
used in user interface elements.

\retval kPreferences_ResultOK
currently, always. as there is no way to detect a problem

(3.1)
*/
Preferences_Result
My_ContextFavorite::
rename	(CFStringRef	inNewName)
{
	_contextName.setCFTypeRef(inNewName);
	return kPreferences_ResultOK;
}// My_ContextFavorite::rename


/*!
Returns the base domain name for all domains that
belong to the specified class.  This is the common
root of all collections for the given class; a
derivative of this name can be used to read/write
preferences, but this domain should never be used
alone (defaults are all stored directly in the
application space, regardless of class).

(3.1)
*/
CFStringRef
My_ContextFavorite::
returnClassDomainNamePrefix		(Preferences_Class	inClass)
{
	CFStringRef		result = nullptr;
	
	
	switch (inClass)
	{
	case kPreferences_ClassFormat:
		result = kMy_PreferencesSubDomainFormats;
		break;
	
	case kPreferences_ClassMacroSet:
		result = kMy_PreferencesSubDomainMacros;
		break;
	
	case kPreferences_ClassSession:
		result = kMy_PreferencesSubDomainSessions;
		break;
	
	case kPreferences_ClassTerminal:
		result = kMy_PreferencesSubDomainTerminals;
		break;
	
	case kPreferences_ClassTranslation:
		result = kMy_PreferencesSubDomainTranslations;
		break;
	
	default:
		// ???
		throw std::invalid_argument("unexpected preferences class value");
		break;
	}
	return result;
}// returnClassDomainNamePrefix


/*!
Returns the name of this context.  This is often
used in user interface elements.

(3.1)
*/
CFStringRef
My_ContextFavorite::
returnName ()
const
{
	return _contextName.returnCFStringRef();
}// My_ContextFavorite::returnName


/*!
Saves any in-memory preferences data model changes to
disk, and updates the in-memory model with any new
settings on disk.  Has the side effect of synchronizing
any other modified application-wide preferences (but
will not save contextual preferences).

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if preferences could not be fully saved for any reason

(3.1)
*/
Preferences_Result
My_ContextFavorite::
save ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef const		kDomainName = _domainName.returnCFStringRef();
	CFArrayRef				favoritesListCFArray = nullptr;
	
	
	// generate or replace the name entry for this collection
	{
		CFRetainRelease		nameCFString(CFStringCreateExternalRepresentation
											(kCFAllocatorDefault, this->returnName(), kMy_SavedNameEncoding,
												'?'/* loss byte */), true/* is retained */);
		
		
		if (nameCFString.exists())
		{
			assert(CFDataGetTypeID() == CFGetTypeID(nameCFString.returnCFTypeRef()));
			CFPreferencesSetAppValue(CFSTR("name"), nameCFString.returnCFTypeRef(), kDomainName);
		}
		
		// regardless of whether a Unicode name was stored (which is preferred),
		// also store a raw string as a backup, just in case and also for
		// convenience when reading the raw XML in some cases
		CFPreferencesSetAppValue(CFSTR("name-string"), this->returnName(), kDomainName);
	}
	
	// ensure the domain name is in the list
	result = copyClassDomainCFArray(returnClass(), favoritesListCFArray);
	assert(kPreferences_ResultOK == result);
	if (kPreferences_ResultOK == result)
	{
		CFIndex		indexForName = findDomainIndexInArray(favoritesListCFArray, kDomainName);
		
		
		if (indexForName < 0)
		{
			// does not exist; add a new entry
			CFRetainRelease		mutableFavoritesList(CFArrayCreateMutableCopy
														(kCFAllocatorDefault, 1 + CFArrayGetCount(favoritesListCFArray)/* capacity */,
															favoritesListCFArray), true/* is retained */);
			
			
			if (false == mutableFavoritesList.exists()) throw std::bad_alloc();
			
			// add an entry to the array
			CFArrayAppendValue(mutableFavoritesList.returnCFMutableArrayRef(), kDomainName);
			
			// update the preferences list
			overwriteClassDomainCFArray(this->returnClass(), mutableFavoritesList.returnCFMutableArrayRef());
		}
		CFRelease(favoritesListCFArray), favoritesListCFArray = nullptr;
	}
	
	// save the specific domain
	if (false == CFPreferencesAppSynchronize(_dictionary.returnTargetApplication()))
	{
		result = kPreferences_ResultGenericFailure;
	}
	return result;
}// My_ContextFavorite::save


/*!
Changes the location of this context in the global list of
contexts for its class.  Has the side effect of synchronizing
any other modified application-wide preferences (but will not
save contextual preferences).

The context is inserted just before or after the given
reference, depending on the value of "inInsertBefore".

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if preferences could not be fully saved for any reason

(3.1)
*/
Preferences_Result
My_ContextFavorite::
shift	(My_ContextFavoritePtr		inRelativeTo,
		 Boolean					inInsertBefore)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef const		kSelfDomainName = _domainName.returnCFStringRef();
	CFStringRef const		kRefDomainName = inRelativeTo->_domainName.returnCFStringRef();
	CFArrayRef				favoritesListCFArray = nullptr;
	
	
	// ensure the domain name is in the list
	result = copyClassDomainCFArray(returnClass(), favoritesListCFArray);
	assert(kPreferences_ResultOK == result);
	if (kPreferences_ResultOK == result)
	{
		CFIndex const		kOriginalIndexOfSelf = findDomainIndexInArray(favoritesListCFArray, kSelfDomainName);
		CFIndex const		kOriginalIndexOfRef = findDomainIndexInArray(favoritesListCFArray, kRefDomainName);
		assert(kOriginalIndexOfSelf >= 0);
		assert(kOriginalIndexOfRef >= 0);
		CFRetainRelease		favoritesList(favoritesListCFArray, true/* is retained */);
		CFRetainRelease		mutableFavoritesList(CFArrayCreateMutableCopy
													(kCFAllocatorDefault, 1 + CFArrayGetCount(favoritesListCFArray)/* capacity */,
														favoritesListCFArray), true/* is retained */);
		CFIndex				newIndexOfRef = -1;
		
		
		if (false == mutableFavoritesList.exists()) throw std::bad_alloc();
		
		// first erase the current domain
		CFArrayRemoveValueAtIndex(mutableFavoritesList.returnCFMutableArrayRef(), kOriginalIndexOfSelf);
		
		// after removing, determine the new index of the reference point
		// and insert the item where it is supposed to be
		newIndexOfRef = findDomainIndexInArray(mutableFavoritesList.returnCFArrayRef(), kRefDomainName);
		assert(newIndexOfRef >= 0);
		if (false == inInsertBefore) ++newIndexOfRef;
		CFArrayInsertValueAtIndex(mutableFavoritesList.returnCFMutableArrayRef(), newIndexOfRef, kSelfDomainName);
		overwriteClassDomainCFArray(this->returnClass(), mutableFavoritesList.returnCFMutableArrayRef());
	}
	
	// save changes to the list
	if (false == CFPreferencesAppSynchronize(_dictionary.returnTargetApplication()))
	{
		result = kPreferences_ResultGenericFailure;
	}
	return result;
}// My_ContextFavorite::shift


/*!
Tests this class.  Returns true only if successful.
Information on failures is printed to the console.

(3.1)
*/
Boolean
My_ContextFavorite::
unitTest	(Preferences_Class	inClass,
			 CFStringRef		inName)
{
	Boolean					result = true;
	My_ContextFavorite*		testObjectPtr = new My_ContextFavorite(inClass, inName);
	
	
	result &= Console_Assert("class is set correctly",
								inClass == testObjectPtr->returnClass());
	result &= Console_Assert("name is set correctly",
								kCFCompareEqualTo == CFStringCompare
														(inName, testObjectPtr->returnName(),
															0/* options */));
	
	testObjectPtr->rename(CFSTR("__test_renamed__"));
	result &= Console_Assert("rename worked",
								kCFCompareEqualTo == CFStringCompare
														(CFSTR("__test_renamed__"), testObjectPtr->returnName(),
															0/* options */));
	
	result &= My_ContextInterface::unitTest(testObjectPtr);
	
	delete testObjectPtr, testObjectPtr = nullptr;
	
	return result;
}// My_ContextFavorite::unitTest


/*!
Constructs a new preference definition.

See create().

(4.0)
*/
My_PreferenceDefinition::
My_PreferenceDefinition	(Preferences_Tag		inTag,
						 CFStringRef			inKeyName,
						 FourCharCode			inKeyValueType,
						 size_t					inNonDictionaryValueSize,
						 Preferences_Class		inClass)
:
tag(inTag),
keyName(inKeyName),
keyValueType(inKeyValueType),
nonDictionaryValueSize(inNonDictionaryValueSize),
preferenceClass(inClass)
{
}// My_PreferenceDefinition constructor


/*!
Constructs a new preference definition and automatically
registers it in one or more hash tables for efficient access.

The specified tag should be fully documented in the header
(Preferences.h) and "inNonDictionaryValueSize" should be
consistent with the type given in that documentation.

This may throw an exception (like std::bad_alloc) if the
setting cannot be constructed for some strange reason.

The new definition is optionally returned.  Definitions are
registered globally and never destroyed, and they can be
later retrieved with findByKeyName() and findByTag(), so it
is rare to actually need the newly-created instance.

Once located, a definition’s public member variables can be
used to read the attributes of that setting (such as its
tag).

See also createIndexed().

(4.0)
*/
void
My_PreferenceDefinition::
create	(Preferences_Tag			inTag,
		 CFStringRef				inKeyName,
		 FourCharCode				inKeyValueType,
		 size_t						inNonDictionaryValueSize,
		 Preferences_Class			inClass,
		 My_PreferenceDefinition**	outResultPtrPtrOrNull)
{
	My_PreferenceDefinition*	newPtr = new My_PreferenceDefinition(inTag, inKeyName, inKeyValueType,
																	 inNonDictionaryValueSize, inClass);
	
	// register this in the global by-name hash, allowing no duplicates
	assert(_definitionsByKeyName.end() == _definitionsByKeyName.find(inKeyName));
	_definitionsByKeyName[inKeyName] = newPtr;
	assert(_definitionsByKeyName.end() != _definitionsByKeyName.find(inKeyName));
	
	// register this in the global by-tag hash, allowing no duplicates
	assert(_definitionsByTag.end() == _definitionsByTag.find(inTag));
	_definitionsByTag[inTag] = newPtr;
	assert(_definitionsByTag.end() != _definitionsByTag.find(inTag));
	
	if (nullptr != outResultPtrPtrOrNull) *outResultPtrPtrOrNull = newPtr;
}// My_PreferenceDefinition::create


/*!
A convenience routine that calls create() with the typical
key value type (typeNetEvents_CFBooleanRef) and non-dictionary
value size (sizeof(Boolean)) for flags.  This simplifies the
construction of many preferences, as this type is very common.

(4.0)
*/
inline void
My_PreferenceDefinition::
createFlag	(Preferences_Tag			inTag,
			 CFStringRef				inKeyName,
			 Preferences_Class			inClass,
			 My_PreferenceDefinition**	outResultPtrPtrOrNull)
{
	create(inTag, inKeyName, typeNetEvents_CFBooleanRef, sizeof(Boolean), inClass,
			outResultPtrPtrOrNull);
}// My_PreferenceDefinition::createFlag


/*!
Like create(), but repeats a definition for the specified
number of iterations, keeping all settings the same except
for the underlying key name string.

The tag must be an indexed tag, with space in its value for
adding up to the specified number of indexes.

The key name template must contain a format for an unsigned
32-bit integer (typically, "%02u"); "inNumberOfIndexedKeys"
is the number of key names that should be generated and
registered as valid.

(4.0)
*/
void
My_PreferenceDefinition::
createIndexed	(Preferences_Tag		inTag,
				 Preferences_Index		inNumberOfIndexedKeys,
				 CFStringRef			inKeyNameTemplate,
				 FourCharCode			inKeyValueType,
				 size_t					inNonDictionaryValueSize,
				 Preferences_Class		inClass)
{
	for (Preferences_Index i = 1; i <= inNumberOfIndexedKeys; ++i)
	{
		CFRetainRelease		generatedKeyName(createKeyAtIndex(inKeyNameTemplate, i), true/* is retained */);
		
		
		create(Preferences_ReturnTagVariantForIndex(inTag, i), generatedKeyName.returnCFStringRef(),
				inKeyValueType, inNonDictionaryValueSize, inClass);
	}
}// My_PreferenceDefinition::createIndexed


/*!
A convenience routine that calls create() with the typical
key value type (typeCFArrayRef) and non-dictionary value size
(sizeof(RGBColor)) for RGB colors.  This simplifies the
construction of many preferences, as this type is very common.

The array is expected to contain 3 CFNumberRefs that have
floating-point values between 0.0 and 1.0, for intensity,
where white is represented by all values being 1.0.

(4.0)
*/
inline void
My_PreferenceDefinition::
createRGBColor	(Preferences_Tag			inTag,
				 CFStringRef				inKeyName,
				 Preferences_Class			inClass,
				 My_PreferenceDefinition**	outResultPtrPtrOrNull)
{
	create(inTag, inKeyName, typeCFArrayRef, sizeof(RGBColor), inClass, outResultPtrPtrOrNull);
}// My_PreferenceDefinition::createRGBColor


/*!
Scans a hash table for the specified key name, and
returns the full definition of the matching setting.

(4.0)
*/
My_PreferenceDefinition*
My_PreferenceDefinition::
findByKeyName	(CFStringRef	inKeyName)
{
	My_PreferenceDefinition*					result = nullptr;
	DefinitionPtrByKeyName::const_iterator		toDefPtr = _definitionsByKeyName.find(inKeyName);
	
	
	//Console_WriteValueCFString("find preference by key name", inKeyName); // debug
	if (toDefPtr != _definitionsByKeyName.end()) result = toDefPtr->second;
	return result;
}// My_PreferenceDefinition::findByKeyName


/*!
Scans a hash table for the specified key name, and
returns the full definition of the matching setting
(or nullptr if none is found).

(4.0)
*/
My_PreferenceDefinition*
My_PreferenceDefinition::
findByTag	(Preferences_Tag	inTag)
{
	My_PreferenceDefinition*				result = nullptr;
	DefinitionPtrByTag::const_iterator		toDefPtr = _definitionsByTag.find(inTag);
	
	
	if (_definitionsByTag.end() != toDefPtr) result = toDefPtr->second;
	return result;
}// My_PreferenceDefinition::findByTag


/*!
Ensures that the Preferences module has been
properly initialized.  If Preferences_Init()
needs to be called, it is called.

(3.0)
*/
Preferences_Result
assertInitialized ()
{
	Preferences_Result		result = kPreferences_ResultOK; // kPreferences_ResultNotInitialized?
	
	
	unless (gInitialized) Preferences_Init();
	return result;
}// assertInitialized


/*!
Notifies all listeners for the specified preference
state change, passing the given context to the
listener.

(3.0)
*/
void
changeNotify	(Preferences_Change			inWhatChanged,
				 Preferences_ContextRef		inContextOrNull,
				 Boolean					inIsInitialValue)
{
	Preferences_ChangeContext	context;
	
	
	context.contextRef = inContextOrNull;
	context.firstCall = inIsInitialValue;
	// invoke listener callback routines appropriately, from the preferences listener model
	ListenerModel_NotifyListenersOfEvent(gPreferenceEventListenerModel, inWhatChanged, &context);
}// changeNotify


/*!
Internal version of Preferences_ContextGetData().

(3.1)
*/
Preferences_Result
contextGetData		(My_ContextInterfacePtr		inContextPtr,
					 Preferences_Class			inDataClass,
					 Preferences_Tag			inDataPreferenceTag,
					 size_t						inDataStorageSize,
					 void*						outDataStorage,
					 size_t*					outActualSizePtrOrNull)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	switch (inDataClass)
	{
	case kPreferences_ClassFormat:
		result = getFormatPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
		break;
	
	case kPreferences_ClassGeneral:
		result = getGeneralPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
		break;
	
	case kPreferences_ClassMacroSet:
		result = getMacroPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
		break;
	
	case kPreferences_ClassSession:
		result = getSessionPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
		break;
	
	case kPreferences_ClassTerminal:
		result = getTerminalPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
		break;
	
	case kPreferences_ClassTranslation:
		result = getTranslationPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
		break;
	
	default:
		// unrecognized preference class
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	return result;
}// contextGetData


/*!
Reads an array of 3 CFNumber elements and puts their
values into an RGBColor structure.

Returns "true" only if successful.

(3.1)
*/
Boolean
convertCFArrayToRGBColor	(CFArrayRef		inArray,
							 RGBColor*		outColorPtr)
{
	Boolean		result = false;
	
	
	if ((nullptr != inArray) && (CFArrayGetCount(inArray) == 3))
	{
		CFNumberRef		redValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 0));
		CFNumberRef		greenValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 1));
		CFNumberRef		blueValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 2));
		Float64			floatValue = 0L;
		
		
		// CFNumber is technically more flexible than is allowed;
		// so read a large signed value, even though a small unsigned
		// value is what is assumed
		if (CFNumberGetValue(redValue, kCFNumberFloat64Type, &floatValue))
		{
			floatValue *= RGBCOLOR_INTENSITY_MAX;
			outColorPtr->red = STATIC_CAST(floatValue, UInt16);
			if (CFNumberGetValue(greenValue, kCFNumberFloat64Type, &floatValue))
			{
				floatValue *= RGBCOLOR_INTENSITY_MAX;
				outColorPtr->green = STATIC_CAST(floatValue, UInt16);
				if (CFNumberGetValue(blueValue, kCFNumberFloat64Type, &floatValue))
				{
					floatValue *= RGBCOLOR_INTENSITY_MAX;
					outColorPtr->blue = STATIC_CAST(floatValue, UInt16);
					result = true;
				}
			}
		}
	}
	else
	{
		// unexpected number of elements
	}
	
	return result;
}// convertCFArrayToRGBColor


/*!
Reads an RGBColor structure and puts the values
into an array of 3 CFNumber elements.

Returns "true" only if successful.

(3.1)
*/
Boolean
convertRGBColorToCFArray	(RGBColor const*	inColorPtr,
							 CFArrayRef&		outNewCFArray)
{
	Boolean		result = false;
	
	
	if (nullptr != inColorPtr)
	{
		CFNumberRef		componentValues[] = { nullptr, nullptr, nullptr };
		SInt16			i = 0;
		Float32			floatValue = 0L;
		
		
		floatValue = inColorPtr->red;
		floatValue /= RGBCOLOR_INTENSITY_MAX;
		componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
		if (nullptr != componentValues[i])
		{
			++i;
			floatValue = inColorPtr->green;
			floatValue /= RGBCOLOR_INTENSITY_MAX;
			componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
			if (nullptr != componentValues[i])
			{
				++i;
				floatValue = inColorPtr->blue;
				floatValue /= RGBCOLOR_INTENSITY_MAX;
				componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
				if (nullptr != componentValues[i])
				{
					++i;
					
					// store colors in array
					outNewCFArray = CFArrayCreate(kCFAllocatorDefault,
													REINTERPRET_CAST(componentValues, void const**),
													sizeof(componentValues) / sizeof(CFNumberRef),
													&kCFTypeArrayCallBacks);
					if (nullptr != outNewCFArray)
					{
						// success!
						result = true;
					}
					
					--i;
					CFRelease(componentValues[i]);
				}
				--i;
				CFRelease(componentValues[i]);
			}
			--i;
			CFRelease(componentValues[i]);
		}
	}
	else
	{
		// unexpected input
	}
	
	return result;
}// convertRGBColorToCFArray


/*!
Creates an ordered Core Foundation array that contains
Core Foundation string references, specifying all the
domains containing collections for this class.  You must
release the array when finished with it.

If the array cannot be found, it will be nullptr; however,
this is a serious error in preferences initialization!

See also overwriteClassDomainCFArray().

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is unrecognized

\retval kPreferences_ResultBadVersionDataNotAvailable
if the array was not found

(3.1)
*/
Preferences_Result
copyClassDomainCFArray	(Preferences_Class	inClass,
						 CFArrayRef&		outCFArrayOfCFStrings)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// set default value
	outCFArrayOfCFStrings = nullptr;
	
	// figure out which MacTelnet preferences key holds the relevant list of domains
	switch (inClass)
	{
	case kPreferences_ClassGeneral:
		// not applicable
		result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassFormat:
		readMacTelnetArrayPreference(CFSTR("favorite-formats"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassMacroSet:
		readMacTelnetArrayPreference(CFSTR("favorite-macro-sets"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassSession:
		readMacTelnetArrayPreference(CFSTR("favorite-sessions"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassTerminal:
		readMacTelnetArrayPreference(CFSTR("favorite-terminals"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassTranslation:
		readMacTelnetArrayPreference(CFSTR("favorite-translations"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	default:
		// ???
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// copyClassDomainCFArray


/*!
Reads the preferences on disk and creates lists of
preferences contexts for every collection that is found.
This way, user interface elements (for instance) can
maintain an accurate list of available collections, and
attempts to create new contexts will simply add to that
established list.

IMPORTANT:	Only do this once, preferably at startup.

(3.1)
*/
Preferences_Result
createAllPreferencesContextsFromDisk ()
{
	typedef std::vector< Preferences_Class >	PrefClassList;
	static Boolean			gWasCalled = false;
	PrefClassList			allClassesSupportingCollections;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	assert(false == gWasCalled);
	gWasCalled = true;
	
	// for every class that can have collections, create ALL contexts
	// (based on saved names) for that class, so that the list is current
	allClassesSupportingCollections.push_back(kPreferences_ClassSession);
	allClassesSupportingCollections.push_back(kPreferences_ClassMacroSet);
	allClassesSupportingCollections.push_back(kPreferences_ClassTerminal);
	allClassesSupportingCollections.push_back(kPreferences_ClassTranslation);
	allClassesSupportingCollections.push_back(kPreferences_ClassFormat);
	for (PrefClassList::const_iterator toClass = allClassesSupportingCollections.begin();
			toClass != allClassesSupportingCollections.end(); ++toClass)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		CFArrayRef				namesInClass = nullptr;
		
		
		prefsResult = copyClassDomainCFArray(*toClass, namesInClass);
		if (kPreferences_ResultOK == prefsResult);
		{
			CFIndex const	kNumberOfFavorites = CFArrayGetCount(namesInClass);
			
			
			for (CFIndex i = 0; i < kNumberOfFavorites; ++i)
			{
				// simply creating a context will ensure it is retained
				// internally; so, the return value can be ignored (save
				// for verifying that it was created successfully)
				CFStringRef const		kDomainName = CFUtilities_StringCast(CFArrayGetValueAtIndex
																				(namesInClass, i));
				CFStringRef const		kFavoriteName = findDomainUserSpecifiedName(kDomainName);
				Preferences_ContextRef	newContext = Preferences_NewContextFromFavorites(*toClass, kFavoriteName, kDomainName);
				
				
				assert(nullptr != newContext);
			}
			CFRelease(namesInClass), namesInClass = nullptr;
		}
	}
	
	return result;
}// createAllPreferencesContextsFromDisk


/*!
Creates and returns a Core Foundation dictionary
that can be used to look up default preference keys
(as defined in "DefaultPreferences.plist").

Returns nullptr if unsuccessful for any reason.

(3.0)
*/
CFDictionaryRef
createDefaultPrefDictionary ()
{
	CFDictionaryRef		result = nullptr;
	CFURLRef			fileURL = nullptr;
	
	
	fileURL = CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), CFSTR("DefaultPreferences"),
										CFSTR("plist")/* type string */, nullptr/* subdirectory path */);
	if (nullptr != fileURL)
	{
		CFDataRef   fileData = nullptr;
		SInt32		errorCode = 0;
		
		
		unless (CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL, &fileData, 
															nullptr/* properties */, nullptr/* desired properties */, &errorCode))
		{
			// Not all data was successfully retrieved, but let the caller determine if anything
			// important is missing.
			// NOTE: Technically, the error code returned in "errorCode" is not an OSStatus.
			//       If negative, it is an Apple error, and if positive it is scheme-specific.
		}
		
		{
			CFPropertyListRef   propertyList = nullptr;
			CFStringRef			errorString = nullptr;
			
			
			propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, fileData,
															kCFPropertyListImmutable, &errorString);
			if (nullptr != errorString)
			{
				// could report/return error here...
				CFRelease(errorString), errorString = nullptr;
			}
			else
			{
				// the XML file actually contains a dictionary
				result = CFUtilities_DictionaryCast(propertyList);
			}
		}
		
		// finally, release the file data
		CFRelease(fileData), fileData = nullptr;
	}
	
	return result;
}// createDefaultPrefDictionary


/*!
Utility routine for constructing a preferences key for an
indexed setting.  The specified key string MUST contain
(only) the format string "%02u", to indicate where the index
will go in the result.

You must call CFRelease() on the string when finished with
it (unless the result is nullptr to indicate an error).

(3.1)
*/
CFStringRef
createKeyAtIndex	(CFStringRef	inTemplate,
					 UInt32			inOneBasedIndex)
{
	CFStringRef		result = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */,
														inTemplate, inOneBasedIndex);
	
	
	return result;
}// createKeyAtIndex


/*!
Disposes of memory used for the given alias structure.

(3.0)
*/
void
deleteAliasData		(My_AliasInfoPtr*	inoutAliasDataPtr)
{
	if ((nullptr != inoutAliasDataPtr) && (nullptr != *inoutAliasDataPtr))
	{
		if (nullptr != (*inoutAliasDataPtr)->alias)
		{
			if ((*inoutAliasDataPtr)->isResource)
			{
				ReleaseResource(REINTERPRET_CAST((*inoutAliasDataPtr)->alias, Handle)),
					(*inoutAliasDataPtr)->alias = nullptr;
			}
			else
			{
				Memory_DisposeHandle(REINTERPRET_CAST(&((*inoutAliasDataPtr)->alias), Handle*));
			}
		}
		Memory_DisposePtr(REINTERPRET_CAST(inoutAliasDataPtr, Ptr*));
	}
}// deleteAliasData


/*!
Locates all undeleted alias references and cleans
up the memory used by them.

(3.0)
*/
void
deleteAllAliasNodes ()
{
	My_AliasInfoPtr				dataPtr = nullptr;
	My_AliasInfoList::iterator	aliasInfoIterator;
	
	
	// look for the given alias in the linked list
	for (aliasInfoIterator = gAliasList().begin(); aliasInfoIterator != gAliasList().end(); ++aliasInfoIterator)
	{
		dataPtr = *aliasInfoIterator;
		deleteAliasData(&dataPtr); // dispose of the Preferences module internal data structure associated with the node
		gAliasList().erase(aliasInfoIterator);
	}
}// deleteAllAliasNodes


/*!
Locates the alias structure with the specified
ID in the list in memory, or returns nullptr if
there is no match.

(3.0)
*/
My_AliasInfoPtr
findAlias		(Preferences_AliasID	inAliasID)
{
	My_AliasInfoPtr				result = nullptr;
	My_AliasInfoList::iterator	aliasInfoIterator;
	
	
	// look for the given alias in the linked list
	aliasInfoIterator = find_if(gAliasList().begin(), gAliasList().end(), aliasInfoResourceIDMatches(inAliasID));
	if (aliasInfoIterator != gAliasList().end())
	{
		result = *aliasInfoIterator;
	}
	
	return result;
}// findAlias


/*!
Reads an alias resource corresponding to a given
MacTelnet alias ID.

If successfully created, you may assume that the
"AliasHandle" you provide will become a resource
handle.  As such, you must eventually free its
memory using ReleaseResource().

\retval true
if successful; the alias in "outAliasPtr" will be
valid

\retval false
if the resource cannot be read for any reason; the
output alias handle should then be considered invalid

(3.0)
*/
Boolean
findAliasOnDisk		(Preferences_AliasID	inAliasID,
					 AliasHandle*			outAliasPtr)
{
	Boolean		result = false;
	
	
	if (nullptr != outAliasPtr)
	{
		SInt16		oldResFile = CurResFile();
		
		
		// the MacTelnet alias ID is defined as the resource ID of the alias resource
		AppResources_UseResFile(kAppResources_FileIDPreferences);
		*outAliasPtr = REINTERPRET_CAST(Get1Resource(rAliasType, inAliasID), AliasHandle);
		if ((ResError() == noErr) && (nullptr != *outAliasPtr)) result = true;
		UseResFile(oldResFile);
	}
	return result;
}// findAliasOnDisk


/*!
Given an arrays of strings with preferences domains, tries to
find the specified domain.

If the dictionary is not found, -1 is returned; otherwise, the
zero-based index into the array is returned.  This index can be
used with CFArray APIs.

(3.1)
*/
CFIndex
findDomainIndexInArray	(CFArrayRef		inArray,
						 CFStringRef	inDomainName)
{
	CFIndex			result = -1;
	CFIndex const	kArraySize = CFArrayGetCount(inArray);
	CFIndex			i = 0;
	CFStringRef		domainName = nullptr;
	
	
	for (i = 0; ((result < 0) && (i < kArraySize)); ++i)
	{
		domainName = CFUtilities_StringCast(CFArrayGetValueAtIndex(inArray, i));
		if (kCFCompareEqualTo == CFStringCompare(inDomainName, domainName, 0/* flags */))
		{
			result = i;
		}
	}
	return result;
}// findDomainIndexInArray


/*!
Reads preferences in the given domain and attempts to
find a key that indicates the user’s preferred name
for this set of preferences.  The result could be
nullptr; if not, you must call CFRelease() on it.

(3.1)
*/
CFStringRef
findDomainUserSpecifiedName		(CFStringRef	inDomainName)
{
	CFStringRef		result = nullptr;
	CFDataRef		externalStringRepresentationCFData = nullptr;
	
	
	// in order to support many languages, the "name" field is stored as
	// Unicode data (string external representation); however, if that
	// is not available, purely for convenience a "name-string" alternate
	// is supported, holding a raw string
	externalStringRepresentationCFData = CFUtilities_DataCast(CFPreferencesCopyAppValue(CFSTR("name"), inDomainName));
	if (nullptr != externalStringRepresentationCFData)
	{
		// Unicode string was found
		result = CFStringCreateFromExternalRepresentation
					(kCFAllocatorDefault, externalStringRepresentationCFData,
						kCFStringEncodingUnicode);
		CFRelease(externalStringRepresentationCFData), externalStringRepresentationCFData = nullptr;
	}
	else
	{
		// raw string was found
		result = CFUtilities_StringCast(CFPreferencesCopyAppValue(CFSTR("name-string"), inDomainName));
	}
	return result;
}// findDomainUserSpecifiedName


/*!
Retrieves the context that stores default settings for
the specified class.  Returns true unless this fails.

(3.1)
*/
Boolean
getDefaultContext	(Preferences_Class			inClass,
					 My_ContextInterfacePtr&	outContextPtr)
{
	Boolean		result = true;
	
	
	outContextPtr = nullptr;
	switch (inClass)
	{
	case kPreferences_ClassFormat:
		outContextPtr = &(gFormatDefaultContext());
		break;
	
	case kPreferences_ClassGeneral:
		outContextPtr = &(gGeneralDefaultContext());
		break;
	
	case kPreferences_ClassMacroSet:
		outContextPtr = &(gMacroSetDefaultContext());
		break;
	
	case kPreferences_ClassSession:
		outContextPtr = &(gSessionDefaultContext());
		break;
	
	case kPreferences_ClassTerminal:
		outContextPtr = &(gTerminalDefaultContext());
		break;
	
	case kPreferences_ClassTranslation:
		outContextPtr = &(gTranslationDefaultContext());
		break;
	
	default:
		// ???
		result = false;
		break;
	}
	
	return result;
}// getDefaultContext


/*!
Retrieves the context that stores “factory defaults”
settings.  Returns true unless this fails.

(3.1)
*/
Boolean
getFactoryDefaultsContext	(My_ContextInterfacePtr&	outContextPtr)
{
	Boolean		result = true;
	
	
	outContextPtr = &(gFactoryDefaultsContext());
	result = (nullptr != outContextPtr);
	return result;
}// getFactoryDefaultsContext


/*!
Returns preference data for a font or color setting.

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.0)
*/
Preferences_Result	
getFormatPreference		(My_ContextInterfaceConstPtr	inContextPtr,
						 Preferences_Tag				inDataPreferenceTag,
						 size_t							inDataSize,
						 void*							outDataPtr,
						 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef			keyName = nullptr;
		FourCharCode		keyValueType = '----';
		Preferences_Class	dataClass = kPreferences_ClassFormat;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == kPreferences_ClassFormat);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagFontCharacterWidthMultiplier:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						Float32* const	data = REINTERPRET_CAST(outDataPtr, Float32*);
						
						
						*data = inContextPtr->returnFloat(keyName);
						if (0 == *data)
						{
							// failed; make default
							*data = 1.0; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagFontName:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							StringPtr const		data = REINTERPRET_CAST(outDataPtr, StringPtr);
							
							
							if (false == CFStringGetPascalString(valueCFString, data, inDataSize, kCFStringEncodingMacRoman))
							{
								// failed; make empty string
								PLstrcpy(data, "\p");
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagFontSize:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16* const	data = REINTERPRET_CAST(outDataPtr, SInt16*);
						
						
						*data = inContextPtr->returnInteger(keyName);
						if (0 == *data)
						{
							// failed; make default
							*data = 12; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagTerminalColorMatteBackground:
				case kPreferences_TagTerminalColorBlinkingForeground:
				case kPreferences_TagTerminalColorBlinkingBackground:
				case kPreferences_TagTerminalColorBoldForeground:
				case kPreferences_TagTerminalColorBoldBackground:
				case kPreferences_TagTerminalColorNormalForeground:
				case kPreferences_TagTerminalColorNormalBackground:
				case kPreferences_TagTerminalColorANSIBlack:
				case kPreferences_TagTerminalColorANSIRed:
				case kPreferences_TagTerminalColorANSIGreen:
				case kPreferences_TagTerminalColorANSIYellow:
				case kPreferences_TagTerminalColorANSIBlue:
				case kPreferences_TagTerminalColorANSIMagenta:
				case kPreferences_TagTerminalColorANSICyan:
				case kPreferences_TagTerminalColorANSIWhite:
				case kPreferences_TagTerminalColorANSIBlackBold:
				case kPreferences_TagTerminalColorANSIRedBold:
				case kPreferences_TagTerminalColorANSIGreenBold:
				case kPreferences_TagTerminalColorANSIYellowBold:
				case kPreferences_TagTerminalColorANSIBlueBold:
				case kPreferences_TagTerminalColorANSIMagentaBold:
				case kPreferences_TagTerminalColorANSICyanBold:
				case kPreferences_TagTerminalColorANSIWhiteBold:
					{
						assert(typeCFArrayRef == keyValueType);
						CFArrayRef		valueCFArray = inContextPtr->returnArrayCopy(keyName);
						
						
						if (nullptr == valueCFArray)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							RGBColor* const		data = REINTERPRET_CAST(outDataPtr, RGBColor*);
							
							
							if (false == convertCFArrayToRGBColor(valueCFArray, data))
							{
								// failed; make black
								data->red = data->green = data->blue = 0;
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFArray), valueCFArray = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalMarginLeft:
				case kPreferences_TagTerminalMarginRight:
				case kPreferences_TagTerminalMarginTop:
				case kPreferences_TagTerminalMarginBottom:
				case kPreferences_TagTerminalPaddingLeft:
				case kPreferences_TagTerminalPaddingRight:
				case kPreferences_TagTerminalPaddingTop:
				case kPreferences_TagTerminalPaddingBottom:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						Float32			valueFloat32 = inContextPtr->returnFloat(keyName);
						Float32* const	data = REINTERPRET_CAST(outDataPtr, Float32*);
						
						
						// note that precisely zero is returned by Core Foundation to show errors; if
						// the user actually wants zero, a tiny value like 0.000001 should be used
						*data = valueFloat32;
						if (0 == *data)
						{
							// failed; make default
							*data = 0; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				default:
					// unrecognized tag
					result = kPreferences_ResultUnknownTagOrClass;
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getFormatPreference


/*!
Returns preference data for a global setting.

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.0)
*/
Preferences_Result	
getGeneralPreference	(My_ContextInterfaceConstPtr	inContextPtr,
						 Preferences_Tag				inDataPreferenceTag,
						 size_t							inDataSize,
						 void*							outDataPtr,
						 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef			keyName = nullptr;
		FourCharCode		keyValueType = '----';
		Preferences_Class	dataClass = kPreferences_ClassGeneral;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == kPreferences_ClassGeneral);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagBellSound:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFStringRef* const	data = REINTERPRET_CAST(outDataPtr, CFStringRef*);
							
							
							*data = valueCFString;
							// do not release because the string is returned
						}
					}
					break;
				
				case kPreferences_TagCaptureFileCreator:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							OSType*		outOSTypePtr = REINTERPRET_CAST(outDataPtr, OSType*);
							CFIndex		usedBufferLength = 0;
							
							
							if (4 != CFStringGetBytes(valueCFString, CFRangeMake(0, 4),
														CFStringGetFastestEncoding(valueCFString),
														'\0'/* loss byte */, false/* is external representation */,
														REINTERPRET_CAST(outOSTypePtr, UInt8*),
														sizeof(*outOSTypePtr), &usedBufferLength))
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagArrangeWindowsUsingTabs:
				case kPreferences_TagCopySelectedText:
				case kPreferences_TagCursorBlinks:
				case kPreferences_TagCursorMovesPriorToDrops:
				case kPreferences_TagDontAutoClose:
				case kPreferences_TagDontAutoNewOnApplicationReopen:
				case kPreferences_TagDontDimBackgroundScreens:
				case kPreferences_TagFocusFollowsMouse:
				case kPreferences_TagHeadersCollapsed:
				case kPreferences_TagMacrosMenuVisible:
				case kPreferences_TagMenuItemKeys:
				case kPreferences_TagPureInverse:
				case kPreferences_TagRandomTerminalFormats:
				case kPreferences_TagSimplifiedUserInterface:
					{
						assert(typeNetEvents_CFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagCopyTableThreshold:
					assert(typeNetEvents_CFNumberRef == keyValueType);
					{
						UInt16*		outUInt16Ptr = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						*outUInt16Ptr = STATIC_CAST(inContextPtr->returnInteger(keyName), UInt16);
						if (*outUInt16Ptr == 0)
						{
							// failed; make default
							*outUInt16Ptr = 8; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagInfoWindowColumnOrdering:
					{
						assert(typeCFArrayRef == keyValueType);
						CFArrayRef* const	data = REINTERPRET_CAST(outDataPtr, CFArrayRef*);
						
						
						*data = inContextPtr->returnArrayCopy(keyName);
						if (nullptr == *data)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagKioskAllowsForceQuit:
				case kPreferences_TagKioskShowsMenuBar:
				case kPreferences_TagKioskShowsOffSwitch:
				case kPreferences_TagKioskShowsScrollBar:
				case kPreferences_TagKioskUsesSuperfluousEffects:
					{
						assert(typeNetEvents_CFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagMapBackquote:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("\\e"), kCFCompareCaseInsensitive))
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true;
							}
							else
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagMarginBell:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("bell"), kCFCompareCaseInsensitive))
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true;
							}
							else
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagNewCommandShortcutEffect:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							UInt32*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt32*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("shell"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kCommandNewSessionShell;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("log-in shell"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kCommandNewSessionLoginShell;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("dialog"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kCommandNewSessionDialog;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("default"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kCommandNewSessionDefaultFavorite;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagNotification:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							SInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, SInt16*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("ignore"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlert_NotifyDoNothing;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("badge"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlert_NotifyDisplayDiamondMark;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("animate"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlert_NotifyDisplayIconAndDiamondMark;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("alert"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlert_NotifyAlsoDisplayAlert;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagNotifyOfBeeps:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("notify"), kCFCompareCaseInsensitive))
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true;
							}
							else
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalCursorType:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							TerminalView_CursorType*	storedValuePtr = REINTERPRET_CAST(outDataPtr, TerminalView_CursorType*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("block"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_CursorTypeBlock;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("underline"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_CursorTypeUnderscore;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("thick underline"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_CursorTypeThickUnderscore;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("vertical bar"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_CursorTypeVerticalLine;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("thick vertical bar"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_CursorTypeThickVerticalLine;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalResizeAffectsFontSize:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("font"), kCFCompareCaseInsensitive))
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true;
							}
							else
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagVisualBell:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("visual"), kCFCompareCaseInsensitive))
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("audio+visual"), kCFCompareCaseInsensitive))
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true;
							}
							else
							{
								*(REINTERPRET_CAST(outDataPtr, Boolean*)) = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagWasClipboardShowing:
				case kPreferences_TagWasCommandLineShowing:
				case kPreferences_TagWasControlKeypadShowing:
				case kPreferences_TagWasFunctionKeypadShowing:
				case kPreferences_TagWasSessionInfoShowing:
				case kPreferences_TagWasVT220KeypadShowing:
					{
						assert(typeNetEvents_CFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagWindowStackingOrigin:
					{
						assert(typeCFArrayRef == keyValueType);
						CFArrayRef		valueCFArray = inContextPtr->returnArrayCopy(keyName);
						
						
						if (nullptr == valueCFArray)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFNumberRef		singleCoord = nullptr;
							Point*			outPointPtr = REINTERPRET_CAST(outDataPtr, Point*);
							
							
							singleCoord = CFUtilities_NumberCast(CFArrayGetValueAtIndex(valueCFArray, 0));
							unless (CFNumberGetValue(singleCoord, kCFNumberSInt16Type, &outPointPtr->h))
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							singleCoord = CFUtilities_NumberCast(CFArrayGetValueAtIndex(valueCFArray, 1));
							unless (CFNumberGetValue(singleCoord, kCFNumberSInt16Type, &outPointPtr->v))
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							
							// set reasonable values if there is an error
							if (outPointPtr->h <= 0) outPointPtr->h = 40;
							if (outPointPtr->v <= 0) outPointPtr->v = 40;
							
							CFRelease(valueCFArray), valueCFArray = nullptr;
						}
					}
					break;
				
				default:
					// unrecognized tag
					result = kPreferences_ResultUnknownTagOrClass;
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getGeneralPreference


/*!
Retrieves the list that stores any in-memory settings for the
specified class (only works for classes that can be collections).
Returns true unless this fails.

IMPORTANT:	This list will only contain contexts created with
			Preferences_NewContextFromFavorites() that have
			not been destroyed by Preferences_ReleaseContext().
			There is no guarantee that this list will have
			every on-disk collection for the given class.  But,
			see Preferences_CreateContextNameArray().

(3.1)
*/
Boolean
getListOfContexts	(Preferences_Class				inClass,
					 My_FavoriteContextList*&		outListPtr)
{
	Boolean		result = true;
	
	
	outListPtr = nullptr;
	switch (inClass)
	{
	case kPreferences_ClassFormat:
		outListPtr = &gFormatNamedContexts();
		break;
	
	case kPreferences_ClassMacroSet:
		outListPtr = &gMacroSetNamedContexts();
		break;
	
	case kPreferences_ClassSession:
		outListPtr = &gSessionNamedContexts();
		break;
	
	case kPreferences_ClassTerminal:
		outListPtr = &gTerminalNamedContexts();
		break;
	
	case kPreferences_ClassTranslation:
		outListPtr = &gTranslationNamedContexts();
		break;
	
	case kPreferences_ClassGeneral:
	default:
		// ???
		result = false;
		break;
	}
	
	return result;
}// getListOfContexts


/*!
Returns preference data for a setting for a particular macro.
In this method, 0 is an invalid index.

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.0)
*/
Preferences_Result	
getMacroPreference	(My_ContextInterfaceConstPtr	inContextPtr,
					 Preferences_Tag				inDataPreferenceTag,
					 size_t							inDataSize,
					 void*							outDataPtr,
					 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef			keyName = nullptr;
		FourCharCode		keyValueType = '----';
		Preferences_Class	dataClass = kPreferences_ClassMacroSet;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == kPreferences_ClassMacroSet);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				Preferences_Tag const		kTagWithoutIndex = Preferences_ReturnTagFromVariant(inDataPreferenceTag);
				
				
				switch (kTagWithoutIndex)
				{
				case kPreferences_TagIndexedMacroAction:
					assert(typeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							MacroManager_Action*	storedValuePtr = REINTERPRET_CAST
																		(outDataPtr, MacroManager_Action*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("verbatim"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionSendTextVerbatim;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("text"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionSendTextProcessingEscapes;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("handle URL"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionHandleURL;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("new window with command"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionNewWindowWithCommand;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagIndexedMacroContents:
				case kPreferences_TagIndexedMacroName:
					// all of these keys have Core Foundation string values
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFStringRef* const	data = REINTERPRET_CAST(outDataPtr, CFStringRef*);
							
							
							*data = valueCFString;
							// do not release because the string is returned
						}
					}
					break;
				
				case kPreferences_TagIndexedMacroKey:
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							MacroManager_KeyID*		storedValuePtr = REINTERPRET_CAST
																		(outDataPtr, MacroManager_KeyID*);
							UInt16					keyCode = 0;
							Boolean					isVirtualKey = false;
							
							
							if (virtualKeyParseName(valueCFString, keyCode, isVirtualKey))
							{
								*storedValuePtr = MacroManager_MakeKeyID(isVirtualKey, keyCode);
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagIndexedMacroKeyModifiers:
					{
						assert(typeCFArrayRef == keyValueType);
						CFArrayRef		valueCFArray = inContextPtr->returnArrayCopy(keyName);
						
						
						if (nullptr == valueCFArray)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFIndex const	kArrayLength = CFArrayGetCount(valueCFArray);
							CFStringRef		modifierNameCFString = nullptr;
							UInt32* const	data = REINTERPRET_CAST(outDataPtr, UInt32*);
							
							
							for (CFIndex i = 0; i < kArrayLength; ++i)
							{
								modifierNameCFString = CFUtilities_StringCast
														(CFArrayGetValueAtIndex(valueCFArray, i));
								if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("command"),
																			kCFCompareCaseInsensitive))
								{
									*data |= cmdKey;
								}
								else if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("control"),
																				kCFCompareCaseInsensitive))
								{
									*data |= controlKey;
								}
								else if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("option"),
																				kCFCompareCaseInsensitive))
								{
									*data |= optionKey;
								}
								else if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("shift"),
																				kCFCompareCaseInsensitive))
								{
									*data |= shiftKey;
								}
							}
						}
					}
					break;
				
				default:
					// unrecognized tag
					result = kPreferences_ResultUnknownTagOrClass;
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getMacroPreference


/*!
Retrieves the context with the given name that stores
settings for the specified class.  Returns true unless
this fails.

IMPORTANT:	There will be only one context per class and
			name combination.  It could be renamed or
			deleted at a later point.

(3.1)
*/
Boolean
getNamedContext		(Preferences_Class			inClass,
					 CFStringRef				inName,
					 My_ContextFavoritePtr&		outContextPtr)
{
	Boolean						result = false;
	My_FavoriteContextList*		listPtr = nullptr;
	
	
	outContextPtr = nullptr;
	if (getListOfContexts(inClass, listPtr))
	{
		My_FavoriteContextList::const_iterator		toContextPtr = std::find_if
																	(listPtr->begin(), listPtr->end(),
																		contextNameEqualTo(inName));
		
		
		if (listPtr->end() != toContextPtr)
		{
			outContextPtr = *toContextPtr;
			result = true;
		}
	}
	
	return result;
}// getNamedContext


/*!
Returns information about the dictionary key used to store the
given preferences tag’s data, the type of data expected in a
dictionary, the size of the data type used to read or write the
data via APIs such as Preferences_ContextGetData(), and an
indication of what class the tag belongs to.

If the tag is actually an indexed tag, it should already have
been “varied” (by adding the index), so that the proper key name
is returned.

If no particular class dictionary is required, then "outClass"
will be "kPreferences_ClassGeneral".  In this case ONLY, you
may use Core Foundation Preferences to store or retrieve the
key value directly in the application’s globals.  However, keys
intended for storage in user Favorites will have a different
class, and must be set in a specific domain.

See also Preferences_Init() and My_PreferenceDefinition.

\retval kPreferences_ResultOK
if the information was found successfully

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.1)
*/
Preferences_Result
getPreferenceDataInfo	(Preferences_Tag		inTag,
						 CFStringRef&			outKeyName,
						 FourCharCode&			outKeyValueType,
						 size_t&				outNonDictionaryValueSize,
						 Preferences_Class&		outClass)
{
	Preferences_Result			result = kPreferences_ResultOK;
	My_PreferenceDefinition*	definitionPtr = nullptr;
	
	
	definitionPtr = My_PreferenceDefinition::findByTag(inTag);
	if (nullptr != definitionPtr)
	{
		// with the global hash tables, this mapping is easy; simply
		// find the corresponding definition and return all information
		outKeyName = definitionPtr->keyName.returnCFStringRef();
		outKeyValueType = definitionPtr->keyValueType;
		outNonDictionaryValueSize = definitionPtr->nonDictionaryValueSize;
		outClass = definitionPtr->preferenceClass;
	}
	else
	{
		result = kPreferences_ResultUnknownTagOrClass;
	}
	return result;
}// getPreferenceDataInfo


/*!
Returns preference data for a session setting.

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.0)
*/
Preferences_Result	
getSessionPreference	(My_ContextInterfaceConstPtr	inContextPtr,
						 Preferences_Tag				inDataPreferenceTag,
						 size_t							inDataSize,
						 void*							outDataPtr,
						 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef			keyName = nullptr;
		FourCharCode		keyValueType = '----';
		Preferences_Class	dataClass = kPreferences_ClassSession;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == kPreferences_ClassSession);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagAssociatedFormatFavorite:
				case kPreferences_TagAssociatedTerminalFavorite:
				case kPreferences_TagAssociatedTranslationFavorite:
				case kPreferences_TagServerHost:
				case kPreferences_TagServerUserID:
					// all of these keys have Core Foundation string values
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFStringRef* const	data = REINTERPRET_CAST(outDataPtr, CFStringRef*);
							
							
							*data = valueCFString;
							// do not release because the string is returned
						}
					}
					break;
				
				case kPreferences_TagAutoCaptureToFile:
				case kPreferences_TagLineModeEnabled:
				case kPreferences_TagLocalEchoEnabled:
				case kPreferences_TagLocalEchoHalfDuplex:
				case kPreferences_TagTektronixPAGEClearsScreen:
					// all of these keys have Core Foundation Boolean values
					{
						assert(typeNetEvents_CFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagCaptureFileAlias:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16						valueInteger = inContextPtr->returnInteger(keyName);
						Preferences_AliasID* const	data = REINTERPRET_CAST(outDataPtr, Preferences_AliasID*);
						
						
						if (0 == valueInteger)
						{
							// failed; make default
							*data = kPreferences_InvalidAliasID;
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							*data = valueInteger;
						}
					}
					break;
				
				case kPreferences_TagCommandLine:
					{
						assert(typeCFArrayRef == keyValueType);
						CFArrayRef* const	data = REINTERPRET_CAST(outDataPtr, CFArrayRef*);
						
						
						*data = inContextPtr->returnArrayCopy(keyName);
						if (nullptr == *data)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagIdleAfterInactivityInSeconds:
				case kPreferences_TagKeepAlivePeriodInMinutes:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16			valueInteger = inContextPtr->returnInteger(keyName);
						UInt16* const	data = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						if (0 == valueInteger)
						{
							// failed; make default
							*data = 0;
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							*data = valueInteger;
						}
					}
					break;
				
				case kPreferences_TagKeyInterruptProcess:
				case kPreferences_TagKeyResumeOutput:
				case kPreferences_TagKeySuspendOutput:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		keystrokeCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if ((nullptr == keystrokeCFString) ||
							(CFStringGetLength(keystrokeCFString) < 2) ||
							(CFStringGetCharacterAtIndex(keystrokeCFString, 0) != '^'))
						{
							// nonexistent, or not a control key sequence
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							char* const		data = REINTERPRET_CAST(outDataPtr, char*);
							
							
							*data = STATIC_CAST(CFStringGetCharacterAtIndex(keystrokeCFString, 1), char);
							CFRelease(keystrokeCFString), keystrokeCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagMapCarriageReturnToCRNull:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Boolean*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Boolean*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("\\015\\000"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = true;
							}
							else
							{
								*storedValuePtr = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagMapDeleteToBackspace:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Boolean*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Boolean*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("backspace"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = true;
							}
							else
							{
								*storedValuePtr = false;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagPasteBlockSize:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16* const	data = REINTERPRET_CAST(outDataPtr, SInt16*);
						
						
						*data = inContextPtr->returnInteger(keyName);
						if (0 == *data)
						{
							// failed; make default
							*data = 128; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagPasteMethod:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Clipboard_PasteMethod*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Clipboard_PasteMethod*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("normal"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kClipboard_PasteMethodStandard;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("throttled"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kClipboard_PasteMethodBlock;
							}
							else
							{
								// failed; make default
								*storedValuePtr = kClipboard_PasteMethodStandard; // arbitrary
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagServerPort:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16* const	data = REINTERPRET_CAST(outDataPtr, SInt16*);
						
						
						*data = inContextPtr->returnInteger(keyName);
						if (0 == *data)
						{
							// failed; make default
							*data = 23; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagServerProtocol:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Session_Protocol*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Session_Protocol*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("ftp"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_ProtocolFTP;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("sftp"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_ProtocolSFTP;
							}
							else if ((kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("ssh"), kCFCompareCaseInsensitive)) ||
										(kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("ssh-1"), kCFCompareCaseInsensitive)))
							{
								*storedValuePtr = kSession_ProtocolSSH1;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("ssh-2"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_ProtocolSSH2;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("telnet"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_ProtocolTelnet;
							}
							else
							{
								// failed; make default
								*storedValuePtr = kSession_ProtocolSSH1; // arbitrary
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTektronixMode:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							VectorInterpreter_Mode*		storedValuePtr = REINTERPRET_CAST(outDataPtr, VectorInterpreter_Mode*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("off"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kVectorInterpreter_ModeDisabled;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("4014"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kVectorInterpreter_ModeTEK4014;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("4105"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kVectorInterpreter_ModeTEK4105;
							}
							else
							{
								// failed; make default
								*storedValuePtr = kVectorInterpreter_ModeDisabled; // arbitrary
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				default:
					// unrecognized tag
					result = kPreferences_ResultUnknownTagOrClass;
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getSessionPreference


/*!
Returns preference data for a terminal setting.  Note
that getFormatPreference() now handles fonts and
colors (formerly terminal settings).

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.0)
*/
Preferences_Result	
getTerminalPreference	(My_ContextInterfaceConstPtr	inContextPtr,
						 Preferences_Tag				inDataPreferenceTag,
						 size_t							inDataSize,
						 void*							outDataPtr,
						 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef			keyName = nullptr;
		FourCharCode		keyValueType = '----';
		Preferences_Class	dataClass = kPreferences_ClassTerminal;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == kPreferences_ClassTerminal);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagDataReceiveDoNotStripHighBit:
				case kPreferences_TagTerminalClearSavesLines:
				case kPreferences_TagTerminalLineWrap:
				case kPreferences_TagVT100FixLineWrappingBug:
				case kPreferences_TagXTerm256ColorsEnabled:
				case kPreferences_TagXTermColorEnabled:
				case kPreferences_TagXTermGraphicsEnabled:
				case kPreferences_TagXTermWindowAlterationEnabled:
					// all of these keys have Core Foundation Boolean values
					{
						assert(typeNetEvents_CFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagEMACSMetaKey:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR(""), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_EMACSMetaKeyOff;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("control+command"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_EMACSMetaKeyControlCommand;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("option"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_EMACSMetaKeyOption;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagMapArrowsForEMACS:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							*(REINTERPRET_CAST(outDataPtr, Boolean*)) = (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("down-arrow"), kCFCompareCaseInsensitive)) ? true : false;
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagMapKeypadTopRowForVT220:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							*(REINTERPRET_CAST(outDataPtr, Boolean*)) = (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("keypad-clear"), kCFCompareCaseInsensitive)) ? false : true;
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagPageKeysControlLocalTerminal:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							*(REINTERPRET_CAST(outDataPtr, Boolean*)) = (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("end"), kCFCompareCaseInsensitive)) ? true : false;
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalAnswerBackMessage:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFStringRef*	storedValuePtr = REINTERPRET_CAST(outDataPtr, CFStringRef*);
							
							
							*storedValuePtr = valueCFString;
							// do not release because the string is returned
						}
					}
					break;
				
				case kPreferences_TagTerminalEmulatorType:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Terminal_Emulator*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Terminal_Emulator*);
							
							
							*storedValuePtr = Terminal_EmulatorReturnForName(valueCFString);
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalScreenColumns:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16			valueInteger = inContextPtr->returnInteger(keyName);
						UInt16* const	data = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						*data = STATIC_CAST(valueInteger, UInt16);
						if (0 == *data)
						{
							// failed; make default
							*data = 80; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagTerminalScreenRows:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16			valueInteger = inContextPtr->returnInteger(keyName);
						UInt16* const	data = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						*data = STATIC_CAST(valueInteger, UInt16);
						if (0 == *data)
						{
							// failed; make default
							*data = 24; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagTerminalScreenScrollbackRows:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16			valueInteger = inContextPtr->returnInteger(keyName);
						UInt16* const	data = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						*data = STATIC_CAST(valueInteger, UInt16);
						if (0 == *data)
						{
							// failed; make default
							*data = 200; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagTerminalScreenScrollbackType:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Terminal_ScrollbackType*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Terminal_ScrollbackType*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("off"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_ScrollbackTypeDisabled;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("unlimited"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_ScrollbackTypeUnlimited;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("distributed"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_ScrollbackTypeDistributed;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("fixed"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_ScrollbackTypeFixed;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalScrollDelay:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt16				valueInteger = inContextPtr->returnInteger(keyName);
						EventTime* const	data = REINTERPRET_CAST(outDataPtr, EventTime*);
						
						
						*data = STATIC_CAST(valueInteger, EventTime) * kEventDurationMillisecond;
					}
					break;
				
				default:
					// unrecognized tag
					result = kPreferences_ResultUnknownTagOrClass;
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getTerminalPreference


/*!
Returns preference data for a translation setting.

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(3.1)
*/
Preferences_Result	
getTranslationPreference	(My_ContextInterfaceConstPtr	inContextPtr,
							 Preferences_Tag				inDataPreferenceTag,
							 size_t							inDataSize,
							 void*							outDataPtr,
							 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef			keyName = nullptr;
		FourCharCode		keyValueType = '----';
		Preferences_Class	dataClass = kPreferences_ClassTranslation;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == kPreferences_ClassTranslation);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagBackupFontName:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							StringPtr const		data = REINTERPRET_CAST(outDataPtr, StringPtr);
							
							
							if (false == CFStringGetPascalString(valueCFString, data, inDataSize, kCFStringEncodingMacRoman))
							{
								// failed; make empty string
								PLstrcpy(data, "\p");
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTextEncodingIANAName:
					{
						assert(typeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFStringRef* const	data = REINTERPRET_CAST(outDataPtr, CFStringRef*);
							
							
							*data = valueCFString;
							// do not release because the string is returned
						}
					}
					break;
				
				case kPreferences_TagTextEncodingID:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt32						valueInteger = inContextPtr->returnLong(keyName);
						CFStringEncoding* const		data = REINTERPRET_CAST(outDataPtr, CFStringEncoding*);
						
						
						*data = STATIC_CAST(valueInteger, CFStringEncoding);
					}
					break;
				
				default:
					// unrecognized tag
					result = kPreferences_ResultUnknownTagOrClass;
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getTranslationPreference


/*!
Adds default values for known preference keys.  Existing
values are kept and any other defaults are added.

\retval noErr
if successful

\retval some (probably resource-related) OS error
if some component could not be set up properly

(2.6)
*/
OSStatus
mergeInDefaultPreferences ()
{
	CFDictionaryRef		defaultPrefDictionary = createDefaultPrefDictionary();
	OSStatus			result = noErr;
	
	
	// copy bundle's DefaultPreferences.plist
	if (nullptr == defaultPrefDictionary) result = resNotFound;
	else
	{
		// read the default preferences dictionary; for settings
		// saved in XML, this will also make CFPreferences calls
		// to register (in memory only) the appropriate updates;
		// other values are written to the specified handles,
		// where the data must be extracted for saving elsewhere
		result = readPreferencesDictionary(defaultPrefDictionary, true/* merge */);
		CFRelease(defaultPrefDictionary), defaultPrefDictionary = nullptr;
		
		if (noErr == result)
		{
			// save the preferences...
			(Boolean)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
		}
	}
	
	return result;
}// mergeInDefaultPreferences


/*!
Replaces the list of collection domains for a given class of
settings, with the given list.  Typically, you start by calling
copyClassDomainCFArray() to get the original list, then you
create a mutable copy, make your changes and pass it into this
routine.

\retval kPreferences_ResultOK
if the array was overwritten successfully

\retval kPreferences_ResultUnknownTagOrClass
if the given preference class is unknown or does not have an array value

(3.1)
*/
Preferences_Result
overwriteClassDomainCFArray		(Preferences_Class	inClass,
								 CFArrayRef			inCFArrayOfCFStrings)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// figure out which MacTelnet preferences key should hold this list of domains
	switch (inClass)
	{
	case kPreferences_ClassFormat:
		setMacTelnetPreference(CFSTR("favorite-formats"), inCFArrayOfCFStrings);
		break;
	
	case kPreferences_ClassMacroSet:
		setMacTelnetPreference(CFSTR("favorite-macro-sets"), inCFArrayOfCFStrings);
		break;
	
	case kPreferences_ClassSession:
		setMacTelnetPreference(CFSTR("favorite-sessions"), inCFArrayOfCFStrings);
		break;
	
	case kPreferences_ClassTerminal:
		setMacTelnetPreference(CFSTR("favorite-terminals"), inCFArrayOfCFStrings);
		break;
	
	case kPreferences_ClassTranslation:
		setMacTelnetPreference(CFSTR("favorite-translations"), inCFArrayOfCFStrings);
		break;
	
	case kPreferences_ClassGeneral:
	default:
		// ???
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// overwriteClassDomainCFArray


/*!
Reads MacTelnet’s XML preferences as requested.
Also prints to standard output information about
the data being read (if debugging is enabled).

The resultant Core Foundation type must be released
when you are finished using it.

IMPORTANT:	Do not use for keys that do not really
			have array values.

(3.1)
*/
void
readMacTelnetArrayPreference	(CFStringRef	inKey,
								 CFArrayRef&	outValue)
{
	assert(nullptr != inKey);
	outValue = CFUtilities_ArrayCast(CFPreferencesCopyAppValue(inKey, kCFPreferencesCurrentApplication));
	//Console_WriteValueCFTypeOf(CFStringGetCStringPtr(inKey, kCFStringEncodingMacRoman)/* NOTE: risky... */, outValue);
#if 0
	if (nullptr != outValue)
	{
		// for debugging
		CFStringRef		descriptionCFString = CFCopyDescription(outValue);
		char const*		keyString = nullptr;
		char const*		descriptionString = nullptr;
		Boolean			disposeKeyString = false;
		Boolean			disposeDescriptionString = false;
		
		
		keyString = CFStringGetCStringPtr(inKey, CFStringGetFastestEncoding(inKey));
		if (nullptr == keyString)
		{
			// allocate double the size because the string length assumes 16-bit characters
			size_t const	kBufferSize = INTEGER_DOUBLED(CFStringGetLength(inKey));
			char*			bufferPtr = new char[kBufferSize];
			
			
			if (nullptr != bufferPtr)
			{
				keyString = bufferPtr;
				disposeKeyString = true;
				if (CFStringGetCString(inKey, bufferPtr, kBufferSize,
										CFStringGetFastestEncoding(inKey)))
				{
					// no idea what the problem is, but just bail
					std::strcpy(bufferPtr, "(error while trying to copy key data)");
				}
			}
		}
		
		if (nullptr == descriptionCFString)
		{
			descriptionString = "(no value description available)";
		}
		else
		{
			descriptionString = CFStringGetCStringPtr
								(descriptionCFString, CFStringGetFastestEncoding(descriptionCFString));
			if (nullptr == descriptionString)
			{
				// allocate double the size because the string length assumes 16-bit characters
				size_t const	kBufferSize = INTEGER_DOUBLED(CFStringGetLength(descriptionCFString));
				char*			bufferPtr = new char[kBufferSize];
				
				
				if (nullptr != bufferPtr)
				{
					descriptionString = bufferPtr;
					disposeDescriptionString = true;
					if (CFStringGetCString(descriptionCFString, bufferPtr, kBufferSize,
											CFStringGetFastestEncoding(descriptionCFString)))
					{
						// no idea what the problem is, but just bail
						std::strcpy(bufferPtr, "(error while trying to copy description data)");
					}
				}
			}
		}
		
		assert(nullptr != keyString);
		assert(nullptr != descriptionString);
		//std::cerr << "reading " << keyString << " " << descriptionString << std::endl;
		
		if (disposeKeyString) delete [] keyString, keyString = nullptr;
		if (disposeDescriptionString) delete [] descriptionString, descriptionString = nullptr;
	}
#endif
}// readMacTelnetArrayPreference


/*!
Like readMacTelnetArrayPreference(), but is useful for
reading coordinate values.  The work of reading a
CFArrayRef containing two CFNumberRefs is done for you.

Zeroes are returned for nonexistent values.

(3.1)
*/
void
readMacTelnetCoordPreference	(CFStringRef	inKey,
								 SInt16&		outX,
								 SInt16&		outY)
{
	CFArrayRef		coords = nullptr;
	
	
	outX = 0;
	outY = 0;
	readMacTelnetArrayPreference(inKey, coords);
	if (nullptr != coords)
	{
		// the array should be a list of two numbers, in the order (X, Y)
		if (CFArrayGetCount(coords) > 0)
		{
			CFNumberRef		leftCoord = CFUtilities_NumberCast(CFArrayGetValueAtIndex(coords, 0));
			
			
			if (nullptr != leftCoord)
			{
				if (true != CFNumberGetValue(leftCoord, kCFNumberSInt16Type, &outX))
				{
					// okay, some kind of error; but best conversion result will still be in "outX"
				}
			}
		}
		if (CFArrayGetCount(coords) > 1)
		{
			CFNumberRef		topCoord = CFUtilities_NumberCast(CFArrayGetValueAtIndex(coords, 1));
			
			
			if (nullptr != topCoord)
			{
				if (true != CFNumberGetValue(topCoord, kCFNumberSInt16Type, &outY))
				{
					// okay, some kind of error; but best conversion result will still be in "outY"
				}
			}
		}
		CFRelease(coords), coords = nullptr;
	}
}// readMacTelnetCoordPreference


/*!
Updates stored preference values using master preferences from
the given dictionary.  If merging, conflicting keys are
skipped; otherwise, they are replaced with the new dictionary
values.

All of the keys in the specified dictionary must be of type
CFStringRef.

\retval noErr
currently always returned; there is no way to detect errors

(3.0)
*/
OSStatus
readPreferencesDictionary	(CFDictionaryRef	inPreferenceDictionary,
							 Boolean			inMerge)
{
	My_ContextInterfacePtr		contextPtr = nullptr;
	Boolean						gotContext = getDefaultContext(kPreferences_ClassGeneral, contextPtr);
	
	
	assert(gotContext);
	return readPreferencesDictionaryInContext(contextPtr, inPreferenceDictionary, inMerge);
}// readPreferencesDictionary


/*!
Updates stored preference values in the specified context, using
master preferences from the given dictionary.  If merging,
conflicting keys are skipped; otherwise, they are replaced with
the new dictionary values.

All of the keys in the specified dictionary must be of type
CFStringRef.

\retval noErr
currently always returned; there is no way to detect errors

(4.0)
*/
OSStatus
readPreferencesDictionaryInContext		(My_ContextInterfacePtr		inContextPtr,
										 CFDictionaryRef			inPreferenceDictionary,
										 Boolean					inMerge)
{
	// the keys can have their values copied directly into the application
	// preferences property list; they are identical in format and even use
	// the same key names
	CFIndex const	kDictLength = CFDictionaryGetCount(inPreferenceDictionary);
	CFIndex			i = 0;
	void const*		keyValue = nullptr;
	void const**	keys = new void const*[kDictLength];
	OSStatus		result = noErr;
	
	
	CFDictionaryGetKeysAndValues(inPreferenceDictionary, keys, nullptr/* values */);
	for (i = 0; i < kDictLength; ++i)
	{
		CFStringRef const	kKey = CFUtilities_StringCast(keys[i]);
		Boolean				useDefault = true;
		
		
		if (inMerge)
		{
			// when merging, do not replace any key that is already defined
			CFPropertyListRef	foundValue = inContextPtr->returnValueCopy(kKey);
			
			
			if (nullptr != foundValue)
			{
				useDefault = false;
				CFRelease(foundValue), foundValue = nullptr;
			}
			else
			{
				// value is not yet defined
				useDefault = true;
			}
		}
		if (useDefault)
		{
			if (CFDictionaryGetValueIfPresent(inPreferenceDictionary, kKey, &keyValue))
			{
				My_PreferenceDefinition*	definitionPtr = My_PreferenceDefinition::findByKeyName(kKey);
				
				
				if (nullptr == definitionPtr)
				{
					static Preferences_Index	gDummyIndex = 1;
					Preferences_Tag				dummyTag = Preferences_ReturnTagVariantForIndex('DUM\0', gDummyIndex++);
					
					
					Console_Warning(Console_WriteValueCFString, "using anonymous tag for unknown preference dictionary key", kKey);
					inContextPtr->addValue(dummyTag, kKey, keyValue);
				}
				else
				{
					//Console_WriteValueCFString("using given dictionary value for key", kKey); // debug
					assert(kCFCompareEqualTo == CFStringCompare(kKey, definitionPtr->keyName.returnCFStringRef(), 0/* options */));
					inContextPtr->addValue(definitionPtr->tag, definitionPtr->keyName.returnCFStringRef(), keyValue);
				}
			}
		}
	}
	delete [] keys;
	
	return result;
}// readPreferencesDictionaryInContext


/*!
Marks an alias as having changed.  This also marks the
alias resource by calling ChangedResource(), if the alias
is based on a resource.  Any Resource Manager error may
be returned.

(3.0)
*/
OSStatus
setAliasChanged		(My_AliasInfoPtr	inoutAliasDataPtr)
{
	OSStatus		result = noErr;
	
	
	if (nullptr != inoutAliasDataPtr)
	{
		inoutAliasDataPtr->wasChanged = true;
		if (inoutAliasDataPtr->isResource)
		{
			ChangedResource(REINTERPRET_CAST(inoutAliasDataPtr->alias, Handle));
			result = ResError();
		}
	}
	else result = memPCErr;
	
	return result;
}// setAliasChanged


/*!
Modifies the indicated font or color preference using
the given data (see Preferences.h and the definition of
each tag for comments on what data format is expected
for each one).

(3.0)
*/
Preferences_Result	
setFormatPreference		(My_ContextInterfacePtr		inContextPtr,
						 Preferences_Tag			inDataPreferenceTag,
						 size_t						inDataSize,
						 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Preferences_Class		dataClass = kPreferences_ClassFormat;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == kPreferences_ClassFormat);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagFontCharacterWidthMultiplier:
				{
					Float32 const* const	data = REINTERPRET_CAST(inDataPtr, Float32 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addFloat(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagFontName:
				{
					ConstStringPtr const	data = REINTERPRET_CAST(inDataPtr, ConstStringPtr);
					CFStringRef				fontNameCFString = nullptr;
					
					
					fontNameCFString = CFStringCreateWithPascalString(kCFAllocatorDefault, data, kCFStringEncodingMacRoman);
					if (nullptr == fontNameCFString) result = kPreferences_ResultGenericFailure;
					else
					{
						assert(typeCFStringRef == keyValueType);
						inContextPtr->addString(inDataPreferenceTag, keyName, (data) ? fontNameCFString : CFSTR(""));
						CFRelease(fontNameCFString), fontNameCFString = nullptr;
					}
				}
				break;
			
			case kPreferences_TagFontSize:
				{
					SInt16 const* const		data = REINTERPRET_CAST(inDataPtr, SInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalColorMatteBackground:
			case kPreferences_TagTerminalColorBlinkingForeground:
			case kPreferences_TagTerminalColorBlinkingBackground:
			case kPreferences_TagTerminalColorBoldForeground:
			case kPreferences_TagTerminalColorBoldBackground:
			case kPreferences_TagTerminalColorNormalForeground:
			case kPreferences_TagTerminalColorNormalBackground:
			case kPreferences_TagTerminalColorANSIBlack:
			case kPreferences_TagTerminalColorANSIRed:
			case kPreferences_TagTerminalColorANSIGreen:
			case kPreferences_TagTerminalColorANSIYellow:
			case kPreferences_TagTerminalColorANSIBlue:
			case kPreferences_TagTerminalColorANSIMagenta:
			case kPreferences_TagTerminalColorANSICyan:
			case kPreferences_TagTerminalColorANSIWhite:
			case kPreferences_TagTerminalColorANSIBlackBold:
			case kPreferences_TagTerminalColorANSIRedBold:
			case kPreferences_TagTerminalColorANSIGreenBold:
			case kPreferences_TagTerminalColorANSIYellowBold:
			case kPreferences_TagTerminalColorANSIBlueBold:
			case kPreferences_TagTerminalColorANSIMagentaBold:
			case kPreferences_TagTerminalColorANSICyanBold:
			case kPreferences_TagTerminalColorANSIWhiteBold:
				{
					RGBColor const* const	data = REINTERPRET_CAST(inDataPtr, RGBColor const*);
					CFArrayRef				colorCFArray = nullptr;
					
					
					if (convertRGBColorToCFArray(data, colorCFArray))
					{
						assert(typeCFArrayRef == keyValueType);
						inContextPtr->addArray(inDataPreferenceTag, keyName, colorCFArray);
						CFRelease(colorCFArray), colorCFArray = nullptr;
					}
					else result = kPreferences_ResultGenericFailure;
				}
				break;
			
			case kPreferences_TagTerminalMarginLeft:
			case kPreferences_TagTerminalMarginRight:
			case kPreferences_TagTerminalMarginTop:
			case kPreferences_TagTerminalMarginBottom:
			case kPreferences_TagTerminalPaddingLeft:
			case kPreferences_TagTerminalPaddingRight:
			case kPreferences_TagTerminalPaddingTop:
			case kPreferences_TagTerminalPaddingBottom:
				{
					Float32 const	data = *(REINTERPRET_CAST(inDataPtr, Float32 const*));
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addFloat(inDataPreferenceTag, keyName, data);
				}
				break;
			
			default:
				// unrecognized tag
				result = kPreferences_ResultUnknownTagOrClass;
				break;
			}
		}
	}
	
	return result;
}// setFormatPreference


/*!
Modifies the indicated global preference using the given
data (see Preferences.h and the definition of each tag
for comments on what data format is expected for each one).

(3.0)
*/
Preferences_Result	
setGeneralPreference	(My_ContextInterfacePtr		inContextPtr,
						 Preferences_Tag			inDataPreferenceTag,
						 size_t						inDataSize,
						 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Preferences_Class		dataClass = kPreferences_ClassGeneral;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == kPreferences_ClassGeneral);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagArrangeWindowsUsingTabs:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagBellSound:
				{
					CFStringRef const	data = *(REINTERPRET_CAST(inDataPtr, CFStringRef const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, data);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagCaptureFileCreator:
				{
					OSType const	data = *(REINTERPRET_CAST(inDataPtr, OSType const*));
					CFStringRef		stringRef = CFStringCreateWithBytes
												(kCFAllocatorDefault,
													REINTERPRET_CAST(&data, UInt8 const*),
													sizeof(data),
													kCFStringEncodingMacRoman, false/* is external representation */);
					
					
					if (nullptr != stringRef)
					{
						assert(typeCFStringRef == keyValueType);
						setMacTelnetPreference(keyName, stringRef);
						CFRelease(stringRef), stringRef = nullptr;
					}
				}
				break;
			
			case kPreferences_TagCopySelectedText:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagCopyTableThreshold:
				{
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					CFNumberRef		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &data);
					
					
					if (nullptr != numberRef)
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						setMacTelnetPreference(keyName, numberRef);
						CFRelease(numberRef), numberRef = nullptr;
					}
				}
				break;
			
			case kPreferences_TagCursorBlinks:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagCursorMovesPriorToDrops:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagDontAutoClose:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagDontAutoNewOnApplicationReopen:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagDontDimBackgroundScreens:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagFocusFollowsMouse:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagHeadersCollapsed:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagInfoWindowColumnOrdering:
				{
					CFArrayRef const* const		data = REINTERPRET_CAST(inDataPtr, CFArrayRef const*);
					
					
					assert(typeCFArrayRef == keyValueType);
					inContextPtr->addArray(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagKioskAllowsForceQuit:
			case kPreferences_TagKioskShowsMenuBar:
			case kPreferences_TagKioskShowsOffSwitch:
			case kPreferences_TagKioskShowsScrollBar:
			case kPreferences_TagKioskUsesSuperfluousEffects:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagMacrosMenuVisible:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagMapBackquote:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("\\e") : CFSTR(""));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagMarginBell:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("bell") : CFSTR("ignore"));
				}
				break;
			
			case kPreferences_TagMenuItemKeys:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagNewCommandShortcutEffect:
				{
					UInt32 const	data = *(REINTERPRET_CAST(inDataPtr, UInt32 const*));
					
					
					assert(typeCFStringRef == keyValueType);
					switch (data)
					{
					case kCommandNewSessionDialog:
						setMacTelnetPreference(keyName, CFSTR("dialog"));
						break;
					
					case kCommandNewSessionDefaultFavorite:
						setMacTelnetPreference(keyName, CFSTR("default"));
						break;
					
					case kCommandNewSessionLoginShell:
						setMacTelnetPreference(keyName, CFSTR("log-in shell"));
						break;
					
					case kCommandNewSessionShell:
					default:
						setMacTelnetPreference(keyName, CFSTR("shell"));
						break;
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagNotification:
				{
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					
					
					assert(typeCFStringRef == keyValueType);
					switch (data)
					{
					case kAlert_NotifyDoNothing:
						setMacTelnetPreference(keyName, CFSTR("ignore"));
						break;
					
					case kAlert_NotifyDisplayIconAndDiamondMark:
						setMacTelnetPreference(keyName, CFSTR("animate"));
						break;
					
					case kAlert_NotifyAlsoDisplayAlert:
						setMacTelnetPreference(keyName, CFSTR("alert"));
						break;
					
					case kAlert_NotifyDisplayDiamondMark:
					default:
						setMacTelnetPreference(keyName, CFSTR("badge"));
						break;
					}
					Alert_SetNotificationPreferences(data);
				}
				break;
			
			case kPreferences_TagNotifyOfBeeps:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("notify") : CFSTR("ignore"));
				}
				break;
			
			case kPreferences_TagPureInverse:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagRandomTerminalFormats:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagSimplifiedUserInterface:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagTerminalCursorType:
				{
					TerminalView_CursorType const	data = *(REINTERPRET_CAST(inDataPtr, TerminalView_CursorType const*));
					
					
					assert(typeCFStringRef == keyValueType);
					switch (data)
					{
					case kTerminalView_CursorTypeUnderscore:
						setMacTelnetPreference(keyName, CFSTR("underline"));
						break;
					
					case kTerminalView_CursorTypeVerticalLine:
						setMacTelnetPreference(keyName, CFSTR("vertical bar"));
						break;
					
					case kTerminalView_CursorTypeThickUnderscore:
						setMacTelnetPreference(keyName, CFSTR("thick underline"));
						break;
					
					case kTerminalView_CursorTypeThickVerticalLine:
						setMacTelnetPreference(keyName, CFSTR("thick vertical bar"));
						break;
					
					case kTerminalView_CursorTypeBlock:
					default:
						setMacTelnetPreference(keyName, CFSTR("block"));
						break;
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagTerminalResizeAffectsFontSize:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("font") : CFSTR("screen"));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagVisualBell:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("visual") : CFSTR("audio"));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagWasClipboardShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasCommandLineShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasControlKeypadShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasFunctionKeypadShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasSessionInfoShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasVT220KeypadShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWindowStackingOrigin:
				{
					Point const		data = *(REINTERPRET_CAST(inDataPtr, Point const*));
					CFNumberRef		leftCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type,
																&data.h);
					
					
					if (nullptr != leftCoord)
					{
						CFNumberRef		topCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type,
																	&data.v);
						
						
						if (nullptr != topCoord)
						{
							CFNumberRef		values[] = { leftCoord, topCoord };
							CFArrayRef		coords = CFArrayCreate(kCFAllocatorDefault,
																	REINTERPRET_CAST(values, void const**),
																	sizeof(values) / sizeof(CFNumberRef),
																	&kCFTypeArrayCallBacks);
							
							
							if (nullptr != coords)
							{
								assert(typeCFArrayRef == keyValueType);
								setMacTelnetPreference(keyName, coords);
								CFRelease(coords), coords = nullptr;
							}
							CFRelease(topCoord), topCoord = nullptr;
						}
						CFRelease(leftCoord), leftCoord = nullptr;
					}
				}
				break;
			
			default:
				// unrecognized tag
				result = kPreferences_ResultUnknownTagOrClass;
				break;
			}
		}
	}
	
	return result;
}// setGeneralPreference


/*!
Modifies the indicated preference for a specific macro using
the given data (see Preferences.h and the definition of each
tag for comments on what data format is expected for each one).
In this method, 0 is an invalid index.

(3.1)
*/
Preferences_Result	
setMacroPreference	(My_ContextInterfacePtr		inContextPtr,
					 Preferences_Tag			inDataPreferenceTag,
					 size_t						inDataSize,
					 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Preferences_Class		dataClass = kPreferences_ClassMacroSet;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == kPreferences_ClassMacroSet);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			Preferences_Tag const		kTagWithoutIndex = Preferences_ReturnTagFromVariant(inDataPreferenceTag);
			
			
			switch (kTagWithoutIndex)
			{
			case kPreferences_TagIndexedMacroAction:
				{
					MacroManager_Action const	data = *(REINTERPRET_CAST(inDataPtr, MacroManager_Action const*));
					
					
					switch (data)
					{
					case kMacroManager_ActionSendTextVerbatim:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("verbatim"));
						break;
					
					case kMacroManager_ActionSendTextProcessingEscapes:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("text"));
						break;
					
					case kMacroManager_ActionHandleURL:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("handle URL"));
						break;
					
					case kMacroManager_ActionNewWindowWithCommand:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("new window with command"));
						break;
					
					default:
						// ???
						assert(false && "attempt to save macro with invalid action type");
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("text"));
						break;
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagIndexedMacroContents:
			case kPreferences_TagIndexedMacroName:
				// all of these keys have Core Foundation string values
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagIndexedMacroKey:
				{
					MacroManager_KeyID const	data = *(REINTERPRET_CAST(inDataPtr, MacroManager_KeyID const*));
					UInt16 const				kKeyCode = MacroManager_KeyIDKeyCode(data);
					Boolean const				kIsVirtualKey = MacroManager_KeyIDIsVirtualKey(data);
					
					
					if (kIsVirtualKey)
					{
						CFRetainRelease		virtualKeyName(virtualKeyCreateName(kKeyCode), true/* is retained */);
						
						
						if (virtualKeyName.exists())
						{
							inContextPtr->addString(inDataPreferenceTag, keyName,
													virtualKeyName.returnCFStringRef());
						}
						else
						{
							// ???
							assert(false && "attempt to save macro with unknown virtual key code");
							inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR(""));
						}
					}
					else
					{
						UniChar				keyCodeAsUnicode = kKeyCode;
						CFRetainRelease		characterCFString(CFStringCreateWithCharacters
																(kCFAllocatorDefault, &keyCodeAsUnicode, 1),
																true/* is retained */);
						
						
						if (characterCFString.exists())
						{
							inContextPtr->addString(inDataPreferenceTag, keyName,
													characterCFString.returnCFStringRef());
						}
						else
						{
							// ???
							assert(false && "unable to translate key character into Unicode string");
							inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR(""));
						}
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagIndexedMacroKeyModifiers:
				{
					UInt32 const	data = *(REINTERPRET_CAST(inDataPtr, UInt32 const*));
					CFStringRef		values[4/* arbitrary, WARNING this must be no smaller than the step count below! */];
					size_t			keysAdded = 0;
					
					
					if (data & cmdKey)
					{
						values[keysAdded] = CFSTR("command");
						++keysAdded;
					}
					if (data & controlKey)
					{
						values[keysAdded] = CFSTR("control");
						++keysAdded;
					}
					if (data & optionKey)
					{
						values[keysAdded] = CFSTR("option");
						++keysAdded;
					}
					if (data & shiftKey)
					{
						values[keysAdded] = CFSTR("shift");
						++keysAdded;
					}
					
					// now create and save the array of selected modifier key names
					{
						CFArrayRef		modifierCFArray = CFArrayCreate(kCFAllocatorDefault,
																		REINTERPRET_CAST(values, void const**),
																		keysAdded, &kCFTypeArrayCallBacks);
						
						
						if (nullptr != modifierCFArray)
						{
							assert(typeCFArrayRef == keyValueType);
							inContextPtr->addArray(inDataPreferenceTag, keyName, modifierCFArray);
							CFRelease(modifierCFArray), modifierCFArray = nullptr;
						}
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			default:
				// unrecognized tag
				result = kPreferences_ResultUnknownTagOrClass;
				break;
			}
		}
	}
	
	return result;
}// setMacroPreference


/*!
Like setMacTelnetPreference(), but is useful for creating
coordinate values.  The work of creating a CFArrayRef
containing two CFNumberRefs is done for you.

Returns "true" only if it was possible to create the
necessary Core Foundation types.

(3.1)
*/
Boolean
setMacTelnetCoordPreference		(CFStringRef	inKey,
								 SInt16			inX,
								 SInt16			inY)
{
	CFNumberRef		leftCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &inX);
	Boolean			result = false;
	
	
	if (nullptr != leftCoord)
	{
		CFNumberRef		topCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &inY);
		
		
		if (nullptr != topCoord)
		{
			CFNumberRef		values[] = { leftCoord, topCoord };
			CFArrayRef		coords = CFArrayCreate(kCFAllocatorDefault,
													REINTERPRET_CAST(values, void const**),
													sizeof(values) / sizeof(CFNumberRef),
													&kCFTypeArrayCallBacks);
			
			
			if (nullptr != coords)
			{
				setMacTelnetPreference(inKey, coords);
				result = true; // technically, it’s not possible to see if setMacTelnetPreference() succeeded...
				CFRelease(coords), coords = nullptr;
			}
			CFRelease(topCoord), topCoord = nullptr;
		}
		CFRelease(leftCoord), leftCoord = nullptr;
	}
	
	return result;
}// setMacTelnetCoordPreference


/*!
Updates MacTelnet’s XML preferences as requested.
Also prints to standard output information about
the data being written (if debugging is enabled).

(3.1)
*/
void
setMacTelnetPreference	(CFStringRef		inKey,
						 CFPropertyListRef	inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, kCFPreferencesCurrentApplication);
#if 1
	{
		// for debugging
		CFStringRef		descriptionCFString = CFCopyDescription(inValue);
		char const*		keyString = nullptr;
		char const*		descriptionString = nullptr;
		Boolean			disposeKeyString = false;
		Boolean			disposeDescriptionString = false;
		
		
		keyString = CFStringGetCStringPtr(inKey, CFStringGetFastestEncoding(inKey));
		if (nullptr == keyString)
		{
			// allocate double the size because the string length assumes 16-bit characters
			size_t const	kBufferSize = INTEGER_DOUBLED(CFStringGetLength(inKey));
			char*			bufferPtr = new char[kBufferSize];
			
			
			if (nullptr != bufferPtr)
			{
				keyString = bufferPtr;
				disposeKeyString = true;
				if (CFStringGetCString(inKey, bufferPtr, kBufferSize,
										CFStringGetFastestEncoding(inKey)))
				{
					// no idea what the problem is, but just bail
					std::strcpy(bufferPtr, "(error while trying to copy key data)");
				}
			}
		}
		
		if (nullptr == descriptionCFString)
		{
			descriptionString = "(no value description available)";
		}
		else
		{
			descriptionString = CFStringGetCStringPtr
								(descriptionCFString, CFStringGetFastestEncoding(descriptionCFString));
			if (nullptr == descriptionString)
			{
				// allocate double the size because the string length assumes 16-bit characters
				size_t const	kBufferSize = INTEGER_DOUBLED(CFStringGetLength(descriptionCFString));
				char*			bufferPtr = new char[kBufferSize];
				
				
				if (nullptr != bufferPtr)
				{
					descriptionString = bufferPtr;
					disposeDescriptionString = true;
					if (CFStringGetCString(descriptionCFString, bufferPtr, kBufferSize,
											CFStringGetFastestEncoding(descriptionCFString)))
					{
						// no idea what the problem is, but just bail
						std::strcpy(bufferPtr, "(error while trying to copy description data)");
					}
				}
			}
		}
		
		assert(nullptr != keyString);
		assert(nullptr != descriptionString);
		//std::cerr << "saving " << keyString << " " << descriptionString << std::endl;
		
		if (disposeKeyString) delete [] keyString, keyString = nullptr;
		if (disposeDescriptionString) delete [] descriptionString, descriptionString = nullptr;
	}
#endif
}// setMacTelnetPreference


/*!
Modifies the indicated session preference using the
given data (see Preferences.h and the definition of
each tag for comments on what data format is expected
for each one).

(3.0)
*/
Preferences_Result	
setSessionPreference	(My_ContextInterfacePtr		inContextPtr,
						 Preferences_Tag			inDataPreferenceTag,
						 size_t						inDataSize,
						 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Preferences_Class		dataClass = kPreferences_ClassSession;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == kPreferences_ClassSession);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagAssociatedFormatFavorite:
			case kPreferences_TagAssociatedTerminalFavorite:
			case kPreferences_TagAssociatedTranslationFavorite:
			case kPreferences_TagServerHost:
			case kPreferences_TagServerUserID:
				// all of these keys have Core Foundation string values
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagAutoCaptureToFile:
			case kPreferences_TagLineModeEnabled:
			case kPreferences_TagLocalEchoEnabled:
			case kPreferences_TagLocalEchoHalfDuplex:
			case kPreferences_TagTektronixPAGEClearsScreen:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagCaptureFileAlias:
				{
					Preferences_AliasID const* const	data = REINTERPRET_CAST(inDataPtr, Preferences_AliasID const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagCommandLine:
				{
					CFArrayRef const* const		data = REINTERPRET_CAST(inDataPtr, CFArrayRef const*);
					
					
					assert(typeCFArrayRef == keyValueType);
					inContextPtr->addArray(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagDataReadBufferSize:
			case kPreferences_TagPasteBlockSize:
			case kPreferences_TagServerPort:
				{
					SInt16 const* const		data = REINTERPRET_CAST(inDataPtr, SInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagIdleAfterInactivityInSeconds:
			case kPreferences_TagKeepAlivePeriodInMinutes:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagKeyInterruptProcess:
			case kPreferences_TagKeyResumeOutput:
			case kPreferences_TagKeySuspendOutput:
				{
					UniChar				charArray[2];
					CFStringRef			charCFString = nullptr;
					char const* const	data = REINTERPRET_CAST(inDataPtr, char const*);
					
					
					charArray[0] = '^';
					charArray[1] = *data + '@'; // convert to visible character
					charCFString = CFStringCreateWithCharacters
									(kCFAllocatorDefault, charArray, sizeof(charArray) / sizeof(UniChar));
					if (nullptr != charCFString)
					{
						inContextPtr->addString(inDataPreferenceTag, keyName, charCFString);
						CFRelease(charCFString), charCFString = nullptr;
					}
				}
				break;
			
			case kPreferences_TagMapCarriageReturnToCRNull:
				{
					Boolean const* const	data = REINTERPRET_CAST(inDataPtr, Boolean const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, (*data) ? CFSTR("\\015\\000") : CFSTR("\\015\\012"));
				}
				break;
			
			case kPreferences_TagMapDeleteToBackspace:
				{
					Boolean const* const	data = REINTERPRET_CAST(inDataPtr, Boolean const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, (*data) ? CFSTR("backspace") : CFSTR("delete"));
				}
				break;
			
			case kPreferences_TagPasteMethod:
				{
					Clipboard_PasteMethod const* const	data = REINTERPRET_CAST(inDataPtr, Clipboard_PasteMethod const*);
					
					
					assert(typeCFStringRef == keyValueType);
					switch (*data)
					{
					case kClipboard_PasteMethodStandard:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("normal"));
						break;
					
					case kClipboard_PasteMethodBlock:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("throttled"));
						break;
					
					default:
						// ???
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("normal"));
						break;
					}
				}
				break;
			
			case kPreferences_TagServerProtocol:
				{
					Session_Protocol const* const	data = REINTERPRET_CAST(inDataPtr, Session_Protocol const*);
					
					
					assert(typeCFStringRef == keyValueType);
					switch (*data)
					{
					case kSession_ProtocolFTP:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("ftp"));
						break;
					
					case kSession_ProtocolSFTP:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("sftp"));
						break;
					
					case kSession_ProtocolSSH1:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("ssh-1"));
						break;
					
					case kSession_ProtocolSSH2:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("ssh-2"));
						break;
					
					case kSession_ProtocolTelnet:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("telnet"));
						break;
					
					default:
						// ???
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("ssh-1"));
						break;
					}
				}
				break;
			
			case kPreferences_TagTektronixMode:
				{
					VectorInterpreter_Mode const* const		data = REINTERPRET_CAST(inDataPtr, VectorInterpreter_Mode const*);
					
					
					assert(typeCFStringRef == keyValueType);
					switch (*data)
					{
					case kVectorInterpreter_ModeDisabled:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("off"));
						break;
					
					case kVectorInterpreter_ModeTEK4014:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("4014"));
						break;
					
					case kVectorInterpreter_ModeTEK4105:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("4105"));
						break;
					
					default:
						// ???
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("off"));
						break;
					}
				}
				break;
			
			default:
				// unrecognized tag
				result = kPreferences_ResultUnknownTagOrClass;
				break;
			}
		}
	}
	
	return result;
}// setSessionPreference


/*!
Modifies the indicated terminal preference (which no
longer includes font and color), using the given data.
See Preferences.h and the definition of each tag for
comments on what data format is expected for each one.

(3.0)
*/
Preferences_Result	
setTerminalPreference	(My_ContextInterfacePtr		inContextPtr,
						 Preferences_Tag			inDataPreferenceTag,
						 size_t						inDataSize,
						 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Preferences_Class		dataClass = kPreferences_ClassTerminal;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == kPreferences_ClassTerminal);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagDataReceiveDoNotStripHighBit:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagEMACSMetaKey:
				{
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					
					
					assert(typeCFStringRef == keyValueType);
					switch (data)
					{
					case kSession_EMACSMetaKeyOption:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("option"));
						break;
					
					case kSession_EMACSMetaKeyControlCommand:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("control+command"));
						break;
					
					case kSession_EMACSMetaKeyOff:
					default:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR(""));
						break;
					}
				}
				break;
			
			case kPreferences_TagMapArrowsForEMACS:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-emacs-move-down"),
											(data) ? CFSTR("down-arrow") : CFSTR(""));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-emacs-move-left"),
											(data) ? CFSTR("left-arrow") : CFSTR(""));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-emacs-move-right"),
											(data) ? CFSTR("right-arrow") : CFSTR(""));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-emacs-move-up"),
											(data) ? CFSTR("up-arrow") : CFSTR(""));
				}
				break;
			
			case kPreferences_TagMapKeypadTopRowForVT220:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-vt220-pf1"),
											(data) ? CFSTR("") : CFSTR("keypad-clear"));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-vt220-pf2"),
											(data) ? CFSTR("") : CFSTR("keypad-="));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-vt220-pf3"),
											(data) ? CFSTR("") : CFSTR("keypad-/"));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-vt220-pf4"),
											(data) ? CFSTR("") : CFSTR("keypad-*"));
				}
				break;
			
			case kPreferences_TagPageKeysControlLocalTerminal:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-terminal-end"),
											(data) ? CFSTR("end") : CFSTR(""));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-terminal-home"),
											(data) ? CFSTR("home") : CFSTR(""));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-terminal-page-down"),
											(data) ? CFSTR("page-down") : CFSTR(""));
					inContextPtr->addString(inDataPreferenceTag, CFSTR("command-key-terminal-page-up"),
											(data) ? CFSTR("page-up") : CFSTR(""));
				}
				break;
			
			case kPreferences_TagTerminalAnswerBackMessage:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalClearSavesLines:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagTerminalEmulatorType:
				if (inDataSize >= sizeof(Terminal_Emulator))
				{
					Terminal_Emulator const		data = *(REINTERPRET_CAST(inDataPtr, Terminal_Emulator const*));
					CFRetainRelease				nameCFString = Terminal_EmulatorReturnDefaultName(data);
					
					
					assert(typeCFStringRef == keyValueType);
					if (false == nameCFString.exists()) result = kPreferences_ResultGenericFailure;
					else
					{
						inContextPtr->addString(inDataPreferenceTag, keyName, nameCFString.returnCFStringRef());
					}
				}
				break;
			
			case kPreferences_TagTerminalLineWrap:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagTerminalScreenColumns:
			case kPreferences_TagTerminalScreenRows:
			case kPreferences_TagTerminalScreenScrollbackRows:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalScreenScrollbackType:
				{
					Terminal_ScrollbackType const		data = *(REINTERPRET_CAST(inDataPtr, Terminal_ScrollbackType const*));
					
					
					assert(typeCFStringRef == keyValueType);
					switch (data)
					{
					case kTerminal_ScrollbackTypeDisabled:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("off"));
						break;
					
					case kTerminal_ScrollbackTypeUnlimited:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("unlimited"));
						break;
					
					case kTerminal_ScrollbackTypeDistributed:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("distributed"));
						break;
					
					case kTerminal_ScrollbackTypeFixed:
					default:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("fixed"));
						break;
					}
				}
				break;
			
			case kPreferences_TagTerminalScrollDelay:
				{
					EventTime const* const		data = REINTERPRET_CAST(inDataPtr, EventTime const*);
					EventTime					junk = *data / kEventDurationMillisecond;
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, STATIC_CAST(junk, SInt16));
				}
				break;
			
			case kPreferences_TagVT100FixLineWrappingBug:
			case kPreferences_TagXTerm256ColorsEnabled:
			case kPreferences_TagXTermColorEnabled:
			case kPreferences_TagXTermGraphicsEnabled:
			case kPreferences_TagXTermWindowAlterationEnabled:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			default:
				// unrecognized tag
				result = kPreferences_ResultUnknownTagOrClass;
				break;
			}
		}
	}
	
	return result;
}// setTerminalPreference


/*!
Modifies the indicated translation preference using the
given data (see Preferences.h and the definition of each
tag for comments on what data format is expected for each
one).

(3.1)
*/
Preferences_Result	
setTranslationPreference	(My_ContextInterfacePtr		inContextPtr,
							 Preferences_Tag			inDataPreferenceTag,
							 size_t						inDataSize,
							 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Preferences_Class		dataClass = kPreferences_ClassTranslation;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == kPreferences_ClassTranslation);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagBackupFontName:
				{
					ConstStringPtr const	data = REINTERPRET_CAST(inDataPtr, ConstStringPtr);
					CFStringRef				fontNameCFString = nullptr;
					
					
					fontNameCFString = CFStringCreateWithPascalString(kCFAllocatorDefault, data, kCFStringEncodingMacRoman);
					if (nullptr == fontNameCFString) result = kPreferences_ResultGenericFailure;
					else
					{
						assert(typeCFStringRef == keyValueType);
						inContextPtr->addString(inDataPreferenceTag, keyName, (data) ? fontNameCFString : CFSTR(""));
						CFRelease(fontNameCFString), fontNameCFString = nullptr;
					}
				}
				break;
			
			case kPreferences_TagTextEncodingIANAName:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTextEncodingID:
				{
					CFStringEncoding const		data = *(REINTERPRET_CAST(inDataPtr, CFStringEncoding const*));
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addLong(inDataPreferenceTag, keyName, STATIC_CAST(data, SInt32));
				}
				break;
			
			default:
				// unrecognized tag
				result = kPreferences_ResultUnknownTagOrClass;
				break;
			}
		}
	}
	
	return result;
}// setTranslationPreference


/*!
Creates a name that describes the specified virtual key
code, or returns "nullptr" if nothing fits.

INCOMPLETE.

Virtual key codes are poorly documented, but they are
described in older Inside Macintosh books!

IMPORTANT:	This should mirror the implementation of
			virtualKeyParseName().

(4.0)
*/
CFStringRef
virtualKeyCreateName	(UInt16		inVirtualKeyCode)
{
	CFStringRef		result = nullptr;
	
	
	// INCOMPLETE!!!
	// (yes, these really are assigned as bizarrely as they seem...)
	switch (inVirtualKeyCode)
	{
	case 0x24:
		result = CFSTR("return");
		break;
	
	case 0x33:
		result = CFSTR("backward-delete");
		break;
	
	case 0x35:
		result = CFSTR("escape");
		break;
	
	case 0x47:
		result = CFSTR("clear");
		break;
	
	case 0x4C:
		result = CFSTR("enter");
		break;
	
	case 0x60:
		result = CFSTR("f5");
		break;
	
	case 0x61:
		result = CFSTR("f6");
		break;
	
	case 0x62:
		result = CFSTR("f7");
		break;
	
	case 0x63:
		result = CFSTR("f3");
		break;
	
	case 0x64:
		result = CFSTR("f8");
		break;
	
	case 0x65:
		result = CFSTR("f9");
		break;
	
	case 0x67:
		result = CFSTR("f11");
		break;
	
	case 0x69:
		result = CFSTR("f13");
		break;
	
	case 0x6A:
		result = CFSTR("f16");
		break;
	
	case 0x6B:
		result = CFSTR("f14");
		break;
	
	case 0x6D:
		result = CFSTR("f10");
		break;
	
	case 0x6F:
		result = CFSTR("f12");
		break;
	
	case 0x71:
		result = CFSTR("f15");
		break;
	
	case 0x73:
		result = CFSTR("home");
		break;
	
	case 0x74:
		result = CFSTR("page-up");
		break;
	
	case 0x75:
		result = CFSTR("forward-delete");
		break;
	
	case 0x76:
		result = CFSTR("f4");
		break;
	
	case 0x77:
		result = CFSTR("end");
		break;
	
	case 0x78:
		result = CFSTR("f2");
		break;
	
	case 0x79:
		result = CFSTR("page-down");
		break;
	
	case 0x7A:
		result = CFSTR("f1");
		break;
	
	case 0x7B:
		result = CFSTR("left-arrow");
		break;
	
	case 0x7C:
		result = CFSTR("right-arrow");
		break;
	
	case 0x7D:
		result = CFSTR("down-arrow");
		break;
	
	case 0x7E:
		result = CFSTR("up-arrow");
		break;
	
	default:
		// ???
		break;
	}
	
	if (nullptr != result) CFRetain(result);
	return result;
}// virtualKeyCreateName


/*!
Parses a name that might have been originally created
with virtualKeyCreateName(), and fills in information
about the key (character or virtual) that it describes.
Returns true only if the name is recognized.

INCOMPLETE.

A valid string would be a key with a single character,
e.g. "A", but it could also be a descriptive name like
"home" or "f12".

Virtual key codes are poorly documented, but they are
described in older Inside Macintosh books!

IMPORTANT:	This should mirror the implementation of
			virtualKeyCreateName().

(4.0)
*/
Boolean
virtualKeyParseName	(CFStringRef	inName,
					 UInt16&		outCharacterOrKeyCode,
					 Boolean&		outIsVirtualKey)
{
	Boolean		result = true;
	
	
	if (1 == CFStringGetLength(inName))
	{
		outCharacterOrKeyCode = STATIC_CAST(CFStringGetCharacterAtIndex(inName, 0), UInt16);
		outIsVirtualKey = false;
	}
	else
	{
		CFOptionFlags const		kCompareFlags = kCFCompareCaseInsensitive;
		
		
		outIsVirtualKey = true;
		
		// INCOMPLETE!!!
		// (yes, these really are assigned as bizarrely as they seem...)
		if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("return"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x24;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("backward-delete"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x33;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("escape"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x35;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("clear"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x47;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("enter"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x4C;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f5"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x60;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f6"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x61;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f7"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x62;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f3"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x63;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f8"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x64;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f9"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x65;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f11"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x67;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f13"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x69;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f16"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x6A;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f14"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x6B;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f10"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x6D;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f12"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x6F;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f15"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x71;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("home"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x73;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("page-up"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x74;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("forward-delete"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x75;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f4"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x76;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("end"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x77;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f2"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x78;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("page-down"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x79;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f1"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x7A;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("left-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x7B;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("right-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x7C;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("down-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x7D;
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("up-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = 0x7E;
		}
		else
		{
			// ???
			result = false;
			outCharacterOrKeyCode = 0;
			outIsVirtualKey = false;
		}
	}
	return result;
}// virtualKeyParseName


Boolean					virtualKeyParseName						(CFStringRef, UInt16&, Boolean&);

} // anonymous namespace


#pragma mark Internal Methods: Unit Tests
namespace {

/*!
Tests the CFDictionary context interface.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messagess should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest000_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextCFDictionary::unitTest();
	return result;
}// unitTest000_Begin


/*!
Tests the CFPreferences (defaults) context interface.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messagess should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest001_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextDefault::unitTest();
	return result;
}// unitTest001_Begin


/*!
Tests the CFPreferences-in-sub-domain context with
the Session class.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messagess should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest002_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextFavorite::unitTest(kPreferences_ClassSession, CFSTR("__test_session__"));
	return result;
}// unitTest002_Begin


/*!
Tests the CFPreferences-in-sub-domain context with
the Format class.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messagess should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest003_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextFavorite::unitTest(kPreferences_ClassFormat, CFSTR("__test_format__"));
	return result;
}// unitTest003_Begin

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
