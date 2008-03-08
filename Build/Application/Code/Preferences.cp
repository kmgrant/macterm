/*###############################################################

	Preferences.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CFKeyValueInterface.h>
#include <CFUtilities.h>
#include <Console.h>
#include <ListenerModel.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlockReferenceLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <WindowInfo.h>

// resource includes
#include "ApplicationVersion.h"
#include "StringResources.h"
#include "GeneralResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Clipboard.h"
#include "Commands.h"
#include "Folder.h"
#include "NetEvents.h"
#include "Preferences.h"
#include "Session.h"
#include "tekdefs.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "UIStrings.h"



#pragma mark Constants
namespace {

CFStringEncoding const		kMy_SavedNameEncoding = kCFStringEncodingUnicode;

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
*/
class My_ContextInterface
{
public:
	//! inserts array value into dictionary
	void
	addArray	(CFStringRef	inKey,
				 CFArrayRef		inValue)
	{
		_implementorPtr->addArray(inKey, inValue);
	}
	
	//! inserts data value into dictionary
	void
	addData		(CFStringRef	inKey,
				 CFDataRef		inValue)
	{
		_implementorPtr->addData(inKey, inValue);
	}
	
	//! inserts true/false value into dictionary
	void
	addFlag		(CFStringRef	inKey,
				 Boolean		inValue)
	{
		_implementorPtr->addFlag(inKey, inValue);
	}
	
	//! inserts floating-point value into dictionary
	void
	addFloat	(CFStringRef	inKey,
				 Float32		inValue)
	{
		_implementorPtr->addFloat(inKey, inValue);
	}
	
	//! inserts short integer value into dictionary
	void
	addInteger	(CFStringRef	inKey,
				 SInt16			inValue)
	{
		_implementorPtr->addInteger(inKey, inValue);
	}
	
	//! inserts short integer value into dictionary
	void
	addLong		(CFStringRef	inKey,
				 SInt32			inValue)
	{
		_implementorPtr->addLong(inKey, inValue);
	}
	
	//! inserts string value into dictionary
	void
	addString	(CFStringRef	inKey,
				 CFStringRef	inValue)
	{
		_implementorPtr->addString(inKey, inValue);
	}
	
	//! inserts arbitrary value into dictionary
	void
	addValue	(CFStringRef		inKey,
				 CFPropertyListRef	inValue)
	{
		_implementorPtr->addValue(inKey, inValue);
	}
	
	//! delete this key-value set from application preferences
	virtual Preferences_Result
	destroy () NO_METHOD_IMPL = 0;
	
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
	
	virtual
	~My_ContextInterface ();
	
	void
	setImplementor	(CFKeyValueInterface*);

private:
	Preferences_Class		_preferencesClass;
	CFKeyValueInterface*	_implementorPtr;
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
	My_ContextCFDictionary	(Preferences_Class, CFMutableDictionaryRef = nullptr);
	
	//!\name New Methods In This Class
	//@{
	
	//! returns the CFMutableDictionaryRef being managed (do not edit it yourself)
	CFMutableDictionaryRef
	returnDictionary () const;
	
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
A context for storing data in a CFDictionary that will
ultimately be saved in Favorites under Preferences.
*/
class My_ContextFavorite:
public My_ContextCFDictionary
{
public:
	My_ContextFavorite	(Preferences_Class, CFStringRef);
	
	//!\name New Methods In This Class
	//@{
	
	//! test routine
	static Boolean
	unitTest ();
	
	//@}
	
	//!\name My_ContextInterface Methods
	//@{
	
	//! delete keys in this dictionary, and their values, from application preferences
	Preferences_Result
	destroy ();
	
	//! alter the name under which this is saved; useful in UI elements
	Preferences_Result
	rename	(CFStringRef);
	
	//! the name under which this is saved; useful in UI elements
	CFStringRef
	returnName () const;
	
	//! save changes to this dictionary in the application preferences
	Preferences_Result
	save ();
	
	//@}

protected:
	CFMutableDictionaryRef
	createClassDictionary	(Preferences_Class, CFStringRef);

private:
	CFRetainRelease		_contextName;	//!< CFStringRef; a display name for this context
};
typedef My_ContextFavorite const*	My_ContextFavoriteConstPtr;
typedef My_ContextFavorite*			My_ContextFavoritePtr;

/*!
Keeps track of all named contexts in memory.
*/
typedef std::vector< My_ContextFavoritePtr >	My_FavoriteContextList;

/*!
A context specifically for storing defaults.  It
doesn’t actually manage a dictionary, it uses the
Core Foundation Preferences APIs instead; though,
the consistency of this API compared to that of
other contexts is useful.
*/
class My_ContextDefault:
public My_ContextInterface
{
public:
	My_ContextDefault	(Preferences_Class	inClass);
	
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
	
	//! test routine
	static Boolean
	unitTest ();

protected:

private:
	CFRetainRelease			_contextName;	//!< CFStringRef; a display name for this context
	CFKeyValuePreferences	_dictionary;	//!< handles key value lookups
};
typedef My_ContextDefault const*	My_ContextDefaultConstPtr;
typedef My_ContextDefault*			My_ContextDefaultPtr;

/*!
Implicitly-saved location and size information for a
window.  Used to remember the user’s preferred
window arrangement.
*/
struct MyWindowArrangement
{
	MyWindowArrangement		(SInt16		inX = 0,
							 SInt16		inY = 0,
							 SInt16		inWidth = 0,
							 SInt16		inHeight = 0)
	:
	x(inX),
	y(inY),
	width(inWidth),
	height(inHeight)
	{
	}
	
	SInt16		x;			//!< window content region location, left edge
	SInt16		y;			//!< window content region location, top edge
	SInt16		width;		//!< window content region size, horizontally
	SInt16		height;		//!< window content region size, vertically
};

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_Ref			gPreferenceEventListenerModel = nullptr;
Boolean						gHaveRunConverter = false;
Boolean						gInitialized = false;
My_AliasInfoList&			gAliasList ()	{ static My_AliasInfoList x; return x; }
My_ContextPtrLocker&		gMyContextPtrLocks ()	{ static My_ContextPtrLocker x; return x; }
My_ContextReferenceLocker&	gMyContextRefLocks ()	{ static My_ContextReferenceLocker x; return x; }
My_ContextInterface&		gGeneralDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassGeneral); return x; }
My_ContextInterface&		gFormatDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassFormat); return x; }
My_FavoriteContextList&		gFormatNamedContexts ()		{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gMacroSetDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassMacroSet); return x; }
My_FavoriteContextList&		gMacroSetNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gSessionDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassSession); return x; }
My_FavoriteContextList&		gSessionNamedContexts ()	{ static My_FavoriteContextList x; return x; }
My_ContextInterface&		gTerminalDefaultContext ()	{ static My_ContextDefault x(kPreferences_ClassTerminal); return x; }
My_FavoriteContextList&		gTerminalNamedContexts ()	{ static My_FavoriteContextList x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Preferences_Result		assertInitialized						();
void					changeNotify							(Preferences_Change,
																 Preferences_ContextRef = nullptr, Boolean = false);
Boolean					convertCFArrayToRGBColor				(CFArrayRef, RGBColor*);
Boolean					convertRGBColorToCFArray				(RGBColor const*, CFArrayRef&);
Preferences_Result		copyClassDictionaryByName				(Preferences_Class, CFStringRef, CFMutableDictionaryRef&);
Preferences_Result		copyClassDictionaryCFArray				(Preferences_Class, CFArrayRef&);
Preferences_Result		createAllPreferencesContextsFromDisk	();
CFDictionaryRef			createDefaultPrefDictionary				();
void					deleteAliasData							(My_AliasInfoPtr*);
void					deleteAllAliasNodes						();
My_AliasInfoPtr			findAlias								(Preferences_AliasID);
Boolean					findAliasOnDisk							(Preferences_AliasID, AliasHandle*);
CFIndex					findDictionaryIndexInArrayByName		(CFArrayRef, CFStringRef);
Boolean					getDefaultContext						(Preferences_Class, My_ContextInterfacePtr&);
Preferences_Result		getFormatPreference						(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getGeneralPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getListOfContexts						(Preferences_Class, My_FavoriteContextList*&);
Preferences_Result		getMacroSetPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getNamedContext							(Preferences_Class, CFStringRef,
																 My_ContextInterfacePtr&);
Preferences_Result		getPreferenceDataInfo					(Preferences_Tag, CFStringRef&, FourCharCode&,
																 size_t&, Preferences_Class&);
Preferences_Result		getSessionPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Preferences_Result		getTerminalPreference					(My_ContextInterfaceConstPtr, Preferences_Tag,
																 size_t, void*, size_t*);
Boolean					getWindowPreferences					(Preferences_Tag, MyWindowArrangement&);
OSStatus				initPreferences							(Boolean);
Preferences_Result		overwriteClassDictionaryCFArray			(Preferences_Class, CFArrayRef);
void					readMacTelnetCoordPreference			(CFStringRef, SInt16&, SInt16&);
void					readMacTelnetArrayPreference			(CFStringRef, CFArrayRef&);
OSStatus				readPreferencesDictionary				(CFDictionaryRef, Boolean);
OSStatus				setAliasChanged							(My_AliasInfoPtr);
Preferences_Result		setFormatPreference						(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setGeneralPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setMacroSetPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Boolean					setMacTelnetCoordPreference				(CFStringRef, SInt16, SInt16);
void					setMacTelnetPreference					(CFStringRef, CFPropertyListRef);
Preferences_Result		setSessionPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Preferences_Result		setTerminalPreference					(My_ContextInterfacePtr, Preferences_Tag,
																 size_t, void const*);
Boolean					unitTest000_Begin						();
Boolean					unitTest001_Begin						();
Boolean					unitTest002_Begin						();

} // anonymous namespace



#pragma mark Functors

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
		SInt16				currentPrefsVersion = 1; // only a default...
		
		
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
				
				
				// start with an initial set
				initPreferences(true/* brand new */);
				
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
					Alert_ReportOSStatus(launchResult);
				}
			}
		}
	}
	
	// to ensure that the rest of the application can depend on its
	// known keys being defined, ALWAYS merge in default values for
	// any keys that may not already be defined (by the data on disk,
	// by the Preferences Converter, etc.)
	initPreferences(false/* brand new */);
	
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
Creates a new preferences context that copies the data
of the specified original context.  The reference is
automatically retained, but you need to invoke
Preferences_ReleaseContext() when finished.

