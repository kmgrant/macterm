/*!	\file Network.mm
	\brief APIs dealing with the local machine’s Internet
	protocol addresses.
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

#import "Network.h"
#import <UniversalDefines.h>

// Mac includes
#import <CoreServices/CoreServices.h>
#import <SystemConfiguration/SystemConfiguration.h>

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <ListenerModel.h>

// application includes
#import "ConstantsRegistry.h"



#pragma mark Internal Method Prototypes
namespace {

void				changeNotify									(Network_Change, void*, Boolean = false);
SCDynamicStoreRef	createDynamicStore							();
void				regenerateAddressListWithCompletionBlock		(void (^)());
void				systemConfigChanged							(SCDynamicStoreRef, CFArrayRef, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

SCDynamicStoreRef&		gDynamicStore ()		{ static SCDynamicStoreRef _ = createDynamicStore(); return _; }
ListenerModel_Ref&		gNetworkChangeListenerModel ()
						{
							static ListenerModel_Ref		_ = ListenerModel_New
															(kListenerModel_StyleStandard,
																kConstantsRegistry_ListenerModelDescriptorNetwork);
							
							
							return _;
						}
NSMutableArray*&		gRawAddressStringArray()		{ static NSMutableArray* _ = [[NSMutableArray alloc] init]; return _; }
BOOL					gAddressListRebuildInProgress = NO;

} // anonymous namespace



#pragma mark Public Methods


/*!
Creates a new CFArrayRef containing CFStringRef values for all
known local network addresses of the current host.  Call
CFRelease() on the array when you are finished using it.

If the optional parameter "outIsCompletePtr" is provided, the
caller is told whether or not the list is complete.  The list
is asynchronously regenerated at certain times, such as upon
first request and when the system has indicated a change to the
IPv4 or IPv6 characteristics of the computer.

(2016.10)
*/
void
Network_CopyLocalHostAddresses	(CFArrayRef&		outAddresses,
								 Boolean*		outIsCompletePtr)
{
	// now that the IP address list is in use, set up the
	// systemConfigChanged() callback to detect any
	// changes to the IPv4 or IPv6 setup of the computer
	// (does nothing if already initialized previously)
	gDynamicStore();
	
	if ((0 == gRawAddressStringArray().count) && (NO == gAddressListRebuildInProgress))
	{
		// no addresses available and no rebuild has happened;
		// initiate a background rebuild of the list (note
		// that this can also happen automatically later; see
		// systemConfigChanged())
		regenerateAddressListWithCompletionBlock(nil);
	}
	
	if (nullptr != outIsCompletePtr)
	{
		*outIsCompletePtr = (NO == gAddressListRebuildInProgress);
	}
	
	outAddresses = BRIDGE_CAST([gRawAddressStringArray() copy], CFArrayRef);
}// CopyLocalHostAddresses


/*!
Initiates an asynchronous rebuild of the address list,
which will trigger "kNetwork_ChangeAddressListWillRebuild"
and "kNetwork_ChangeAddressListDidRebuild" changes (which
can be monitored using Network_StartMonitoring()).

Normally this is not necessary because this module will
receive automatic notification of significant IPv4 and IPv6
configuration changes from System Configuration Framework.
This function could be used to implement a user interface
for forcing a refresh.

(2016.10)
*/
void
Network_ResetLocalHostAddresses ()
{
	regenerateAddressListWithCompletionBlock(nil); // returns immediately but can trigger monitors, etc.
}// ResetLocalHostAddresses


