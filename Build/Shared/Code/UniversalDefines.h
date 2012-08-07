/*!	\file UniversalDefines.h
	\brief Definitions with "#define" that can be applied to
	ANY Mac program.
	
	Do not simply put whatever you want in this file: a good
	rule of thumb is, do not place anything in this file that
	is specific to an application or library.  These defines
	should be applicable to any modern Mac program.
*/
/*###############################################################

	Universal Defines
		© 1998-2012 by Kevin Grant
	
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

#ifndef __UNIVERSALDEFINES__
#define __UNIVERSALDEFINES__

#if !REZ

// standard-C includes
#ifdef __cplusplus
#	include <cassert>
#	include <climits>
#else
#	include <assert.h>
#	include <limits.h>
#endif

#define __CF_USE_FRAMEWORK_INCLUDES__
#include <Carbon/Carbon.h>



/*###############################################################
	UNUSED ARGUMENTS (PREVENTING COMPILER WARNINGS)
###############################################################*/

//! use to make unused function parameters unnamed;
//! e.g. int f (int arg) would be int f (int UNUSED_ARGUMENT(arg))
#define UNUSED_ARGUMENT(arg)



/*###############################################################
	“C IMPROVED”
###############################################################*/

//! C string type, defined as a NULL-terminated character array
typedef char*			CStringPtr;
typedef char const*		ConstCStringPtr;

//! in C++0x, there will be a "nullptr" keyword...start using it now!
#define nullptr 0L

//! à la Perl, this is my preferred if-not term
#define unless(x)		if (!(x))

//! when a method is declared private but is not implemented
//! (e.g. a constructor for a class not meant to be instantiated),
//! use this macro to clarify intent:
//!		void myMethod () NO_METHOD_IMPL;
#define NO_METHOD_IMPL

//! C++ helper to handle compilers that don’t define things in the CPP_STD:: namespace
#ifdef __MACH__
#	define CPP_STD
#else
#	define CPP_STD		::std
#endif

//! useful “cast operators” that are completely unnecessary, but make code
//! more self-documenting (they clarify the programmer’s intentions)
#ifdef __cplusplus
#define BRIDGE_CAST(ref, intendedType)			((intendedType)(ref))
#define CONST_CAST(ref, intendedType)			const_cast< intendedType >(ref)
#define VOLATILE_CAST(ref, intendedType)		const_cast< intendedType >(ref) // C++ says const_cast overrides volatile too
#define REINTERPRET_CAST(ref, intendedType)		reinterpret_cast< intendedType >(ref)
#define STATIC_CAST(ref, intendedType)			static_cast< intendedType >(ref)
#else
#define BRIDGE_CAST(ref, intendedType)			((intendedType)(ref))	//!< “I am exchanging NSFoo* and CFFooRef because they are DOCUMENTED as being toll-free bridged”
#define CONST_CAST(ref, intendedType)			((intendedType)(ref))	//!< “I know I am turning a constant pointer into a non-constant one”
#define VOLATILE_CAST(ref, intendedType)		((intendedType)(ref))	//!< “I know I am turning a volatile value into a non-volatile one”
#define REINTERPRET_CAST(ref, intendedType)		((intendedType)(ref))	//!< “I want to look at data as something else” (e.g. integer to pointer)
#define STATIC_CAST(ref, intendedType)			((intendedType)(ref))	//!< identical to a cast in the traditional sense (e.g. "char" to "int")
#endif

//! a static assertion fails at COMPILE TIME if the asserted
//! condition is false; admittedly the "negative array size"
//! error isn't all that clear but at least it fails with
//! the right file and line; the name of the array is also a
//! hint about which condition is false (in C++11 this macro
//! will no longer be necessary)
#define STATIC_ASSERT(cond,name) \
	typedef int assert_##name[-1 + 2 * (cond)]


/*###############################################################
	MAC OS CONSTANTS AND UTILITIES
###############################################################*/

enum
{
	//! for consistency purposes, this is very useful
	RGBCOLOR_INTENSITY_MAX = 65535
};

//! use this to “reference” a constant that is not available in current headers (for example,
//! something declared in the headers of a future OS); the 2nd parameter is ignored, the 1st
//! value is substituted directly; this is useful both to retain the extra context provided
//! by the named constant, and to serve as a search target if in the future it is desirable
//! to replace instances of this macro with “real” constant references when they are available
#define FUTURE_SYMBOL(v,n)	(v)