If the specified context is detached (has an empty
name, meaning it is not tracked globally), its copy is
also detached.  Otherwise, the copy remains tracked
unless "inForceDetach" is true.  (Specifying true for
an already-detached source has no effect.)

Since tracked contexts are named, cloning a tracked
context will generate a unique name for the copy.
See Preferences_ContextRename().

The initial name of the new context is a variation on
the name of the base context.

If any problems occur, nullptr is returned; otherwise,
a reference to the new context is returned.

WARNING:	Currently, cloning a Default context is
			naïve, because it holds global application
			preferences.  The resulting copy will
			correctly contain the necessary keys, but
			will also have many other irrelevant keys
			that will never be used.

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
		result = Preferences_NewDetachedContext(baseClass);
	}
	else
	{
		CFStringRef		nameCFString = nullptr;
		
		
		// INCOMPLETE: Base the new name on the name of the original.
		// INCOMPLETE: Scan the list of all contexts for the given class
		// and find a unique name.  For now, just assume one.
		nameCFString = CFSTR("copy"); // TEMPORARY; LOCALIZE THIS
		
		result = Preferences_NewContext(baseClass, nameCFString);
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
				
				
				resultPtr->addValue(kKeyCFStringRef, keyValueCFType.returnCFTypeRef());
			}
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
Preferences_NewDetachedContext	(Preferences_Class		inClass)
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
}// NewDetachedContext


/*!
Creates a new preferences context according to the
given specifications, or returns an existing one that
matches.  The reference is automatically retained, but
you need to invoke Preferences_ReleaseContext() when
finished.

Contexts are used in order to make changes to settings
(and save changes) within the constraints of a single
named dictionary of a particular class.  To the user,
this is a collection such as a Session Favorite.

If any problems occur, nullptr is returned; otherwise,
a reference to the new context is returned.

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
Preferences_NewContext	(Preferences_Class		inClass,
						 CFStringRef			inNameOrNullToAutoGenerateUniqueName)
{
	Preferences_ContextRef		result = nullptr;
	
	
	// INCOMPLETE: When nullptr is given, scan the list of all
	// contexts for the given class and find a unique name.
	// For now, just assume one.
	if (nullptr == inNameOrNullToAutoGenerateUniqueName)
	{
		// TEMPORARY
		inNameOrNullToAutoGenerateUniqueName = CFSTR("untitled"); // LOCALIZE THIS
	}
	
	try
	{
		My_ContextInterfacePtr		contextPtr = nullptr;
		
		
		if (false == getNamedContext(inClass, inNameOrNullToAutoGenerateUniqueName, contextPtr))
		{
			My_FavoriteContextList*		listPtr = nullptr;
			
			
			if (getListOfContexts(inClass, listPtr))
			{
				My_ContextFavoritePtr	newDictionary = new My_ContextFavorite
															(inClass, inNameOrNullToAutoGenerateUniqueName);
				
				
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
	return result;
}// NewContext


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
Preferences_NewContext() or similar routines, and deletes
the context *if* no other locks remain.  Your copy of the
reference is set to nullptr.

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
			My_ContextCFDictionaryPtr	ptr = REINTERPRET_CAST(*inoutRefPtr, My_ContextCFDictionaryPtr);
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
Automatically sizes and/or positions a window based
on the specified preferences.  If the preferences
for the size are all-zero, the window size will not
change.  Similarly, if the preferences for the
location are all-zero, then the window will not move.
The window is not allowed to become smaller than the
specified minimum size.

Although an entire rectangle is saved, you can always
choose to ignore parts of it when restoring the
window location.  This is important for windows that
have boundary constraints, because the saved rectangle
might reflect an earlier version of the window that
had different size constraints; NEVER restore window
boundary elements unless they are flexible.

The window will not move to an offscreen location.

This routine will return the change in width and
height, if any, between the given initial size and
the new, preferred size.  This is useful for windows
with controls that need to be moved and/or resized
appropriately when the window changes size (although
this is somewhat deprecated on Mac OS X because the
act of resizing the window should trigger event
handlers that synchronize controls automatically).

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultBadVersionDataNotAvailable
if preferences for the specified tag cannot be found

(3.0)
*/
Preferences_Result
Preferences_ArrangeWindow	(WindowRef								inWindow,
							 Preferences_Tag						inWindowPreferenceTag,
							 Point*									inoutMinimumSizeFinalSizePtr,
							 Preferences_WindowBoundaryElements		inBoundaryElementsToRestore)
{
	Preferences_Result		result = kPreferences_ResultOK;
	MyWindowArrangement		arrangementPrefs;
	Boolean					foundPrefs = false;
	
	
	foundPrefs = getWindowPreferences(inWindowPreferenceTag, arrangementPrefs); // find the data, wherever it is
	if (!foundPrefs) result = kPreferences_ResultBadVersionDataNotAvailable;
	else
	{
		Rect		originalStructureBounds;
		Rect		windowBounds;
		Rect		visibleScreenRect;
		OSStatus	error = noErr;
		
		
		GetWindowBounds(inWindow, kWindowStructureRgn, &originalStructureBounds);
		GetWindowBounds(inWindow, kWindowContentRgn, &windowBounds);
		
		// calculate this area to make sure that windows aren’t arranged invisibly
		RegionUtilities_GetPositioningBounds(inWindow, &visibleScreenRect);
		
		// change the requested parts of the window boundaries; a zero saved value
		// means “no preference”, and in the case of dimensions, the window cannot
		// be smaller than the initial value either
		if ((inBoundaryElementsToRestore & kPreferences_WindowBoundaryElementLocationH) &&
			(arrangementPrefs.x != 0))
		{
			windowBounds.right = arrangementPrefs.x + (windowBounds.right - windowBounds.left);
			windowBounds.left = arrangementPrefs.x;
		}
		if ((inBoundaryElementsToRestore & kPreferences_WindowBoundaryElementLocationV) &&
			(arrangementPrefs.y != 0))
		{
			windowBounds.bottom = arrangementPrefs.y + (windowBounds.bottom - windowBounds.top);
			windowBounds.top = arrangementPrefs.y;
		}
		if (inBoundaryElementsToRestore & kPreferences_WindowBoundaryElementWidth)
		{
			if (arrangementPrefs.width > inoutMinimumSizeFinalSizePtr->h)
			{
				windowBounds.right = windowBounds.left + arrangementPrefs.width;
			}
			else
			{
				windowBounds.right = windowBounds.left + inoutMinimumSizeFinalSizePtr->h;
			}
		}
		if (inBoundaryElementsToRestore & kPreferences_WindowBoundaryElementHeight)
		{
			if (arrangementPrefs.height > inoutMinimumSizeFinalSizePtr->v)
			{
				windowBounds.bottom = windowBounds.top + arrangementPrefs.height;
			}
			else
			{
				windowBounds.bottom = windowBounds.top + inoutMinimumSizeFinalSizePtr->v;
			}
		}
		SetWindowBounds(inWindow, kWindowContentRgn, &windowBounds);
		
		inoutMinimumSizeFinalSizePtr->h = 0;
		inoutMinimumSizeFinalSizePtr->v = 0;
		error = ConstrainWindowToScreen(inWindow, kWindowContentRgn, kWindowConstrainStandardOptions,
										nullptr/* constraint rectangle */,
										&windowBounds/* new structure boundaries */);
		if (error == noErr)
		{
			// figure out change in window size
			inoutMinimumSizeFinalSizePtr->h = ((windowBounds.right - windowBounds.left) -
												(originalStructureBounds.right - originalStructureBounds.left));
			inoutMinimumSizeFinalSizePtr->v = ((windowBounds.bottom - windowBounds.top) -
												(originalStructureBounds.bottom - originalStructureBounds.top));
		}
	}
	return result;
}// ArrangeWindow


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
Returns preference data corresponding to the specified tag.

Data is obtained sequentially by context; the “global” context
is consulted last, and is only used if no preceding context
contains the specified preferences data.  In this way, it is
possible to ask for data generically without having to know
exactly how the user provided it (or even if the user provided
it at all); e.g. there may be preferences specific to a window,
preferences specific to a workspace, and then global preferences
that are used if neither of the former is defined.

Incomplete.

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