/*!
Arranges for a callback to be invoked for the given type
of network-related change.

The "inSetupFlags" parameter can be used to customize the
setup of the monitor, such as by requesting that the
callback be invoked immediately (for initialization) in
addition to the normal triggers.

\retval kNetwork_ResultOK
if no error occurs

\retval kNetwork_ResultParameterError
if the specified change type is not valid

(2016.10)
*/
Network_Result
Network_StartMonitoring		(Network_Change				inForWhatChange,
							 ListenerModel_ListenerRef	inListener,
							 Network_MonitorFlags		inSetupFlags)
{
	Network_Result		result = kNetwork_ResultParameterError;
	
	
	switch (inForWhatChange)
	{
	// Keep this in sync with Network_StopMonitoring().
	case kNetwork_ChangeAddressListDidRebuild:
	case kNetwork_ChangeAddressListWillRebuild:
		{
			OSStatus	error = noErr;
			
			
			error = ListenerModel_AddListenerForEvent(gNetworkChangeListenerModel(), inForWhatChange,
														inListener);
			if (error != noErr)
			{
				result = kNetwork_ResultMonitorFailed;
			}
			else
			{
				result = kNetwork_ResultOK;
				
				// if requested, invoke the callback immediately so that
				// the receiver can react accordingly given the initial
				// value of a particular setting
				if (inSetupFlags & kNetwork_MonitorFlagNotifyOfInitialValue)
				{
					changeNotify(inForWhatChange, nullptr/* context */, true/* is initial value */);
				}
			}
		}
		break;
	
	default:
		// unsupported tag for notifiers
		result = kNetwork_ResultParameterError;
		break;
	}
	
	return result;
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked when
the specified network-related change occurs.

\retval kNetwork_ResultOK
if no error occurs

\retval kNetwork_ResultParameterError
if the specified change type is not valid

(2016.10)
*/
Network_Result
Network_StopMonitoring	(Network_Change				inForWhatChange,
						 ListenerModel_ListenerRef	inListener)
{
	Network_Result		result = kNetwork_ResultParameterError;
	
	
	switch (inForWhatChange)
	{
	// Keep this in sync with Network_StartMonitoring().
	case kNetwork_ChangeAddressListDidRebuild:
	case kNetwork_ChangeAddressListWillRebuild:
		ListenerModel_RemoveListenerForEvent(gNetworkChangeListenerModel(), inForWhatChange, inListener);
		result = kNetwork_ResultOK;
		break;
	
	default:
		// unsupported tag for notifiers
		result = kNetwork_ResultParameterError;
		break;
	}
	
	return result;
}// StopMonitoring


#pragma mark Internal Methods
namespace {

/*!
Notifies all listeners for the specified network
state change, passing the given context to the
listener.

(2016.10)
*/
void
changeNotify	(Network_Change		inWhatChanged,
				 void*				inContextOrNull,
				 Boolean				inIsInitialValue)
{
#pragma unused(inIsInitialValue)
	// invoke listener callback routines appropriately, from the network listener model
	ListenerModel_NotifyListenersOfEvent(gNetworkChangeListenerModel(), inWhatChanged, inContextOrNull);
}// changeNotify


/*!
Allocates and configures a dynamic store to invoke the
systemConfigChanged() local callback when IPv4 or IPv6
characteristics of the network environment change.

IMPORTANT:	Do not call this directly; use gDynamicStore().

(2016.10)
*/
SCDynamicStoreRef
createDynamicStore ()
{
	SCDynamicStoreRef	result = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("MacTerm"),
														systemConfigChanged, nullptr/* context */);
	
	
	if (nullptr != result)
	{
		NSArray*	patternArray = @[@"State:/Network/Interface/.*/IPv4",
										@"State:/Network/Interface/.*/IPv6"];
		Boolean		notificationSetupOK = SCDynamicStoreSetNotificationKeys
											(result, nullptr/* raw keys */,
												BRIDGE_CAST(patternArray, CFArrayRef));
		
		
		if (false == notificationSetupOK)
		{
			Console_Warning(Console_WriteValueCString,
							"unable to set up IP address auto-refresh; system config. framework error",
							SCErrorString(SCError()));
		}
		else
		{
			CFRunLoopSourceRef	asRunLoopSourceRef = SCDynamicStoreCreateRunLoopSource
														(kCFAllocatorDefault, result, 0/* index */);
			CFRetainRelease		runLoopSource(asRunLoopSourceRef, CFRetainRelease::kAlreadyRetained);
			
			
			CFRunLoopAddSource(CFRunLoopGetCurrent(), asRunLoopSourceRef, kCFRunLoopDefaultMode);
		}
	}
	
	return result;
}// createDynamicStore


/*!
Initiates an asynchronous rebuild of the list of addresses
for the current host.

If a completion block is given, this function will wait
until the regeneration is complete before executing the
block.  If the block is nil, this returns immediately
without necessarily having a complete address list stored.

(2016.10)
*/
void
regenerateAddressListWithCompletionBlock		(void	(^inCompletionBlock)())
{
	dispatch_queue_t	originalQueue = dispatch_get_current_queue();
	
	
	if (NO == gAddressListRebuildInProgress)
	{
		gAddressListRebuildInProgress = YES;
		
		// asynchronously find the list of addresses, as this is
		// not guaranteed to return immediately
		//Console_WriteLine("initiating search for current IP addresses (raw strings)…"); // debug
		
		changeNotify(kNetwork_ChangeAddressListWillRebuild, nullptr/* context */);
		
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0/* flags */),
		^{
			NSHost*				currentHost = [NSHost currentHost]; // may block indefinitely
			__block NSArray*	addressList = currentHost.addresses; // returns a copy
			
			
			[gRawAddressStringArray() removeAllObjects];
			[gRawAddressStringArray() addObjectsFromArray:addressList];
			
			// despite "copy" attribute of property, system
			// complains if this is freed; for now, leave it
			//[addressList release], addressList = nil;
			
			//Console_WriteLine("completed IP address search"); // debug
			gAddressListRebuildInProgress = NO;
			
			dispatch_async(originalQueue,
			^{
				changeNotify(kNetwork_ChangeAddressListDidRebuild, nullptr/* context */);
				if (nil != inCompletionBlock)
				{
					// notify the caller
					dispatch_async(originalQueue, inCompletionBlock);
				}
			});
		});
	}
}// regenerateAddressListWithCompletionBlock


/*!
Responds to monitored changes in the system configuration
framework by rebuilding the IP address list.

(2016.10)
*/
void
systemConfigChanged		(SCDynamicStoreRef		inDynamicStore,
						 CFArrayRef				inModifiedKeys,
						 void*					inContext)
{
#pragma unused(inContext)
	assert(inDynamicStore == gDynamicStore());
	
	NSArray*	asKeyArray = BRIDGE_CAST(inModifiedKeys, NSArray*);
	BOOL		debug = NO;
	
	
	Console_WriteLine("detected IPv4 or IPv6 system configuration changes");
	if (debug)
	{
		for (id object : asKeyArray)
		{
			assert([object isKindOfClass:NSString.class]);
			NSString*	asString = STATIC_CAST(object, NSString*);
			
			
			Console_WriteValueCFString("change to key", BRIDGE_CAST(asString, CFStringRef));
		}
	}
	
	Network_ResetLocalHostAddresses();
}// systemConfigChanged

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
