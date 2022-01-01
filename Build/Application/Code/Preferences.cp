/*!	\file Preferences.cp
	\brief Interfaces to access and modify user preferences,
	or be notified when they are changed.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#include "Preferences.h"
#include <UniversalDefines.h>

// standard-C++ includes
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

// pseudo-standard-C++ includes
#if __has_include(<tr1/unordered_map>)
#	include <tr1/unordered_map>
#	include <tr1/unordered_set>
#	ifndef unordered_map_namespace
#		define unordered_map_namespace std::tr1
#	endif
#	ifndef unordered_set_namespace
#		define unordered_set_namespace std::tr1
#	endif
#elif __has_include(<unordered_map>)
#	include <unordered_map>
#	include <unordered_set>
#	ifndef unordered_map_namespace
#		define unordered_map_namespace std
#	endif
#	ifndef unordered_set_namespace
#		define unordered_set_namespace std
#	endif
#else
#	error "Do not know how to find <unordered_map> with this compiler."
#endif

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h> // for kVK... virtual key codes (TEMPORARY; deprecated)
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CFKeyValueInterface.h>
#include <CFUtilities.h>
#include <CocoaUserDefaults.h>
#include <Console.h>
#include <ListenerModel.h>
#include <Registrar.template.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlockReferenceLocker.template.h>
#include <MemoryBlockReferenceTracker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>

// application includes
#include "AppResources.h"
#include "Commands.h"
#include "Keypads.h"
#include "MacroManager.h"
#include "Session.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "UIStrings.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

CFStringEncoding const		kMy_SavedNameEncoding = kCFStringEncodingUnicode;
CFStringRef const			kMy_PreferencesSubDomainAutoSave = CFSTR("net.macterm.MacTerm.autosave");
CFStringRef const			kMy_PreferencesSubDomainFormats = CFSTR("net.macterm.MacTerm.formats");
CFStringRef const			kMy_PreferencesSubDomainMacros = CFSTR("net.macterm.MacTerm.macros");
CFStringRef const			kMy_PreferencesSubDomainSessions = CFSTR("net.macterm.MacTerm.sessions");
CFStringRef const			kMy_PreferencesSubDomainTerminals = CFSTR("net.macterm.MacTerm.terminals");
CFStringRef const			kMy_PreferencesSubDomainTranslations = CFSTR("net.macterm.MacTerm.translations");
CFStringRef const			kMy_PreferencesSubDomainWorkspaces = CFSTR("net.macterm.MacTerm.workspaces");

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

typedef MemoryBlockReferenceTracker< Preferences_ContextRef >				My_ContextReferenceTracker;
typedef Registrar< Preferences_ContextRef, My_ContextReferenceTracker >		My_ContextReferenceRegistrar;

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
	
	My_ContextReferenceRegistrar	refValidator;	//! ensures this reference is recognized as a valid one
	Preferences_ContextRef			selfRef;		//!< convenient, redundant self-reference
	
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
	
	//! returns true only if the specified key is defined in the dictionary
	virtual Boolean
	exists	(CFStringRef	inKey) const
	{
		return _implementorPtr->exists(inKey);
	}
	
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
	inline Quills::Prefs::Class
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
	My_ContextInterface	(Quills::Prefs::Class);
	
	void
	setImplementor	(CFKeyValueInterface*);

private:
	Quills::Prefs::Class	_preferencesClass;	//!< hint as to what keys are likely to be present
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
	My_ContextCFDictionary	(Quills::Prefs::Class, CFDictionaryRef);
	
	My_ContextCFDictionary	(Quills::Prefs::Class, CFMutableDictionaryRef = nullptr);
	
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
	My_ContextDefault	(Quills::Prefs::Class);
	
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
	My_ContextFavorite	(Quills::Prefs::Class, CFStringRef, CFStringRef = nullptr);
	
	//!\name New Methods In This Class
	//@{
	
	//! returns the common prefix for any domain of a collection in the given class
	static CFStringRef
	returnClassDomainNamePrefix	(Quills::Prefs::Class);
	
	//! rearrange this context relative to another context
	Preferences_Result
	shift	(My_ContextFavorite*, Boolean = false);
	
	//! test routine
	static Boolean
	unitTest	(Quills::Prefs::Class, CFStringRef);
	
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
	createDomainName	(Quills::Prefs::Class, CFStringRef);

private:
	CFRetainRelease			_contextName;	//!< CFStringRef; a display name for this context
	CFRetainRelease			_domainName;	//!< CFStringRef; an arbitrary but class-based sub-domain for settings
	CFKeyValuePreferences	_dictionary;	//!< handles key value lookups in the chosen sub-domain
};
typedef My_ContextFavorite const*	My_ContextFavoriteConstPtr;
typedef My_ContextFavorite*			My_ContextFavoritePtr;

/*!
Keeps track of all named contexts in memory.  Named contexts
directly correspond to data on disk (unlike, say, temporary
contexts that exist only in memory).  For access to variables
of this type, use getListOfContexts() or (in extremely rare
cases) getMutableListOfContexts	().

While these lists should probably contain smarter pointers,
there are only two APIs that will manage the contents of the
lists: Preferences_NewContextFromFavorites() and
Preferences_ContextDeleteFromFavorites().
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
				 Quills::Prefs::Class, My_PreferenceDefinition** = nullptr);
	
	static void
	createFlag	(Preferences_Tag, CFStringRef, Quills::Prefs::Class,
				 My_PreferenceDefinition** = nullptr);
	
	static void
	createIndexed	(Preferences_Tag, Preferences_Index, CFStringRef, FourCharCode,
					 size_t, Quills::Prefs::Class);
	
	static void
	createRGBColor	(Preferences_Tag, CFStringRef, Quills::Prefs::Class,
					 My_PreferenceDefinition** = nullptr);
	
	static My_PreferenceDefinition*
	findByKeyName	(CFStringRef);
	
	static My_PreferenceDefinition*
	findByTag	(Preferences_Tag);
	
	static Boolean
	isValidKeyName	(CFStringRef);
	
	static void
	registerIndirectKeyName		(CFStringRef);
	
	Preferences_Tag			tag;						//!< tag that describes this setting
	CFRetainRelease			keyName;					//!< key used to store this in XML or CFPreferences
	FourCharCode			keyValueType;				//!< property list type of key (e.g. kPreferences_DataTypeCFArrayRef)
	size_t					nonDictionaryValueSize;		//!< bytes required for data buffers that read/write this value
	Quills::Prefs::Class	preferenceClass;			//!< the class of context that this tag is generally used in
	
protected:
	My_PreferenceDefinition		(Preferences_Tag, CFStringRef, FourCharCode, size_t, Quills::Prefs::Class);

private:
	typedef std::map< Preferences_Tag, class My_PreferenceDefinition* >		DefinitionPtrByTag;
	typedef unordered_map_namespace::unordered_map
			<
				CFStringRef,						// key type - name string
				class My_PreferenceDefinition*,		// value type - preference setting data pointer
				returnCFStringHashCode,				// hash code generator
				isCFStringPairEqual					// key comparator
			>	DefinitionPtrByKeyName;
	typedef unordered_set_namespace::unordered_set
			<
				CFStringRef,						// value type - name string
				returnCFStringHashCode,				// hash code generator
				isCFStringPairEqual					// value comparator
			>	KeyNameSet;
	
	static DefinitionPtrByKeyName	_definitionsByKeyName;
	static DefinitionPtrByTag		_definitionsByTag;
	static KeyNameSet				_indirectKeyNames;
};
My_PreferenceDefinition::DefinitionPtrByKeyName		My_PreferenceDefinition::_definitionsByKeyName;
My_PreferenceDefinition::DefinitionPtrByTag			My_PreferenceDefinition::_definitionsByTag;
My_PreferenceDefinition::KeyNameSet					My_PreferenceDefinition::_indirectKeyNames;

/*!
A wrapper for a list of keys, that can be used externally to
refer indefinitely to a particular set of preference tags.

This is commonly used to take a “set”, so that it is possible
to detect if any keys have been removed from a set of
preferences over time.
*/
class My_TagSet
{
public:
	My_TagSet	(My_ContextInterfacePtr);
	My_TagSet	(std::vector< Preferences_Tag > const&);
	
	Preferences_TagSetRef	selfRef;		//!< convenient, redundant self-reference
	CFRetainRelease			contextKeys;	//!< CFMutableArrayRef; the keys for which preferences are defined

	static void
	extendKeyListWithKeyList	(CFMutableArrayRef, CFArrayRef);

	static void
	extendKeyListWithTags		(CFMutableArrayRef, std::vector< Preferences_Tag > const&);

protected:
	CFMutableArrayRef
	returnNewKeyListForTags		(std::vector< Preferences_Tag > const&);
};
typedef My_TagSet const*	My_TagSetConstPtr;
typedef My_TagSet*			My_TagSetPtr;

typedef MemoryBlockPtrLocker< Preferences_TagSetRef, My_TagSet >	My_TagSetPtrLocker;
typedef LockAcquireRelease< Preferences_TagSetRef, My_TagSet >		My_TagSetAutoLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Preferences_Result		assertInitialized						();
void					changeNotify							(Preferences_Change, Preferences_ContextRef = nullptr,
																 Boolean = false);
Preferences_Result		contextGetData							(My_ContextInterfacePtr, Quills::Prefs::Class, Preferences_Tag,
																 size_t, void*);
Boolean					convertCFArrayToCGFloatRGBColor			(CFArrayRef, CGFloatRGBColor*);
Boolean					convertCFArrayToHIRect					(CFArrayRef, Preferences_TopLeftCGRect&);
Boolean					convertCGFloatRGBColorToCFArray			(CGFloatRGBColor const*, CFArrayRef&);
Boolean					convertHIRectToCFArray					(Preferences_TopLeftCGRect const&, CFArrayRef&);
Preferences_Result		copyClassDomainCFArray					(Quills::Prefs::Class, CFArrayRef&);
CFDictionaryRef			copyDefaultPrefDictionary				();
CFStringRef				copyDomainUserSpecifiedName				(CFStringRef);
CFStringRef				copyUserSpecifiedName					(CFDataRef, CFStringRef);
Preferences_Result		createAllPreferencesContextsFromDisk	();
CFStringRef				createKeyAtIndex						(CFStringRef, UInt32);
CFIndex					findDomainIndexInArray					(CFArrayRef, CFStringRef);
Boolean					getDefaultContext						(Quills::Prefs::Class, My_ContextInterfacePtr&);
Boolean					getFactoryDefaultsContext				(My_ContextInterfacePtr&);
Preferences_Result		getFormatPreference						(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getGeneralPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getListOfContexts						(Quills::Prefs::Class, My_FavoriteContextList const*&);
Preferences_Result		getMacroPreference						(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getMutableListOfContexts				(Quills::Prefs::Class, My_FavoriteContextList*&);
Boolean					getNamedContext							(Quills::Prefs::Class, CFStringRef,
																 My_ContextFavoritePtr&);
Preferences_Result		getPreferenceDataInfo					(Preferences_Tag, CFStringRef&,
																 FourCharCode&, size_t&, Quills::Prefs::Class&);
Preferences_Result		getSessionPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getTerminalPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getTranslationPreference				(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getWorkspacePreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					mergeInDefaultPreferences				();
Preferences_Result		overwriteClassDomainCFArray				(Quills::Prefs::Class, CFArrayRef);
void					readApplicationArrayPreference			(CFStringRef, CFArrayRef&);
Boolean					readPreferencesDictionary				(CFDictionaryRef, Boolean);
Boolean					readPreferencesDictionaryInContext		(My_ContextInterfacePtr, CFDictionaryRef, Boolean,
																 Quills::Prefs::Class*, CFStringRef*);
void					setApplicationPreference				(CFStringRef, CFPropertyListRef);
Preferences_Result		setFormatPreference						(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setGeneralPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setMacroPreference						(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setSessionPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setTerminalPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setTranslationPreference				(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setWorkspacePreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
CFStringRef				virtualKeyCreateName					(UInt16);
Boolean					virtualKeyParseName						(CFStringRef, UInt16&, Boolean&);
Boolean					writePreferencesDictionaryFromContext	(My_ContextInterfacePtr, CFMutableDictionaryRef,
																 Boolean, Boolean);
Boolean					unitTest000_Begin						();
Boolean					unitTest001_Begin						();
Boolean					unitTest002_Begin						();
Boolean					unitTest003_Begin						();

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_Ref			gPreferenceEventListenerModel = nullptr;
Boolean						gInitializing = false;
Boolean						gInitialized = false;
My_ContextPtrLocker&		gMyContextPtrLocks ()	{ static My_ContextPtrLocker x; return x; }
My_ContextReferenceLocker&	gMyContextRefLocks ()	{ static My_ContextReferenceLocker x; return x; }
My_ContextReferenceTracker&	gMyContextValidRefs ()	{ static My_ContextReferenceTracker x; return x; }
My_ContextInterface&		gFactoryDefaultsContext ()	{ static My_ContextCFDictionary x(Quills::Prefs::_FACTORY_DEFAULTS, copyDefaultPrefDictionary()); return x; }
My_ContextInterface&		gGeneralDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::GENERAL); return x; }
My_ContextInterface&		gAutoSaveDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::_RESTORE_AT_LAUNCH); return x; }
My_FavoriteContextList&		gAutoSaveNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gFormatDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::FORMAT); return x; }
My_FavoriteContextList&		gFormatNamedContexts ()		{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gMacroSetDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::MACRO_SET); return x; }
My_FavoriteContextList&		gMacroSetNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gSessionDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::SESSION); return x; }
My_FavoriteContextList&		gSessionNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gTerminalDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::TERMINAL); return x; }
My_FavoriteContextList&		gTerminalNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gTranslationDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::TRANSLATION); return x; }
My_FavoriteContextList&		gTranslationNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gWorkspaceDefaultContext ()	{ static My_ContextDefault x(Quills::Prefs::WORKSPACE); return x; }
My_FavoriteContextList&		gWorkspaceNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_TagSetPtrLocker&			gMyTagSetPtrLocks ()	{ static My_TagSetPtrLocker x; return x; }

} // anonymous namespace

#pragma mark Functors
namespace {

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
	: _name(inName, CFRetainRelease::kNotYetRetained) {}
	
	bool
	operator ()	(My_ContextInterfacePtr		inContextPtr)
	{
		return (_name.exists() && (nullptr != inContextPtr->returnName()) &&
				(kCFCompareEqualTo == CFStringCompare
										(inContextPtr->returnName(), _name.returnCFStringRef(), 0/* options */)));
	}

protected:

private:
	CFRetainRelease		_name;
};

} // anonymous namespace


#pragma mark Public Methods