(3.0)
*/
Preferences_Result
Preferences_ContextGetData	(Preferences_ContextRef		inContext,
							 Preferences_Tag			inDataPreferenceTag,
							 size_t						inDataStorageSize,
							 void*						outDataStorage,
							 size_t*					outActualSizePtrOrNull)
{
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0;
	Preferences_Class		dataClass = kPreferences_ClassGeneral;
	
	
	result = getPreferenceDataInfo(inDataPreferenceTag, keyName, keyValueType, actualSize, dataClass);
	if (kPreferences_ResultOK == result)
	{
		My_ContextAutoLocker	ptr(gMyContextPtrLocks(), inContext);
		
		
		switch (dataClass)
		{
		case kPreferences_ClassFormat:
			result = getFormatPreference(ptr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
			break;
		
		case kPreferences_ClassGeneral:
			result = getGeneralPreference(ptr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
			break;
		
		case kPreferences_ClassMacroSet:
			result = getMacroSetPreference(ptr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
			break;
		
		case kPreferences_ClassSession:
			result = getSessionPreference(ptr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
			break;
		
		case kPreferences_ClassTerminal:
			result = getTerminalPreference(ptr, inDataPreferenceTag, inDataStorageSize, outDataStorage, outActualSizePtrOrNull);
			break;
		
		case kPreferences_ClassWindow:
		default:
			// unrecognized preference class
			result = kPreferences_ResultUnknownTagOrClass;
			break;
		}
	}
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
Saves any in-memory preferences data model changes to
disk, and updates the in-memory model with any new
settings on disk.  This has the side effect of saving
global preferences changes as well, but it will not
commit changes to other specific contexts.

\retval kPreferences_ResultOK
if no error occurred

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
Sets a preference corresponding to the specified tag.  See also
Preferences_SetWindowArrangementData().

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
	Preferences_Result		result = kPreferences_ResultOK;
	CFStringRef				keyName = nullptr;
	FourCharCode			keyValueType = '----';
	size_t					actualSize = 0;
	Preferences_Class		dataClass = kPreferences_ClassGeneral;
	
	
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
			result = setMacroSetPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassSession:
			result = setSessionPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassTerminal:
			result = setTerminalPreference(ptr, inDataPreferenceTag, inDataSize, inDataPtr);
			break;
		
		case kPreferences_ClassWindow:
		default:
			result = kPreferences_ResultUnknownTagOrClass;
			break;
		}
	}
	return result;
}// ContextSetData


/*!
Provides a list of the names of valid contexts in the
specified preferences class; none of these names should
be used when calling Preferences_NewContext(), otherwise
the creation will fail.

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
				My_ContextCFDictionaryPtr	contextPtr = *toContextPtr;
				CFStringRef					thisName = contextPtr->returnName();
				
				
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
			GetIndString(notificationMessage, rStringsNoteAlerts, siNotificationAlert);
			Alert_SetNotificationMessage(notificationMessage);
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
				My_ContextCFDictionaryPtr	contextPtr = *toContextPtr;
				
				
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
												outDataStorage, outActualSizePtrOrNull);
		}
	}
	return result;
}// GetData


/*!
Returns the default context for the specified class.
Preferences written to the "kPreferences_ClassGeneral"
default context are in fact global settings.

This is equivalent to, but more efficient and desirable
than, invoking Preferences_NewContext() using the
appropriate default name, which should be avoided as it
will make it possible for preference changes in each
context to trump one another.

WARNING:	Do not dispose of default contexts.

\retval kPreferences_ResultOK
if the requested context was found

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
				error = InsertMenuItemTextWithCFString(inoutMenuRef, nameCFString, inAfterItemIndex + i,
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
Reads the current size and position of a window and
adds that data into a record with the specified tag in
the preferences data model in memory.  If any problems
occur, no preference is set.

NOTE:	Not all boundaries of a window may be important,
		but all are saved.  The code that reads back
		window preferences is responsible for ignoring
		parts that are not needed or desired (e.g. a
		window whose position matters but whose size is
		fixed).

(3.0)
*/
void
Preferences_SetWindowArrangementData	(WindowRef			inWindow,
										 Preferences_Tag	inWindowPreferenceTag)
{
	if (nullptr != inWindow)
	{
		Rect		rect;
		SInt16		x = 0;
		SInt16		y = 0;
		UInt16		width = 0;
		UInt16		height = 0;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(inWindow, kWindowContentRgn, &rect);
		assert_noerr(error);
		x = rect.left;
		y = rect.top;
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		
		switch (inWindowPreferenceTag)
		{
		case kPreferences_WindowTagCommandLine:
			setMacTelnetCoordPreference(CFSTR("window-commandline-position-pixels"), x, y);
			setMacTelnetCoordPreference(CFSTR("window-commandline-size-pixels"), width, height);
			break;
		
		case kPreferences_WindowTagControlKeypad:
			setMacTelnetCoordPreference(CFSTR("window-controlkeys-position-pixels"), x, y);
			break;
		
		case kPreferences_WindowTagFunctionKeypad:
			setMacTelnetCoordPreference(CFSTR("window-functionkeys-position-pixels"), x, y);
			break;
		
		case kPreferences_WindowTagMacroSetup:
			setMacTelnetCoordPreference(CFSTR("window-macroeditor-position-pixels"), x, y);
			setMacTelnetCoordPreference(CFSTR("window-macroeditor-size-pixels"), width, height);
			break;
		
		case kPreferences_WindowTagPreferences:
			setMacTelnetCoordPreference(CFSTR("window-preferences-position-pixels"), x, y);
			break;
		
		case kPreferences_WindowTagVT220Keypad:
			setMacTelnetCoordPreference(CFSTR("window-vt220keys-position-pixels"), x, y);
			break;
		
		default:
			// ???
			break;
		}
	}
}// SetWindowArrangementData


/*!
Arranges for a callback to be invoked whenever a user
preference changes.  The context passed to listeners
is currently reserved and set to nullptr.

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
	// Preferences_StopMonitoring() and the comments in "Preferences.h".
	case kPreferences_TagArrangeWindowsUsingTabs:
	case kPreferences_TagBellSound:
	case kPreferences_TagCursorBlinks:
	case kPreferences_TagDontDimBackgroundScreens:
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
	// Keep this in sync with Preferences_StartMonitoring() and the
	// comments in "Preferences.h".
	case kPreferences_TagArrangeWindowsUsingTabs:
	case kPreferences_TagBellSound:
	case kPreferences_TagCursorBlinks:
	case kPreferences_TagDontDimBackgroundScreens:
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
_preferencesClass(inClass),
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
}// My_ContextInterface destructor


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
		inTestObjectPtr->addArray(CFSTR("__test_array_key__"), testArray.returnCFArrayRef());
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
		inTestObjectPtr->addData(CFSTR("__test_data_key__"), REINTERPRET_CAST(testData.returnCFTypeRef(), CFDataRef));
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
		
		
		inTestObjectPtr->addFlag(CFSTR("__test_flag_key_1__"), kFlag1);
		result &= Console_Assert("returned flag is true", kFlag1 == inTestObjectPtr->returnFlag(CFSTR("__test_flag_key_1__")));
		inTestObjectPtr->addFlag(CFSTR("__test_flag_key_2__"), kFlag2);
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
		
		
		inTestObjectPtr->addFloat(CFSTR("__test_float_key_1__"), kFloat1);
		inTestObjectPtr->addFloat(CFSTR("__test_float_key_2__"), kFloat2);
		inTestObjectPtr->addFloat(CFSTR("__test_float_key_3__"), kFloat3);
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_1__"));
		result &= Console_Assert("returned float is close to zero",
									(returnedFloat > (kFloat1 - kTolerance)) && (returnedFloat < (kFloat1 + kTolerance)));
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_2__"));
		result &= Console_Assert("returned float is close to 1",
									(returnedFloat > (kFloat2 - kTolerance)) && (returnedFloat < (kFloat2 + kTolerance)));
		returnedFloat = inTestObjectPtr->returnFloat(CFSTR("__test_float_key_3__"));
		result &= Console_Assert("returned float is close to -36.4",
									(returnedFloat > (kFloat3 - kTolerance)) && (returnedFloat < (kFloat3 + kTolerance)));
		inTestObjectPtr->addFloat(CFSTR("__test_float_key_4__"), kFloat4);
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
		
		
		inTestObjectPtr->addInteger(CFSTR("__test_integer_key_1__"), kInteger1);
		inTestObjectPtr->addInteger(CFSTR("__test_integer_key_2__"), kInteger2);
		inTestObjectPtr->addInteger(CFSTR("__test_integer_key_3__"), kInteger3);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_int_key_1__"));
		result &= Console_Assert("returned integer is zero", returnedInteger == kInteger1);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_integer_key_2__"));
		result &= Console_Assert("returned integer is 1", returnedInteger == kInteger2);
		returnedInteger = inTestObjectPtr->returnInteger(CFSTR("__test_integer_key_3__"));
		result &= Console_Assert("returned integer is -77", returnedInteger == kInteger3);
		inTestObjectPtr->addInteger(CFSTR("__test_integer_key_4__"), kInteger4);
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
		
		
		inTestObjectPtr->addLong(CFSTR("__test_long_key_1__"), kInteger1);
		inTestObjectPtr->addLong(CFSTR("__test_long_key_2__"), kInteger2);
		inTestObjectPtr->addLong(CFSTR("__test_long_key_3__"), kInteger3);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_int_key_1__"));
		result &= Console_Assert("returned long integer is zero", returnedInteger == kInteger1);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_long_key_2__"));
		result &= Console_Assert("returned long integer is 1", returnedInteger == kInteger2);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_long_key_3__"));
		result &= Console_Assert("returned long integer is -9124152", returnedInteger == kInteger3);
		inTestObjectPtr->addLong(CFSTR("__test_long_key_4__"), kInteger4);
		returnedInteger = inTestObjectPtr->returnLong(CFSTR("__test_long_key_4__"));
		result &= Console_Assert("returned long integer is 161124507", returnedInteger == kInteger4);
		result &= Console_Assert("nonexistent long integer is exactly zero", 0 == inTestObjectPtr->returnLong(CFSTR("long does not exist")));
	}
	
	// string values
	{
		CFStringRef			kString1 = CFSTR("my first test string");
		CFStringRef			kString2 = CFSTR("This is a somewhat more interesting test string.  Hopefully it still works!");
		CFRetainRelease		copiedString;
		
		
		inTestObjectPtr->addString(CFSTR("__test_string_key_1__"), kString1);
		copiedString = inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_1__"));
		result &= Console_Assert("returned string 1 exists", copiedString.exists());
		result &= Console_Assert("returned string 1 is correct",
									kCFCompareEqualTo == CFStringCompare(copiedString.returnCFStringRef(), kString1, 0/* options */));
		inTestObjectPtr->addString(CFSTR("__test_string_key_2__"), kString2);
		copiedString = inTestObjectPtr->returnStringCopy(CFSTR("__test_string_key_2__"));
		result &= Console_Assert("returned string 2 exists", copiedString.exists());
		result &= Console_Assert("returned string 2 is correct",
									kCFCompareEqualTo == CFStringCompare(copiedString.returnCFStringRef(), kString2, 0/* options */));
	}
	
	return result;
}// My_ContextInterface::unitTest


