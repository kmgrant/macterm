/*!	\file RandomWrap.h
	\brief Creates STL-compatible class for random() and
	other standard APIs.
	
	STL algorithms such as random_shuffle() are easier to
	use with no-argument calls such as random() and
	arc4random(), using this class.
*/
/*###############################################################

	Data Access Library
	© 1998-2017 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#ifndef __RANDOMWRAP__
#define __RANDOMWRAP__

// standard-C includes
#include <cstdlib>

// standard-C++ includes
#include <functional>



#pragma mark Types

class RandomWrap:
public std::unary_function< unsigned long, unsigned long >
{
public:
	enum Algorithm
	{
		kAlgorithmRandom		= 0,	// use random()
		kAlgorithmArc4Random	= 1		// use arc4random()
	};
	
	inline RandomWrap	(Algorithm = kAlgorithmRandom);
	
	inline unsigned long
	operator () (unsigned long) const;

protected:
private:
	Algorithm	_algorithm;
};



#pragma mark Public Methods

RandomWrap::
RandomWrap	(Algorithm		inAlgorithm)
:
std::unary_function< unsigned long, unsigned long >(),
_algorithm(inAlgorithm)
{
}// one-argument constructor


/*!
Calls either arc4random() or random(), and returns a random
number within the specified domain (strictly less than).

(2.0)
*/
unsigned long
RandomWrap::
operator ()	(unsigned long		inDomain)
const
{
	unsigned long		result = (kAlgorithmArc4Random == _algorithm)
									? ::arc4random()
									: STATIC_CAST(::random(), unsigned long);
	
	
	result %= inDomain;
	return result;
}// operator ()

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