//! useful integer routines, for efficiency; optimized for the PowerPC platform
#ifdef __cplusplus
inline long				INTEGER_ABSOLUTE	(long a)			{ return ((a >= 0) ? a : -a); }	//!< take absolute value of an integer
inline long				INTEGER_MAXIMUM		(long a, long b)	{ return ((a > b) ? a : b); }	//!< return largest of two signed integers
inline long				INTEGER_MINIMUM		(long a, long b)	{ return ((a < b) ? a : b); }	//!< return smallest of two signed integers
inline size_t			INTEGER_MEGABYTES	(unsigned int a)	{ return (a << 20); }			//!< return byte count for integer value measured in MB
inline size_t			INTEGER_KILOBYTES	(unsigned int a)	{ return (a << 10); }			//!< return byte count for integer value measured in K
inline unsigned int		INTEGER_OCTUPLED	(unsigned long a)	{ return (a << 3); }			//!< multiply integer by 8
inline unsigned int		INTEGER_QUADRUPLED	(unsigned long a)	{ return (a << 2); }			//!< multiply integer by 4
inline unsigned int		INTEGER_TRIPLED		(unsigned long a)	{ return ((a << 1) + a); }		//!< multiply integer by 3
inline unsigned int		INTEGER_DOUBLED		(unsigned long a)	{ return (a << 1); }			//!< multiply integer by 2
inline unsigned int		INTEGER_HALVED		(unsigned long a)	{ return (a >> 1); }			//!< divide integer by 2
inline unsigned int		INTEGER_QUARTERED	(unsigned long a)	{ return (a >> 2); }			//!< divide integer by 4
inline unsigned int		INTEGER_EIGHTHED	(unsigned long a)	{ return (a >> 3); }			//!< divide integer by 8
#else
#	define INTEGER_ABSOLUTE(a)		(((a) >= 0) ? (a) : (-(a)))
#	define INTEGER_MAXIMUM(a, b)	(((a) > (b)) ? (a) : (b))
#	define INTEGER_MINIMUM(a, b)	(((a) < (b)) ? (a) : (b))
#	define INTEGER_MEGABYTES(a)		((size_t)((a) << 20))
#	define INTEGER_KILOBYTES(a)		((size_t)((a) << 10))
#	define INTEGER_OCTUPLED(a)		((a) << 3)
#	define INTEGER_QUADRUPLED(a)	((a) << 2)
#	define INTEGER_TRIPLED(a)		(((a) << 1) + (a))
#	define INTEGER_DOUBLED(a)		((a) << 1)
#	define INTEGER_HALVED(a)		((a) >> 1)
#	define INTEGER_QUARTERED(a)		((a) >> 2)
#	define INTEGER_EIGHTHED(a)		((a) >> 3)
#endif

//! useful floating point routines, for efficiency; optimized for the PowerPC platform
#ifdef __cplusplus
inline double FLOAT64_ABSOLUTE		(double a)				{ return ((a >= 0) ? a : -a); }		//!< take absolute value of a floating-point number
inline double FLOAT64_MAXIMUM		(double a, double b)	{ return ((a > b) ? a : b); }		//!< return largest of two signed floating-point numbers
inline double FLOAT64_MINIMUM		(double a, double b)	{ return ((a < b) ? a : b); }		//!< return smallest of two signed floating-point numbers
inline double FLOAT64_OCTUPLED		(double a)				{ return (a * 8.0); }				//!< multiply floating-point number by 8
inline double FLOAT64_QUADRUPLED	(double a)				{ return (a * 4.0); }				//!< multiply floating-point number by 4
inline double FLOAT64_TRIPLED		(double a)				{ return ((a * 2.0) + a); }			//!< multiply floating-point number by 3
inline double FLOAT64_DOUBLED		(double a)				{ return (a * 2.0); }				//!< multiply floating-point number by 2
inline double FLOAT64_HALVED		(double a)				{ return (a / 2.0); }				//!< divide floating-point number by 2
inline double FLOAT64_QUARTERED		(double a)				{ return (a / 4.0); }				//!< divide floating-point number by 4
inline double FLOAT64_EIGHTHED		(double a)				{ return (a / 8.0); }				//!< divide floating-point number by 8
#else
#	define FLOAT64_ABSOLUTE(a)		(((a) >= 0) ? (a) : (-(a)))
#	define FLOAT64_MAXIMUM(a, b)	(((a) > (b)) ? (a) : (b))
#	define FLOAT64_MINIMUM(a, b)	(((a) < (b)) ? (a) : (b))
#	define FLOAT64_OCTUPLED(a)		((a) * 8.0)
#	define FLOAT64_QUADRUPLED(a)	((a) * 4.0)
#	define FLOAT64_TRIPLED(a)		(((a) * 2.0) + (a))
#	define FLOAT64_DOUBLED(a)		((a) * 2.0)
#	define FLOAT64_HALVED(a)		((a) / 2.0)
#	define FLOAT64_QUARTERED(a)		((a) / 4.0)
#	define FLOAT64_EIGHTHED(a)		((a) / 8.0)
#endif

// HIObjectGetEventTarget() is defined back to 10.2, but the 10.3 headers
// do not declare HIViewInstallEventHandler(); to minimize the need to
// use 10.4 headers when 10.3 headers should do, define this macro here
#ifndef HIViewInstallEventHandler
#define HIViewInstallEventHandler( target, handler, numTypes, list, userData, outHandlerRef ) \
       InstallEventHandler( HIObjectGetEventTarget( (HIObjectRef) (target) ), (handler), (numTypes), (list), (userData), (outHandlerRef) )
#endif

#endif /* REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