/*!
Constructor.  See Preferences_NewDetachedContext().

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
Console_WriteLine("context implemented by");
CFShow(_dictionary.returnDictionary());
}// My_ContextCFDictionary 2-argument constructor


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
Returns the dictionary being manipulated.  You should not
directly change this dictionary.

(3.1)
*/
CFMutableDictionaryRef
My_ContextCFDictionary::
returnDictionary ()
const
{
	return _dictionary.returnDictionary();
}// My_ContextCFDictionary::returnDictionary


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
Constructor.  See Preferences_NewContext().

(3.1)
*/
My_ContextFavorite::
My_ContextFavorite	(Preferences_Class	inClass,
					 CFStringRef		inNameOrNull)
:
My_ContextCFDictionary(inClass, createClassDictionary(inClass, inNameOrNull)),
_contextName(inNameOrNull)
{
	// the parent retains the created dictionary, so
	// release the copy made by createClassDictionary()
	if (nullptr != returnDictionary()) CFRelease(returnDictionary());
}// My_ContextFavorite 2-argument constructor


/*!
Returns a new dictionary for the context with the specified
name and class.  If preferences already exist, the dictionary
is initialized with their values; otherwise, the dictionary
is empty.

(3.1)
*/
CFMutableDictionaryRef
My_ContextFavorite::
createClassDictionary	(Preferences_Class	inClass,
						 CFStringRef		inNameOrNull)
{
	Preferences_Result			error = kPreferences_ResultOK;
	CFMutableDictionaryRef		result = nullptr;
	
	
	error = copyClassDictionaryByName(inClass, inNameOrNull, result);
	if (kPreferences_ResultOK != error)
	{
		result = nullptr;
		if (kPreferences_ResultUnknownName == error)
		{
			// if the name is simply unknown, the dictionary does not exist; create one!
			result = createDictionary();
			error = (nullptr == result) ? kPreferences_ResultGenericFailure : kPreferences_ResultOK;
		}
		
		if (kPreferences_ResultOK != error)
		{
			throw std::runtime_error("unable to construct data dictionary for given class and context name");
		}
	}
	
	return result;
}// My_ContextFavorite::createClassDictionary


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
	
	
	// figure out which MacTelnet preferences key holds the
	// relevant list of Favorites dictionaries
	(Preferences_Result)copyClassDictionaryCFArray(this->returnClass(), favoritesListCFArray);
	if (nullptr != favoritesListCFArray)
	{
		CFIndex		indexForName = findDictionaryIndexInArrayByName(favoritesListCFArray, this->returnName());
		
		
		if (indexForName >= 0)
		{
			// then an entry with this name exists on disk; destroy it!
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
			overwriteClassDictionaryCFArray(this->returnClass(), mutableFavoritesList.returnCFMutableArrayRef());
			if (false == CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication))
			{
				result = kPreferences_ResultGenericFailure;
			}
		}
	}
	
	return result;
}// My_ContextFavorite::destroy


/*!
Changes the name of this context.  This is often
used in user interface elements.

\retval kPreferences_ResultOK
if no error occurred

\retval kPreferences_ResultGenericFailure
if rename did not occur for any reason

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
Saves changes to the dictionary controlled by this
instance, overwriting any previously-saved dictionary
for the same name.  Has the side effect of synchronizing
any other modified application-wide preferences (but
will not save preferences from other contexts).

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
	CFArrayRef				favoritesListCFArray = nullptr;
	
	
Console_WriteValueCFString("saving, name", this->returnName());
	// figure out which MacTelnet preferences key holds the
	// relevant list of Favorites dictionaries
	result = copyClassDictionaryCFArray(this->returnClass(), favoritesListCFArray);
	if (nullptr == favoritesListCFArray) result = kPreferences_ResultGenericFailure;
	else if (kPreferences_ResultOK == result)
	{
		CFIndex				indexForName = findDictionaryIndexInArrayByName(favoritesListCFArray, this->returnName());
		CFRetainRelease		mutableFavoritesList(CFArrayCreateMutableCopy
													(kCFAllocatorDefault, CFArrayGetCount(favoritesListCFArray)/* capacity */,
														favoritesListCFArray), true/* is retained */);
		
		
		// generate or replace the name entry for this collection
		{
			CFRetainRelease		nameCFString(CFStringCreateExternalRepresentation
												(kCFAllocatorDefault, this->returnName(), kMy_SavedNameEncoding,
													'?'/* loss byte */), true/* is retained */);
			
			
			if (nameCFString.exists())
			{
				assert(CFDataGetTypeID() == CFGetTypeID(nameCFString.returnCFTypeRef()));
				CFDictionarySetValue(this->returnDictionary(), CFSTR("name"), nameCFString.returnCFTypeRef());
			}
			
			// regardless of whether a Unicode name was stored (which is preferred),
			// also store a raw string as a backup, just in case and also for
			// convenience when reading the raw XML in some cases
			CFDictionarySetValue(this->returnDictionary(), CFSTR("name-string"), this->returnName());
		}
		
		if (false == mutableFavoritesList.exists()) result = kPreferences_ResultGenericFailure;
		else if (indexForName < 0)
		{
			// was not previously saved; create something new!
			CFArrayAppendValue(mutableFavoritesList.returnCFMutableArrayRef(), this->returnDictionary());
			if (findDictionaryIndexInArrayByName(favoritesListCFArray, this->returnName()) < 0)
			{
				result = kPreferences_ResultOK;
			}
			else
			{
				result = kPreferences_ResultGenericFailure;
			}
		}
		else
		{
			// replace this entry in the array
			CFArraySetValueAtIndex(mutableFavoritesList.returnCFMutableArrayRef(), indexForName, this->returnDictionary());
			
			// update the preferences list
			result = overwriteClassDictionaryCFArray(this->returnClass(), mutableFavoritesList.returnCFMutableArrayRef());
			if (false == CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication))
			{
				result = kPreferences_ResultGenericFailure;
			}
		}
	}
Console_WriteValue("save result", result);
	return result;
}// My_ContextFavorite::save


/*!
Tests this class.  Returns true only if successful.
Information on failures is printed to the console.

(3.1)
*/
Boolean
My_ContextFavorite::
unitTest ()
{
	Boolean					result = true;
	My_ContextFavorite*		testObjectPtr = new My_ContextFavorite
												(kPreferences_ClassGeneral, CFSTR("__test__"));
	CFArrayRef				keyListCFArray = nullptr;
	
	
	result &= Console_Assert("class is set correctly",
								kPreferences_ClassGeneral == testObjectPtr->returnClass());
	result &= Console_Assert("name is set correctly",
								kCFCompareEqualTo == CFStringCompare
														(CFSTR("__test__"), testObjectPtr->returnName(),
															0/* options */));
	
	testObjectPtr->rename(CFSTR("__test_renamed__"));
	result &= Console_Assert("rename worked",
								kCFCompareEqualTo == CFStringCompare
														(CFSTR("__test_renamed__"), testObjectPtr->returnName(),
															0/* options */));
	
	keyListCFArray = testObjectPtr->returnKeyListCopy();
	result &= Console_Assert("key list exists", nullptr != keyListCFArray);
	result &= Console_Assert("key list is initially empty",
								0 == CFArrayGetCount(keyListCFArray));
	
	result &= My_ContextInterface::unitTest(testObjectPtr);
	
	CFRelease(keyListCFArray), keyListCFArray = nullptr;
	delete testObjectPtr, testObjectPtr = nullptr;
	
	return result;
}// My_ContextFavorite::unitTest


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
Attempts to find a Core Foundation dictionary of settings
that match the specified class and set name.

\retval kPreferences_ResultOK
if a matching dictionary was found and retrieved successfully

\retval kPreferences_ResultUnknownTagOrClass
if "inClass" is not valid

\retval kPreferences_ResultUnknownName
if no matching dictionary could be found

\retval kPreferences_ResultGenericFailure
other issue

(3.1)
*/
Preferences_Result
copyClassDictionaryByName	(Preferences_Class			inClass,
							 CFStringRef				inName,
							 CFMutableDictionaryRef&	outNewCFDictionary)
{
	Preferences_Result		result = kPreferences_ResultUnknownName;
	CFArrayRef				favoritesListCFArray = nullptr;
	
	
	// set default value
	outNewCFDictionary = nullptr;
	
	// retrieve the list of Favorites dictionaries
	result = copyClassDictionaryCFArray(inClass, favoritesListCFArray);
	if (kPreferences_ResultOK == result)
	{
		CFIndex		indexForName = findDictionaryIndexInArrayByName(favoritesListCFArray, inName);
		
		
		if (indexForName < 0) result = kPreferences_ResultUnknownName;
		else
		{
			CFDictionaryRef const		kDataCFDictionary = CFUtilities_DictionaryCast
															(CFArrayGetValueAtIndex(favoritesListCFArray, indexForName));
			
			
			if (nullptr == kDataCFDictionary) result = kPreferences_ResultGenericFailure;
			else
			{
				outNewCFDictionary = CFDictionaryCreateMutableCopy
										(kCFAllocatorDefault, 0/* capacity, or 0 for unlimited */,
											kDataCFDictionary);
				if (nullptr == outNewCFDictionary) result = kPreferences_ResultGenericFailure;
				else
				{
					// success!
					result = kPreferences_ResultOK;
				}
			}
		}
		CFRelease(favoritesListCFArray), favoritesListCFArray = nullptr;
	}
	
	return result;
}// copyClassDictionaryByName