/*!
Initializes this module.  Factory default values are copied
for any important user preferences that have not been set yet,
and context structures are constructed to represent all user
collections read from disk.

WARNING:	This is called automatically as needed; therefore,
			its implementation should not invoke any Preferences
			module routine that would loop back into this code.

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
	
	
	// set a flag to detect the state of incomplete initialization
	gInitializing = true;
	
	// IMPORTANT: The Python front-end will perform a preemptive check of
	//            the "prefs-version" key and automatically run a program
	//            to upgrade user preferences if they are out of date.
	//            That occurs before Cocoa is even initialized, before
	//            anything attempts to read the application bundle’s
	//            current preferences; so all references to settings at
	//            this point can be assumed to be up-to-date.
	
	// Create definitions for all possible settings; these are used for
	// efficient access later, and to guarantee no duplication.
	// IMPORTANT: The data types used here are documented in Preferences.h,
	//            and relied upon in other methods.  They are also used in
	//            the PrefsConverter.  Check for consistency!
	// WARNING: The “preferences class” chosen for a setting should match
	//          the type of panel that displays it to the user.  The
	//          inheritance setting will only be correctly displayed in
	//          the Default case if the default context for the setting’s
	//          class matches the context currently being edited by a panel.
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("prefs-version"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("auto-save"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("favorite-formats"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("favorite-macro-sets"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("favorite-sessions"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("favorite-terminals"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("favorite-translations"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("favorite-workspaces"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("macro-order")); // for future use
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("terminal-when-cursor-near-right-margin")); // for future use
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("name")); // for creating default collections
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("name-string")); // for creating default collections
	My_PreferenceDefinition::createFlag(kPreferences_TagArrangeWindowsFullScreen,
										CFSTR("window-terminal-full-screen"), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::createFlag(kPreferences_TagArrangeWindowsUsingTabs,
										CFSTR("terminal-use-tabs"), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedFormatFavoriteLightMode,
									CFSTR("format-favorite"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedFormatFavoriteDarkMode,
									CFSTR("format-favorite-dark"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedMacroSetFavorite,
									CFSTR("macro-set-favorite"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedTerminalFavorite,
									CFSTR("terminal-favorite"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagAssociatedTranslationFavorite,
									CFSTR("translation-favorite"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagBackgroundNewDataHandler,
									CFSTR("data-receive-when-in-background"), kPreferences_DataTypeCFStringRef,
									sizeof(Session_Watch), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagBackupFontName,
									CFSTR("terminal-backup-font-family"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::TRANSLATION);
	My_PreferenceDefinition::create(kPreferences_TagBellSound,
									CFSTR("terminal-when-bell-sound-basename"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagCaptureAutoStart,
										CFSTR("terminal-capture-auto-start"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagCaptureFileDirectoryURL,
									CFSTR("terminal-capture-directory-bookmark"), kPreferences_DataTypeCFDataRef,
									sizeof(Preferences_URLInfo), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagCaptureFileName,
									CFSTR("terminal-capture-file-name-string"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagCaptureFileNameAllowsSubstitutions,
										CFSTR("terminal-capture-file-name-is-generated"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagCaptureFileLineEndings,
									CFSTR("terminal-capture-file-line-endings"), kPreferences_DataTypeCFStringRef,
									sizeof(Session_LineEnding), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagCommandLine,
									CFSTR("command-line-token-strings"), kPreferences_DataTypeCFArrayRef,
									sizeof(CFArrayRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagCopySelectedText,
										CFSTR("terminal-auto-copy-on-select"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagCopyTableThreshold,
									CFSTR("spaces-per-tab"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagCursorBlinks,
										CFSTR("terminal-cursor-blinking"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagCursorMovesPriorToDrops,
										CFSTR("terminal-cursor-auto-move-on-drop"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagDataReceiveDoNotStripHighBit,
										CFSTR("data-receive-do-not-strip-high-bit"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagDataReadBufferSize,
									CFSTR("data-receive-buffer-size-bytes"), kPreferences_DataTypeCFNumberRef,
									sizeof(SInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagDontAutoClose,
										CFSTR("no-auto-close"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagDontAutoNewOnApplicationReopen,
										CFSTR("no-auto-new"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagDontDimBackgroundScreens,
										CFSTR("terminal-no-dim-on-deactivate"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagEmacsMetaKey,
									CFSTR("key-map-emacs-meta"), kPreferences_DataTypeCFStringRef,
									sizeof(UInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagFadeBackgroundWindows,
										CFSTR("terminal-fade-in-background"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagFadeAlpha,
									CFSTR("terminal-fade-alpha"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createFlag(kPreferences_TagFocusFollowsMouse,
										CFSTR("terminal-focus-follows-mouse"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagFontCharacterWidthMultiplier,
									CFSTR("terminal-font-width-multiplier"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagFontName,
									CFSTR("terminal-font-family"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagFontSize,
									CFSTR("terminal-font-size-points"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float64), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagFunctionKeyLayout,
									CFSTR("key-map-function-keys"), kPreferences_DataTypeCFStringRef,
									sizeof(Session_FunctionKeyLayout), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagHeadersCollapsed,
										CFSTR("window-terminal-toolbar-invisible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagIdleAfterInactivityHandler,
									CFSTR("data-receive-when-idle"), kPreferences_DataTypeCFStringRef,
									sizeof(Session_Watch), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagIdleAfterInactivityInSeconds,
									CFSTR("data-receive-idle-seconds"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroAction, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-action"), kPreferences_DataTypeCFStringRef,
											sizeof(UInt32), Quills::Prefs::MACRO_SET);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroContents, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-contents-string"), kPreferences_DataTypeCFStringRef,
											sizeof(CFStringRef), Quills::Prefs::MACRO_SET);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroKey, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-key"), kPreferences_DataTypeCFStringRef,
											sizeof(UInt32), Quills::Prefs::MACRO_SET);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroKeyModifiers, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-modifiers"), kPreferences_DataTypeCFArrayRef,
											sizeof(UInt32), Quills::Prefs::MACRO_SET);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedMacroName, kMacroManager_MaximumMacroSetSize,
											CFSTR("macro-%02u-name-string"), kPreferences_DataTypeCFStringRef,
											sizeof(CFStringRef), Quills::Prefs::MACRO_SET);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedWindowCommandType, kPreferences_MaximumWorkspaceSize,
											CFSTR("window-%02u-session-built-in"), kPreferences_DataTypeCFStringRef/* "shell", "dialog", "default" */,
											sizeof(UInt32), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedWindowFrameBounds, kPreferences_MaximumWorkspaceSize,
											CFSTR("window-%02u-frame-bounds-pixels"), kPreferences_DataTypeCFArrayRef,
											sizeof(Preferences_TopLeftCGRect), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedWindowScreenBounds, kPreferences_MaximumWorkspaceSize,
											CFSTR("window-%02u-screen-bounds-pixels"), kPreferences_DataTypeCFArrayRef,
											sizeof(Preferences_TopLeftCGRect), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedWindowSessionFavorite, kPreferences_MaximumWorkspaceSize,
											CFSTR("window-%02u-session-favorite"), kPreferences_DataTypeCFStringRef,
											sizeof(CFStringRef), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::createIndexed(kPreferences_TagIndexedWindowTitle, kPreferences_MaximumWorkspaceSize,
											CFSTR("window-%02u-name-string"), kPreferences_DataTypeCFStringRef,
											sizeof(CFStringRef), Quills::Prefs::WORKSPACE);
	My_PreferenceDefinition::createFlag(kPreferences_TagITermGraphicsEnabled,
										CFSTR("terminal-emulator-iterm-enable-graphics"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagKeepAlivePeriodInMinutes,
									CFSTR("data-send-keepalive-period-minutes"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagKeyInterruptProcess,
									CFSTR("command-key-interrupt-process"), kPreferences_DataTypeCFStringRef,
									sizeof(char), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagKeyResumeOutput,
									CFSTR("command-key-resume-output"), kPreferences_DataTypeCFStringRef,
									sizeof(char), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagKeySuspendOutput,
									CFSTR("command-key-suspend-output"), kPreferences_DataTypeCFStringRef,
									sizeof(char), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskAllowsForceQuit,
										CFSTR("kiosk-force-quit-enabled"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskShowsMenuBar,
										CFSTR("kiosk-menu-bar-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskShowsScrollBar,
										CFSTR("kiosk-scroll-bar-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagKioskShowsWindowFrame,
										CFSTR("kiosk-window-frame-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagLineModeEnabled,
										CFSTR("line-mode-enabled"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagLocalEchoEnabled,
										CFSTR("data-send-local-echo-enabled"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagMapArrowsForEmacs,
									CFSTR("command-key-emacs-move-down"), kPreferences_DataTypeCFStringRef,
									sizeof(Boolean), Quills::Prefs::SESSION);
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-emacs-move-up"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-emacs-move-left"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-emacs-move-right"));
	My_PreferenceDefinition::create(kPreferences_TagMapBackquote,
									CFSTR("key-map-backquote"), kPreferences_DataTypeCFStringRef/* keystroke string, e.g. blank "" or escape "\e" */,
									sizeof(Boolean), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagMapDeleteToBackspace,
									CFSTR("key-map-delete"), kPreferences_DataTypeCFStringRef,
									sizeof(Boolean), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagMapKeypadTopRowForVT220,
									CFSTR("command-key-vt220-pf1")/* TEMPORARY - one of several key names used */, kPreferences_DataTypeCFStringRef,
									sizeof(Boolean), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-vt220-pf2"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-vt220-pf3"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-vt220-pf4"));
	My_PreferenceDefinition::create(kPreferences_TagNewCommandShortcutEffect,
									CFSTR("new-means"), kPreferences_DataTypeCFStringRef/* "shell", "dialog", "default" */,
									sizeof(UInt32), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagNewLineMapping,
									CFSTR("key-map-new-line"), kPreferences_DataTypeCFStringRef,
									sizeof(UInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagNoAnimations,
										CFSTR("no-animations"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagNoPasteWarning,
										CFSTR("data-send-paste-no-warning"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagNotification,
									CFSTR("when-alert-in-background"), kPreferences_DataTypeCFStringRef/* "alert", "animate", "badge", "ignore" */,
									sizeof(UInt16), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagNotifyOfBeeps,
									CFSTR("terminal-when-bell-in-background"), kPreferences_DataTypeCFStringRef/* "notify", "ignore" */,
									sizeof(Boolean), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagPageKeysControlLocalTerminal,
									CFSTR("command-key-terminal-end")/* TEMPORARY - one of several key names used */, kPreferences_DataTypeCFStringRef,
									sizeof(Boolean), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagPasteAllowBracketedMode,
										CFSTR("data-send-paste-allow-bracketed-mode"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagPasteNewLineDelay,
									CFSTR("data-send-paste-line-delay-milliseconds"), kPreferences_DataTypeCFNumberRef,
									sizeof(Preferences_TimeInterval), Quills::Prefs::SESSION);
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-terminal-home"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-terminal-page-up"));
	My_PreferenceDefinition::registerIndirectKeyName(CFSTR("command-key-terminal-page-down"));
	My_PreferenceDefinition::createFlag(kPreferences_TagPureInverse,
										CFSTR("terminal-inverse-selections"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagRandomTerminalFormats,
										CFSTR("terminal-format-random"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagScrollDelay,
									CFSTR("terminal-scroll-delay-milliseconds"), kPreferences_DataTypeCFNumberRef,
									sizeof(Preferences_TimeInterval), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagServerHost,
									CFSTR("server-host"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagServerPort,
									CFSTR("server-port"), kPreferences_DataTypeCFNumberRef,
									sizeof(SInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagServerProtocol,
									CFSTR("server-protocol"), kPreferences_DataTypeCFStringRef,
									sizeof(UInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagServerUserID,
									CFSTR("server-user-id"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagSixelGraphicsEnabled,
										CFSTR("terminal-emulator-sixel-enable-graphics"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTektronixMode,
									CFSTR("tek-mode"), kPreferences_DataTypeCFStringRef,
									sizeof(UInt16), Quills::Prefs::SESSION);
	My_PreferenceDefinition::createFlag(kPreferences_TagTektronixPAGEClearsScreen,
										CFSTR("tek-page-clears-screen"), Quills::Prefs::SESSION);
	My_PreferenceDefinition::create(kPreferences_TagTerminalAnswerBackMessage,
									CFSTR("terminal-emulator-answerback"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagTerminalClearSavesLines,
										CFSTR("terminal-clear-saves-lines"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlack,
											CFSTR("terminal-color-ansi-black-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIRed,
											CFSTR("terminal-color-ansi-red-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIGreen,
											CFSTR("terminal-color-ansi-green-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIYellow,
											CFSTR("terminal-color-ansi-yellow-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlue,
											CFSTR("terminal-color-ansi-blue-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIMagenta,
											CFSTR("terminal-color-ansi-magenta-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSICyan,
											CFSTR("terminal-color-ansi-cyan-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIWhite,
											CFSTR("terminal-color-ansi-white-normal-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlackBold,
											CFSTR("terminal-color-ansi-black-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIRedBold,
											CFSTR("terminal-color-ansi-red-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIGreenBold,
											CFSTR("terminal-color-ansi-green-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIYellowBold,
											CFSTR("terminal-color-ansi-yellow-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIBlueBold,
											CFSTR("terminal-color-ansi-blue-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIMagentaBold,
											CFSTR("terminal-color-ansi-magenta-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSICyanBold,
											CFSTR("terminal-color-ansi-cyan-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorANSIWhiteBold,
											CFSTR("terminal-color-ansi-white-bold-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBlinkingForeground,
											CFSTR("terminal-color-blinking-foreground-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBlinkingBackground,
											CFSTR("terminal-color-blinking-background-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBoldForeground,
											CFSTR("terminal-color-bold-foreground-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorBoldBackground,
											CFSTR("terminal-color-bold-background-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createFlag(kPreferences_TagAutoSetCursorColor,
										CFSTR("terminal-color-cursor-auto"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorCursorBackground,
											CFSTR("terminal-color-cursor-background-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorMatteBackground,
											CFSTR("terminal-color-matte-background-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorNormalForeground,
											CFSTR("terminal-color-normal-foreground-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createRGBColor(kPreferences_TagTerminalColorNormalBackground,
											CFSTR("terminal-color-normal-background-rgb"), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalCursorType,
									CFSTR("terminal-cursor-shape"),
									kPreferences_DataTypeCFStringRef/* "block", "underline", "thick underline", "vertical bar", "thick vertical bar" */,
									sizeof(Terminal_CursorType), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalEmulatorType,
									CFSTR("terminal-emulator-type"), kPreferences_DataTypeCFStringRef,
									sizeof(Emulation_FullType), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalImageNormalBackground,
									CFSTR("terminal-image-normal-background-url"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::createFlag(kPreferences_TagTerminalLineWrap,
										CFSTR("terminal-line-wrap"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginLeft,
									CFSTR("terminal-margin-left-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginRight,
									CFSTR("terminal-margin-right-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginTop,
									CFSTR("terminal-margin-top-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMarginBottom,
									CFSTR("terminal-margin-bottom-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalMousePointerColor,
									CFSTR("terminal-mouse-pointer-color"),
									kPreferences_DataTypeCFStringRef/* "red", "black", "white" */,
									sizeof(TerminalView_MousePointerColor), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingLeft,
									CFSTR("terminal-padding-left-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingRight,
									CFSTR("terminal-padding-right-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingTop,
									CFSTR("terminal-padding-top-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalPaddingBottom,
									CFSTR("terminal-padding-bottom-em"), kPreferences_DataTypeCFNumberRef,
									sizeof(Float32), Quills::Prefs::FORMAT);
	My_PreferenceDefinition::create(kPreferences_TagTerminalResizeAffectsFontSize,
									CFSTR("terminal-resize-affects"), kPreferences_DataTypeCFStringRef/* "screen" or "font" */,
									sizeof(Boolean), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenColumns,
									CFSTR("terminal-screen-dimensions-columns"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenRows,
									CFSTR("terminal-screen-dimensions-rows"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenScrollbackRows,
									CFSTR("terminal-scrollback-size-lines"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt32), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalScreenScrollbackType,
									CFSTR("terminal-scrollback-type"), kPreferences_DataTypeCFStringRef,
									sizeof(UInt16), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagTerminalShowMarginAtColumn,
									CFSTR("terminal-show-margin-at-column"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagTextEncodingIANAName,
									CFSTR("terminal-text-encoding-name"), kPreferences_DataTypeCFStringRef,
									sizeof(CFStringRef), Quills::Prefs::TRANSLATION);
	My_PreferenceDefinition::create(kPreferences_TagTextEncodingID,
									CFSTR("terminal-text-encoding-id"), kPreferences_DataTypeCFNumberRef,
									sizeof(CFStringEncoding), Quills::Prefs::TRANSLATION);
	My_PreferenceDefinition::create(kPreferences_TagVisualBell,
									CFSTR("terminal-when-bell"), kPreferences_DataTypeCFStringRef/* "visual" or "audio+visual" */,
									sizeof(Boolean), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasClipboardShowing,
										CFSTR("window-clipboard-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasCommandLineShowing,
										CFSTR("window-commandline-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasControlKeypadShowing,
										CFSTR("window-controlkeys-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasFunctionKeypadShowing,
										CFSTR("window-functionkeys-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasSessionInfoShowing,
										CFSTR("window-sessioninfo-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagWasVT220KeypadShowing,
										CFSTR("window-vt220keys-visible"), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagWindowStackingOrigin,
									CFSTR("window-terminal-position-pixels"), kPreferences_DataTypeCFArrayRef/* 2 CFNumberRefs, pixels from top-left */,
									sizeof(CGPoint), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::create(kPreferences_TagWindowTabPreferredEdge,
									CFSTR("window-terminal-tab-edge"), kPreferences_DataTypeCFStringRef/* "top", "bottom", "left" or "right" */,
									sizeof(CGRectEdge), Quills::Prefs::GENERAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagTerminal24BitColorEnabled,
										CFSTR("terminal-emulator-enable-color-24bit"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagVT100FixLineWrappingBug,
										CFSTR("terminal-emulator-vt100-fix-line-wrapping-bug"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermBackgroundColorEraseEnabled,
										CFSTR("terminal-emulator-xterm-enable-background-color-erase"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTerm256ColorsEnabled,
										CFSTR("terminal-emulator-xterm-enable-color-256"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermColorEnabled,
										CFSTR("terminal-emulator-xterm-enable-color"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermGraphicsEnabled,
										CFSTR("terminal-emulator-xterm-enable-graphics"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::createFlag(kPreferences_TagXTermWindowAlterationEnabled,
										CFSTR("terminal-emulator-xterm-enable-window-alteration-sequences"), Quills::Prefs::TERMINAL);
	My_PreferenceDefinition::create(kPreferences_TagXTermReportedPatchLevel,
									CFSTR("terminal-emulator-xterm-reported-patch-level"), kPreferences_DataTypeCFNumberRef,
									sizeof(UInt16), Quills::Prefs::TERMINAL);
	
	// to ensure that the rest of the application can depend on its
	// known keys being defined, ALWAYS merge in default values for
	// any keys that may not already be defined (by the data on disk,
	// by the Preferences Converter, etc.)
	// NOTE: the Cocoa NSUserDefaults class supports the concept of
	// a registered domain, which *would* avoid the need to actually
	// replicate defaults; instead, the defaults are simply referred
	// to as needed; a better future solution would be to register
	// all defaults instead of copying them into user preferences
	UNUSED_RETURN(Boolean)mergeInDefaultPreferences();
	
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
		gInitializing = false;
	}
	
	// if keypads were open at last Quit, construct them now;
	// otherwise, wait until each one is requested by the user
	// (TEMPORARY; a bit of a hack to do this here...but there
	// is no cleanup function in the Keypads module)
	{
		Boolean		windowIsVisible = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWasControlKeypadShowing,
									sizeof(windowIsVisible), &windowIsVisible))
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			Keypads_SetVisible(kKeypads_WindowTypeControlKeys, true);
		}
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWasFunctionKeypadShowing,
									sizeof(windowIsVisible), &windowIsVisible))
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			Keypads_SetVisible(kKeypads_WindowTypeFunctionKeys, true);
		}
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWasVT220KeypadShowing,
									sizeof(windowIsVisible), &windowIsVisible))
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			Keypads_SetVisible(kKeypads_WindowTypeVT220Keys, true);
		}
	}
	
	// configure the Alert module according to animation settings
	{
		Boolean		noAnimations = false;
		
		
		// determine if animation should occur
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoAnimations,
									sizeof(noAnimations), &noAnimations))
		{
			noAnimations = false; // assume a value, if preference can’t be found
		}
		
		Alert_SetIsAnimationAllowed(false == noAnimations);
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
		// save floating-window visibility preferences implicitly
		// (TEMPORARY; a bit of a hack to do this here...but there
		// is no cleanup function in the Keypads module)
		{
			Preferences_Result	prefsResult = kPreferences_ResultOK;
			Boolean				windowIsVisible = false;
			
			
			windowIsVisible = Keypads_IsVisible(kKeypads_WindowTypeControlKeys);
			prefsResult = Preferences_SetData(kPreferences_TagWasControlKeypadShowing,
												sizeof(windowIsVisible), &windowIsVisible);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteValue, "failed to store visibility of control keys palette, error", prefsResult);
			}
			windowIsVisible = Keypads_IsVisible(kKeypads_WindowTypeFunctionKeys);
			prefsResult = Preferences_SetData(kPreferences_TagWasFunctionKeypadShowing,
												sizeof(windowIsVisible), &windowIsVisible);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteValue, "failed to store visibility of function keys palette, error", prefsResult);
			}
			windowIsVisible = Keypads_IsVisible(kKeypads_WindowTypeVT220Keys);
			prefsResult = Preferences_SetData(kPreferences_TagWasVT220KeypadShowing,
												sizeof(windowIsVisible), &windowIsVisible);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteValue, "failed to store visibility of VT220 keys palette, error", prefsResult);
			}
			
			if (false == CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication))
			{
				Console_Warning(Console_WriteLine, "failed to save keypad visibility settings");
			}
		}
		
		// dispose of the listener model
		ListenerModel_Dispose(&gPreferenceEventListenerModel);
		
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

WARNING:		Although "inRestrictedSetOrNull" is optional, it is
			strongly recommended because the source context
			might contain a large number of irrelevant settings
			(especially if it is a Default context).

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
								 Boolean					inForceDetach,
								 Preferences_TagSetRef		inRestrictedSetOrNull)
{
	My_ContextAutoLocker		basePtr(gMyContextPtrLocks(), inBaseContext);
	Quills::Prefs::Class		baseClass = Quills::Prefs::GENERAL;
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
		Preferences_Result	prefsResult = Preferences_ContextCopy(inBaseContext, result, inRestrictedSetOrNull);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			// failed to copy!
			Preferences_ReleaseContext(&result);
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
Preferences_NewContext	(Quills::Prefs::Class	inClass)
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
Returns a reference to the context for the given named set
of preferences, or "nullptr" if any problems occur.  If no
matching data is saved already, the resulting context will
refer to NEW data; call Preferences_IsContextNameInUse() to
have control over this data creation.

As a special exception, if the Default name is chosen then
the given class’ default context is returned, as if you had
called Preferences_GetDefaultContext().  Unlike that call,
this function retains the default context so that it has to
be released like any other context returned by this function.

Contexts from Favorites focus on one named dictionary in a
particular class (to the user, this is a collection).  These
changes are mirrored to disk in the system’s standard ways.

The reference is automatically retained but you need to call
Preferences_ReleaseContext() when finished.  Note that the
reference is internally held as long as the corresponding
data exists on disk so “releasing” your reference does not
endanger the saved data (to actually destroy the saved data,
see Preferences_ContextDeleteFromFavorites()).

If you need a standalone copy based on an existing user
collection, first use this routine to acquire the collection
you want, and then use Preferences_NewCloneContext().  Keep
in mind that Preferences_NewCloneContext() allows two kinds
of copies: in-memory only (temporary), or named copies that
are synchronized to disk.

NOTE:	In most cases, provide only the class and context
		name.  The domain name (last parameter) should be
		"nullptr" unless it’s internal initialization code.

IMPORTANT:	Contexts can be renamed so do not hold onto
			the name indefinitely.  (Unfortunately, some
			preference values have historically done this;
			they store only a name and therefore “forget”
			what they’re referring to if the user renames
			the target collection.)  Callbacks can be used
			to detect renames, at which point references
			can be updated.

(3.1)
*/
Preferences_ContextRef
Preferences_NewContextFromFavorites		(Quills::Prefs::Class	inClass,
										 CFStringRef			inNameOrNullToAutoGenerateUniqueName,
										 CFStringRef			inDomainNameIfInitializingOrNull)
{
	Preferences_ContextRef		result = nullptr;
	
	
	// the name may be explicitly nullptr, but otherwise it must not be empty
	if ((nullptr == inNameOrNullToAutoGenerateUniqueName) || (CFStringGetLength(inNameOrNullToAutoGenerateUniqueName) > 0))
	{
		CFRetainRelease		defaultName(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowDefaultFavoriteName),
										CFRetainRelease::kAlreadyRetained);
		CFStringRef			contextName = inNameOrNullToAutoGenerateUniqueName;
		Boolean				releaseName = false;
		Boolean				badInput = false;
		
		
		// when nullptr is given, scan the list of all contexts for the
		// given class and find a unique name
		if (nullptr == inNameOrNullToAutoGenerateUniqueName)
		{
			Preferences_Result		prefsResult = kPreferences_ResultOK;
			
			
			prefsResult = Preferences_CreateUniqueContextName
							(inClass, contextName/* new name */);
			if (kPreferences_ResultUnknownTagOrClass == prefsResult)
			{
				// caller has provided a class that does not support
				// named collections!
				Console_Warning(Console_WriteValue, "Preferences_NewContextFromFavorites() cannot be called with class", STATIC_CAST(inClass, SInt32));
				badInput = true;
			}
			else
			{
				assert(kPreferences_ResultOK == prefsResult);
				releaseName = true;
			}
		}
		
		// see if the name matches the Default context (special case);
		// this is sometimes useful if the Default name happens to
		// appear alongside other named contexts, such as in a menu
		if (kCFCompareEqualTo == CFStringCompare(defaultName.returnCFStringRef(), contextName, 0/* options */))
		{
			My_ContextInterfacePtr	defaultContextPtr = nullptr;
			Boolean					getDefaultOK = getDefaultContext(inClass, defaultContextPtr);
			
			
			if (false == getDefaultOK)
			{
				Console_Warning(Console_WriteLine, "failed to find default context when given Default name");
			}
			else
			{
				// usually a Default context should not be found this way
				// but it may be useful (e.g. if found in a menu of choices);
				// unlike Preferences_GetDefaultContext(), this function is
				// meant to return a “new” context so the result is retained
				//Console_Warning(Console_WriteValue, "returning Default context by name; preferences class", inClass);
				result = REINTERPRET_CAST(defaultContextPtr, Preferences_ContextRef);
				Preferences_RetainContext(result);
			}
		}
		
		if ((nullptr == result) && (false == badInput))
		{
			try
			{
				My_ContextFavoritePtr	contextPtr = nullptr;
				
				
				if (false == getNamedContext(inClass, contextName, contextPtr))
				{
					My_FavoriteContextList*		listPtr = nullptr;
					
					
					if (getMutableListOfContexts(inClass, listPtr))
					{
						My_ContextFavoritePtr	newDictionary = new My_ContextFavorite
																	(inClass, contextName,
																		inDomainNameIfInitializingOrNull);
						
						
						// the act of placing a context in this list establishes a special
						// type of extra reference that Preferences_ReleaseContext() will
						// honor, and Preferences_ContextDeleteFromFavorites() is the only
						// expected mechanism for deleting items from the list later (in
						// response to a user request!)
						contextPtr = newDictionary;
						listPtr->push_back(newDictionary);
						assert(true == getNamedContext(inClass, contextName, contextPtr));
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
		}
		
		if (releaseName)
		{
			CFRelease(contextName), contextName = nullptr;
		}
	}
	
	return result;
}// NewContextFromFavorites


/*!
Creates an empty tag set that can be extended by multiple
calls to Preferences_TagSetMerge().

Although the set is initially empty, as a slight optimization
you can suggest the size that it will ultimately require so
that the first memory allocation is enough.

(4.1)
*/
Preferences_TagSetRef
Preferences_NewTagSet	(size_t		inSuggestedSize)
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	emptyList(inSuggestedSize);
	
	
	try
	{
		result = REINTERPRET_CAST(new My_TagSet(emptyList), Preferences_TagSetRef);
	}
	catch (std::exception const&	inException)
	{
		Console_WriteLine(inException.what());
		result = nullptr;
	}
	return result;
}// NewTagSet (UInt16)


/*!
Creates an object representing the preferences tags that are
currently defined in the specified context.

This can be used with APIs such as Preferences_ContextCopy(),
to restrict their scope to the tags in the set.

(4.0)
*/
Preferences_TagSetRef
Preferences_NewTagSet	(Preferences_ContextRef		inContext)
{
	Preferences_TagSetRef	result = nullptr;
	
	
	try
	{
		My_ContextAutoLocker	contextPtr(gMyContextPtrLocks(), inContext);
		
		
		result = REINTERPRET_CAST(new My_TagSet(contextPtr), Preferences_TagSetRef);
	}
	catch (std::exception const&	inException)
	{
		Console_WriteLine(inException.what());
		result = nullptr;
	}
	return result;
}// NewTagSet (Preferences_ContextRef)


/*!
Creates an object representing the specified preferences tags.

This can be used with APIs such as Preferences_ContextCopy(),
to restrict their scope to the tags in the set.

(4.0)
*/
Preferences_TagSetRef
Preferences_NewTagSet	(std::vector< Preferences_Tag > const&		inTags)
{
	Preferences_TagSetRef	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_TagSet(inTags), Preferences_TagSetRef);
	}
	catch (std::exception const&	inException)
	{
		Console_WriteLine(inException.what());
		result = nullptr;
	}
	return result;
}// NewTagSet (Preferences_Tag...)


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
Releases one lock on the specified context and deletes the context
*if* no other locks remain.  Your copy of the reference is set to
nullptr either way.

Note that a context created by Preferences_NewContextFromFavorites()
will have an implicit extra reference that can only be removed by a
call to Preferences_ContextDeleteFromFavorites(), and doing so will
delete the ON DISK version of the data.  (This includes contexts
duplicated by Preferences_NewCloneContext(), if that routine was not
told to detach the duplicate from disk.)

(3.1)
*/
void
Preferences_ReleaseContext	(Preferences_ContextRef*	inoutRefPtr)
{
	if ((nullptr != inoutRefPtr) && (nullptr != *inoutRefPtr))
	{
		gMyContextRefLocks().releaseLock(*inoutRefPtr);
		unless (gMyContextRefLocks().isLocked(*inoutRefPtr))
		{
			// the context has no external locks; in most situations this
			// is sufficient to delete the data but for preferences contexts
			// it is also necessary to ensure that no attachment to on-disk
			// data remains
			My_ContextInterfacePtr			ptr = REINTERPRET_CAST(*inoutRefPtr, My_ContextInterfacePtr);
			My_FavoriteContextList const*	listPtr = nullptr;
			Boolean							shouldDelete = false;
			
			
			if (getListOfContexts(ptr->returnClass(), listPtr))
			{
				// only allow the in-memory version to be destroyed if there
				// is no reference to the context in the Favorites list
				if (listPtr->end() == std::find(listPtr->begin(), listPtr->end(), ptr))
				{
					shouldDelete = true;
				}
				else
				{
					// if the intent is to destroy preferences, the routine
					// Preferences_ContextDeleteFromFavorites() must be called
					// prior to releasing the in-memory context for the last time
				#if 0
					// note that this warning should only be enabled for debugging;
					// it is a valid warning in most situations but it is INCORRECT
					// to warn in any code that is intentionally creating a new
					// set of preferences on disk (such code would retain and
					// release a context, and leave behind data that should not be
					// automatically destroyed)
					Console_Warning(Console_WriteLine, "synchronized-context release aborted because data on disk has not been explicitly destroyed by the user");
				#endif
				}
			}
			else
			{
				shouldDelete = true;
			}
			
			if (shouldDelete)
			{
				// context reference has no locks and the data is not
				// being synchronized on disk
				delete ptr;
			}
		}
		*inoutRefPtr = nullptr;
	}
}// ReleaseContext


/*!
Disposes of the specified set.  Your copy of the reference is
set to "nullptr".

(4.0)
*/
void
Preferences_ReleaseTagSet	(Preferences_TagSetRef*		inoutRefPtr)
{
	if (gMyTagSetPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteLine, "attempt to dispose of locked tag set");
	}
	else
	{
		// clean up
		{
			My_TagSetAutoLocker		ptr(gMyTagSetPtrLocks(), *inoutRefPtr);
			
			
			delete ptr;
		}
		*inoutRefPtr = nullptr;
	}
}// ReleaseTagSet


/*!
Copies all the key-value pairs (or, if "inRestrictedSetOrNull" is
defined, only those keys) from the source to the destination,
regardless of class.

When copying all keys, any other keys defined in the destination
are unchanged and are not removed.

When copying a restricted set of keys, every key in the set is
checked: if it no longer exists in the source, it will be deleted
in the destination, otherwise the value is copied.  No other keys
are checked in the restricted case.  This is very useful when
working with contexts that might be huge supersets, such as
default contexts.

Unlike Preferences_NewCloneContext(), this routine does not
create any new contexts.  It might be used to apply changes in a
temporary context to a more permanent one.

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
							 Preferences_ContextRef		inDestinationContext,
							 Preferences_TagSetRef		inRestrictedSetOrNull)
{
	My_ContextAutoLocker	basePtr(gMyContextPtrLocks(), inBaseContext);
	My_ContextAutoLocker	destPtr(gMyContextPtrLocks(), inDestinationContext);
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if ((nullptr == basePtr) || (nullptr == destPtr)) result = kPreferences_ResultInvalidContextReference;
	else
	{
		My_TagSetAutoLocker		setPtr(gMyTagSetPtrLocks(), inRestrictedSetOrNull);
		CFRetainRelease			sourceKeys(basePtr->returnKeyListCopy(), CFRetainRelease::kAlreadyRetained);
		CFArrayRef				sourceKeysCFArray = (nullptr != setPtr)
														? setPtr->contextKeys.returnCFArrayRef()
														: sourceKeys.returnCFArrayRef();
		
		
		if (nullptr == sourceKeysCFArray) result = kPreferences_ResultOneOrMoreNamesNotAvailable;
		else
		{
			CFIndex const	kNumberOfKeys = CFArrayGetCount(sourceKeysCFArray);
			
			
			for (CFIndex i = 0; i < kNumberOfKeys; ++i)
			{
				CFStringRef const	kKeyCFStringRef = CFUtilities_StringCast
														(CFArrayGetValueAtIndex(sourceKeysCFArray, i));
				Boolean				addValue = true;
				
				
				if (nullptr != inRestrictedSetOrNull)
				{
					//Console_WriteValueCFString("check restricted-set key", kKeyCFStringRef); // debug
					unless (basePtr->exists(kKeyCFStringRef))
					{
						// no longer exists; delete from destination
						destPtr->deleteValue(kKeyCFStringRef);
						addValue = false;
					}
				}
				
				if (addValue)
				{
					CFRetainRelease		keyValueCFType(basePtr->returnValueCopy(kKeyCFStringRef),
														CFRetainRelease::kAlreadyRetained);
					
					
					destPtr->addValue(0/* do not notify */, kKeyCFStringRef, keyValueCFType.returnCFTypeRef());
				}
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
	Quills::Prefs::Class	dataClass = Quills::Prefs::GENERAL;
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
Removes the implicit extra reference to context data that is
created by using Preferences_NewContextFromFavorites(), and
deletes the on-disk version of the preference data.  (This
will become permanent the next time the system is asked to
synchronize user preferences.)

WARNING:	Since this has the side effect of destroying data on
		disk, it is not a “normal” clean-up operation; it
		should only be called when explicitly requested by
		the user.  Currently, this is only possible when the
		minus-sign button in the Preferences window is used.

A preferences context should not be used after its data is
destroyed, otherwise the data may be recreated on disk (in a
domain that has since been forgotten by the application).  If
you want a copy that isn’t synchronized, first use the
Preferences_NewCloneContext() API with appropriate options.

\retval kPreferences_ResultOK
if the persistent store is successfully deleted

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultGenericFailure
if the specified context is not synchronized with Favorites

(4.1)
*/
Preferences_Result
Preferences_ContextDeleteFromFavorites	(Preferences_ContextRef		inContext)
{
	Preferences_Result		result = kPreferences_ResultOK;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		// attempt to destroy the context (this should always work for any
		// context that represents a user Favorite; it will never work for
		// a temporary in-memory-only context, a Default context or other
		// type of context)
		result = ptr->destroy();
		if (kPreferences_ResultOK != result)
		{
			Console_Warning(Console_WriteValue, "attempt to destroy data failed, error", result);
		}
		else
		{
			My_FavoriteContextList*		listPtr = nullptr;
			
			
			// explicitly remove the reference to this data from the internal list
			// of synchronized contexts so the last Preferences_ReleaseContext()
			// call is able to free the context’s memory
			if (getMutableListOfContexts(ptr->returnClass(), listPtr))
			{
				My_ContextInterfacePtr		asPtrValue = &(*ptr); // STL requires more exact type match
				
				
				listPtr->erase(std::remove(listPtr->begin(), listPtr->end(), asPtrValue),
								listPtr->end());
				assert(listPtr->end() == std::find(listPtr->begin(), listPtr->end(), asPtrValue));
			}
			
			// notify listeners (e.g. this is how the Preferences window’s list
			// of contexts will be refreshed)
			changeNotify(kPreferences_ChangeNumberOfContexts);
		}
	}
	return result;
}// ContextDeleteFromFavorites


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
the returned data was read from the defaults.  This is also
always true if "inContext" is identical to the default context
of its class; that is, it always returns true if "inContext"
matches what Preferences_GetDefaultContext() would return for
Preferences_ContextReturnClass(inContext).

\retval kPreferences_ResultOK
if data is acquired without errors; "outActualSizePtr" then
points to the exact number of bytes used by the data

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist, and there was no
request to search defaults as a fallback

\retval kPreferences_ResultNoMoreGeneralContext
if the requested data was not found in the specified context,
there was a request to search defaults, and there was a problem
finding the default context

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
							 Boolean*					outIsDefaultOrNull)
{
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0;
	Boolean					isDefault = false;
	Quills::Prefs::Class	dataClass = Quills::Prefs::GENERAL;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
		Preferences_ContextRef	defaultContext = nullptr;
		Boolean					searchDefaults = inSearchDefaults;
		
		
		// WARNING: This is slightly fragile because it assumes that
		// the caller actually knows the correct class to use for the
		// given tag, when looking up a context that stores defaults.
		// Worse, since all defaults are implicitly stored in the
		// same place, ANY default context can successfully be used
		// to *read* a setting but the code below will only set the
		// "isDefault" flag to the expected value if the *correct*
		// default context (the one for the setting’s class) has been
		// given.  It may be better for the following code to check
		// all known default contexts and set "isDefault" to true if
		// the given context matches ANY default context.  In the
		// meantime, this is fine as long as callers (e.g. preference
		// panels) are never displaying settings that are inconsistent
		// with the class of other settings in the same panel.
		if (kPreferences_ResultOK == Preferences_GetDefaultContext(&defaultContext, dataClass))
		{
			isDefault = (inContext == defaultContext);
		}
		
		if (nullptr != ptr)
		{
			result = contextGetData(ptr, dataClass, inDataPreferenceTag,
									inDataStorageSize, outDataStorage);
			if ((searchDefaults) && (kPreferences_ResultOK == result))
			{
				searchDefaults = false;
			}
		}
		else if (false == searchDefaults)
		{
			result = kPreferences_ResultInvalidContextReference;
		}
		
		if (searchDefaults)
		{
			// not available...try another context
			Preferences_ContextRef		alternateContext = defaultContext;
			
			
			isDefault = true;
			if (nullptr == alternateContext)
			{
				result = kPreferences_ResultNoMoreGeneralContext;
			}
			else
			{
				My_ContextAutoLocker	alternatePtr(gMyContextPtrLocks(), alternateContext);
				
				
				result = contextGetData(alternatePtr, dataClass, inDataPreferenceTag,
										inDataStorageSize, outDataStorage);
			}
			
			if (kPreferences_ResultOK != result)
			{
				// not available...try yet another context
				Preferences_ContextRef		rootContext = nullptr;
				
				
				result = Preferences_GetDefaultContext(&rootContext);
				if (kPreferences_ResultOK != result)
				{
					result = kPreferences_ResultNoMoreGeneralContext;
				}
				else
				{
					My_ContextAutoLocker	rootPtr(gMyContextPtrLocks(), rootContext);
					
					
					result = contextGetData(rootPtr, dataClass, inDataPreferenceTag,
											inDataStorageSize, outDataStorage);
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
Returns true only if the specified context is one that could be
returned by Preferences_GetDefaultContext().

\retval kPreferences_ResultOK
if the context name was found

\retval kPreferences_ResultUnknownName
if the context name was not found

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

(4.1)
*/
Boolean
Preferences_ContextIsDefault	(Preferences_ContextRef		inContext,
								 Quills::Prefs::Class		inClass)
{
	Boolean						result = false;
	Preferences_ContextRef		actualDefaultContext = nullptr;
	Preferences_Result			preferencesResult = Preferences_GetDefaultContext
													(&actualDefaultContext, inClass);
	
	
	if (kPreferences_ResultOK == preferencesResult)
	{
		result = (inContext == actualDefaultContext);
	}
	
	return result;
}// ContextIsDefault


/*!
Returns true only if the specified context is still valid.  An
invalid context was either never valid, or invalidated by having
lost all its retain counts (causing deallocation).

(4.0)
*/
Boolean
Preferences_ContextIsValid	(Preferences_ContextRef		inContext)
{
	Boolean		result = false;
	
	
	if ((nullptr != inContext) && (gMyContextValidRefs().end() != gMyContextValidRefs().find(inContext)))
	{
		result = true;
	}
	return result;
}// ContextIsValid


/*!
Adds key-value data to an existing preferences context using
XML data (presumably read from a file, but technically the
same as Preferences_ContextSaveAsXMLData()).  The data must
represent a root dictionary.

If the data contains name information (such as a "name-string"
key), an attempt is made to rename the context accordingly.
The rename only succeeds if the context is of a type that
allows names (such as user Favorites) and if the new name is
not already in use by another context of the same class.  If
a rename occurs, it sends the usual rename notification.

\retval kPreferences_ResultOK
if the data is valid and the context is successfully modified

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultBadVersionDataNotAvailable
if the data is invalid in some way

(4.0)
*/
Preferences_Result
Preferences_ContextMergeInXMLData	(Preferences_ContextRef		inContext,
									 CFDataRef					inData,
									 Quills::Prefs::Class*		outInferredClassOrNull,
									 CFStringRef*				outInferredNameOrNull)
{
	Preferences_Result		result = kPreferences_ResultBadVersionDataNotAvailable;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr) result = kPreferences_ResultInvalidContextReference;
	else
	{
		CFErrorRef				errorCFObject = nullptr;
		CFPropertyListFormat	actualFormat = kCFPropertyListXMLFormat_v1_0;
		CFPropertyListRef		root = CFPropertyListCreateWithData
										(kCFAllocatorDefault, inData, kCFPropertyListImmutable,
											&actualFormat, &errorCFObject);
		
		
		if (nullptr == root)
		{
			if (nullptr != errorCFObject)
			{
				CFRetainRelease		errorReleaser(errorCFObject, CFRetainRelease::kAlreadyRetained);
				
				
				result = kPreferences_ResultBadVersionDataNotAvailable;
				Console_Warning(Console_WriteValueCFError, "unable to create preferences property list from XML data, error", errorCFObject);
			}
		}
		else
		{
			CFDictionaryRef		rootAsDictionary = CFUtilities_DictionaryCast(root);
			Boolean				readOK = false;
			
			
			assert(CFPropertyListIsValid(root, kCFPropertyListXMLFormat_v1_0));
			// reading the dictionary will also trigger the context rename if name data exists
			readOK = readPreferencesDictionaryInContext(ptr, rootAsDictionary, true/* merge */,
														outInferredClassOrNull, outInferredNameOrNull);
			if (false == readOK)
			{
				result = kPreferences_ResultBadVersionDataNotAvailable;
				Console_Warning(Console_WriteLine, "unable to read dictionary created from XML data");
			}
			else
			{
				// successfully merged
				result = kPreferences_ResultOK;
			}
			CFRelease(root), root = nullptr;
		}
	}
	return result;
}// ContextMergeInXMLData


/*!
Convenience method to invoke Preferences_ContextMergeInXMLData()
using the data in an XML file, as specified by an FSRef.

Returns anything that Preferences_ContextMergeInXMLData() can
return.

(4.0)
*/
Preferences_Result
Preferences_ContextMergeInXMLFileRef	(Preferences_ContextRef		inContext,
										 FSRef const&				inFile,
										 Quills::Prefs::Class*		outInferredClassOrNull,
										 CFStringRef*				outInferredNameOrNull)
{
	Preferences_Result	result = kPreferences_ResultBadVersionDataNotAvailable;
	CFURLRef			fileURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &inFile);
	
	
	if (nullptr == fileURL)
	{
		Console_Warning(Console_WriteLine, "unable to create URL for file");
	}
	else
	{
		result = Preferences_ContextMergeInXMLFileURL(inContext, fileURL, outInferredClassOrNull,
														outInferredNameOrNull);
		CFRelease(fileURL), fileURL = nullptr;
	}
	return result;
}// ContextMergeInXMLFileRef


/*!
Convenience method to invoke Preferences_ContextMergeInXMLData()
using the data in an XML file, as specified by a URL.

Returns anything that Preferences_ContextMergeInXMLData() can
return.

(4.0)
*/
Preferences_Result
Preferences_ContextMergeInXMLFileURL	(Preferences_ContextRef		inContext,
										 CFURLRef					inFileURL,
										 Quills::Prefs::Class*		outInferredClassOrNull,
										 CFStringRef*				outInferredNameOrNull)
{
	Preferences_Result	result = kPreferences_ResultBadVersionDataNotAvailable;
	CFDataRef			xmlData = nullptr;
	SInt32				errorForFileURL = 0;
	Boolean				readOK = false;
	
	
	readOK = CFURLCreateDataAndPropertiesFromResource
				(kCFAllocatorDefault, inFileURL, &xmlData, nullptr/* returned properties dictionary */,
					nullptr/* list of desired properties */, &errorForFileURL);
	if (false == readOK)
	{
		Console_Warning(Console_WriteValue, "unable to read XML data from file, URL error", errorForFileURL);
	}
	else
	{
		result = Preferences_ContextMergeInXMLData(inContext, xmlData, outInferredClassOrNull,
													outInferredNameOrNull);
		CFRelease(xmlData), xmlData = nullptr;
	}
	
	return result;
}// ContextMergeInXMLFileURL


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
through calls like Preferences_CreateContextNameArray();
you may wish to use those routines again to achieve the
correct ordering.

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
		
		
		if (false == getMutableListOfContexts(derivedPtr->returnClass(), listPtr))
		{
			result = kPreferences_ResultGenericFailure;
		}
		else
		{
			auto	toMovedContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
														[=](My_ContextFavoritePtr p) { return (derivedPtr == p); });
			auto	toRefContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
													[=](My_ContextFavoritePtr p) { return (derivedReferencePtr == p); });
			
			
			if ((listPtr->end() == toMovedContextPtr) || (listPtr->end() == toRefContextPtr))
			{
				result = kPreferences_ResultInvalidContextReference;
			}
			else
			{
				listPtr->erase(toMovedContextPtr);
				toRefContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
												[=](My_ContextFavoritePtr p) { return (derivedReferencePtr == p); });
				assert(listPtr->end() != toRefContextPtr);
				if (false == inInsertBefore)
				{
					std::advance(toRefContextPtr, +1);
				}
				listPtr->insert(toRefContextPtr, derivedPtr);
				
				// now change the saved version of the list
				result = derivedPtr->shift(derivedReferencePtr, inInsertBefore);
				
				changeNotify(kPreferences_ChangeNumberOfContexts);
			}
		}
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
		My_ContextFavoritePtr			derivedPtr = STATIC_CAST(&*ptr, My_ContextFavoritePtr);
		My_FavoriteContextList const*	listPtr = nullptr;
		
		
		if (false == getListOfContexts(derivedPtr->returnClass(), listPtr)) result = kPreferences_ResultGenericFailure;
		else
		{
			// find the position of the specified context in the list
			auto	toContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
												[=](My_ContextFavoritePtr p) { return (derivedPtr == p); });
			
			
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
"Quills::Prefs::GENERAL" is returned if there is no way
to tell what the class is.

(4.0)
*/
Quills::Prefs::Class
Preferences_ContextReturnClass		(Preferences_ContextRef		inContext)
{
	Quills::Prefs::Class	result = Quills::Prefs::GENERAL;
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
	}
	return result;
}// ContextSave


/*!
Converts in-memory preferences data model changes to XML data,
suitable for writing to a UTF-8-encoded text file.

Normally only the keys that belong to the source context’s class
are saved and any other settings are ignored.  To force all keys
to be included, set "kPreferences_ExportFlagIncludeAllClasses" in
"inExportOptions" (note that this will make the data dictionary
very large if the source context contains unrelated settings such
as global defaults).  This option is typically suitable only for
debugging, to see every setting that has been made directly.

A context is not required to contain every possible setting from
its class; normally, when data is read back, any missing values
are determined by “inheriting” from defaults at the time of the
read.  If you want to export an exact snapshot however, filling
in inheritance “gaps” by copying values explicitly from defaults,
then set "kPreferences_ExportFlagCopyDefaultsAsNeeded" in the
"inExportOptions" parameter.  This may produce a larger file but
the data will always be imported exactly as it was.

This data can later recreate a context using a method like
Preferences_ContextMergeInXMLData().

Unlike Preferences_ContextSave(), this has no side effects.

You must invoke CFRelease() to free the data when finished.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultGenericFailure
if any other problem prevents data generation

(2016.10)
*/
Preferences_Result
Preferences_ContextSaveAsXMLData	(Preferences_ContextRef		inContext,
									 Preferences_ExportFlags		inExportOptions,
									 CFDataRef&					outData)
{
	Preferences_Result		result = kPreferences_ResultGenericFailure;
	My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
	
	
	if (nullptr == ptr)
	{
		result = kPreferences_ResultInvalidContextReference;
	}
	else
	{
		CFRetainRelease		targetDictionary(CFDictionaryCreateMutable(kCFAllocatorDefault, 0/* capacity or zero for no limit */,
																		&kCFTypeDictionaryKeyCallBacks,
																		&kCFTypeDictionaryValueCallBacks),
												CFRetainRelease::kAlreadyRetained);
		Boolean				currentClassOnly = (0 == (inExportOptions & kPreferences_ExportFlagIncludeAllClasses));
		Boolean				writeOK = false;
		
		
		writeOK = writePreferencesDictionaryFromContext(ptr, targetDictionary.returnCFMutableDictionaryRef(),
														false/* merge */, currentClassOnly);
		if (false == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to read source context into dictionary");
			result = kPreferences_ResultGenericFailure;
		}
		else
		{
			Boolean		writeData = true; // initially...
			
			
			if (inExportOptions & kPreferences_ExportFlagCopyDefaultsAsNeeded)
			{
				// caller has requested that gaps be filled in using defaults;
				// merge in data from the same class wherever it is missing
				My_ContextInterfacePtr		defaultsPtr = nullptr;
				
				
				if (getDefaultContext(Preferences_ContextReturnClass(inContext), defaultsPtr))
				{
					// attempt to read defaults of the same class as well
					// (for conflicting settings, nothing is overwritten)
					writeOK = writePreferencesDictionaryFromContext
								(defaultsPtr, targetDictionary.returnCFMutableDictionaryRef(),
									true/* merge */, currentClassOnly);
					if (false == writeOK)
					{
						Console_Warning(Console_WriteLine, "failed to merge in default values");
						result = kPreferences_ResultGenericFailure;
						writeData = false;
					}
				}
				else
				{
					Console_Warning(Console_WriteLine, "failed to find default context for class");
					result = kPreferences_ResultGenericFailure;
					writeData = false;
				}
			}
			
			if (writeData)
			{
				CFErrorRef		errorCFObject = nullptr;
				
				
				outData = CFPropertyListCreateData
							(kCFAllocatorDefault, targetDictionary.returnCFMutableDictionaryRef(),
								kCFPropertyListXMLFormat_v1_0, 0/* options */, &errorCFObject);
				if (nullptr == outData)
				{
					result = kPreferences_ResultGenericFailure;
					Console_Warning(Console_WriteLine, "unable to create XML data for export of preferences");
					if (nullptr != errorCFObject)
					{
						CFRetainRelease		errorReleaser(errorCFObject, CFRetainRelease::kAlreadyRetained);
						
						
						Console_Warning(Console_WriteValueCFError, "unable to create property list data, error", errorCFObject);
					}
				}
				else
				{
					// success!
					result = kPreferences_ResultOK;
				}
			}
		}
	}
	return result;
}// ContextSaveAsXMLData


/*!
Convenience method to invoke Preferences_ContextSaveAsXMLData()
and then writing that data to an XML file, as specified by a URL.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultInvalidContextReference
if the specified context does not exist

\retval kPreferences_ResultGenericFailure
if any other problem prevents a save

(4.0)
*/
Preferences_Result
Preferences_ContextSaveAsXMLFileURL		(Preferences_ContextRef		inContext,
										 Preferences_ExportFlags		inExportOptions,
										 CFURLRef					inFileURL)
{
	Preferences_Result	result = kPreferences_ResultGenericFailure;
	
	
	if (nullptr == inContext)
	{
		result = kPreferences_ResultInvalidContextReference;
	}
	else
	{
		CFDataRef	xmlData = nullptr;
		
		
		result = Preferences_ContextSaveAsXMLData(inContext, inExportOptions, xmlData);
		if (kPreferences_ResultOK == result)
		{
			CFRetainRelease		dataReleaser(xmlData, CFRetainRelease::kAlreadyRetained);
			CFRetainRelease		streamObject(CFWriteStreamCreateWithFile(kCFAllocatorDefault, inFileURL),
												CFRetainRelease::kAlreadyRetained);
			
			
			if (streamObject.exists() && CFWriteStreamOpen(streamObject.returnCFWriteStreamRef()))
			{
				CFIndex		bytesWritten = 0; // 0 or -1 on error
				
				
				// IMPORTANT: since the property list has already been converted into
				// XML data by Preferences_ContextSaveAsXMLData(), just write it out
				// directly using the stream; if CFPropertyListWrite() were used
				// instead, the result would be XML containing only a raw data element!
				bytesWritten = CFWriteStreamWrite(streamObject.returnCFWriteStreamRef(),
													CFDataGetBytePtr(xmlData), CFDataGetLength(xmlData));
				if (bytesWritten > 0)
				{
					// success!
					result = kPreferences_ResultOK;
				}
				else
				{
					result = kPreferences_ResultGenericFailure;
					Console_Warning(Console_WriteLine, "unable to write XML data for export of preferences");
				}
				
				CFWriteStreamClose(streamObject.returnCFWriteStreamRef());
			}
		}
	}
	return result;
}// ContextSaveAsXMLFileURL


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
	Quills::Prefs::Class	dataClass = Quills::Prefs::GENERAL;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
		
		
		switch (dataClass)
		{
		case Quills::Prefs::FORMAT:
			result = setFormatPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case Quills::Prefs::GENERAL:
			result = setGeneralPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case Quills::Prefs::MACRO_SET:
			result = setMacroPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case Quills::Prefs::SESSION:
			result = setSessionPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case Quills::Prefs::TERMINAL:
			result = setTerminalPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case Quills::Prefs::TRANSLATION:
			result = setTranslationPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case Quills::Prefs::WORKSPACE:
			result = setWorkspacePreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
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

For convenience, you may set "inIncludeDefaultItem" to true
to have an item named “Default” appear first in the list
(commonly useful for user interface elements).

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
Preferences_CreateContextNameArray	(Quills::Prefs::Class	inClass,
									 CFArrayRef&			outNewArrayOfNewCFStrings,
									 Boolean				inIncludeDefaultItem)
{
	Preferences_Result				result = kPreferences_ResultGenericFailure;
	My_FavoriteContextList const*	listPtr = nullptr;
	
	
	if (false == getListOfContexts(inClass, listPtr)) result = kPreferences_ResultOneOrMoreNamesNotAvailable;
	else
	{
		CFMutableArrayRef	newArray = CFArrayCreateMutable(kCFAllocatorDefault, listPtr->size(),
															&kCFTypeArrayCallBacks);
		
		
		if (nullptr != newArray)
		{
			CFIndex 	i = 0;
			auto		toContextPtr = listPtr->begin();
			
			
			if (inIncludeDefaultItem)
			{
				CFRetainRelease		defaultName(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowDefaultFavoriteName),
												CFRetainRelease::kAlreadyRetained);
				
				
				CFArraySetValueAtIndex(newArray, i, defaultName.returnCFStringRef());
				++i;
			}
			
			result = kPreferences_ResultOK; // initially...
			for (; toContextPtr != listPtr->end(); ++toContextPtr)
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

See also Preferences_IsContextNameInUse().

\retval kPreferences_ResultOK
if a name was created successfully

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is not valid

\retval kPreferences_ResultGenericFailure
if any other error occurs

(3.1)
*/
Preferences_Result
Preferences_CreateUniqueContextName		(Quills::Prefs::Class	inClass,
										 CFStringRef&			outNewName,
										 CFStringRef			inBaseNameOrNull)
{
	Preferences_Result				result = kPreferences_ResultGenericFailure;
	My_FavoriteContextList const*	listPtr = nullptr;
	
	
	if (false == getListOfContexts(inClass, listPtr))
	{
		result = kPreferences_ResultUnknownTagOrClass;
	}
	else
	{
		CFIndex const			kMaxTries = 100; // arbitrary
		CFStringRef const		kBaseName = (nullptr == inBaseNameOrNull)
												? CFSTR("untitled") // LOCALIZE THIS
												: inBaseNameOrNull;
		CFMutableStringRef		nameTemplateCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* maximum length */, kBaseName);
		
		
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
					if (false == Preferences_IsContextNameInUse(inClass, currentNameCFString))
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
Dumps a text representation of the specified context’s settings
to the console.  Currently, XML format is used.

(4.0)
*/
void
Preferences_DebugDumpContext	(Preferences_ContextRef		inContext)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFDataRef				dataRef = nullptr;
	Boolean					dumpOK = false;
	
	
	// include all classes in order to see an accurate view of everything
	// that was directly saved in the specified context
	prefsResult = Preferences_ContextSaveAsXMLData
					(inContext, (kPreferences_ExportFlagIncludeAllClasses), dataRef);
	if (kPreferences_ResultOK == prefsResult)
	{
		CFRetainRelease		dataReleaser(dataRef, CFRetainRelease::kAlreadyRetained);
		CFRetainRelease		asString(CFStringCreateFromExternalRepresentation
										(kCFAllocatorDefault, dataRef, kCFStringEncodingUTF8),
										CFRetainRelease::kAlreadyRetained);
		
		
		if (asString.exists())
		{
			Console_WriteValueAddress("dumping debug version of context", inContext);
			Console_WriteValueCFString("context dump", asString.returnCFStringRef());
			dumpOK = true;
		}
	}
	
	if (false == dumpOK)
	{
		Console_Warning(Console_WriteValueAddress, "failed to dump debug version of context", inContext);
	}
}// DebugDumpContext


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
Preferences_GetContextsInClass	(Quills::Prefs::Class						inClass,
								 std::vector< Preferences_ContextRef >&		outContextsList)
{
	Boolean					result = false;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef	defaultContext = nullptr;
	
	
	prefsResult = Preferences_GetDefaultContext(&defaultContext, inClass);
	if (kPreferences_ResultOK == prefsResult)
	{
		My_FavoriteContextList const*	listPtr = nullptr;
		
		
		outContextsList.push_back(defaultContext);
		assert(false == outContextsList.empty());
		assert(outContextsList.back() == defaultContext);
		
		if (getListOfContexts(inClass, listPtr))
		{
			auto	toContextPtr = listPtr->begin();
			
			
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
						 void*				outDataStorage)
{
	Preferences_Result			result = kPreferences_ResultOK;
	Preferences_ContextRef		context = nullptr;
	CFStringRef					keyName = nullptr;
	FourCharCode				keyValueType = '----';
	size_t						actualSize = 0;
	Quills::Prefs::Class		dataClass = Quills::Prefs::GENERAL;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		result = Preferences_GetDefaultContext(&context, dataClass);
		if (kPreferences_ResultOK == result)
		{
			result = Preferences_ContextGetData(context, inDataPreferenceTag, inDataStorageSize,
												outDataStorage, false/* search defaults too */);
		}
	}
	return result;
}// GetData


/*!
Returns the default context for the specified class.
Preferences written to the "Quills::Prefs::GENERAL"
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
								 Quills::Prefs::Class		inClass)
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
Returns true only if there is a context in the given class
that has the specified name, or the Default name is given.

This is useful for Preferences_NewContextFromFavorites() calls,
in case you want to avoid creating new contexts for names that
do not correspond to existing contexts.

See also Preferences_CreateUniqueContextName().

(4.0)
*/
Boolean
Preferences_IsContextNameInUse		(Quills::Prefs::Class	inClass,
									 CFStringRef			inProposedName)
{
	Boolean		result = false;
	
	
	if (nullptr != inProposedName)
	{
		My_FavoriteContextList const*	listPtr = nullptr;
		
		
		if (getListOfContexts(inClass, listPtr))
		{
			auto	toContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
												contextNameEqualTo(inProposedName));
			
			
			if (listPtr->end() != toContextPtr)
			{
				result = true;
			}
		}
		
		// although the Default name does NOT directly match
		// any context, it is recognized as a reserved name
		// because this is sometimes necessary (for example,
		// a menu that binds to valid context names may
		// contain a Default item as well)
		if (false == result)
		{
			CFRetainRelease		defaultName(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowDefaultFavoriteName),
												CFRetainRelease::kAlreadyRetained);
			
			
			if (kCFCompareEqualTo == CFStringCompare(defaultName.returnCFStringRef(), inProposedName, 0/* options */))
			{
				result = true;
			}
		}
	}
	return result;
}// IsContextNameInUse


/*!
Deletes all auto-save contexts stored on disk (that is, all
contexts in the Quills::Prefs::_RESTORE_AT_LAUNCH class).

This should be invoked after the application has launched in
normal mode (without requiring a restore).  It should also be
invoked before new windows open, or any other action occurs
that may require a brand new auto-save context.

\retval kPreferences_ResultOK
always; no other errors are currently defined

(4.0)
*/
Preferences_Result
Preferences_PurgeAutosaveContexts ()
{
	typedef std::set< My_ContextFavoritePtr >	ContextPtrSet;
	Preferences_Result		result = kPreferences_ResultOK;
	ContextPtrSet			destroyedPtrs; // used only to compare addresses, never to dereference them
	
	
	for (auto contextPtr : gAutoSaveNamedContexts())
	{
		// don’t delete anything twice!
		if (destroyedPtrs.end() == destroyedPtrs.find(contextPtr))
		{
			if (nullptr != contextPtr)
			{
				destroyedPtrs.insert(contextPtr);
				contextPtr->destroy();
			}
		}
	}
	
	// it is only safe to release the contexts while iterating over
	// a different container, because Preferences_ReleaseContext()
	// will directly erase elements from the original list above
	for (auto prefContextPtr : destroyedPtrs)
	{
		// a copy must be made, since releasing the context will
		// overwrite the location of the reference (which originates
		// inside the memory block that might be deallocated here)
		Preferences_ContextRef		ref = prefContextPtr->selfRef;
		
		
		Preferences_ReleaseContext(&ref);
	}
	
	// the list should be empty at this point, but erase it anyway
	// so that a global save has no ability to resurrect contexts
	gAutoSaveNamedContexts().clear();
	
	return result;
}// PurgeAutosaveContexts


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
	auto					contextSaver = [](My_ContextInterfacePtr p) { p->save(); };
	
	
	// make sure all open context dictionaries are in preferences too
	std::for_each(gAutoSaveNamedContexts().begin(), gAutoSaveNamedContexts().end(), contextSaver);
	std::for_each(gFormatNamedContexts().begin(), gFormatNamedContexts().end(), contextSaver);
	std::for_each(gMacroSetNamedContexts().begin(), gMacroSetNamedContexts().end(), contextSaver);
	std::for_each(gSessionNamedContexts().begin(), gSessionNamedContexts().end(), contextSaver);
	std::for_each(gTerminalNamedContexts().begin(), gTerminalNamedContexts().end(), contextSaver);
	std::for_each(gTranslationNamedContexts().begin(), gTranslationNamedContexts().end(), contextSaver);
	std::for_each(gWorkspaceNamedContexts().begin(), gWorkspaceNamedContexts().end(), contextSaver);
	
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
	Quills::Prefs::Class		dataClass = Quills::Prefs::GENERAL;
	
	
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
	case kPreferences_TagMapBackquote:
	case kPreferences_TagNewCommandShortcutEffect:
	case kPreferences_TagNotifyOfBeeps:
	case kPreferences_TagPureInverse:
	case kPreferences_TagScrollDelay:
	case kPreferences_TagTerminalCursorType:
	case kPreferences_TagTerminalMousePointerColor:
	case kPreferences_TagTerminalResizeAffectsFontSize:
	case kPreferences_TagTerminalShowMarginAtColumn:
	case kPreferences_ChangeContextName:
	case kPreferences_ChangeNumberOfContexts:
		result = assertInitialized();
		if (result == kPreferences_ResultOK)
		{
			Boolean		addOK = false;
			
			
			addOK = ListenerModel_AddListenerForEvent(gPreferenceEventListenerModel, inForWhatChange,
														inListener);
			if (false == addOK)
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
	case kPreferences_TagMapBackquote:
	case kPreferences_TagNewCommandShortcutEffect:
	case kPreferences_TagNotifyOfBeeps:
	case kPreferences_TagPureInverse:
	case kPreferences_TagScrollDelay:
	case kPreferences_TagTerminalCursorType:
	case kPreferences_TagTerminalMousePointerColor:
	case kPreferences_TagTerminalResizeAffectsFontSize:
	case kPreferences_TagTerminalShowMarginAtColumn:
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


/*!
Adds the keys from the specified source tag set to the
destination tag set.

NOTE:	

\retval kPreferences_ResultOK
if no error occurs

\retval kPreferences_ResultGenericFailure
if the tag sets cannot be merged (perhaps because one is invalid)

(4.1)
*/
Preferences_Result
Preferences_TagSetMerge		(Preferences_TagSetRef	inoutTagSet,
							 Preferences_TagSetRef	inSourceTagSet)
{
	Preferences_Result		result = kPreferences_ResultGenericFailure;
	My_TagSetAutoLocker		sourcePtr(gMyTagSetPtrLocks(), inSourceTagSet);
	My_TagSetAutoLocker		destinationPtr(gMyTagSetPtrLocks(), inoutTagSet);
	
	
	My_TagSet::extendKeyListWithKeyList(destinationPtr->contextKeys.returnCFMutableArrayRef(),
										sourcePtr->contextKeys.returnCFArrayRef());
	result = kPreferences_ResultOK;
	
	return result;
}// TagSetMerge


#pragma mark Internal Methods
namespace {

/*!
Constructor.  Used only by subclasses.

(3.1)
*/
My_ContextInterface::
My_ContextInterface		(Quills::Prefs::Class	inClass)
:
refValidator(REINTERPRET_CAST(this, Preferences_ContextRef), gMyContextValidRefs()),
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
	
	
	if (nullptr == _listenerModel)
	{
		_listenerModel = ListenerModel_New(kListenerModel_StyleStandard, 'PCtx');
		assert(nullptr != _listenerModel);
	}
	
	if (nullptr != _listenerModel)
	{
		result = ListenerModel_AddListenerForEvent(_listenerModel, inForWhatChange, inListener);
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
Quills::Prefs::Class
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
													&kCFTypeArrayCallBacks), CFRetainRelease::kAlreadyRetained);
		CFRetainRelease		copiedArray;
		
		
		result &= Console_Assert("test array exists", testArray.exists());
		result &= Console_Assert("test array is the right size", 2 == CFArrayGetCount(testArray.returnCFArrayRef()));
		inTestObjectPtr->addArray(0/* do not notify */, CFSTR("__test_array_key__"), testArray.returnCFArrayRef());
		copiedArray.setWithNoRetain(inTestObjectPtr->returnArrayCopy(CFSTR("__test_array_key__")));
		result &= Console_Assert("returned array exists", copiedArray.exists());
		result &= Console_Assert("returned array is the right size", 2 == CFArrayGetCount(copiedArray.returnCFArrayRef()));
	}
	
	// data values
	{
		char const* const	kData = "my test string";
		size_t const		kDataSize = (1 + std::strlen(kData)) * sizeof(char);
		CFRetainRelease		testData(CFDataCreate(kCFAllocatorDefault, REINTERPRET_CAST(kData, UInt8 const*),
													kDataSize), CFRetainRelease::kAlreadyRetained);
		CFDataRef			testDataRef = REINTERPRET_CAST(testData.returnCFTypeRef(), CFDataRef);
		CFRetainRelease		copiedValue;
		
		
		result &= Console_Assert("test data exists", testData.exists());
		result &= Console_Assert("test data is the right size", STATIC_CAST(kDataSize, CFIndex) == CFDataGetLength(testDataRef));
		inTestObjectPtr->addData(0/* do not notify */, CFSTR("__test_data_key__"), REINTERPRET_CAST(testData.returnCFTypeRef(), CFDataRef));
		copiedValue.setWithNoRetain(inTestObjectPtr->returnValueCopy(CFSTR("__test_data_key__")));
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
		Float32 const	kFloat3 = -36.4f;
		Float32 const	kFloat4 = 5312.79195f;
		Float32 const	kTolerance = 0.1f; // for fuzzy equality
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
		copiedString.setWithNoRetain(inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_1__")));
		result &= Console_Assert("returned string 1 exists", copiedString.exists());
		result &= Console_Assert("returned string 1 is correct",
									kCFCompareEqualTo == CFStringCompare(copiedString.returnCFStringRef(), kString1, 0/* options */));
		inTestObjectPtr->addString(0/* do not notify */, CFSTR("__test_string_key_2__"), kString2);
		copiedString.setWithNoRetain(inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_2__")));
		result &= Console_Assert("returned string 2 exists", copiedString.exists());
		result &= Console_Assert("returned string 2 is correct",
									kCFCompareEqualTo == CFStringCompare(copiedString.returnCFStringRef(), kString2, 0/* options */));
		inTestObjectPtr->deleteValue(CFSTR("__test_string_key_2__"));
		copiedString.setWithNoRetain(inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_2__")));
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
My_ContextCFDictionary	(Quills::Prefs::Class	inClass,
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
My_ContextCFDictionary	(Quills::Prefs::Class		inClass,
						 CFMutableDictionaryRef		inDictionaryOrNull)
:
My_ContextInterface(inClass),
_dictionary(inDictionaryOrNull)
{
	if (nullptr == _dictionary.returnDictionary())
	{
		CFRetainRelease		newDictionary(createDictionary(), CFRetainRelease::kAlreadyRetained);
		
		
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
	CFMutableDictionaryRef		result = CFDictionaryCreateMutable
											(kCFAllocatorDefault, 0/* capacity */,
												&kCFTypeDictionaryKeyCallBacks,
												&kCFTypeDictionaryValueCallBacks);
	
	
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
	My_ContextCFDictionary*		testObjectPtr = new My_ContextCFDictionary(Quills::Prefs::GENERAL);
	CFArrayRef					keyListCFArray = nullptr;
	
	
	result &= Console_Assert("class is set correctly",
								Quills::Prefs::GENERAL == testObjectPtr->returnClass());
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
	
	if (nullptr != keyListCFArray)
	{
		CFRelease(keyListCFArray), keyListCFArray = nullptr;
	}
	delete testObjectPtr, testObjectPtr = nullptr;
	
	return result;
}// My_ContextCFDictionary::unitTest


/*!
Constructor.  See Preferences_GetDefaultContext().

(3.1)
*/
My_ContextDefault::
My_ContextDefault	(Quills::Prefs::Class	inClass)
:
My_ContextInterface(inClass),
_contextName(CFSTR("Default")/* TEMPORARY - LOCALIZE  THIS */, CFRetainRelease::kNotYetRetained),
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
	My_ContextDefault*	testObjectPtr = new My_ContextDefault(Quills::Prefs::GENERAL);
	CFArrayRef			keyListCFArray = nullptr;
	
	
	result &= Console_Assert("class is set correctly",
								Quills::Prefs::GENERAL == testObjectPtr->returnClass());
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
	
	if (nullptr != keyListCFArray)
	{
		CFRelease(keyListCFArray), keyListCFArray = nullptr;
	}
	delete testObjectPtr, testObjectPtr = nullptr;
	
	return result;
}// My_ContextDefault::unitTest


/*!
Constructor.  See Preferences_NewContextFromFavorites().

(3.1)
*/
My_ContextFavorite::
My_ContextFavorite	(Quills::Prefs::Class	inClass,
					 CFStringRef			inFavoriteName,
					 CFStringRef			inDomainName)
:
My_ContextInterface(inClass),
_contextName(inFavoriteName, CFRetainRelease::kNotYetRetained),
_domainName((nullptr != inDomainName) ? inDomainName : createDomainName(inClass, inFavoriteName),
			CFRetainRelease::kNotYetRetained),
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
createDomainName	(Quills::Prefs::Class	inClass,
					 CFStringRef			inName)
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
		My_FavoriteContextList const*	favoritesListPtr = nullptr;
		
		
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
				CFStringAppendFormat(possibleName, nullptr/* options */, CFSTR("%d"), (int)possibleSuffix);
				result = possibleName;
				
				nameInUse = false; // initially...
				for (auto prefContextPtr : *favoritesListPtr)
				{
					CFStringRef const	kThisDomain = prefContextPtr->_domainName.returnCFStringRef();
					
					
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
	UNUSED_RETURN(Preferences_Result)copyClassDomainCFArray(this->returnClass(), favoritesListCFArray);
	if (nullptr != favoritesListCFArray)
	{
		CFIndex		indexForName = findDomainIndexInArray(favoritesListCFArray, kDomainName);
		
		
		if (indexForName >= 0)
		{
			// then an entry with this name exists on disk; remove it from the list!
			CFRetainRelease		mutableFavoritesList(CFArrayCreateMutableCopy
														(kCFAllocatorDefault, CFArrayGetCount(favoritesListCFArray)/* capacity */,
															favoritesListCFArray), CFRetainRelease::kAlreadyRetained);
			
			
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
	_contextName.setWithRetain(inNewName);
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
returnClassDomainNamePrefix		(Quills::Prefs::Class	inClass)
{
	CFStringRef		result = nullptr;
	
	
	switch (inClass)
	{
	case Quills::Prefs::_RESTORE_AT_LAUNCH:
		result = kMy_PreferencesSubDomainAutoSave;
		break;
	
	case Quills::Prefs::FORMAT:
		result = kMy_PreferencesSubDomainFormats;
		break;
	
	case Quills::Prefs::MACRO_SET:
		result = kMy_PreferencesSubDomainMacros;
		break;
	
	case Quills::Prefs::SESSION:
		result = kMy_PreferencesSubDomainSessions;
		break;
	
	case Quills::Prefs::TERMINAL:
		result = kMy_PreferencesSubDomainTerminals;
		break;
	
	case Quills::Prefs::TRANSLATION:
		result = kMy_PreferencesSubDomainTranslations;
		break;
	
	case Quills::Prefs::WORKSPACE:
		result = kMy_PreferencesSubDomainWorkspaces;
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
												'?'/* loss byte */), CFRetainRelease::kAlreadyRetained);
		
		
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
															favoritesListCFArray), CFRetainRelease::kAlreadyRetained);
			
			
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
		CFRetainRelease		favoritesList(favoritesListCFArray, CFRetainRelease::kAlreadyRetained);
		CFRetainRelease		mutableFavoritesList(CFArrayCreateMutableCopy
													(kCFAllocatorDefault, 1 + CFArrayGetCount(favoritesListCFArray)/* capacity */,
														favoritesListCFArray), CFRetainRelease::kAlreadyRetained);
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
unitTest	(Quills::Prefs::Class	inClass,
			 CFStringRef			inName)
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
						 Quills::Prefs::Class	inClass)
:
tag(inTag),
keyName(inKeyName, CFRetainRelease::kNotYetRetained),
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
		 Quills::Prefs::Class		inClass,
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
A convenience routine that calls create() with the typical key
value type (kPreferences_DataTypeCFBooleanRef) and non-dictionary
value size (sizeof(Boolean)) for flags.  This simplifies the
construction of many preferences, as this type is very common.

(4.0)
*/
inline void
My_PreferenceDefinition::
createFlag	(Preferences_Tag			inTag,
			 CFStringRef				inKeyName,
			 Quills::Prefs::Class		inClass,
			 My_PreferenceDefinition**	outResultPtrPtrOrNull)
{
	create(inTag, inKeyName, kPreferences_DataTypeCFBooleanRef, sizeof(Boolean), inClass,
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
				 Quills::Prefs::Class	inClass)
{
	for (Preferences_Index i = 1; i <= inNumberOfIndexedKeys; ++i)
	{
		CFRetainRelease		generatedKeyName(createKeyAtIndex(inKeyNameTemplate, i), CFRetainRelease::kAlreadyRetained);
		
		
		create(Preferences_ReturnTagVariantForIndex(inTag, i), generatedKeyName.returnCFStringRef(),
				inKeyValueType, inNonDictionaryValueSize, inClass);
	}
}// My_PreferenceDefinition::createIndexed


/*!
A convenience routine that calls create() with the typical key
value type (kPreferences_DataTypeCFArrayRef) and non-dictionary
value size (sizeof(CGFloatRGBColor)) for RGB colors.  This
simplifies the construction of many preferences, as this type
is very common.

The array is expected to contain 3 CFNumberRefs that have
floating-point values between 0.0 and 1.0, for intensity,
where white is represented by all values being 1.0.

(4.0)
*/
inline void
My_PreferenceDefinition::
createRGBColor	(Preferences_Tag			inTag,
				 CFStringRef				inKeyName,
				 Quills::Prefs::Class		inClass,
				 My_PreferenceDefinition**	outResultPtrPtrOrNull)
{
	create(inTag, inKeyName, kPreferences_DataTypeCFArrayRef, sizeof(CGFloatRGBColor), inClass, outResultPtrPtrOrNull);
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
	My_PreferenceDefinition*	result = nullptr;
	auto						toDefPtr = _definitionsByKeyName.find(inKeyName);
	
	
	//Console_WriteValueCFString("find preference by key name", inKeyName); // debug
	if (toDefPtr != _definitionsByKeyName.end())
	{
		result = toDefPtr->second;
	}
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
	My_PreferenceDefinition*	result = nullptr;
	auto						toDefPtr = _definitionsByTag.find(inTag);
	
	
	if (toDefPtr != _definitionsByTag.end())
	{
		result = toDefPtr->second;
	}
	return result;
}// My_PreferenceDefinition::findByTag


/*!
Returns true if the specified key had a definition created
for it, or was registered as an indirect key.  In other words,
any name that is “expected” in the application preferences
should return true, and anything unknown will not.

(4.0)
*/
Boolean
My_PreferenceDefinition::
isValidKeyName	(CFStringRef	inKeyName)
{
	auto		toDefPtr = _definitionsByKeyName.find(inKeyName);
	Boolean		result = false;
	
	
	if (toDefPtr != _definitionsByKeyName.end())
	{
		result = true;
	}
	else
	{
		auto	toKey = _indirectKeyNames.find(inKeyName);
		
		
		if (toKey != _indirectKeyNames.end())
		{
			result = true;
		}
	}
	return result;
}// My_PreferenceDefinition::isValidKeyName


/*!
If a low-level key is considered valid when it appears in
application preferences, but is not tied to a particular
high-level tag or definition structure, register it with this
method.  Then, isValidKeyName() will return true for this key
along with any keys that have actual definitions.

This is typically used for keys that are truly special (like
"prefs-version"), and keys that have a single high-level tag
but values that are spread across multiple low-level key names
(such as some command key settings).

Currently, this registry has little purpose other than to quell
warnings for keys that are not used in the usual way, but still
technically valid keys.

(4.0)
*/
void
My_PreferenceDefinition::
registerIndirectKeyName		(CFStringRef	inKeyName)
{
	auto	toKey = _indirectKeyNames.find(inKeyName);
	
	
	if (toKey == _indirectKeyNames.end())
	{
		_indirectKeyNames.insert(inKeyName);
	}
}// My_PreferenceDefinition::registerIndirectKeyName


/*!
Constructor.

(4.0)
*/
My_TagSet::
My_TagSet	(My_ContextInterfacePtr		inContextPtr)
:
selfRef(REINTERPRET_CAST(this, Preferences_TagSetRef)),
contextKeys(inContextPtr->returnKeyListCopy(), CFRetainRelease::kAlreadyRetained)
{
#if 0
	// DEBUG
	{
		CFArrayRef		sourceKeysCFArray = this->contextKeys.returnCFArrayRef();
		
		
		if (nullptr == sourceKeysCFArray)
		{
			Console_Warning(Console_WriteLine, "null tags array!");
		}
		else
		{
			CFIndex const	kNumberOfKeys = CFArrayGetCount(sourceKeysCFArray);
			
			
			for (CFIndex i = 0; i < kNumberOfKeys; ++i)
			{
				CFStringRef const	kKeyCFStringRef = CFUtilities_StringCast
														(CFArrayGetValueAtIndex(sourceKeysCFArray, i));
				
				
				Console_WriteValueCFString("context set contains key", kKeyCFStringRef);
			}
		}
	}
#endif
}// My_TagSet 1-argument (Preferences_ContextRef) constructor


/*!
Constructor.

(4.0)
*/
My_TagSet::
My_TagSet	(std::vector< Preferences_Tag > const&		inTags)
:
selfRef(REINTERPRET_CAST(this, Preferences_TagSetRef)),
contextKeys(returnNewKeyListForTags(inTags), CFRetainRelease::kAlreadyRetained)
{
}// My_TagSet 1-argument (vector) constructor


/*!
Extends an array containing Core Foundation Strings to include
the given key names.

(4.1)
*/
void
My_TagSet::
extendKeyListWithKeyList	(CFMutableArrayRef		inoutArray,
						 	 CFArrayRef				inKeys)
{
	CFArrayAppendArray(inoutArray, inKeys, CFRangeMake(0, CFArrayGetCount(inKeys)));
}// My_TagSet::extendKeyListWithKeyList


/*!
Extends an array containing Core Foundation Strings to add
the key names corresponding to the given preference tags.

(4.1)
*/
void
My_TagSet::
extendKeyListWithTags	(CFMutableArrayRef						inoutArray,
						 std::vector< Preferences_Tag > const&	inTags)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFStringRef				keyNameCFString = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					nonDictionaryValueSize = 0;
	Quills::Prefs::Class	prefsClass = Quills::Prefs::GENERAL;
	
	
	for (auto prefsTag : inTags)
	{
		if (0 != prefsTag)
		{
			prefsResult = getPreferenceDataInfo(prefsTag, keyNameCFString, keyValueType,
												nonDictionaryValueSize, prefsClass);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteValueFourChars, "failed to extend key list with tag", prefsTag);
				Console_Warning(Console_WriteValue, "preferences lookup error was", prefsResult);
			}
			else
			{
				if (nullptr != keyNameCFString)
				{
					CFArrayAppendValue(inoutArray, keyNameCFString);
				}
			}
		}
	}
}// My_TagSet::extendKeyListWithTags


/*!
Creates an array containing Core Foundation Strings with
the key names corresponding to the given preference tags.

(4.0)
*/
CFMutableArrayRef
My_TagSet::
returnNewKeyListForTags		(std::vector< Preferences_Tag > const&		inTags)
{
	CFMutableArrayRef	result = CFArrayCreateMutable(kCFAllocatorDefault,
														STATIC_CAST(inTags.size(), CFIndex)/* capacity */,
														&kCFTypeArrayCallBacks);
	
	
	if (nullptr != result)
	{
		extendKeyListWithTags(result, inTags);
	}
	return result;
}// My_TagSet::returnNewKeyListForTags


/*!
Ensures that the Preferences module has been
properly initialized.  If Preferences_Init()
needs to be called, it is called.

If this is called at a point during module
initialization (meaning it has begun but it
has not completed), the result will be
"kPreferences_ResultNotInitialized".

(3.0)
*/
Preferences_Result
assertInitialized ()
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (gInitializing)
	{
		// this is in the call tree of Preferences_Init() itself!
		result = kPreferences_ResultNotInitialized;
	}
	else if (false == gInitialized)
	{
		Preferences_Init();
	}
	
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
					 Quills::Prefs::Class		inDataClass,
					 Preferences_Tag			inDataPreferenceTag,
					 size_t						inDataStorageSize,
					 void*						outDataStorage)
{
	Preferences_Result		result = kPreferences_ResultOK;
	size_t					actualSize = 0;
	
	
	switch (inDataClass)
	{
	case Quills::Prefs::FORMAT:
		result = getFormatPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	case Quills::Prefs::GENERAL:
		result = getGeneralPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	case Quills::Prefs::MACRO_SET:
		result = getMacroPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	case Quills::Prefs::SESSION:
		result = getSessionPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	case Quills::Prefs::TERMINAL:
		result = getTerminalPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	case Quills::Prefs::TRANSLATION:
		result = getTranslationPreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	case Quills::Prefs::WORKSPACE:
		result = getWorkspacePreference(inContextPtr, inDataPreferenceTag, inDataStorageSize, outDataStorage, &actualSize);
		break;
	
	default:
		// unrecognized preference class
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	if (inDataStorageSize != actualSize)
	{
		Console_Warning(Console_WriteValueFourChars, "incorrect request for preference data with tag", inDataPreferenceTag);
		Console_Warning(Console_WriteValue, "required byte count for value", actualSize);
	}
	
	return result;
}// contextGetData


/*!
Reads an array of 3 CFNumber elements and puts their
values into a CGFloatRGBColor structure.

Returns "true" only if successful.

(2016.09)
*/
Boolean
convertCFArrayToCGFloatRGBColor		(CFArrayRef			inArray,
									 CGFloatRGBColor*	outColorPtr)
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
			outColorPtr->red = STATIC_CAST(floatValue, float);
			if (CFNumberGetValue(greenValue, kCFNumberFloat64Type, &floatValue))
			{
				outColorPtr->green = STATIC_CAST(floatValue, float);
				if (CFNumberGetValue(blueValue, kCFNumberFloat64Type, &floatValue))
				{
					outColorPtr->blue = STATIC_CAST(floatValue, float);
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
}// convertCFArrayToCGFloatRGBColor


/*!
Reads an array of 4 CFNumber elements and puts their
values into a structure in the order origin.x, origin.y,
size.width, size.height.  The origin should be treated
as the top-left corner.

Returns "true" only if successful.  Note, however, that
a rectangle containing all zeroes may also be a problem.

(4.0)
*/
Boolean
convertCFArrayToHIRect	(CFArrayRef						inArray,
						 Preferences_TopLeftCGRect&		outRect)
{
	Boolean		result = false;
	
	
	if ((nullptr != inArray) && (CFArrayGetCount(inArray) == 4))
	{
		CFNumberRef		xValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 0));
		CFNumberRef		yValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 1));
		CFNumberRef		widthValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 2));
		CFNumberRef		heightValue = CFUtilities_NumberCast(CFArrayGetValueAtIndex(inArray, 3));
		Float32			floatValue = 0L;
		
		
		if (CFNumberGetValue(xValue, kCFNumberFloat32Type, &floatValue))
		{
			outRect.origin.x = floatValue;
			if (CFNumberGetValue(yValue, kCFNumberFloat32Type, &floatValue))
			{
				outRect.origin.y = floatValue;
				if (CFNumberGetValue(widthValue, kCFNumberFloat32Type, &floatValue))
				{
					outRect.size.width = floatValue;
					if (CFNumberGetValue(heightValue, kCFNumberFloat32Type, &floatValue))
					{
						outRect.size.height = floatValue;
						result = true;
					}
				}
			}
		}
	}
	else
	{
		// unexpected number of elements
	}
	return result;
}// convertCFArrayToHIRect


/*!
Reads a CGFloatRGBColor structure and puts the values
into an array of 3 CFNumber elements.

Returns "true" only if successful.

(2016.09)
*/
Boolean
convertCGFloatRGBColorToCFArray		(CGFloatRGBColor const*		inColorPtr,
									 CFArrayRef&				outNewCFArray)
{
	Boolean		result = false;
	
	
	outNewCFArray = nullptr;
	
	if (nullptr != inColorPtr)
	{
		CFNumberRef		componentValues[] = { nullptr, nullptr, nullptr };
		SInt16			i = 0;
		Float32			floatValue = 0L;
		
		
		// WARNING: this is not exception-safe
		floatValue = inColorPtr->red;
		componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
		if (nullptr != componentValues[i])
		{
			++i;
			floatValue = inColorPtr->green;
			componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
			if (nullptr != componentValues[i])
			{
				++i;
				floatValue = inColorPtr->blue;
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
}// convertCGFloatRGBColorToCFArray


/*!
Utility routine for “packing” a rectangular value (origin x, y
and size width, height) into a CFArray of 4 floating-point
numbers.  Returns true only if successful.

See also convertCFArrayToHIRect().

(4.0)
*/
Boolean
convertHIRectToCFArray	(Preferences_TopLeftCGRect const&	inRect,
						 CFArrayRef&						outNewCFArray)
{
	CFNumberRef		componentValues[] = { nullptr, nullptr, nullptr, nullptr };
	SInt16			i = 0;
	Float32			floatValue = 0L;
	Boolean			result = false;
	
	
	outNewCFArray = nullptr;
	
	// WARNING: this is not exception-safe
	floatValue = inRect.origin.x;
	componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
	if (nullptr != componentValues[i])
	{
		++i;
		floatValue = inRect.origin.y;
		componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
		if (nullptr != componentValues[i])
		{
			++i;
			floatValue = inRect.size.width;
			componentValues[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
			if (nullptr != componentValues[i])
			{
				++i;
				floatValue = inRect.size.height;
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
		--i;
		CFRelease(componentValues[i]);
	}
	return result;
}// convertHIRectToCFArray


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
copyClassDomainCFArray	(Quills::Prefs::Class	inClass,
						 CFArrayRef&			outCFArrayOfCFStrings)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// set default value
	outCFArrayOfCFStrings = nullptr;
	
	// figure out which main application preferences key holds the relevant list of domains
	switch (inClass)
	{
	case Quills::Prefs::GENERAL:
		// not applicable
		result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::_RESTORE_AT_LAUNCH:
		readApplicationArrayPreference(CFSTR("auto-save"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::FORMAT:
		readApplicationArrayPreference(CFSTR("favorite-formats"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::MACRO_SET:
		readApplicationArrayPreference(CFSTR("favorite-macro-sets"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::SESSION:
		readApplicationArrayPreference(CFSTR("favorite-sessions"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::TERMINAL:
		readApplicationArrayPreference(CFSTR("favorite-terminals"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::TRANSLATION:
		readApplicationArrayPreference(CFSTR("favorite-translations"), outCFArrayOfCFStrings);
		if (nullptr == outCFArrayOfCFStrings) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case Quills::Prefs::WORKSPACE:
		readApplicationArrayPreference(CFSTR("favorite-workspaces"), outCFArrayOfCFStrings);
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
Creates and returns a Core Foundation dictionary
that can be used to look up default preference keys
(as defined in "DefaultPreferences.plist").

Returns nullptr if unsuccessful for any reason.

(3.0)
*/
CFDictionaryRef
copyDefaultPrefDictionary ()
{
	CFDictionaryRef		result = nullptr;
	CFRetainRelease		urlObject(CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), CFSTR("DefaultPreferences"),
															CFSTR("plist")/* type string */, nullptr/* subdirectory path */),
									CFRetainRelease::kAlreadyRetained);
	
	
	if (urlObject.exists())
	{
		CFURLRef			fileURL = urlObject.returnCFURLRef();
		CFRetainRelease		streamObject(CFReadStreamCreateWithFile(kCFAllocatorDefault, fileURL),
											CFRetainRelease::kAlreadyRetained);
		
		
		if (streamObject.exists() && CFReadStreamOpen(streamObject.returnCFReadStreamRef()))
		{
			CFPropertyListRef   	propertyList = nullptr;
			CFPropertyListFormat	actualFormat = kCFPropertyListXMLFormat_v1_0;
			CFErrorRef				errorCFObject = nullptr;
			
			
			propertyList = CFPropertyListCreateWithStream
							(kCFAllocatorDefault, streamObject.returnCFReadStreamRef(),
								0/* length or 0 for all */, kCFPropertyListImmutable,
								&actualFormat, &errorCFObject);
			if (nullptr != propertyList)
			{
				// the XML file actually contains a dictionary
				result = CFUtilities_DictionaryCast(propertyList);
			}
			
			if (nullptr != errorCFObject)
			{
				CFRetainRelease		errorReleaser(errorCFObject, CFRetainRelease::kAlreadyRetained);
				
				
				Console_Warning(Console_WriteValueCFError, "unable to create default preferences property list from XML data, error", errorCFObject);
			}
			
			CFReadStreamClose(streamObject.returnCFReadStreamRef());
		}
	}
	
	return result;
}// copyDefaultPrefDictionary


/*!
Reads preferences in the given domain and attempts to
find a key that indicates the user’s preferred name
for this set of preferences.  The result could be
nullptr; if not, you must call CFRelease() on it.

(3.1)
*/
CFStringRef
copyDomainUserSpecifiedName		(CFStringRef	inDomainName)
{
	CFStringRef			result = nullptr;
	CFRetainRelease		dataObject(CFPreferencesCopyAppValue(CFSTR("name"), inDomainName),
									CFRetainRelease::kAlreadyRetained);
	CFRetainRelease		stringObject(CFPreferencesCopyAppValue(CFSTR("name-string"), inDomainName),
										CFRetainRelease::kAlreadyRetained);
	
	
	// in order to support many languages, the "name" field is stored as
	// Unicode data (string external representation); however, if that
	// is not available, purely for convenience a "name-string" alternate
	// is supported, holding a raw string
	result = copyUserSpecifiedName(CFUtilities_DataCast(dataObject.returnCFTypeRef()),
									stringObject.returnCFStringRef());
	
	return result;
}// copyDomainUserSpecifiedName


/*!
A name for a context may be stored in two ways, and not
necessarily both: as a "name" key of Core Foundation Data
type (external representation of Unicode) and as a
"name-string" key of Core Foundation String type.  Pass
one or both of those values (nullptr is acceptable) and
a string is constructed appropriately.

(4.0)
*/
CFStringRef
copyUserSpecifiedName		(CFDataRef		inNameExternalUnicodeDataValue,
							 CFStringRef	inNameStringValue)
{
	CFStringRef		result = nullptr;
	
	
	if (nullptr != inNameExternalUnicodeDataValue)
	{
		// Unicode string was found
		result = CFStringCreateFromExternalRepresentation
					(kCFAllocatorDefault, inNameExternalUnicodeDataValue, kCFStringEncodingUnicode);
	}
	else if (nullptr != inNameStringValue)
	{
		// raw string was found
		result = CFStringCreateCopy(kCFAllocatorDefault, inNameStringValue);
	}
	else
	{
		result = CFSTR("");
		CFRetain(result);
	}
	return result;
}// copyUserSpecifiedName


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
	typedef std::vector< Quills::Prefs::Class >		PrefClassList;
	static Boolean			gWasCalled = false;
	PrefClassList			allClassesSupportingCollections;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	assert(false == gWasCalled);
	gWasCalled = true;
	
	// for every class that can have collections, create ALL contexts
	// (based on saved names) for that class, so that the list is current
	allClassesSupportingCollections.push_back(Quills::Prefs::_RESTORE_AT_LAUNCH);
	allClassesSupportingCollections.push_back(Quills::Prefs::WORKSPACE);
	allClassesSupportingCollections.push_back(Quills::Prefs::SESSION);
	allClassesSupportingCollections.push_back(Quills::Prefs::TERMINAL);
	allClassesSupportingCollections.push_back(Quills::Prefs::FORMAT);
	allClassesSupportingCollections.push_back(Quills::Prefs::MACRO_SET);
	allClassesSupportingCollections.push_back(Quills::Prefs::TRANSLATION);
	for (auto prefsClass : allClassesSupportingCollections)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		CFArrayRef				namesInClass = nullptr;
		
		
		prefsResult = copyClassDomainCFArray(prefsClass, namesInClass);
		if ((nullptr != namesInClass) && (0 == CFArrayGetCount(namesInClass)) &&
			(Quills::Prefs::FORMAT == prefsClass))
		{
			// the Format type is a special case; if there are no user-custom
			// collections yet, then copy in all the default color schemes
			// (this gives the user a list of Formats by default)
			CFArrayRef		fileNameArray = CFUtilities_ArrayCast(CFBundleGetValueForInfoDictionaryKey(AppResources_ReturnBundleForInfo(),
																										CFSTR("MyDefaultFormatPropertyLists")));
			CFIndex			fileNameCount = (fileNameArray) ? CFArrayGetCount(fileNameArray) : 0;
			
			
			for (CFIndex i = 0; i < fileNameCount; ++i)
			{
				// read a default format into a context, then create a target
				// context that will save it in the Format collections list
				// using the same name as that of the source context
				CFStringRef		fileNameNoExtension = CFUtilities_StringCast(CFArrayGetValueAtIndex(fileNameArray, i));
				CFURLRef		fileURL = CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), fileNameNoExtension,
																	CFSTR("plist")/* type string */, nullptr/* subdirectory path */);
				
				
				if (nullptr != fileURL)
				{
					// create a class-specific context so that it will automatically
					// be stored in the appropriate preferences domain on disk (the
					// name is implicitly changed by copying in the source context)
					Preferences_ContextWrap		savedFormat(Preferences_NewContextFromFavorites(Quills::Prefs::FORMAT, nullptr/* generate name */),
															Preferences_ContextWrap::kAlreadyRetained);
					
					
					if (savedFormat.exists())
					{
						CFStringRef		inferredName = nullptr;
						
						
						prefsResult = Preferences_ContextMergeInXMLFileURL(savedFormat.returnRef(), fileURL,
																			nullptr/* class */, &inferredName);
						if (kPreferences_ResultOK == prefsResult)
						{
							if (nullptr != inferredName)
							{
								prefsResult = Preferences_ContextRename(savedFormat.returnRef(), inferredName);
								if (kPreferences_ResultOK != prefsResult)
								{
									Console_Warning(Console_WriteValueCFString,
													"unable to rename default format; name should be", inferredName);
								}
							}
							
							prefsResult = Preferences_ContextSave(savedFormat.returnRef());
							if (kPreferences_ResultOK == prefsResult)
							{
								// success!
							}
						}
					}
					CFRelease(fileURL), fileURL = nullptr;
				}
			}
			
			// now that the set of Formats has been changed, reinitialize the array
			prefsResult = copyClassDomainCFArray(prefsClass, namesInClass);
		}
		
		// create contexts for every domain that was found for this class
		if (kPreferences_ResultOK == prefsResult)
		{
			CFIndex const	kNumberOfFavorites = CFArrayGetCount(namesInClass);
			
			
			for (CFIndex i = 0; i < kNumberOfFavorites; ++i)
			{
				// simply creating a context will ensure it is retained
				// internally; so, the return value can be ignored (save
				// for verifying that it was created successfully)
				CFStringRef const		kDomainName = CFUtilities_StringCast(CFArrayGetValueAtIndex
																				(namesInClass, i));
				CFRetainRelease			favoriteNameCFString(copyDomainUserSpecifiedName(kDomainName), CFRetainRelease::kAlreadyRetained);
				Preferences_ContextRef	newContext = Preferences_NewContextFromFavorites(prefsClass, favoriteNameCFString.returnCFStringRef(), kDomainName);
				
				
				if (nullptr == newContext)
				{
					Console_Warning(Console_WriteValueCFString, "sister domain could not be loaded; core application preferences refer to missing domain", kDomainName);
					Console_Warning(Console_WriteValueCFString, "user-specified name for missing domain", favoriteNameCFString.returnCFStringRef());
				}
			}
			CFRelease(namesInClass), namesInClass = nullptr;
		}
	}
	
	return result;
}// createAllPreferencesContextsFromDisk


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
Retrieves the context that stores default settings for
the specified class.  Returns true unless this fails.

(3.1)
*/
Boolean
getDefaultContext	(Quills::Prefs::Class		inClass,
					 My_ContextInterfacePtr&	outContextPtr)
{
	Boolean		result = true;
	
	
	outContextPtr = nullptr;
	switch (inClass)
	{
	case Quills::Prefs::_RESTORE_AT_LAUNCH:
		outContextPtr = &(gAutoSaveDefaultContext());
		break;
	
	case Quills::Prefs::FORMAT:
		outContextPtr = &(gFormatDefaultContext());
		break;
	
	case Quills::Prefs::GENERAL:
		outContextPtr = &(gGeneralDefaultContext());
		break;
	
	case Quills::Prefs::MACRO_SET:
		outContextPtr = &(gMacroSetDefaultContext());
		break;
	
	case Quills::Prefs::SESSION:
		outContextPtr = &(gSessionDefaultContext());
		break;
	
	case Quills::Prefs::TERMINAL:
		outContextPtr = &(gTerminalDefaultContext());
		break;
	
	case Quills::Prefs::TRANSLATION:
		outContextPtr = &(gTranslationDefaultContext());
		break;
	
	case Quills::Prefs::WORKSPACE:
		outContextPtr = &(gWorkspaceDefaultContext());
		break;
	
	default:
		// ???
		result = false;
		break;
	}
	
	// default contexts must be retained when they are first
	// constructed, so that another retain/release will not
	// destroy them
	if (nullptr != outContextPtr)
	{
		if (false == gMyContextRefLocks().isLocked(outContextPtr->selfRef))
		{
			Preferences_RetainContext(outContextPtr->selfRef);
			assert(gMyContextRefLocks().isLocked(outContextPtr->selfRef));
		}
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
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::FORMAT;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::FORMAT);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagAutoSetCursorColor:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagFontCharacterWidthMultiplier:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagFontSize:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						Float64* const		data = REINTERPRET_CAST(outDataPtr, Float64*);
						
						
						*data = inContextPtr->returnInteger(keyName);
						if (0 == *data)
						{
							// failed; make default
							*data = 12.0; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagTerminalColorCursorBackground:
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
						assert(kPreferences_DataTypeCFArrayRef == keyValueType);
						CFArrayRef		valueCFArray = inContextPtr->returnArrayCopy(keyName);
						
						
						if (nullptr == valueCFArray)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CGFloatRGBColor* const		data = REINTERPRET_CAST(outDataPtr, CGFloatRGBColor*);
							
							
							if (false == convertCFArrayToCGFloatRGBColor(valueCFArray, data))
							{
								// failed; make black
								data->red = data->green = data->blue = 0;
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFArray), valueCFArray = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalImageNormalBackground:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagFadeAlpha:
				case kPreferences_TagTerminalMarginLeft:
				case kPreferences_TagTerminalMarginRight:
				case kPreferences_TagTerminalMarginTop:
				case kPreferences_TagTerminalMarginBottom:
				case kPreferences_TagTerminalPaddingLeft:
				case kPreferences_TagTerminalPaddingRight:
				case kPreferences_TagTerminalPaddingTop:
				case kPreferences_TagTerminalPaddingBottom:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
				
				case kPreferences_TagTerminalMousePointerColor:
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							TerminalView_MousePointerColor*		storedValuePtr = REINTERPRET_CAST(outDataPtr, TerminalView_MousePointerColor*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("red"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_MousePointerColorRed;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("black"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_MousePointerColorBlack;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("white"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminalView_MousePointerColorWhite;
							}
							else
							{
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
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::GENERAL;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::GENERAL);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagBellSound:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagCaptureFileLineEndings:
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Session_LineEnding*		storedValuePtr = REINTERPRET_CAST(outDataPtr, Session_LineEnding*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("cr"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_LineEndingCR;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("lf"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_LineEndingLF;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("crlf"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_LineEndingCRLF;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagCopySelectedText:
				case kPreferences_TagCursorBlinks:
				case kPreferences_TagCursorMovesPriorToDrops:
				case kPreferences_TagDontAutoClose:
				case kPreferences_TagDontAutoNewOnApplicationReopen:
				case kPreferences_TagDontDimBackgroundScreens:
				case kPreferences_TagFadeBackgroundWindows:
				case kPreferences_TagFocusFollowsMouse:
				case kPreferences_TagHeadersCollapsed:
				case kPreferences_TagPureInverse:
				case kPreferences_TagRandomTerminalFormats:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagCopyTableThreshold:
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
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
				
				case kPreferences_TagKioskAllowsForceQuit:
				case kPreferences_TagKioskShowsMenuBar:
				case kPreferences_TagKioskShowsScrollBar:
				case kPreferences_TagKioskShowsWindowFrame:
				case kPreferences_TagNoAnimations:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagMapBackquote:
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagTerminalShowMarginAtColumn:
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						UInt16*		outUInt16Ptr = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						*outUInt16Ptr = STATIC_CAST(inContextPtr->returnInteger(keyName), UInt16);
						if (*outUInt16Ptr == 0)
						{
							// failed; make default
							*outUInt16Ptr = 0; // 0 means “off”
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagNewCommandShortcutEffect:
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							SessionFactory_SpecialSession*		storedValuePtr = REINTERPRET_CAST(outDataPtr, SessionFactory_SpecialSession*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("shell"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSessionFactory_SpecialSessionShell;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("log-in shell"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSessionFactory_SpecialSessionLogInShell;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("dialog"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSessionFactory_SpecialSessionInteractiveSheet;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("default"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSessionFactory_SpecialSessionDefaultFavorite;
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
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("ignore"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlertMessages_NotificationTypeDoNothing;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("badge"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlertMessages_NotificationTypeMarkDockIcon;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("animate"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlertMessages_NotificationTypeMarkDockIconAndBounceOnce;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("alert"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kAlertMessages_NotificationTypeMarkDockIconAndBounceRepeatedly;
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
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Terminal_CursorType*	storedValuePtr = REINTERPRET_CAST(outDataPtr, Terminal_CursorType*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("block"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_CursorTypeBlock;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("underline"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_CursorTypeUnderscore;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("thick underline"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_CursorTypeThickUnderscore;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("vertical bar"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_CursorTypeVerticalLine;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("thick vertical bar"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTerminal_CursorTypeThickVerticalLine;
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
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagWindowStackingOrigin:
					{
						assert(kPreferences_DataTypeCFArrayRef == keyValueType);
						CFArrayRef		valueCFArray = inContextPtr->returnArrayCopy(keyName);
						
						
						if (nullptr == valueCFArray)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFNumberRef		singleCoord = nullptr;
							CGPoint*			outPointPtr = REINTERPRET_CAST(outDataPtr, CGPoint*);
							
							
							singleCoord = CFUtilities_NumberCast(CFArrayGetValueAtIndex(valueCFArray, 0));
							unless (CFNumberGetValue(singleCoord, kCFNumberCGFloatType, &outPointPtr->x))
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							singleCoord = CFUtilities_NumberCast(CFArrayGetValueAtIndex(valueCFArray, 1));
							unless (CFNumberGetValue(singleCoord, kCFNumberCGFloatType, &outPointPtr->y))
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							
							// set reasonable X value if there is an error
							if (outPointPtr->x <= 0)
							{
								outPointPtr->x = 40;
							}
							
							// set reasonable Y value if there is an error
							if (outPointPtr->y <= 0)
							{
								outPointPtr->y = 40;
							}
							
							CFRelease(valueCFArray), valueCFArray = nullptr;
						}
					}
					break;
				
				case kPreferences_TagWindowTabPreferredEdge:
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CGRectEdge*		storedValuePtr = REINTERPRET_CAST(outDataPtr, CGRectEdge*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("left"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = CGRectMinXEdge;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("right"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = CGRectMaxXEdge;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("bottom"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = CGRectMinYEdge;
							}
							else
							{
								*storedValuePtr = CGRectMaxYEdge;
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
}// getGeneralPreference


/*!
Retrieves the list that stores any in-memory settings for the
specified class (only works for classes that can be collections).
Returns true unless this fails.

See also getMutableListOfContexts() and
Preferences_CreateContextNameArray().

(3.1)
*/
Boolean
getListOfContexts	(Quills::Prefs::Class				inClass,
					 My_FavoriteContextList const*&		outListPtr)
{
	// reuse the implementation of the method that returns a mutable pointer
	My_FavoriteContextList*		mutablePtr = nullptr;
	Boolean						result = getMutableListOfContexts(inClass, mutablePtr);
	
	
	outListPtr = mutablePtr;
	return result;
}// getListOfContexts


/*!
Returns preference data for a setting for a particular macro.

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
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::MACRO_SET;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::MACRO_SET);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				Preferences_Tag const		kTagWithoutIndex = Preferences_ReturnTagFromVariant(inDataPreferenceTag);
				
				
				switch (kTagWithoutIndex)
				{
				case kPreferences_TagIndexedMacroAction:
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					{
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							UInt32*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt32*);
							
							
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
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("select matching window"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionSelectMatchingWindow;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("find verbatim"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionFindTextVerbatim;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("find with substitutions"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kMacroManager_ActionFindTextProcessingEscapes;
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
							UInt32*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt32*);
							UInt16		keyCode = 0;
							Boolean		isVirtualKey = false;
							
							
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
						assert(kPreferences_DataTypeCFArrayRef == keyValueType);
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
									*data |= kMacroManager_ModifierKeyMaskCommand;
								}
								else if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("control"),
																				kCFCompareCaseInsensitive))
								{
									*data |= kMacroManager_ModifierKeyMaskControl;
								}
								else if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("option"),
																				kCFCompareCaseInsensitive))
								{
									*data |= kMacroManager_ModifierKeyMaskOption;
								}
								else if (kCFCompareEqualTo == CFStringCompare(modifierNameCFString, CFSTR("shift"),
																				kCFCompareCaseInsensitive))
								{
									*data |= kMacroManager_ModifierKeyMaskShift;
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
Retrieves a modifiable version of the list that stores any in-memory
settings for the specified class (only works for classes that can be
collections).  Returns true unless this fails.

IMPORTANT:	Unless you have a reason to modify the list, prefer
			the read-only accessor "getListOfContexts()".  The
			presence of a pointer in this list has implications on
			its external reference’s release behavior.  You should
			rely on Preferences_ContextDeleteFromFavorites() and
			Preferences_ContextRepositionRelativeToContext() for
			list changes.

(4.1)
*/
Boolean
getMutableListOfContexts	(Quills::Prefs::Class			inClass,
							 My_FavoriteContextList*&		outListPtr)
{
	Boolean		result = true;
	
	
	outListPtr = nullptr;
	switch (inClass)
	{
	case Quills::Prefs::_RESTORE_AT_LAUNCH:
		outListPtr = &gAutoSaveNamedContexts();
		break;
	
	case Quills::Prefs::FORMAT:
		outListPtr = &gFormatNamedContexts();
		break;
	
	case Quills::Prefs::MACRO_SET:
		outListPtr = &gMacroSetNamedContexts();
		break;
	
	case Quills::Prefs::SESSION:
		outListPtr = &gSessionNamedContexts();
		break;
	
	case Quills::Prefs::TERMINAL:
		outListPtr = &gTerminalNamedContexts();
		break;
	
	case Quills::Prefs::TRANSLATION:
		outListPtr = &gTranslationNamedContexts();
		break;
	
	case Quills::Prefs::WORKSPACE:
		outListPtr = &gWorkspaceNamedContexts();
		break;
	
	case Quills::Prefs::GENERAL:
	default:
		// ???
		result = false;
		break;
	}
	
	return result;
}// getMutableListOfContexts


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
getNamedContext		(Quills::Prefs::Class		inClass,
					 CFStringRef				inName,
					 My_ContextFavoritePtr&		outContextPtr)
{
	Boolean		result = false;
	
	
	outContextPtr = nullptr;
	if ((nullptr != inName) && (CFStringGetLength(inName) > 0))
	{
		My_FavoriteContextList const*	listPtr = nullptr;
		
		
		if (getListOfContexts(inClass, listPtr))
		{
			auto	toContextPtr = std::find_if(listPtr->begin(), listPtr->end(),
												contextNameEqualTo(inName));
			
			
			if (listPtr->end() != toContextPtr)
			{
				outContextPtr = *toContextPtr;
				result = true;
			}
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
will be "Quills::Prefs::GENERAL".  In this case ONLY, you may
use Core Foundation Preferences to store or retrieve the key
value directly in the application’s globals.  However, keys
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
						 Quills::Prefs::Class&	outClass)
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
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::SESSION;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::SESSION);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagAssociatedFormatFavoriteLightMode:
				case kPreferences_TagAssociatedFormatFavoriteDarkMode:
				case kPreferences_TagAssociatedMacroSetFavorite:
				case kPreferences_TagAssociatedTerminalFavorite:
				case kPreferences_TagAssociatedTranslationFavorite:
				case kPreferences_TagCaptureFileName:
				case kPreferences_TagServerHost:
				case kPreferences_TagServerUserID:
					// all of these keys have Core Foundation string values
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagBackgroundNewDataHandler:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Session_Watch*		storedValuePtr = REINTERPRET_CAST(outDataPtr, Session_Watch*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("notify"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_WatchForPassiveData;
							}
							else
							{
								*storedValuePtr = kSession_WatchNothing;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagCaptureAutoStart:
				case kPreferences_TagCaptureFileNameAllowsSubstitutions:
				case kPreferences_TagLineModeEnabled:
				case kPreferences_TagLocalEchoEnabled:
				case kPreferences_TagNoPasteWarning:
				case kPreferences_TagPasteAllowBracketedMode:
				case kPreferences_TagTektronixPAGEClearsScreen:
					// all of these keys have Core Foundation Boolean values
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagCaptureFileDirectoryURL:
					{
						assert(kPreferences_DataTypeCFDataRef == keyValueType);
						CFRetainRelease				dataObject(inContextPtr->returnValueCopy(keyName),
																CFRetainRelease::kAlreadyRetained);
						Preferences_URLInfo* const	data = REINTERPRET_CAST(outDataPtr, Preferences_URLInfo*);
						
						
						(*data).urlRef = nullptr;
						(*data).isStale = false;
						
						if (false == dataObject.exists())
						{
							// failed
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							CFDataRef						objectAsData = dataObject.returnCFDataRef();
							CFURLBookmarkResolutionOptions	resolveOptions = (kCFBookmarkResolutionWithoutUIMask/* |
																				kCFBookmarkResolutionWithoutMountingMask*/);
							CFErrorRef						errorCFObject = nullptr;
							Boolean							isStale = false;
							CFURLRef						newDirectoryURL = CFURLCreateByResolvingBookmarkData
																				(kCFAllocatorDefault, objectAsData,
																					resolveOptions, nullptr/* relative-URL info */,
																					nullptr/* properties to include */,
																					&isStale, &errorCFObject);
							
							
							if (nullptr == newDirectoryURL)
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
								if (nullptr != errorCFObject)
								{
									CFRetainRelease		errorReleaser(errorCFObject, CFRetainRelease::kAlreadyRetained);
									
									
									Console_Warning(Console_WriteValueCFError, "unable to create capture-file directory URL from bookmark data, error", errorCFObject);
								}
							}
							else
							{
								(*data).urlRef = newDirectoryURL;
								(*data).isStale = isStale;
								// do not release because the URL is returned
							}
						}
					}
					break;
				
				case kPreferences_TagCommandLine:
					{
						assert(kPreferences_DataTypeCFArrayRef == keyValueType);
						CFArrayRef* const	data = REINTERPRET_CAST(outDataPtr, CFArrayRef*);
						
						
						*data = inContextPtr->returnArrayCopy(keyName);
						if (nullptr == *data)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
					}
					break;
				
				case kPreferences_TagFunctionKeyLayout:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Session_FunctionKeyLayout*		storedValuePtr = REINTERPRET_CAST(outDataPtr, Session_FunctionKeyLayout*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("xterm"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_FunctionKeyLayoutXTerm;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("xterm-xfree86"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_FunctionKeyLayoutXTermXFree86;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("rxvt"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_FunctionKeyLayoutRxvt;
							}
							else
							{
								*storedValuePtr = kSession_FunctionKeyLayoutVT220;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagIdleAfterInactivityHandler:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Session_Watch*		storedValuePtr = REINTERPRET_CAST(outDataPtr, Session_Watch*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("notify"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_WatchForInactivity;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("keep-alive"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_WatchForKeepAlive;
							}
							else
							{
								*storedValuePtr = kSession_WatchNothing;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagIdleAfterInactivityInSeconds:
				case kPreferences_TagKeepAlivePeriodInMinutes:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
							
							
							// convert to invisible character, e.g. 'C' should become control-C
							*data = STATIC_CAST(CFStringGetCharacterAtIndex(keystrokeCFString, 1), char) - '@';
							CFRelease(keystrokeCFString), keystrokeCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagEmacsMetaKey:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							// values will be "Session_EmacsMetaKey"
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR(""), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_EmacsMetaKeyOff;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("shift+option"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_EmacsMetaKeyShiftOption;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("option"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_EmacsMetaKeyOption;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("control+command"), kCFCompareCaseInsensitive))
							{
								// LEGACY VALUE; remap to require Shift and Option
								*storedValuePtr = kSession_EmacsMetaKeyShiftOption;
							}
							else
							{
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagMapArrowsForEmacs:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagMapDeleteToBackspace:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagNewLineMapping:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							// values will be "Session_NewlineMode"
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("\\015"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_NewlineModeMapCR;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("\\015\\012"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_NewlineModeMapCRLF;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("\\015\\000"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_NewlineModeMapCRNull;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("\\012"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kSession_NewlineModeMapLF;
							}
							else
							{
								*storedValuePtr = kSession_NewlineModeMapLF;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagPasteNewLineDelay:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						SInt16								valueInteger = inContextPtr->returnInteger(keyName);
						Preferences_TimeInterval* const		data = REINTERPRET_CAST(outDataPtr, Preferences_TimeInterval*);
						
						
						*data = STATIC_CAST(valueInteger, Preferences_TimeInterval) * kPreferences_TimeIntervalMillisecond;
					}
					break;
				
				case kPreferences_TagScrollDelay:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						SInt16								valueInteger = inContextPtr->returnInteger(keyName);
						Preferences_TimeInterval* const		data = REINTERPRET_CAST(outDataPtr, Preferences_TimeInterval*);
						
						
						*data = STATIC_CAST(valueInteger, Preferences_TimeInterval) * kPreferences_TimeIntervalMillisecond;
						if (*data > 0.050/* arbitrary */)
						{
							// refuse to honor very long scroll delays
							*data = 0;
						}
					}
					break;
				
				case kPreferences_TagServerPort:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("sftp"), kCFCompareCaseInsensitive))
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
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
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::TERMINAL;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::TERMINAL);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagDataReceiveDoNotStripHighBit:
				case kPreferences_TagITermGraphicsEnabled:
				case kPreferences_TagSixelGraphicsEnabled:
				case kPreferences_TagTerminal24BitColorEnabled:
				case kPreferences_TagTerminalClearSavesLines:
				case kPreferences_TagTerminalLineWrap:
				case kPreferences_TagVT100FixLineWrappingBug:
				case kPreferences_TagXTerm256ColorsEnabled:
				case kPreferences_TagXTermBackgroundColorEraseEnabled:
				case kPreferences_TagXTermColorEnabled:
				case kPreferences_TagXTermGraphicsEnabled:
				case kPreferences_TagXTermWindowAlterationEnabled:
					// all of these keys have Core Foundation Boolean values
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				case kPreferences_TagMapKeypadTopRowForVT220:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							Emulation_FullType*		storedValuePtr = REINTERPRET_CAST(outDataPtr, Emulation_FullType*);
							
							
							*storedValuePtr = Terminal_EmulatorReturnForName(valueCFString);
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTerminalScreenColumns:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						SInt32			valueInteger = inContextPtr->returnLong(keyName);
						UInt32* const	data = REINTERPRET_CAST(outDataPtr, UInt32*);
						
						
						*data = STATIC_CAST(valueInteger, UInt32);
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
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
						CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
						
						
						if (nullptr == valueCFString)
						{
							result = kPreferences_ResultBadVersionDataNotAvailable;
						}
						else
						{
							// values will be "Terminal_ScrollbackType"
							UInt16*		storedValuePtr = REINTERPRET_CAST(outDataPtr, UInt16*);
							
							
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
				
				case kPreferences_TagXTermReportedPatchLevel:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						SInt32			valueInteger = inContextPtr->returnInteger(keyName);
						UInt16* const	data = REINTERPRET_CAST(outDataPtr, UInt16*);
						
						
						*data = STATIC_CAST(valueInteger, UInt16);
						if (0 == *data)
						{
							// failed; make default
							*data = 95; // arbitrary (minimum XFree86 patch level that XTerm uses)
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
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::TRANSLATION;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::TRANSLATION);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagBackupFontName:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
				
				case kPreferences_TagTextEncodingIANAName:
					{
						assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
Returns preference data for a setting for a particular window
of a workspace.

\retval kPreferences_ResultOK
if the data was found successfully

\retval kPreferences_ResultBadVersionDataNotAvailable
if the requested data is not in the preferences file

\retval kPreferences_ResultInsufficientBufferSpace
if the given buffer is not large enough for the requested data

\retval kPreferences_ResultUnknownTagOrClass
if the given preference tag is not valid

(4.0)
*/
Preferences_Result	
getWorkspacePreference	(My_ContextInterfaceConstPtr	inContextPtr,
						 Preferences_Tag				inDataPreferenceTag,
						 size_t							inDataSize,
						 void*							outDataPtr,
						 size_t*						outActualSizePtrOrNull)
{
	size_t					actualSize = 0L;
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	if (nullptr != outDataPtr)
	{
		CFStringRef				keyName = nullptr;
		FourCharCode			keyValueType = '----';
		Quills::Prefs::Class	dataClass = Quills::Prefs::WORKSPACE;
		
		
		result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
		if (kPreferences_ResultOK == result)
		{
			assert(dataClass == Quills::Prefs::WORKSPACE);
			if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
			else
			{
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagArrangeWindowsFullScreen:
				case kPreferences_TagArrangeWindowsUsingTabs:
					if (false == inContextPtr->exists(keyName))
					{
						result = kPreferences_ResultBadVersionDataNotAvailable;
					}
					else
					{
						assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
					}
					break;
				
				default:
					// all other settings are per-window
					{
						Preferences_Tag const	kTagWithoutIndex = Preferences_ReturnTagFromVariant
																	(inDataPreferenceTag);
						
						
						switch (kTagWithoutIndex)
						{
						case kPreferences_TagIndexedWindowCommandType:
							assert(kPreferences_DataTypeCFStringRef == keyValueType);
							{
								CFStringRef		valueCFString = inContextPtr->returnStringCopy(keyName);
								
								
								if (nullptr == valueCFString)
								{
									result = kPreferences_ResultBadVersionDataNotAvailable;
								}
								else
								{
									SessionFactory_SpecialSession*		storedValuePtr = REINTERPRET_CAST(outDataPtr, SessionFactory_SpecialSession*);
									
									
									if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("shell"), kCFCompareCaseInsensitive))
									{
										*storedValuePtr = kSessionFactory_SpecialSessionShell;
									}
									else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("log-in shell"), kCFCompareCaseInsensitive))
									{
										*storedValuePtr = kSessionFactory_SpecialSessionLogInShell;
									}
									else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("dialog"), kCFCompareCaseInsensitive))
									{
										*storedValuePtr = kSessionFactory_SpecialSessionInteractiveSheet;
									}
									else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("default"), kCFCompareCaseInsensitive))
									{
										*storedValuePtr = kSessionFactory_SpecialSessionDefaultFavorite;
									}
									else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("none"), kCFCompareCaseInsensitive))
									{
										*storedValuePtr = 0;
									}
									else
									{
										result = kPreferences_ResultBadVersionDataNotAvailable;
									}
									CFRelease(valueCFString), valueCFString = nullptr;
								}
							}
							break;
						
						case kPreferences_TagIndexedWindowFrameBounds:
						case kPreferences_TagIndexedWindowScreenBounds:
							{
								assert(kPreferences_DataTypeCFArrayRef == keyValueType);
								CFArrayRef		valueCFArray = inContextPtr->returnArrayCopy(keyName);
								
								
								if (nullptr == valueCFArray)
								{
									result = kPreferences_ResultBadVersionDataNotAvailable;
								}
								else
								{
									Preferences_TopLeftCGRect* const	data = REINTERPRET_CAST(outDataPtr, Preferences_TopLeftCGRect*);
									
									
									if (false == convertCFArrayToHIRect(valueCFArray, *data))
									{
										// failed; make empty
										data->origin.x = data->origin.y = 0;
										data->size.width = data->size.height = 0;
										result = kPreferences_ResultBadVersionDataNotAvailable;
									}
									if ((0 == data->origin.x) && (0 == data->origin.y) && (0 == data->size.width) && (0 == data->size.height))
									{
										// this is also indicative of a problem
										result = kPreferences_ResultBadVersionDataNotAvailable;
									}
									CFRelease(valueCFArray), valueCFArray = nullptr;
								}
							}
							break;
						
						case kPreferences_TagIndexedWindowSessionFavorite:
						case kPreferences_TagIndexedWindowTitle:
							// all of these keys have Core Foundation string values
							{
								assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
						
						default:
							// unrecognized tag
							result = kPreferences_ResultUnknownTagOrClass;
							break;
						}
					}
					break;
				}
			}
		}
	}
	if (nullptr != outActualSizePtrOrNull) *outActualSizePtrOrNull = actualSize;
	return result;
}// getWorkspacePreference


/*!
Adds default values for known preference keys.  Existing
values are kept and any other defaults are added.

The keys "name" and "name-string" are special, reserved
for named collections and they should never be present
in the global defaults.  They are removed if they exist.

\retval noErr
if successful

\retval some (probably resource-related) OS error
if some component could not be set up properly

(2.6)
*/
Boolean
mergeInDefaultPreferences ()
{
	CFRetainRelease		defaultPrefDictionary(copyDefaultPrefDictionary(), CFRetainRelease::kAlreadyRetained);
	Boolean				result = false;
	
	
	// copy bundle's DefaultPreferences.plist
	if (defaultPrefDictionary.exists())
	{
		// read the default preferences dictionary; for settings
		// saved in XML, this will also make CFPreferences calls
		// to register (in memory only) the appropriate updates;
		// other values are written to the specified handles,
		// where the data must be extracted for saving elsewhere
		result = readPreferencesDictionary(defaultPrefDictionary.returnCFDictionaryRef(), true/* merge */);
		
		// also explicitly delete name keys if they exist globally
		// (they should not)
		setApplicationPreference(CFSTR("name"), nullptr);
		setApplicationPreference(CFSTR("name-string"), nullptr);
		
		if (result)
		{
			// save the preferences...
			UNUSED_RETURN(Boolean)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
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
overwriteClassDomainCFArray		(Quills::Prefs::Class	inClass,
								 CFArrayRef				inCFArrayOfCFStrings)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// figure out which main application preferences key should hold this list of domains
	switch (inClass)
	{
	case Quills::Prefs::_RESTORE_AT_LAUNCH:
		setApplicationPreference(CFSTR("auto-save"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::FORMAT:
		setApplicationPreference(CFSTR("favorite-formats"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::MACRO_SET:
		setApplicationPreference(CFSTR("favorite-macro-sets"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::SESSION:
		setApplicationPreference(CFSTR("favorite-sessions"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::TERMINAL:
		setApplicationPreference(CFSTR("favorite-terminals"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::TRANSLATION:
		setApplicationPreference(CFSTR("favorite-translations"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::WORKSPACE:
		setApplicationPreference(CFSTR("favorite-workspaces"), inCFArrayOfCFStrings);
		break;
	
	case Quills::Prefs::GENERAL:
	default:
		// ???
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// overwriteClassDomainCFArray


/*!
Reads the main application’s preferences as requested.
Also prints to standard output information about the
data being read (if debugging is enabled).

The resultant Core Foundation type must be released
when you are finished using it.

IMPORTANT:	Do not use for keys that do not really
			have array values.

(3.1)
*/
void
readApplicationArrayPreference	(CFStringRef	inKey,
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
			size_t const	kBufferSize = INTEGER_TIMES_2(CFStringGetLength(inKey));
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
				size_t const	kBufferSize = INTEGER_TIMES_2(CFStringGetLength(descriptionCFString));
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
}// readApplicationArrayPreference


/*!
Updates stored preference values using master preferences from
the given dictionary.  If merging, conflicting keys are
skipped; otherwise, they are replaced with the new dictionary
values.

All of the keys in the specified dictionary must be of type
CFStringRef.

\retval true
currently always returned; there is no way to detect errors

(3.0)
*/
Boolean
readPreferencesDictionary	(CFDictionaryRef	inPreferenceDictionary,
							 Boolean			inMerge)
{
	My_ContextInterfacePtr		contextPtr = nullptr;
	Boolean						gotContext = getDefaultContext(Quills::Prefs::GENERAL, contextPtr);
	
	
	assert(gotContext);
	return readPreferencesDictionaryInContext(contextPtr, inPreferenceDictionary, inMerge,
												nullptr/* inferred class */, nullptr/* inferred name */);
}// readPreferencesDictionary


/*!
Updates stored preference values in the specified context, using
master preferences from the given dictionary.  If merging,
conflicting keys are skipped; otherwise, they are replaced with
the new dictionary values.

You can request that the class of the data be inferred by passing
a value other than nullptr for "outInferredClassOrNull".  If you
do, the preference definitions for all keys in the source
dictionary are checked and the dominant class is returned.  So
for example, if the dictionary contains primarily font and color
data, the inferred class would be Quills::Prefs::FORMAT.  This
can be useful for constructing a saved context of the appropriate
class.

The "name-string" key has special significance because it stores
the name of a managed context.  If the source dictionary contains
a name, it is returned in "outInferredNameOrNull" and is NOT
automatically stored as a regular key-value pair.  (Usually you
should call Preferences_ContextRename() on your final target
context with the inferred name.)  A new string is allocated for
an inferred name so you must call CFRelease() when finished.

All of the keys in the specified dictionary must be of type
CFStringRef.

\retval true
currently always returned; there is no way to detect errors

(4.0)
*/
Boolean
readPreferencesDictionaryInContext	(My_ContextInterfacePtr		inContextPtr,
									 CFDictionaryRef			inPreferenceDictionary,
									 Boolean					inMerge,
									 Quills::Prefs::Class*		outInferredClassOrNull,
									 CFStringRef*				outInferredNameOrNull)
{
	// the keys can have their values copied directly into the application
	// preferences property list; they are identical in format and even use
	// the same key names
	typedef std::map< Quills::Prefs::Class, UInt16 >	CountByClass;
	CFIndex const		kDictLength = CFDictionaryGetCount(inPreferenceDictionary);
	CFIndex				i = 0;
	void const*			keyValue = nullptr;
	void const**		keys = new void const*[kDictLength];
	CountByClass		classPopularity; // keep track of keys belonging to each class
	CFPropertyListRef	nameDataValue = nullptr;
	CFPropertyListRef	nameStringValue = nullptr;
	Boolean				result = true;
	
	
	CFDictionaryGetKeysAndValues(inPreferenceDictionary, keys, nullptr/* values */);
	for (i = 0; i < kDictLength; ++i)
	{
		CFStringRef const	kKey = CFUtilities_StringCast(keys[i]);
		Boolean				useKey = true;
		
		
		// check for special name keys; do not overwrite these
		if (kCFCompareEqualTo == CFStringCompare(kKey, CFSTR("name"), 0/* options */))
		{
			useKey = false;
			UNUSED_RETURN(Boolean)CFDictionaryGetValueIfPresent(inPreferenceDictionary, CFSTR("name"), &nameDataValue);
		}
		else if (kCFCompareEqualTo == CFStringCompare(kKey, CFSTR("name-string"), 0/* options */))
		{
			useKey = false;
			UNUSED_RETURN(Boolean)CFDictionaryGetValueIfPresent(inPreferenceDictionary, CFSTR("name-string"), &nameStringValue);
		}
		
		if ((useKey) && (inMerge))
		{
			// when merging, do not replace any key that is already defined
			CFPropertyListRef	foundValue = inContextPtr->returnValueCopy(kKey);
			
			
			if (nullptr != foundValue)
			{
				useKey = false;
				CFRelease(foundValue), foundValue = nullptr;
			}
			else
			{
				// value is not yet defined
				useKey = true;
			}
		}
		
		if (useKey)
		{
			if (CFDictionaryGetValueIfPresent(inPreferenceDictionary, kKey, &keyValue))
			{
				My_PreferenceDefinition*	definitionPtr = My_PreferenceDefinition::findByKeyName(kKey);
				
				
				if (nullptr == definitionPtr)
				{
					static Preferences_Index	gDummyIndex = 1;
					Preferences_Tag				dummyTag = Preferences_ReturnTagVariantForIndex('DUM\0', gDummyIndex++);
					
					
					if (false == My_PreferenceDefinition::isValidKeyName(kKey))
					{
						// only emit a warning for key names that are completely unexpected; either way, register anonymously
						Console_Warning(Console_WriteValueCFString, "using anonymous tag for unknown preference dictionary key", kKey);
					}
					inContextPtr->addValue(dummyTag, kKey, keyValue);
				}
				else
				{
					//Console_WriteValueCFString("using given dictionary value for key", kKey); // debug
					assert(kCFCompareEqualTo == CFStringCompare(kKey, definitionPtr->keyName.returnCFStringRef(), 0/* options */));
					inContextPtr->addValue(definitionPtr->tag, definitionPtr->keyName.returnCFStringRef(), keyValue);
					++(classPopularity[definitionPtr->preferenceClass]);
				}
			}
		}
	}
	delete [] keys;
	
	// check for an established name for the context
	if ((nullptr != outInferredNameOrNull) && ((nullptr != nameDataValue) || (nullptr != nameStringValue)))
	{
		*outInferredNameOrNull = copyUserSpecifiedName(CFUtilities_DataCast(nameDataValue),
														CFUtilities_StringCast(nameStringValue));
	}
	
	// check for the dominant class of these settings
	if (nullptr != outInferredClassOrNull)
	{
		UInt16	highestPopularity = 0;
		
		
		*outInferredClassOrNull = Quills::Prefs::GENERAL;
		for (auto classCountPair : classPopularity)
		{
			if (classCountPair.second > highestPopularity)
			{
				*outInferredClassOrNull = classCountPair.first;
				highestPopularity = classCountPair.second;
			}
		}
	}
	
	return result;
}// readPreferencesDictionaryInContext


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
	Quills::Prefs::Class	dataClass = Quills::Prefs::FORMAT;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::FORMAT);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagAutoSetCursorColor:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagFontCharacterWidthMultiplier:
				{
					Float32 const* const	data = REINTERPRET_CAST(inDataPtr, Float32 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addFloat(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagFontName:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagFontSize:
				{
					Float64 const* const	data = REINTERPRET_CAST(inDataPtr, Float64 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalColorCursorBackground:
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
					CGFloatRGBColor const* const	data = REINTERPRET_CAST(inDataPtr, CGFloatRGBColor const*);
					CFArrayRef						colorCFArray = nullptr;
					
					
					if (convertCGFloatRGBColorToCFArray(data, colorCFArray))
					{
						assert(kPreferences_DataTypeCFArrayRef == keyValueType);
						inContextPtr->addArray(inDataPreferenceTag, keyName, colorCFArray);
						CFRelease(colorCFArray), colorCFArray = nullptr;
					}
					else result = kPreferences_ResultGenericFailure;
				}
				break;
			
			case kPreferences_TagTerminalImageNormalBackground:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagFadeAlpha:
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
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addFloat(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagTerminalMousePointerColor:
				{
					TerminalView_MousePointerColor const	data = *(REINTERPRET_CAST(inDataPtr, TerminalView_MousePointerColor const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case kTerminalView_MousePointerColorBlack:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("black"));
						break;
					
					case kTerminalView_MousePointerColorWhite:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("white"));
						break;
					
					case kTerminalView_MousePointerColorRed:
					default:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("red"));
						break;
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
	Quills::Prefs::Class	dataClass = Quills::Prefs::GENERAL;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::GENERAL);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagBellSound:
				{
					CFStringRef const	data = *(REINTERPRET_CAST(inDataPtr, CFStringRef const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					setApplicationPreference(keyName, data);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagCaptureFileLineEndings:
				{
					Session_LineEnding const	data = *(REINTERPRET_CAST(inDataPtr, Session_LineEnding const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case kSession_LineEndingCR:
						setApplicationPreference(keyName, CFSTR("cr"));
						break;
					
					case kSession_LineEndingCRLF:
						setApplicationPreference(keyName, CFSTR("crlf"));
						break;
					
					case kSession_LineEndingLF:
					default:
						setApplicationPreference(keyName, CFSTR("lf"));
						break;
					}
				}
				break;
			
			case kPreferences_TagCopySelectedText:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagCopyTableThreshold:
				{
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					CFNumberRef		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &data);
					
					
					if (nullptr != numberRef)
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						setApplicationPreference(keyName, numberRef);
						CFRelease(numberRef), numberRef = nullptr;
					}
				}
				break;
			
			case kPreferences_TagCursorBlinks:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagCursorMovesPriorToDrops:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagDontAutoClose:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagDontAutoNewOnApplicationReopen:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagDontDimBackgroundScreens:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagFadeBackgroundWindows:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagFocusFollowsMouse:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagHeadersCollapsed:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagKioskAllowsForceQuit:
			case kPreferences_TagKioskShowsMenuBar:
			case kPreferences_TagKioskShowsScrollBar:
			case kPreferences_TagKioskShowsWindowFrame:
			case kPreferences_TagNoAnimations:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagMapBackquote:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					setApplicationPreference(keyName, (data) ? CFSTR("\\e") : CFSTR(""));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagNewCommandShortcutEffect:
				{
					UInt32 const	data = *(REINTERPRET_CAST(inDataPtr, UInt32 const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case kSessionFactory_SpecialSessionInteractiveSheet:
						setApplicationPreference(keyName, CFSTR("dialog"));
						break;
					
					case kSessionFactory_SpecialSessionDefaultFavorite:
						setApplicationPreference(keyName, CFSTR("default"));
						break;
					
					case kSessionFactory_SpecialSessionShell:
						setApplicationPreference(keyName, CFSTR("shell"));
						break;
					
					case kSessionFactory_SpecialSessionLogInShell:
					default:
						setApplicationPreference(keyName, CFSTR("log-in shell"));
						break;
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagNotification:
				{
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case kAlertMessages_NotificationTypeDoNothing:
						setApplicationPreference(keyName, CFSTR("ignore"));
						break;
					
					case kAlertMessages_NotificationTypeMarkDockIconAndBounceOnce:
						setApplicationPreference(keyName, CFSTR("animate"));
						break;
					
					case kAlertMessages_NotificationTypeMarkDockIconAndBounceRepeatedly:
						setApplicationPreference(keyName, CFSTR("alert"));
						break;
					
					case kAlertMessages_NotificationTypeMarkDockIcon:
					default:
						setApplicationPreference(keyName, CFSTR("badge"));
						break;
					}
					Alert_SetNotificationPreferences(data);
				}
				break;
			
			case kPreferences_TagNotifyOfBeeps:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					setApplicationPreference(keyName, (data) ? CFSTR("notify") : CFSTR("ignore"));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagPureInverse:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagRandomTerminalFormats:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagTerminalCursorType:
				{
					Terminal_CursorType const	data = *(REINTERPRET_CAST(inDataPtr, Terminal_CursorType const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case kTerminal_CursorTypeUnderscore:
						setApplicationPreference(keyName, CFSTR("underline"));
						break;
					
					case kTerminal_CursorTypeVerticalLine:
						setApplicationPreference(keyName, CFSTR("vertical bar"));
						break;
					
					case kTerminal_CursorTypeThickUnderscore:
						setApplicationPreference(keyName, CFSTR("thick underline"));
						break;
					
					case kTerminal_CursorTypeThickVerticalLine:
						setApplicationPreference(keyName, CFSTR("thick vertical bar"));
						break;
					
					case kTerminal_CursorTypeBlock:
					default:
						setApplicationPreference(keyName, CFSTR("block"));
						break;
					}
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagTerminalResizeAffectsFontSize:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					setApplicationPreference(keyName, (data) ? CFSTR("font") : CFSTR("screen"));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagTerminalShowMarginAtColumn:
				{
					UInt16 const	unsignedData = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					SInt16 const	data = STATIC_CAST(unsignedData, SInt16);
					CFNumberRef		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &data);
					
					
					if (nullptr != numberRef)
					{
						assert(kPreferences_DataTypeCFNumberRef == keyValueType);
						setApplicationPreference(keyName, numberRef);
						changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
						CFRelease(numberRef), numberRef = nullptr;
					}
				}
				break;
			
			case kPreferences_TagVisualBell:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					setApplicationPreference(keyName, (data) ? CFSTR("visual") : CFSTR("audio"));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagWasClipboardShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasCommandLineShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasControlKeypadShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasFunctionKeypadShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasSessionInfoShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWasVT220KeypadShowing:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					setApplicationPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
				}
				break;
			
			case kPreferences_TagWindowStackingOrigin:
				{
					CGPoint const	data = *(REINTERPRET_CAST(inDataPtr, CGPoint const*));
					CFNumberRef		leftCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType,
																&data.x);
					
					
					if (nullptr != leftCoord)
					{
						CFNumberRef		topCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType,
																	&data.y);
						
						
						if (nullptr != topCoord)
						{
							CFNumberRef		values[] = { leftCoord, topCoord };
							CFArrayRef		coords = CFArrayCreate(kCFAllocatorDefault,
																	REINTERPRET_CAST(values, void const**),
																	sizeof(values) / sizeof(CFNumberRef),
																	&kCFTypeArrayCallBacks);
							
							
							if (nullptr != coords)
							{
								assert(kPreferences_DataTypeCFArrayRef == keyValueType);
								setApplicationPreference(keyName, coords);
								CFRelease(coords), coords = nullptr;
							}
							CFRelease(topCoord), topCoord = nullptr;
						}
						CFRelease(leftCoord), leftCoord = nullptr;
					}
				}
				break;
			
			case kPreferences_TagWindowTabPreferredEdge:
				{
					CGRectEdge const	data = *(REINTERPRET_CAST(inDataPtr, CGRectEdge const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case CGRectMinXEdge:
						setApplicationPreference(keyName, CFSTR("left"));
						break;
					
					case CGRectMaxXEdge:
						setApplicationPreference(keyName, CFSTR("right"));
						break;
					
					case CGRectMinYEdge:
						setApplicationPreference(keyName, CFSTR("bottom"));
						break;
					
					case CGRectMaxYEdge:
					default:
						setApplicationPreference(keyName, CFSTR("top"));
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
}// setGeneralPreference


/*!
Modifies the indicated preference for a specific macro using
the given data (see Preferences.h and the definition of each
tag for comments on what data format is expected for each one).

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
	Quills::Prefs::Class	dataClass = Quills::Prefs::MACRO_SET;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::MACRO_SET);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			Preferences_Tag const		kTagWithoutIndex = Preferences_ReturnTagFromVariant(inDataPreferenceTag);
			
			
			switch (kTagWithoutIndex)
			{
			case kPreferences_TagIndexedMacroAction:
				{
					UInt32 const	data = *(REINTERPRET_CAST(inDataPtr, UInt32 const*));
					
					
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
					
					case kMacroManager_ActionSelectMatchingWindow:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("select matching window"));
						break;
					
					case kMacroManager_ActionFindTextVerbatim:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("find verbatim"));
						break;
					
					case kMacroManager_ActionFindTextProcessingEscapes:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("find with substitutions"));
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
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagIndexedMacroKey:
				{
					UInt32 const	data = *(REINTERPRET_CAST(inDataPtr, UInt32 const*));
					UInt16 const	kKeyCode = MacroManager_KeyIDKeyCode(data);
					Boolean const	kIsVirtualKey = MacroManager_KeyIDIsVirtualKey(data);
					
					
					if (kIsVirtualKey)
					{
						CFRetainRelease		virtualKeyName(virtualKeyCreateName(kKeyCode), CFRetainRelease::kAlreadyRetained);
						
						
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
																CFRetainRelease::kAlreadyRetained);
						
						
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
					CFStringRef		values[5/* arbitrary, WARNING this must be no smaller than the step count below! */];
					size_t			keysAdded = 0;
					
					
					if (data & kMacroManager_ModifierKeyMaskCommand)
					{
						values[keysAdded] = CFSTR("command");
						++keysAdded;
					}
					if (data & kMacroManager_ModifierKeyMaskControl)
					{
						values[keysAdded] = CFSTR("control");
						++keysAdded;
					}
					if (data & kMacroManager_ModifierKeyMaskOption)
					{
						values[keysAdded] = CFSTR("option");
						++keysAdded;
					}
					if (data & kMacroManager_ModifierKeyMaskShift)
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
							assert(kPreferences_DataTypeCFArrayRef == keyValueType);
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
Updates the main application’s preferences as requested.
Also prints to standard output information about the data
being written (if debugging is enabled).

(3.1)
*/
void
setApplicationPreference	(CFStringRef		inKey,
							 CFPropertyListRef	inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, kCFPreferencesCurrentApplication);
#if 1
	{
		// for debugging
		CFRetainRelease		descriptionObject(CFCopyDescription(inValue), CFRetainRelease::kAlreadyRetained);
		char const*			keyString = nullptr;
		char const*			descriptionString = nullptr;
		Boolean				disposeKeyString = false;
		Boolean				disposeDescriptionString = false;
		
		
		keyString = CFStringGetCStringPtr(inKey, CFStringGetFastestEncoding(inKey));
		if (nullptr == keyString)
		{
			// allocate double the size because the string length assumes 16-bit characters
			size_t const	kBufferSize = INTEGER_TIMES_2(CFStringGetLength(inKey));
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
		
		if (false == descriptionObject.exists())
		{
			descriptionString = "(no value description available)";
		}
		else
		{
			CFStringRef		descriptionCFString = descriptionObject.returnCFStringRef();
			
			
			descriptionString = CFStringGetCStringPtr
								(descriptionCFString, CFStringGetFastestEncoding(descriptionCFString));
			if (nullptr == descriptionString)
			{
				// allocate double the size because the string length assumes 16-bit characters
				size_t const	kBufferSize = INTEGER_TIMES_2(CFStringGetLength(descriptionCFString));
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
}// setApplicationPreference


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
	Quills::Prefs::Class	dataClass = Quills::Prefs::SESSION;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::SESSION);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagAssociatedFormatFavoriteLightMode:
			case kPreferences_TagAssociatedFormatFavoriteDarkMode:
			case kPreferences_TagAssociatedMacroSetFavorite:
			case kPreferences_TagAssociatedTerminalFavorite:
			case kPreferences_TagAssociatedTranslationFavorite:
			case kPreferences_TagCaptureFileName:
			case kPreferences_TagServerHost:
			case kPreferences_TagServerUserID:
				// all of these keys have Core Foundation string values
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagBackgroundNewDataHandler:
				{
					Session_Watch const* const		data = REINTERPRET_CAST(inDataPtr, Session_Watch const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (*data)
					{
					case kSession_WatchForPassiveData:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("notify"));
						break;
					
					case kSession_WatchForInactivity:
					case kSession_WatchForKeepAlive:
						// TEMPORARY; may want a new enumeration type
						assert(false && "incorrect preference value for background new-data handler");
						break;
					
					case kSession_WatchNothing:
					default:
						// ???
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR(""));
						break;
					}
				}
				break;
			
			case kPreferences_TagCaptureAutoStart:
			case kPreferences_TagCaptureFileNameAllowsSubstitutions:
			case kPreferences_TagLineModeEnabled:
			case kPreferences_TagLocalEchoEnabled:
			case kPreferences_TagNoPasteWarning:
			case kPreferences_TagPasteAllowBracketedMode:
			case kPreferences_TagTektronixPAGEClearsScreen:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagCaptureFileDirectoryURL:
				{
					// NOTE: the "isStale" field is used when retrieving the value
					// but is ignored here when storing a new value
					Preferences_URLInfo const* const	data = REINTERPRET_CAST(inDataPtr, Preferences_URLInfo const*);
					CFURLBookmarkCreationOptions		creationOptions = (kCFURLBookmarkCreationSuitableForBookmarkFile);
					CFErrorRef							errorCFObject = nullptr;
					CFRetainRelease						object(CFURLCreateBookmarkData
																(kCFAllocatorDefault, (*data).urlRef,
																	creationOptions,
																	nullptr/* properties to include */,
																	nullptr/* relative-URL info */,
																	&errorCFObject),
																CFRetainRelease::kAlreadyRetained);
					
					
					if (false == object.exists())
					{
						Console_Warning(Console_WriteLine, "unable to create capture-file directory bookmark from URL");
						Console_Warning(Console_WriteValueCFString, "problem URL", CFURLGetString((*data).urlRef));
						if (nullptr != errorCFObject)
						{
							CFRetainRelease		errorReleaser(errorCFObject, CFRetainRelease::kAlreadyRetained);
							
							
							Console_Warning(Console_WriteValueCFError, "unable to create bookmark data, error", errorCFObject);
						}
					}
					else
					{
						assert(kPreferences_DataTypeCFDataRef == keyValueType);
						inContextPtr->addData(inDataPreferenceTag, keyName,
												CFUtilities_DataCast(object.returnCFTypeRef()));
					}
				}
				break;
			
			case kPreferences_TagCommandLine:
				{
					CFArrayRef const* const		data = REINTERPRET_CAST(inDataPtr, CFArrayRef const*);
					
					
					assert(kPreferences_DataTypeCFArrayRef == keyValueType);
					inContextPtr->addArray(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagDataReadBufferSize:
			case kPreferences_TagServerPort:
				{
					SInt16 const* const		data = REINTERPRET_CAST(inDataPtr, SInt16 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagFunctionKeyLayout:
				{
					Session_FunctionKeyLayout const* const		data = REINTERPRET_CAST(inDataPtr, Session_FunctionKeyLayout const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (*data)
					{
					case kSession_FunctionKeyLayoutRxvt:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("rxvt"));
						break;
					
					case kSession_FunctionKeyLayoutXTerm:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("xterm"));
						break;
					
					case kSession_FunctionKeyLayoutXTermXFree86:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("xterm-xfree86"));
						break;
					
					case kSession_FunctionKeyLayoutVT220:
					default:
						// ???
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("vt220"));
						break;
					}
				}
				break;
			
			case kPreferences_TagIdleAfterInactivityHandler:
				{
					Session_Watch const* const		data = REINTERPRET_CAST(inDataPtr, Session_Watch const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (*data)
					{
					case kSession_WatchForInactivity:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("notify"));
						break;
					
					case kSession_WatchForKeepAlive:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("keep-alive"));
						break;
					
					case kSession_WatchForPassiveData:
						// TEMPORARY; may want a new enumeration type
						assert(false && "incorrect preference value for inactivity handler");
						break;
					
					case kSession_WatchNothing:
					default:
						// ???
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR(""));
						break;
					}
				}
				break;
			
			case kPreferences_TagIdleAfterInactivityInSeconds:
			case kPreferences_TagKeepAlivePeriodInMinutes:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
			
			case kPreferences_TagEmacsMetaKey:
				{
					// values will be "Session_EmacsMetaKey"
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (data)
					{
					case kSession_EmacsMetaKeyOption:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("option"));
						break;
					
					case kSession_EmacsMetaKeyShiftOption:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("shift+option"));
						break;
					
					case kSession_EmacsMetaKeyOff:
					default:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR(""));
						break;
					}
				}
				break;
			
			case kPreferences_TagMapArrowsForEmacs:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
			
			case kPreferences_TagMapDeleteToBackspace:
				{
					Boolean const* const	data = REINTERPRET_CAST(inDataPtr, Boolean const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, (*data) ? CFSTR("backspace") : CFSTR("delete"));
				}
				break;
			
			case kPreferences_TagNewLineMapping:
				{
					// values will be "Session_NewlineMode"
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (*data)
					{
					case kSession_NewlineModeMapCR:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("\\015"));
						break;
					
					case kSession_NewlineModeMapCRLF:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("\\015\\012"));
						break;
					
					case kSession_NewlineModeMapCRNull:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("\\015\\000"));
						break;
					
					case kSession_NewlineModeMapLF:
					default:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("\\012"));
						break;
					}
				}
				break;
			
			case kPreferences_TagPasteNewLineDelay:
				{
					Preferences_TimeInterval const* const		data = REINTERPRET_CAST(inDataPtr, Preferences_TimeInterval const*);
					Preferences_TimeInterval					junk = *data / kPreferences_TimeIntervalMillisecond;
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, STATIC_CAST(junk, SInt16));
				}
				break;
			
			case kPreferences_TagScrollDelay:
				{
					Preferences_TimeInterval const* const		data = REINTERPRET_CAST(inDataPtr, Preferences_TimeInterval const*);
					Preferences_TimeInterval					junk = *data / kPreferences_TimeIntervalMillisecond;
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, STATIC_CAST(junk, SInt16));
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			case kPreferences_TagServerProtocol:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					switch (*data)
					{
					case kSession_ProtocolSFTP:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("sftp"));
						break;
					
					case kSession_ProtocolSSH1:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("ssh-1"));
						break;
					
					case kSession_ProtocolSSH2:
						inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("ssh-2"));
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
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
	Quills::Prefs::Class	dataClass = Quills::Prefs::TERMINAL;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::TERMINAL);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagMapKeypadTopRowForVT220:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalEmulatorType:
				if (inDataSize >= sizeof(Emulation_FullType))
				{
					Emulation_FullType const	data = *(REINTERPRET_CAST(inDataPtr, Emulation_FullType const*));
					CFRetainRelease				nameCFString(Terminal_EmulatorReturnDefaultName(data),
																CFRetainRelease::kNotYetRetained);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					if (false == nameCFString.exists()) result = kPreferences_ResultGenericFailure;
					else
					{
						inContextPtr->addString(inDataPreferenceTag, keyName, nameCFString.returnCFStringRef());
					}
				}
				break;
			
			case kPreferences_TagTerminalScreenColumns:
			case kPreferences_TagTerminalScreenRows:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalScreenScrollbackRows:
				{
					UInt32 const* const		data = REINTERPRET_CAST(inDataPtr, UInt32 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addLong(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalScreenScrollbackType:
				{
					// values will be "Terminal_ScrollbackType"
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
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
			
			case kPreferences_TagDataReceiveDoNotStripHighBit:
			case kPreferences_TagITermGraphicsEnabled:
			case kPreferences_TagSixelGraphicsEnabled:
			case kPreferences_TagTerminal24BitColorEnabled:
			case kPreferences_TagTerminalClearSavesLines:
			case kPreferences_TagTerminalLineWrap:
			case kPreferences_TagVT100FixLineWrappingBug:
			case kPreferences_TagXTerm256ColorsEnabled:
			case kPreferences_TagXTermBackgroundColorEraseEnabled:
			case kPreferences_TagXTermColorEnabled:
			case kPreferences_TagXTermGraphicsEnabled:
			case kPreferences_TagXTermWindowAlterationEnabled:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagXTermReportedPatchLevel:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
					inContextPtr->addInteger(inDataPreferenceTag, keyName, *data);
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
	Quills::Prefs::Class	dataClass = Quills::Prefs::TRANSLATION;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::TRANSLATION);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagBackupFontName:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTextEncodingIANAName:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(kPreferences_DataTypeCFStringRef == keyValueType);
					inContextPtr->addString(inDataPreferenceTag, keyName, *data);
				}
				break;
			
			case kPreferences_TagTextEncodingID:
				{
					CFStringEncoding const		data = *(REINTERPRET_CAST(inDataPtr, CFStringEncoding const*));
					
					
					assert(kPreferences_DataTypeCFNumberRef == keyValueType);
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
Modifies the indicated preference for a specific workspace
window, using the given data (see Preferences.h and the
definition of each tag for comments on what data format is
expected for each one).

(4.0)
*/
Preferences_Result	
setWorkspacePreference	(My_ContextInterfacePtr		inContextPtr,
						 Preferences_Tag			inDataPreferenceTag,
						 size_t						inDataSize,
						 void const*				inDataPtr)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0L;
	Quills::Prefs::Class	dataClass = Quills::Prefs::WORKSPACE;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		assert(dataClass == Quills::Prefs::WORKSPACE);
		if (inDataSize < actualSize) result = kPreferences_ResultInsufficientBufferSpace;
		else
		{
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagArrangeWindowsFullScreen:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
				}
				break;
			
			case kPreferences_TagArrangeWindowsUsingTabs:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(kPreferences_DataTypeCFBooleanRef == keyValueType);
					inContextPtr->addFlag(inDataPreferenceTag, keyName, data);
					changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
				}
				break;
			
			default:
				// all other settings are per-window
				{
					Preferences_Tag const	kTagWithoutIndex = Preferences_ReturnTagFromVariant
																(inDataPreferenceTag);
					
					
					switch (kTagWithoutIndex)
					{
					case kPreferences_TagIndexedWindowCommandType:
						{
							UInt32 const	data = *(REINTERPRET_CAST(inDataPtr, UInt32 const*));
							
							
							assert(kPreferences_DataTypeCFStringRef == keyValueType);
							switch (data)
							{
							case kSessionFactory_SpecialSessionInteractiveSheet:
								inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("dialog"));
								break;
							
							case kSessionFactory_SpecialSessionDefaultFavorite:
								inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("default"));
								break;
							
							case kSessionFactory_SpecialSessionShell:
								inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("shell"));
								break;
							
							case kSessionFactory_SpecialSessionLogInShell:
								inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("log-in shell"));
								break;
							
							default:
								inContextPtr->addString(inDataPreferenceTag, keyName, CFSTR("none"));
								break;
							}
							changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
						}
						break;
					
					case kPreferences_TagIndexedWindowFrameBounds:
					case kPreferences_TagIndexedWindowScreenBounds:
						{
							Preferences_TopLeftCGRect const* const	data = REINTERPRET_CAST(inDataPtr, Preferences_TopLeftCGRect const*);
							CFArrayRef								boundsCFArray = nullptr;
							
							
							if (convertHIRectToCFArray(*data, boundsCFArray))
							{
								assert(kPreferences_DataTypeCFArrayRef == keyValueType);
								inContextPtr->addArray(inDataPreferenceTag, keyName, boundsCFArray);
								CFRelease(boundsCFArray), boundsCFArray = nullptr;
							}
							else result = kPreferences_ResultGenericFailure;
						}
						break;
					
					case kPreferences_TagIndexedWindowSessionFavorite:
						{
							CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
							
							
							assert(kPreferences_DataTypeCFStringRef == keyValueType);
							inContextPtr->addString(inDataPreferenceTag, keyName, *data);
							changeNotify(inDataPreferenceTag, inContextPtr->selfRef);
						}
						break;
					
					case kPreferences_TagIndexedWindowTitle:
						{
							CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
							
							
							assert(kPreferences_DataTypeCFStringRef == keyValueType);
							inContextPtr->addString(inDataPreferenceTag, keyName, *data);
						}
						break;
					
					default:
						// unrecognized tag
						result = kPreferences_ResultUnknownTagOrClass;
						break;
					}
				}
				break;
			}
		}
	}
	
	return result;
}// setWorkspacePreference


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
	case kVK_Return: // 0x24
		result = CFSTR("return");
		break;
	
	case kVK_Delete: // 0x33
		result = CFSTR("backward-delete");
		break;
	
	case kVK_Escape: // 0x35
		result = CFSTR("escape");
		break;
	
	case kVK_ANSI_KeypadClear: // 0x47
		result = CFSTR("clear");
		break;
	
	case kVK_ANSI_KeypadEnter: // 0x4C
		result = CFSTR("enter");
		break;
	
	case kVK_F5: // 0x60
		result = CFSTR("f5");
		break;
	
	case kVK_F6: // 0x61
		result = CFSTR("f6");
		break;
	
	case kVK_F7: // 0x62
		result = CFSTR("f7");
		break;
	
	case kVK_F3: // 0x63
		result = CFSTR("f3");
		break;
	
	case kVK_F8: // 0x64
		result = CFSTR("f8");
		break;
	
	case kVK_F9: // 0x65
		result = CFSTR("f9");
		break;
	
	case kVK_F11: // 0x67
		result = CFSTR("f11");
		break;
	
	case kVK_F13: // 0x69
		result = CFSTR("f13");
		break;
	
	case kVK_F16: // 0x6A
		result = CFSTR("f16");
		break;
	
	case kVK_F14: // 0x6B
		result = CFSTR("f14");
		break;
	
	case kVK_F10: // 0x6D
		result = CFSTR("f10");
		break;
	
	case kVK_F12: // 0x6F
		result = CFSTR("f12");
		break;
	
	case kVK_F15: // 0x71
		result = CFSTR("f15");
		break;
	
	case kVK_Home: // 0x73
		result = CFSTR("home");
		break;
	
	case kVK_PageUp: // 0x74
		result = CFSTR("page-up");
		break;
	
	case kVK_ForwardDelete: // 0x75
		result = CFSTR("forward-delete");
		break;
	
	case kVK_F4: // 0x76
		result = CFSTR("f4");
		break;
	
	case kVK_End: // 0x77
		result = CFSTR("end");
		break;
	
	case kVK_F2: // 0x78
		result = CFSTR("f2");
		break;
	
	case kVK_PageDown: // 0x79
		result = CFSTR("page-down");
		break;
	
	case kVK_F1: // 0x7A
		result = CFSTR("f1");
		break;
	
	case kVK_LeftArrow: // 0x7B
		result = CFSTR("left-arrow");
		break;
	
	case kVK_RightArrow: // 0x7C
		result = CFSTR("right-arrow");
		break;
	
	case kVK_DownArrow: // 0x7D
		result = CFSTR("down-arrow");
		break;
	
	case kVK_UpArrow: // 0x7E
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
	
	
	if (0 == CFStringGetLength(inName))
	{
		outCharacterOrKeyCode = 0;
		outIsVirtualKey = false;
	}
	else if (1 == CFStringGetLength(inName))
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
			outCharacterOrKeyCode = kVK_Return; // 0x24
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("backward-delete"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_Delete; // 0x33
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("escape"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_Escape; // 0x35
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("clear"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_ANSI_KeypadClear; // 0x47
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("enter"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_ANSI_KeypadEnter; // 0x4C
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f5"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F5; // 0x60
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f6"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F6; // 0x61
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f7"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F7; // 0x62
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f3"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F3; // 0x63
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f8"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F8; // 0x64
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f9"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F9; // 0x65
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f11"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F11; // 0x67
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f13"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F13; // 0x69
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f16"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F16; // 0x6A
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f14"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F14; // 0x6B
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f10"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F10; // 0x6D
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f12"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F12; // 0x6F
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f15"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F15; // 0x71
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("home"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_Home; // 0x73
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("page-up"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_PageUp; // 0x74
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("forward-delete"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_ForwardDelete; // 0x75
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f4"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F4; // 0x76
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("end"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_End; // 0x77
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f2"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F2; // 0x78
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("page-down"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_PageDown; // 0x79
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("f1"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_F1; // 0x7A
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("left-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_LeftArrow; // 0x7B
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("right-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_RightArrow; // 0x7C
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("down-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_DownArrow; // 0x7D
		}
		else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("up-arrow"), kCompareFlags))
		{
			outCharacterOrKeyCode = kVK_UpArrow; // 0x7E
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


/*!
Updates the given dictionary using stored preference values from
the specified context.  If merging, conflicting keys are skipped;
otherwise, they are replaced with the new dictionary values.

If "inClassKeysOnly" is true, the preference class of the given
context is used to determine which keys to include in the
dictionary.  This is especially useful if the source context is
something large and varied such as the set of global defaults.
Usually this flag should be set to true so that the resulting
dictionary is no larger than it really needs to be.

All of the keys added to the dictionary will be of type
CFStringRef.

\retval true
currently always returned; there is no way to detect errors

(4.0)
*/
Boolean
writePreferencesDictionaryFromContext	(My_ContextInterfacePtr		inContextPtr,
										 CFMutableDictionaryRef		inoutPreferenceDictionary,
										 Boolean					inMerge,
										 Boolean					inClassKeysOnly)
{
	CFRetainRelease		keyListCFArrayObject(inContextPtr->returnKeyListCopy(), CFRetainRelease::kAlreadyRetained);
	CFArrayRef			keyListCFArray = keyListCFArrayObject.returnCFArrayRef();
	Boolean				result = true;
	
	
	for (CFIndex i = 0; i < CFArrayGetCount(keyListCFArray); ++i)
	{
		CFStringRef const	kKey = CFUtilities_StringCast(CFArrayGetValueAtIndex(keyListCFArray, i));
		Boolean				useKey = true;
		
		
		if (inMerge)
		{
			// when merging, do not replace any key that is already defined
			if (CFDictionaryContainsKey(inoutPreferenceDictionary, kKey))
			{
				useKey = false;
			}
			else
			{
				// value is not yet defined
				useKey = true;
			}
		}
		
		// check for class restrictions; the "name" and "name-string" keys
		// are exempt because they are used in all classes to name collections
		if ((useKey) &&
			(inClassKeysOnly) &&
			(kCFCompareEqualTo != CFStringCompare(kKey, CFSTR("name"), 0/* options */)) &&
			(kCFCompareEqualTo != CFStringCompare(kKey, CFSTR("name-string"), 0/* options */)))
		{
			My_PreferenceDefinition*	defPtr = My_PreferenceDefinition::findByKeyName(kKey);
			
			
			// when restricting to a class, do not save a key unless it has
			// a definition that belongs to the source context’s class
			if (nullptr != defPtr)
			{
				useKey = (inContextPtr->returnClass() == defPtr->preferenceClass);
			}
			else
			{
				// unknown, e.g. a preference key written by Mac OS X itself
				useKey = false;
			}
		}
		
		if (useKey)
		{
			CFRetainRelease		prefsValueCFProperty(inContextPtr->returnValueCopy(kKey), CFRetainRelease::kAlreadyRetained);
			
			
			if (prefsValueCFProperty.exists())
			{
				CFDictionaryAddValue(inoutPreferenceDictionary, kKey, prefsValueCFProperty.returnCFTypeRef());
			}
		}
	}
	return result;
}// writePreferencesDictionaryFromContext

} // anonymous namespace


#pragma mark Internal Methods: Unit Tests
namespace {

/*!
Tests the CFDictionary context interface.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
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
returned if any fail, however messages should be
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
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest002_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextFavorite::unitTest(Quills::Prefs::SESSION, CFSTR("__test_session__"));
	return result;
}// unitTest002_Begin


/*!
Tests the CFPreferences-in-sub-domain context with
the Format class.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest003_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextFavorite::unitTest(Quills::Prefs::FORMAT, CFSTR("__test_format__"));
	return result;
}// unitTest003_Begin

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