/*!
Creates a Core Foundation array that contains
Core Foundation dictionary references in no
particular order, for the specified class of
preferences.  You must release the array when
finished with it.

If the dictionary cannot be found, it will be
nullptr.

See also overwriteClassDictionaryCFArray().

(3.1)
*/
Preferences_Result
copyClassDictionaryCFArray	(Preferences_Class		inClass,
							 CFArrayRef&			outCFArrayOfCFDictionaries)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// set default value
	outCFArrayOfCFDictionaries = nullptr;
	
	// figure out which MacTelnet preferences key holds the relevant list of Favorites dictionaries
	switch (inClass)
	{
	case kPreferences_ClassGeneral:
	case kPreferences_ClassWindow:
		// not applicable
		result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassFormat:
		readMacTelnetArrayPreference(CFSTR("favorite-styles"), outCFArrayOfCFDictionaries);
		if (nullptr == outCFArrayOfCFDictionaries) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassSession:
		readMacTelnetArrayPreference(CFSTR("favorite-sessions"), outCFArrayOfCFDictionaries);
		if (nullptr == outCFArrayOfCFDictionaries) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	case kPreferences_ClassTerminal:
		readMacTelnetArrayPreference(CFSTR("favorite-terminals"), outCFArrayOfCFDictionaries);
		if (nullptr == outCFArrayOfCFDictionaries) result = kPreferences_ResultBadVersionDataNotAvailable;
		break;
	
	default:
		// ???
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// copyClassDictionaryCFArray


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
	allClassesSupportingCollections.push_back(kPreferences_ClassFormat);
	for (PrefClassList::const_iterator toClass = allClassesSupportingCollections.begin();
			toClass != allClassesSupportingCollections.end(); ++toClass)
	{
		CFArrayRef		namesInClass = nullptr;
		
		
		result = Preferences_CreateContextNameArray(*toClass, namesInClass);
		if (kPreferences_ResultOK == result)
		{
			CFIndex const	kNumberOfFavorites = CFArrayGetCount(namesInClass);
			
			
			for (CFIndex i = 0; i < kNumberOfFavorites; ++i)
			{
				// simply creating a context will ensure it is retained
				// internally; so, the return value can be ignored (save
				// for verifying that it was created successfully)
				CFStringRef				favoriteName = CFUtilities_StringCast(CFArrayGetValueAtIndex
																				(namesInClass, i));
				Preferences_ContextRef	newContext = Preferences_NewContext(*toClass, favoriteName);
				
				
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
Collections of preferences are arrays of dictionaries,
where the "name" (or, barring that, "name-string") key
is a particular collection’s name; this routine searches
an array of dictionaries for the dictionary that has the
given name.

If the dictionary is not found, -1 is returned; otherwise,
the zero-based index into the array is returned.  This
index can be used with CFArray APIs.

IMPORTANT:	Make sure this is what you want; arrays are
			typically searched when reading saved data,
			whereas getNamedContext() is used to find
			contexts in memory (that may not yet be
			saved).

WARNING:	Since preferences arrays can change, use the
			returned index immediately.

(3.1)
*/
CFIndex
findDictionaryIndexInArrayByName	(CFArrayRef		inArray,
									 CFStringRef	inName)
{
	CFIndex				result = -1;
	CFIndex const		kArraySize = CFArrayGetCount(inArray);
	CFIndex				i = 0;
	CFDictionaryRef		dataCFDictionary = nullptr;
	CFDataRef			externalStringRepresentationCFData = nullptr;
	CFStringRef			newCFString = nullptr;
	Boolean				releaseNameString = false;
	
	
	for (i = 0; ((result < 0) && (i < kArraySize)); ++i)
	{
		dataCFDictionary = CFUtilities_DictionaryCast(CFArrayGetValueAtIndex(inArray, i));
		if (nullptr != dataCFDictionary)
		{
			// in order to support many languages, the "name" field is stored as
			// Unicode data (string external representation); however, if that
			// is not available, purely for convenience a "name-string" alternate
			// is supported, holding a raw string
			externalStringRepresentationCFData = CFUtilities_DataCast(CFDictionaryGetValue(dataCFDictionary, CFSTR("name")));
			if (nullptr != externalStringRepresentationCFData)
			{
				// Unicode string was found
				newCFString = CFStringCreateFromExternalRepresentation
								(kCFAllocatorDefault, externalStringRepresentationCFData,
									kCFStringEncodingUnicode);
				CFRelease(externalStringRepresentationCFData), externalStringRepresentationCFData = nullptr;
				releaseNameString = true;
			}
			else
			{
				// raw string was found
				newCFString = CFUtilities_StringCast(CFDictionaryGetValue(dataCFDictionary, CFSTR("name-string")));
			}
			
			if (nullptr != newCFString)
			{
				if (kCFCompareEqualTo == CFStringCompare(inName, newCFString, 0/* flags */))
				{
					result = i;
				}
			}
			
			if (releaseNameString)
			{
				CFRelease(newCFString), newCFString = nullptr;
			}
		}
	}
	return result;
}// findDictionaryIndexInArrayByName


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
	
	default:
		// ???
		result = false;
		break;
	}
	
	return result;
}// getDefaultContext


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
				case kPreferences_TagMenuItemKeys:
				case kPreferences_TagPureInverse:
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
				
				case kPreferences_TagDynamicResizing:
					{
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = true; // no longer a preference
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
Retrieves the list that stores any in-memory settings for
the specified class (only works for classes that can be
collections).  Returns true unless this fails.

IMPORTANT:	This list will only contain contexts created
			with Preferences_NewContext() that have not
			been destroyed by Preferences_ReleaseContext().
			There is no guarantee that this list will have
			every on-disk collection for the given class.
			But, see Preferences_CreateContextNameArray().

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
	
	case kPreferences_ClassGeneral:
	default:
		// ???
		result = false;
		break;
	}
	
	return result;
}// getListOfContexts


/*!
Returns preference data for a macro setting.

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
getMacroSetPreference	(My_ContextInterfaceConstPtr	inContextPtr,
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
				switch (inDataPreferenceTag)
				{
				case kPreferences_TagMacrosMenuVisible:
					{
						assert(typeNetEvents_CFBooleanRef == keyValueType);
						*(REINTERPRET_CAST(outDataPtr, Boolean*)) = inContextPtr->returnFlag(keyName);
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
}// getMacroSetPreference


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
					 My_ContextInterfacePtr&	outContextPtr)
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
Returns information about the dictionary key used to
store the given preferences tag’s data, the type of
data expected in a dictionary, the size of the data
type MacTelnet uses to read or write the data via
APIs such as Preferences_ContextGetData(), and an
indication of what class the tag belongs to.

If no particular class dictionary is required, then
"outClass" will be "kPreferences_ClassGeneral".  In
this case ONLY, Core Foundation Preferences may be
used to store or retrieve the key value directly in
the application’s globals.  However, keys intended
for storage in user Favorites will have a different
class, and must be set in a specific dictionary; see
copyClassDictionaryCFArray().

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
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// IMPORTANT: The data types used here are documented in Preferences.h,
	//            and relied upon in other methods.  They are also used in
	//            MacTelnetPrefsConverter.  Check for consistency!
	switch (inTag)
	{
	case kPreferences_TagANSIColorsEnabled:
		outKeyName = CFSTR("terminal-emulator-xterm-enable-color");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagArrangeWindowsUsingTabs:
		outKeyName = CFSTR("terminal-use-tabs");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagAssociatedTerminalFavorite:
		outKeyName = CFSTR("terminal-favorite");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(CFStringRef);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagAutoCaptureToFile:
		outKeyName = CFSTR("terminal-capture-auto-start");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagBellSound:
		outKeyName = CFSTR("terminal-when-bell-sound-basename");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(CFStringRef);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagCaptureFileAlias:
		outKeyName = CFSTR("terminal-capture-file-alias-id");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(Preferences_AliasID);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagCaptureFileCreator:
		outKeyName = CFSTR("terminal-capture-file-creator-code");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(OSType);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagCommandLine:
		outKeyName = CFSTR("command-line-token-strings");
		outKeyValueType = typeCFArrayRef;
		outNonDictionaryValueSize = sizeof(CFArrayRef);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagCopySelectedText:
		outKeyName = CFSTR("terminal-auto-copy-on-select");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagCopyTableThreshold:
		outKeyName = CFSTR("spaces-per-tab");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(UInt16);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagCursorBlinks:
		outKeyName = CFSTR("terminal-cursor-blinking");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagCursorMovesPriorToDrops:
		outKeyName = CFSTR("terminal-cursor-auto-move-on-drop");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagDataReceiveDoNotStripHighBit:
		outKeyName = CFSTR("data-receive-do-not-strip-high-bit");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagDataReadBufferSize:
		outKeyName = CFSTR("data-receive-buffer-size-bytes");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(SInt16);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagDontAutoClose:
		outKeyName = CFSTR("no-auto-close");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagDontAutoNewOnApplicationReopen:
		outKeyName = CFSTR("no-auto-new");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagDynamicResizing:
		outKeyName = CFSTR(""); // not used
		outKeyValueType = typeNetEvents_CFBooleanRef; // not used
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagEMACSMetaKey:
		outKeyName = CFSTR("key-map-emacs-meta");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Session_EMACSMetaKey);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagFocusFollowsMouse:
		outKeyName = CFSTR("terminal-focus-follows-mouse");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagFontName:
		outKeyName = CFSTR("terminal-font-family");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Str255);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagFontSize:
		outKeyName = CFSTR("terminal-font-size-points");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(SInt16);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagKeyInterruptProcess:
		outKeyName = CFSTR("command-key-interrupt-process");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(char);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagKeyResumeOutput:
		outKeyName = CFSTR("command-key-resume-output");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(char);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagKeySuspendOutput:
		outKeyName = CFSTR("command-key-suspend-output");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(char);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagKioskAllowsForceQuit:
		outKeyName = CFSTR("kiosk-force-quit-enabled");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagKioskShowsMenuBar:
		outKeyName = CFSTR("kiosk-menu-bar-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagKioskShowsOffSwitch:
		outKeyName = CFSTR("kiosk-off-switch-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagKioskShowsScrollBar:
		outKeyName = CFSTR("kiosk-scroll-bar-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagKioskUsesSuperfluousEffects:
		outKeyName = CFSTR("kiosk-effects-enabled");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagHeadersCollapsed:
		outKeyName = CFSTR("window-terminal-toolbar-invisible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagInfoWindowColumnOrdering:
		outKeyName = CFSTR("window-sessioninfo-column-order");
		outKeyValueType = typeCFArrayRef;
		outNonDictionaryValueSize = sizeof(CFArrayRef);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagLineModeEnabled:
		outKeyName = CFSTR("line-mode-enabled");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagLocalEchoEnabled:
		outKeyName = CFSTR("data-send-local-echo-enabled");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagLocalEchoHalfDuplex:
		outKeyName = CFSTR("data-send-local-echo-half-duplex");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagMacrosMenuVisible:
		outKeyName = CFSTR("menu-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassMacroSet;
		break;
	
	case kPreferences_TagMapArrowsForEMACS:
		outKeyName = CFSTR("command-key-emacs-move-down");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagMapBackquote:
		outKeyName = CFSTR("key-map-backquote");
		outKeyValueType = typeCFStringRef; // any keystroke string (commonly, blank "" or escape "\e")
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagMapCarriageReturnToCRNull:
		outKeyName = CFSTR("key-map-new-line");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagMapDeleteToBackspace:
		outKeyName = CFSTR("key-map-delete");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagMapKeypadTopRowForVT220:
		outKeyName = CFSTR("command-key-vt220-pf1");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagMarginBell:
		outKeyName = CFSTR("terminal-when-cursor-near-right-margin");
		outKeyValueType = typeCFStringRef; // one of: "bell", "ignore"
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagMenuItemKeys:
		outKeyName = CFSTR("menu-key-equivalents");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagNewCommandShortcutEffect:
		outKeyName = CFSTR("new-means");
		outKeyValueType = typeCFStringRef; // one of: "shell", "dialog", "default"
		outNonDictionaryValueSize = sizeof(UInt32);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagNotification:
		outKeyName = CFSTR("when-alert-in-background");
		outKeyValueType = typeCFStringRef; // one of: "alert", "animate", "badge", "ignore"
		outNonDictionaryValueSize = sizeof(SInt16);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagNotifyOfBeeps:
		outKeyName = CFSTR("terminal-when-bell-in-background");
		outKeyValueType = typeCFStringRef; // one of: "notify", "ignore"
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagPageKeysControlLocalTerminal:
		outKeyName = CFSTR("command-key-terminal-end");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagPasteBlockSize:
		outKeyName = CFSTR("data-send-paste-block-size-bytes");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(SInt16);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagPasteMethod:
		outKeyName = CFSTR("data-send-paste-method");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Clipboard_PasteMethod);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagPureInverse:
		outKeyName = CFSTR("terminal-inverse-selections");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagServerHost:
		outKeyName = CFSTR("server-host");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(CFStringRef);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagServerPort:
		outKeyName = CFSTR("server-port");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(SInt16);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagServerProtocol:
		outKeyName = CFSTR("server-protocol");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Session_Protocol);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagSimplifiedUserInterface:
		outKeyName = CFSTR("menu-command-set-simplified");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagTektronixMode:
		outKeyName = CFSTR("tek-mode");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(TektronixMode);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagTektronixPAGEClearsScreen:
		outKeyName = CFSTR("tek-page-clears-screen");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagTerminalAnswerBackMessage:
		outKeyName = CFSTR("terminal-emulator-answerback");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(CFStringRef);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalClearSavesLines:
		outKeyName = CFSTR("terminal-clear-saves-lines");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalColorANSIBlack:
		outKeyName = CFSTR("terminal-color-ansi-black-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIRed:
		outKeyName = CFSTR("terminal-color-ansi-red-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIGreen:
		outKeyName = CFSTR("terminal-color-ansi-green-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIYellow:
		outKeyName = CFSTR("terminal-color-ansi-yellow-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIBlue:
		outKeyName = CFSTR("terminal-color-ansi-blue-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIMagenta:
		outKeyName = CFSTR("terminal-color-ansi-magenta-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSICyan:
		outKeyName = CFSTR("terminal-color-ansi-cyan-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIWhite:
		outKeyName = CFSTR("terminal-color-ansi-white-normal-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIBlackBold:
		outKeyName = CFSTR("terminal-color-ansi-black-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIRedBold:
		outKeyName = CFSTR("terminal-color-ansi-red-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIGreenBold:
		outKeyName = CFSTR("terminal-color-ansi-green-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIYellowBold:
		outKeyName = CFSTR("terminal-color-ansi-yellow-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIBlueBold:
		outKeyName = CFSTR("terminal-color-ansi-blue-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIMagentaBold:
		outKeyName = CFSTR("terminal-color-ansi-magenta-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSICyanBold:
		outKeyName = CFSTR("terminal-color-ansi-cyan-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorANSIWhiteBold:
		outKeyName = CFSTR("terminal-color-ansi-white-bold-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorBlinkingForeground:
		outKeyName = CFSTR("terminal-color-blinking-foreground-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorBlinkingBackground:
		outKeyName = CFSTR("terminal-color-blinking-background-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorBoldForeground:
		outKeyName = CFSTR("terminal-color-bold-foreground-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorBoldBackground:
		outKeyName = CFSTR("terminal-color-bold-background-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorMatteBackground:
		outKeyName = CFSTR("terminal-color-matte-background-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorNormalForeground:
		outKeyName = CFSTR("terminal-color-normal-foreground-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalColorNormalBackground:
		outKeyName = CFSTR("terminal-color-normal-background-rgb");
		outKeyValueType = typeCFArrayRef; // containing 3 CFNumberRefs, floating point between 0 and 1
		outNonDictionaryValueSize = sizeof(RGBColor);
		outClass = kPreferences_ClassFormat;
		break;
	
	case kPreferences_TagTerminalCursorType:
		outKeyName = CFSTR("terminal-cursor-shape");
		outKeyValueType = typeCFStringRef; // one of: "block", "underline", "thick underline", "vertical bar", "thick vertical bar"
		outNonDictionaryValueSize = sizeof(TerminalView_CursorType);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagTerminalEmulatorType:
		outKeyName = CFSTR("terminal-emulator-type");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(Terminal_Emulator);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalLineWrap:
		outKeyName = CFSTR("terminal-line-wrap");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalPaddingLeft:
		outKeyName = CFSTR("terminal-padding-left-em");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(Float32);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalPaddingRight:
		outKeyName = CFSTR("terminal-padding-right-em");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(Float32);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalPaddingTop:
		outKeyName = CFSTR("terminal-padding-top-em");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(Float32);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalPaddingBottom:
		outKeyName = CFSTR("terminal-padding-bottom-em");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(Float32);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalResizeAffectsFontSize:
		outKeyName = CFSTR("terminal-resize-affects");
		outKeyValueType = typeCFStringRef; // one of: "screen", "font"
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagTerminalScreenColumns:
		outKeyName = CFSTR("terminal-screen-dimensions-columns");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(UInt16);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalScreenRows:
		outKeyName = CFSTR("terminal-screen-dimensions-rows");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(UInt16);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalScreenScrollbackRows:
		outKeyName = CFSTR("terminal-scrollback-size-lines");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(UInt16);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTerminalScrollDelay:
		outKeyName = CFSTR("terminal-scroll-delay-milliseconds");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(EventTime);
		outClass = kPreferences_ClassTerminal;
		break;
	
	case kPreferences_TagTextEncoding:
		outKeyName = CFSTR("terminal-text-encoding");
		outKeyValueType = typeNetEvents_CFNumberRef;
		outNonDictionaryValueSize = sizeof(TextEncoding);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagTextTranslationTable:
		outKeyName = CFSTR("terminal-text-translation-table");
		outKeyValueType = typeCFStringRef;
		outNonDictionaryValueSize = sizeof(CFStringRef);
		outClass = kPreferences_ClassSession;
		break;
	
	case kPreferences_TagVisualBell:
		outKeyName = CFSTR("terminal-when-bell");
		outKeyValueType = typeCFStringRef; // one of: "visual", "audio+visual"
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWasClipboardShowing:
		outKeyName = CFSTR("window-clipboard-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWasCommandLineShowing:
		outKeyName = CFSTR("window-commandline-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWasControlKeypadShowing:
		outKeyName = CFSTR("window-controlkeys-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWasFunctionKeypadShowing:
		outKeyName = CFSTR("window-functionkeys-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWasSessionInfoShowing:
		outKeyName = CFSTR("window-sessioninfo-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWasVT220KeypadShowing:
		outKeyName = CFSTR("window-vt220-keys-visible");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagWindowStackingOrigin:
		outKeyName = CFSTR("window-terminal-position-pixels");
		outKeyValueType = typeCFArrayRef; // containing 2 CFNumberRefs, short integers as pixel values
		outNonDictionaryValueSize = sizeof(Point);
		outClass = kPreferences_ClassGeneral;
		break;
	
	case kPreferences_TagXTermSequencesEnabled:
		outKeyName = CFSTR("terminal-emulator-xterm-enable-window-alteration-sequences");
		outKeyValueType = typeNetEvents_CFBooleanRef;
		outNonDictionaryValueSize = sizeof(Boolean);
		outClass = kPreferences_ClassTerminal;
		break;
	
	default:
		// ???
		result = kPreferences_ResultUnknownTagOrClass;
		break;
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
				case kPreferences_TagAssociatedTerminalFavorite:
				case kPreferences_TagServerHost:
				case kPreferences_TagTextTranslationTable:
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
							TektronixMode*		storedValuePtr = REINTERPRET_CAST(outDataPtr, TektronixMode*);
							
							
							if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("off"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTektronixModeNotAllowed;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("4014"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTektronixMode4014;
							}
							else if (kCFCompareEqualTo == CFStringCompare(valueCFString, CFSTR("4105"), kCFCompareCaseInsensitive))
							{
								*storedValuePtr = kTektronixMode4105;
							}
							else
							{
								// failed; make default
								*storedValuePtr = kTektronixModeNotAllowed; // arbitrary
								result = kPreferences_ResultBadVersionDataNotAvailable;
							}
							CFRelease(valueCFString), valueCFString = nullptr;
						}
					}
					break;
				
				case kPreferences_TagTextEncoding:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						SInt32					valueLong = inContextPtr->returnLong(keyName);
						TextEncoding* const		data = REINTERPRET_CAST(outDataPtr, TextEncoding*);
						
						
						*data = STATIC_CAST(valueLong, TextEncoding);
						if (0 == *data)
						{
							// failed; make default
							*data = kTextEncodingMacRoman; // arbitrary
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
				case kPreferences_TagANSIColorsEnabled:
				case kPreferences_TagDataReceiveDoNotStripHighBit:
				case kPreferences_TagTerminalClearSavesLines:
				case kPreferences_TagTerminalLineWrap:
				case kPreferences_TagXTermSequencesEnabled:
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
				
				case kPreferences_TagTerminalPaddingLeft:
				case kPreferences_TagTerminalPaddingRight:
				case kPreferences_TagTerminalPaddingTop:
				case kPreferences_TagTerminalPaddingBottom:
					{
						assert(typeNetEvents_CFNumberRef == keyValueType);
						Float32			valueFloat32 = inContextPtr->returnFloat(keyName);
						Float32* const	data = REINTERPRET_CAST(outDataPtr, Float32*);
						
						
						*data = valueFloat32;
						if (0 == *data)
						{
							// failed; make default
							*data = 0; // arbitrary
							result = kPreferences_ResultBadVersionDataNotAvailable;
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
Fills in a window preferences structure with values
corresponding to the specified tag’s saved preferences
and returns "true"; or, returns "false" on failure.

Not all window tags result in a complete set of boundaries;
in particular, the width or height may be zero, which
indicates no preference at all (or, not applicable).

(3.1)
*/
Boolean
getWindowPreferences	(Preferences_Tag		inWindowPreferenceTag,
						 MyWindowArrangement&	inoutWindowPrefs)
{
	Boolean		result = false;
	
	
	switch (inWindowPreferenceTag)
	{
	case kPreferences_WindowTagCommandLine:
		readMacTelnetCoordPreference(CFSTR("window-commandline-position-pixels"),
										inoutWindowPrefs.x, inoutWindowPrefs.y);
		readMacTelnetCoordPreference(CFSTR("window-commandline-size-pixels"),
										inoutWindowPrefs.width, inoutWindowPrefs.height);
		inoutWindowPrefs.height = 0; // the height of the command line is automatically set
		break;
	
	case kPreferences_WindowTagControlKeypad:
		readMacTelnetCoordPreference(CFSTR("window-controlkeys-position-pixels"),
										inoutWindowPrefs.x, inoutWindowPrefs.y);
		inoutWindowPrefs.width = 0; // the keypad has fixed width
		inoutWindowPrefs.height = 0; // the keypad has fixed height
		break;
	
	case kPreferences_WindowTagFunctionKeypad:
		readMacTelnetCoordPreference(CFSTR("window-functionkeys-position-pixels"),
										inoutWindowPrefs.x, inoutWindowPrefs.y);
		inoutWindowPrefs.width = 0; // the keypad has fixed width
		inoutWindowPrefs.height = 0; // the keypad has fixed height
		break;
	
	case kPreferences_WindowTagMacroSetup:
		readMacTelnetCoordPreference(CFSTR("window-macroeditor-position-pixels"),
										inoutWindowPrefs.x, inoutWindowPrefs.y);
		readMacTelnetCoordPreference(CFSTR("window-macroeditor-size-pixels"),
										inoutWindowPrefs.width, inoutWindowPrefs.height);
		inoutWindowPrefs.height = 0; // the height of the dialog cannot change
		break;
	
	case kPreferences_WindowTagPreferences:
		readMacTelnetCoordPreference(CFSTR("window-preferences-position-pixels"),
										inoutWindowPrefs.x, inoutWindowPrefs.y);
		inoutWindowPrefs.width = 0; // the saved size of this dialog is not restored
		inoutWindowPrefs.height = 0; // the saved size of this dialog is not restored
		break;
	
	case kPreferences_WindowTagVT220Keypad:
		readMacTelnetCoordPreference(CFSTR("window-vt220keys-position-pixels"),
										inoutWindowPrefs.x, inoutWindowPrefs.y);
		inoutWindowPrefs.width = 0; // the keypad has fixed width
		inoutWindowPrefs.height = 0; // the keypad has fixed height
		break;
	
	default:
		break;
	}
	
	return result;
}// getWindowPreferences


/*!
Adds default values for known preference keys.  If "inIsBrandNew"
is true, any existing values for those keys are replaced,
otherwise existing values are kept and any other defaults are
added.

\retval noErr
if successful

\retval some (probably resource-related) OS error
if some component could not be set up properly

(2.6)
*/
OSStatus
initPreferences		(Boolean	inIsBrandNew)
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
		result = readPreferencesDictionary(defaultPrefDictionary, false == inIsBrandNew/* merge */);
		CFRelease(defaultPrefDictionary), defaultPrefDictionary = nullptr;
		
		if (noErr == result)
		{
			// save the preferences...
			(Boolean)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
		}
	}
	
	return result;
}// initPreferences


/*!
Replaces the list of Favorites for a given class of
settings with the given list.  Typically, you start
by calling copyClassDictionaryCFArray() to get the
original list, then you create a mutable copy, make
your changes and pass it into this routine.

\retval kPreferences_ResultOK
if the array was overwritten successfully

\retval kPreferences_ResultUnknownTagOrClass
if the given preference class is unknown or does not have an array value

(3.1)
*/
Preferences_Result
overwriteClassDictionaryCFArray	(Preferences_Class	inClass,
								 CFArrayRef			inCFArrayOfCFDictionaries)
{
	Preferences_Result		result = kPreferences_ResultOK;
	
	
	// figure out which MacTelnet preferences key should hold this list of Favorites dictionaries
	switch (inClass)
	{
	case kPreferences_ClassFormat:
		setMacTelnetPreference(CFSTR("favorite-styles"), inCFArrayOfCFDictionaries);
		break;
	
	case kPreferences_ClassSession:
		setMacTelnetPreference(CFSTR("favorite-sessions"), inCFArrayOfCFDictionaries);
		break;
	
	case kPreferences_ClassTerminal:
		setMacTelnetPreference(CFSTR("favorite-terminals"), inCFArrayOfCFDictionaries);
		break;
	
	case kPreferences_ClassGeneral:
	case kPreferences_ClassWindow:
	default:
		// ???
		result = kPreferences_ResultUnknownTagOrClass;
		break;
	}
	
	return result;
}// overwriteClassDictionaryCFArray


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
Overrides stored preference values using master preferences
from the given dictionary (which may have come from a
"DefaultPreferences.plist" file, for example).

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
			CFPropertyListRef	foundValue = CFPreferencesCopyAppValue(kKey, kCFPreferencesCurrentApplication);
			
			
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
				//Console_WriteValueCFString("using DefaultPreferences.plist value for key", kKey); // debug
				CFPreferencesSetAppValue(kKey, keyValue, kCFPreferencesCurrentApplication);
			}
		}
	}
	delete [] keys;
	
	return result;
}// readPreferencesDictionary


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
			case kPreferences_TagFontName:
				{
					ConstStringPtr const	data = REINTERPRET_CAST(inDataPtr, ConstStringPtr);
					CFStringRef				fontNameCFString = nullptr;
					
					
					fontNameCFString = CFStringCreateWithPascalString(kCFAllocatorDefault, data, kCFStringEncodingMacRoman);
					if (nullptr == fontNameCFString) result = kPreferences_ResultGenericFailure;
					else
					{
						assert(typeCFStringRef == keyValueType);
						inContextPtr->addString(keyName, (data) ? fontNameCFString : CFSTR(""));
						CFRelease(fontNameCFString), fontNameCFString = nullptr;
					}
				}
				break;
			
			case kPreferences_TagFontSize:
				{
					SInt16 const* const		data = REINTERPRET_CAST(inDataPtr, SInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(keyName, *data);
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
						inContextPtr->addArray(keyName, colorCFArray);
						CFRelease(colorCFArray), colorCFArray = nullptr;
					}
					else result = kPreferences_ResultGenericFailure;
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
					changeNotify(inDataPreferenceTag);
				}
				break;
			
			case kPreferences_TagBellSound:
				{
					CFStringRef const	data = *(REINTERPRET_CAST(inDataPtr, CFStringRef const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, data);
					changeNotify(inDataPreferenceTag);
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
					changeNotify(inDataPreferenceTag);
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
					changeNotify(inDataPreferenceTag);
				}
				break;
			
			case kPreferences_TagDynamicResizing:
				{
					//Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					WindowInfo_SetDynamicResizing(nullptr, true/* no longer a preference */);
				}
				break;
			
			case kPreferences_TagFocusFollowsMouse:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag);
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
					inContextPtr->addArray(keyName, *data);
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
			
			case kPreferences_TagMapBackquote:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("\\e") : CFSTR(""));
					changeNotify(inDataPreferenceTag);
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
					changeNotify(inDataPreferenceTag);
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
					
					case kCommandNewSessionShell:
					default:
						setMacTelnetPreference(keyName, CFSTR("shell"));
						break;
					}
					changeNotify(inDataPreferenceTag);
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
					changeNotify(inDataPreferenceTag);
				}
				break;
			
			case kPreferences_TagSimplifiedUserInterface:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag);
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
					changeNotify(inDataPreferenceTag);
				}
				break;
			
			case kPreferences_TagTerminalResizeAffectsFontSize:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("font") : CFSTR("screen"));
					changeNotify(inDataPreferenceTag);
				}
				break;
			
			case kPreferences_TagVisualBell:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? CFSTR("visual") : CFSTR("audio"));
					changeNotify(inDataPreferenceTag);
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
Modifies the indicated macro set preference using the
given data (see Preferences.h and the definition of each
tag for comments on what data format is expected for each
one).

(3.0)
*/
Preferences_Result	
setMacroSetPreference	(My_ContextInterfacePtr		inContextPtr,
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
			switch (inDataPreferenceTag)
			{
			case kPreferences_TagMacrosMenuVisible:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					setMacTelnetPreference(keyName, (data) ? kCFBooleanTrue : kCFBooleanFalse);
					changeNotify(inDataPreferenceTag);
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
}// setMacroSetPreference


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
			case kPreferences_TagAssociatedTerminalFavorite:
			case kPreferences_TagServerHost:
			case kPreferences_TagTextTranslationTable:
				// all of these keys have Core Foundation string values
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(keyName, *data);
				}
				break;
			
			case kPreferences_TagAutoCaptureToFile:
			case kPreferences_TagLineModeEnabled:
			case kPreferences_TagLocalEchoEnabled:
			case kPreferences_TagLocalEchoHalfDuplex:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(keyName, data);
				}
				break;
			
			case kPreferences_TagCaptureFileAlias:
				{
					Preferences_AliasID const* const	data = REINTERPRET_CAST(inDataPtr, Preferences_AliasID const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(keyName, *data);
				}
				break;
			
			case kPreferences_TagCommandLine:
				{
					CFArrayRef const* const		data = REINTERPRET_CAST(inDataPtr, CFArrayRef const*);
					
					
					assert(typeCFArrayRef == keyValueType);
					inContextPtr->addArray(keyName, *data);
				}
				break;
			
			case kPreferences_TagDataReadBufferSize:
			case kPreferences_TagPasteBlockSize:
			case kPreferences_TagServerPort:
				{
					SInt16 const* const		data = REINTERPRET_CAST(inDataPtr, SInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(keyName, *data);
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
						inContextPtr->addString(keyName, charCFString);
						CFRelease(charCFString), charCFString = nullptr;
					}
				}
				break;
			
			case kPreferences_TagMapCarriageReturnToCRNull:
				{
					Boolean const* const	data = REINTERPRET_CAST(inDataPtr, Boolean const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(keyName, (*data) ? CFSTR("\\015\\000") : CFSTR("\\015\\012"));
				}
				break;
			
			case kPreferences_TagMapDeleteToBackspace:
				{
					Boolean const* const	data = REINTERPRET_CAST(inDataPtr, Boolean const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(keyName, (*data) ? CFSTR("backspace") : CFSTR("delete"));
				}
				break;
			
			case kPreferences_TagPasteMethod:
				{
					Clipboard_PasteMethod const* const	data = REINTERPRET_CAST(inDataPtr, Clipboard_PasteMethod const*);
					
					
					assert(typeCFStringRef == keyValueType);
					switch (*data)
					{
					case kClipboard_PasteMethodStandard:
						inContextPtr->addString(keyName, CFSTR("normal"));
						break;
					
					case kClipboard_PasteMethodBlock:
						inContextPtr->addString(keyName, CFSTR("throttled"));
						break;
					
					default:
						// ???
						inContextPtr->addString(keyName, CFSTR("normal"));
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
						inContextPtr->addString(keyName, CFSTR("ftp"));
						break;
					
					case kSession_ProtocolSFTP:
						inContextPtr->addString(keyName, CFSTR("sftp"));
						break;
					
					case kSession_ProtocolSSH1:
						inContextPtr->addString(keyName, CFSTR("ssh-1"));
						break;
					
					case kSession_ProtocolSSH2:
						inContextPtr->addString(keyName, CFSTR("ssh-2"));
						break;
					
					case kSession_ProtocolTelnet:
						inContextPtr->addString(keyName, CFSTR("telnet"));
						break;
					
					default:
						// ???
						inContextPtr->addString(keyName, CFSTR("ssh-1"));
						break;
					}
				}
				break;
			
			case kPreferences_TagTektronixMode:
				{
					TektronixMode const* const	data = REINTERPRET_CAST(inDataPtr, TektronixMode const*);
					
					
					assert(typeCFStringRef == keyValueType);
					switch (*data)
					{
					case kTektronixModeNotAllowed:
						inContextPtr->addString(keyName, CFSTR("off"));
						break;
					
					case kTektronixMode4014:
						inContextPtr->addString(keyName, CFSTR("4014"));
						break;
					
					case kTektronixMode4105:
						inContextPtr->addString(keyName, CFSTR("4105"));
						break;
					
					default:
						// ???
						inContextPtr->addString(keyName, CFSTR("off"));
						break;
					}
				}
				break;
			
			case kPreferences_TagTextEncoding:
				{
					TextEncoding const* const	data = REINTERPRET_CAST(inDataPtr, TextEncoding const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addLong(keyName, *data);
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
			case kPreferences_TagANSIColorsEnabled:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(keyName, data);
				}
				break;
			
			case kPreferences_TagDataReceiveDoNotStripHighBit:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(keyName, data);
				}
				break;
			
			case kPreferences_TagEMACSMetaKey:
				{
					UInt16 const	data = *(REINTERPRET_CAST(inDataPtr, UInt16 const*));
					
					
					assert(typeCFStringRef == keyValueType);
					switch (data)
					{
					case kSession_EMACSMetaKeyOption:
						inContextPtr->addString(keyName, CFSTR("option"));
						break;
					
					case kSession_EMACSMetaKeyControlCommand:
						inContextPtr->addString(keyName, CFSTR("control+command"));
						break;
					
					case kSession_EMACSMetaKeyOff:
					default:
						inContextPtr->addString(keyName, CFSTR(""));
						break;
					}
				}
				break;
			
			case kPreferences_TagMapArrowsForEMACS:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(CFSTR("command-key-emacs-move-down"),
											(data) ? CFSTR("down-arrow") : CFSTR(""));
					inContextPtr->addString(CFSTR("command-key-emacs-move-left"),
											(data) ? CFSTR("left-arrow") : CFSTR(""));
					inContextPtr->addString(CFSTR("command-key-emacs-move-right"),
											(data) ? CFSTR("right-arrow") : CFSTR(""));
					inContextPtr->addString(CFSTR("command-key-emacs-move-up"),
											(data) ? CFSTR("up-arrow") : CFSTR(""));
				}
				break;
			
			case kPreferences_TagMapKeypadTopRowForVT220:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(CFSTR("command-key-vt220-pf1"),
											(data) ? CFSTR("") : CFSTR("keypad-clear"));
					inContextPtr->addString(CFSTR("command-key-vt220-pf2"),
											(data) ? CFSTR("") : CFSTR("keypad-="));
					inContextPtr->addString(CFSTR("command-key-vt220-pf3"),
											(data) ? CFSTR("") : CFSTR("keypad-/"));
					inContextPtr->addString(CFSTR("command-key-vt220-pf4"),
											(data) ? CFSTR("") : CFSTR("keypad-*"));
				}
				break;
			
			case kPreferences_TagPageKeysControlLocalTerminal:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(CFSTR("command-key-terminal-end"),
											(data) ? CFSTR("end") : CFSTR(""));
					inContextPtr->addString(CFSTR("command-key-terminal-home"),
											(data) ? CFSTR("home") : CFSTR(""));
					inContextPtr->addString(CFSTR("command-key-terminal-page-down"),
											(data) ? CFSTR("page-down") : CFSTR(""));
					inContextPtr->addString(CFSTR("command-key-terminal-page-up"),
											(data) ? CFSTR("page-up") : CFSTR(""));
				}
				break;
			
			case kPreferences_TagTerminalAnswerBackMessage:
				{
					CFStringRef const* const	data = REINTERPRET_CAST(inDataPtr, CFStringRef const*);
					
					
					assert(typeCFStringRef == keyValueType);
					inContextPtr->addString(keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalClearSavesLines:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(keyName, data);
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
						inContextPtr->addString(keyName, nameCFString.returnCFStringRef());
					}
				}
				break;
			
			case kPreferences_TagTerminalLineWrap:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(keyName, data);
				}
				break;
			
			case kPreferences_TagTerminalPaddingLeft:
			case kPreferences_TagTerminalPaddingRight:
			case kPreferences_TagTerminalPaddingTop:
			case kPreferences_TagTerminalPaddingBottom:
				{
					Float32 const	data = *(REINTERPRET_CAST(inDataPtr, Float32 const*));
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addFloat(keyName, data);
				}
				break;
			
			case kPreferences_TagTerminalScreenColumns:
			case kPreferences_TagTerminalScreenRows:
			case kPreferences_TagTerminalScreenScrollbackRows:
				{
					UInt16 const* const		data = REINTERPRET_CAST(inDataPtr, UInt16 const*);
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(keyName, *data);
				}
				break;
			
			case kPreferences_TagTerminalScrollDelay:
				{
					EventTime const* const		data = REINTERPRET_CAST(inDataPtr, EventTime const*);
					EventTime					junk = *data / kEventDurationMillisecond;
					
					
					assert(typeNetEvents_CFNumberRef == keyValueType);
					inContextPtr->addInteger(keyName, STATIC_CAST(junk, SInt16));
				}
				break;
			
			case kPreferences_TagXTermSequencesEnabled:
				{
					Boolean const	data = *(REINTERPRET_CAST(inDataPtr, Boolean const*));
					
					
					assert(typeNetEvents_CFBooleanRef == keyValueType);
					inContextPtr->addFlag(CFSTR("terminal-emulator-xterm-enable-color"), data);
					inContextPtr->addFlag(CFSTR("terminal-emulator-xterm-enable-graphics"), data);
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
Tests the preferences-based CFDictionary context.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messagess should be
printed for ALL assertion failures regardless.

(3.1)
*/
Boolean
unitTest002_Begin ()
{
	Boolean		result = true;
	
	
	result = My_ContextFavorite::unitTest();
	return result;
}// unitTest002_Begin

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
