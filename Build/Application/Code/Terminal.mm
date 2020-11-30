/*!	\file Terminal.mm
	\brief Terminal emulators.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#import "Terminal.h"
#import <UniversalDefines.h>

// standard-C includes
#import <algorithm>
#import <cctype>
#import <cstdio>
#import <cstdlib>
#import <cstring>
#import <set>

// standard-C++ includes
#import <algorithm>
#import <iterator>
#import <list>
#import <map>
#import <sstream>
#import <stdexcept>
#import <string>
#import <utility>
#import <vector>

// UNIX includes
extern "C"
{
#	include <errno.h>
#	include <pthread.h>
}

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <Registrar.template.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "Commands.h"
#import "DebugInterface.h"
#import "Emulation.h"
#import "Preferences.h"
#import "PrintTerminal.h"
#import "QuillsTerminal.h"
#import "Session.h"
#import "SixelDecoder.h"
#import "StreamCapture.h"
#import "TerminalLine.h"
#import "TerminalSpeaker.h"
#import "TextTranslation.h"
#import "UIStrings.h"
#import "UTF8Decoder.h"
#import "VTKeys.h"



#pragma mark Constants
namespace {

/*!
Valid terminal emulator parameter values (as stored by the class
My_Emulator) can be any integer starting from 0.  Any negative
value is invalid, but these are overloaded to represent special
states.

A parameter list does not necessarily contain a contiguous range
of undefined values.  For instance, "ESC[;1m" is an undefined
first parameter, but a second parameter that is equal to 1.  For
this reason, ALWAYS check the MAXIMUM number of parameters your
sequence can accept, to see if ANY of them is defined.

The very first (index 0) parameter is a sentinel; if it is set
to kMy_ParamUndefined, the entire parameter list is invalid.  If
it is set to any other sentinel value, then the first (index 0)
parameter is invalid because it is the sentinel, but it changes
the meaning of the parameters that follow, and also shifts them
by one (so index 1 is now the first parameter, not index 0).

IMPORTANT:	This code GUARANTEES that it is safe to rely on
			relational operators such as "< 0" to scan quickly
			for any sentinel value.  You do not have to build
			complex switch-statements to test for all possible
			values.
*/
enum
{
	// all of these must be negative numbers
	kMy_ParamUndefined		= -1,	//!< meta-value; means that the parameter does not actually exist (undefined at index 0 implies empty list)
	kMy_ParamPrivate		= -2,	//!< meta-value; means that the parameters came from an ESC[?... sequence
	kMy_ParamSecondaryDA	= -3,	//!< meta-value; means that the parameters came from an ESC[>... sequence
	kMy_ParamTertiaryDA		= -4,	//!< meta-value; means that the parameters came from an ESC[=... sequence
};

/*!
A parser state represents a recent history of input that limits
what can happen next (based on future input).

The list below contains generic names, however the same values
are often used to define aliases in specific terminal classes
(like My_VT100).  This ties a terminal-specific state to a
generic one, allowing the default parser to do most of the state
determination/transition work on behalf of all terminals!

Note that most complex sequences are defined by a final state
that is reached implicitly be transitioning through trivial
intermediate states (corresponding to characters that are read
while the sequence is incomplete).  This is largely a “correct
by construction” approach.  There are exceptions however, such
as the CSI parameter parser ("ESC[...;...;...X" for some byte X
that is the terminator) which absorbs characters without having
a lot of intermediate states; it follows that certain sequences
such as "ESC[?" do not have states, and cannot, because the data
they require will be absorbed by a parser from a simpler state
(in this case, the "ESC[" state).
*/
typedef UInt32 My_ParserState;
typedef std::pair< My_ParserState, My_ParserState >		My_ParserStatePair;
enum
{
	// key states
	kMy_ParserStateInitial						= 'init',	//!< the very first state, no characters have yet been entered
	kMy_ParserStateAccumulateForEcho			= 'echo',	//!< no sense could be made of the input, so gather it for later translation and display
	
	// generic states - these are automatically handled by the default state determinant based on the data stream
	// IMPORTANT: states that represent an ESC-<CODE> sequence are coded as 'ESC\0' + <CODE> in part to simplify
	// the code that translates from 8-bit streams to 7-bit sequences and vice-versa; if ANY escape sequence
	// cannot follow this convention, look at the state determinant in the VT100 emulator and fix its 8-bit handler!
	kMy_ParserStateSeenNull						= 'Ctl@',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlA					= 'CtlA',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlB					= 'CtlB',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlC					= 'CtlC',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlD					= 'CtlD',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlE					= 'CtlE',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlF					= 'CtlF',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlG					= 'CtlG',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlH					= 'CtlH',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlI					= 'CtlI',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlJ					= 'CtlJ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlK					= 'CtlK',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlL					= 'CtlL',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlM					= 'CtlM',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlN					= 'CtlN',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlO					= 'CtlO',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlP					= 'CtlP',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlQ					= 'CtlQ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlR					= 'CtlR',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlS					= 'CtlS',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlT					= 'CtlT',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlU					= 'CtlU',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlV					= 'CtlV',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlW					= 'CtlW',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlX					= 'CtlX',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlY					= 'CtlY',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlZ					= 'CtlZ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESC						= 'cESC',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk				= 'ESC*',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskA				= 'ES*A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskB				= 'ES*B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskC				= 'ES*C',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskE				= 'ES*E',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskH				= 'ES*H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskK				= 'ES*K',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskQ				= 'ES*Q',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskR				= 'ES*R',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskY				= 'ES*Y',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskZ				= 'ES*Z',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk0				= 'ES*0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk1				= 'ES*1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk2				= 'ES*2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk4				= 'ES*4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk5				= 'ES*5',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk6				= 'ES*6',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsterisk7				= 'ES*7',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskLessThan		= 'ES*<',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCAsteriskEquals		= 'ES*=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus					= 'ESC+',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusA					= 'ES+A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusB					= 'ES+B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusC					= 'ES+C',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusE					= 'ES+E',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusH					= 'ES+H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusK					= 'ES+K',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusQ					= 'ES+Q',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusR					= 'ES+R',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusY					= 'ES+Y',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusZ					= 'ES+Z',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus0					= 'ES+0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus1					= 'ES+1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus2					= 'ES+2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus4					= 'ES+4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus5					= 'ES+5',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus6					= 'ES+6',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlus7					= 'ES+7',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusLessThan			= 'ES+<',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPlusEquals			= 'ES+=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracket			= 'ESC[',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketEquals	= 'ES[=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketExPoint	= 'ES[!',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketExPointp	= 'E[!p',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketGreaterThan	= 'ES[>',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParams	= 'E[;;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsA	= 'E[;A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsB	= 'E[;B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsc	= 'E[;c',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsC	= 'E[;C',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsd	= 'E[;d',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsD	= 'E[;D',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsE	= 'E[;E',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsf	= 'E[;f',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsF	= 'E[;F',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsg	= 'E[;g',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsG	= 'E[;G',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsh	= 'E[;h',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsH	= 'E[;H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsi	= 'E[;i',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsI	= 'E[;I',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsJ	= 'E[;J',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsK	= 'E[;K',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsl	= 'E[;l',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsL	= 'E[;L',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsm	= 'E[;m',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsM	= 'E[;M',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsn	= 'E[;n',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsP	= 'E[;P',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsq	= 'E[;q',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsr	= 'E[;r',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamss	= 'E[;s',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsS	= 'E[;S',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsT	= 'E[;T',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsu	= 'E[;u',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsx	= 'E[;x',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsX	= 'E[;X',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsZ	= 'E[;Z',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsAt	= 'E[;@',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsBackquote	= 'E[;`',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsDollarSign	= 'E[;$',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsQuotes		= 'E[;\"',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsSpace		= 'E[; ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketQuestionMark	= 'ES[?',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketSemicolon	= 'ES[;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen				= 'ESC(',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenA			= 'ES(A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenB			= 'ES(B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenC			= 'ES(C',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenE			= 'ES(E',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenH			= 'ES(H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenK			= 'ES(K',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenQ			= 'ES(Q',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenR			= 'ES(R',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenY			= 'ES(Y',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenZ			= 'ES(Z',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen0			= 'ES(0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen1			= 'ES(1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen2			= 'ES(2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen4			= 'ES(4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen5			= 'ES(5',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen6			= 'ES(6',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen7			= 'ES(7',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenLessThan		= 'ES(<',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenEquals		= 'ES(=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen			= 'ESC)',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenA			= 'ES)A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenB			= 'ES)B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenC			= 'ES)C',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenE			= 'ES)E',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenH			= 'ES)H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenK			= 'ES)K',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenQ			= 'ES)Q',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenR			= 'ES)R',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenY			= 'ES)Y',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenZ			= 'ES)Z',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen0			= 'ES)0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen1			= 'ES)1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen2			= 'ES)2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen4			= 'ES)4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen5			= 'ES)5',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen6			= 'ES)6',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen7			= 'ES)7',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenLessThan	= 'ES)<',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenEquals		= 'ES)=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket		= 'ESC]',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket0		= 'ES]0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket1		= 'ES]1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket2		= 'ES]2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket3		= 'ES]3',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket4		= 'ES]4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket0Semi	= 'E]0;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket1Semi	= 'E]1;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket2Semi	= 'E]2;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket3Semi	= 'E]3;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket4Semi	= 'E]4;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket13		= 'E]13',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket133		= ']133',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket1337	= '1337',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket1337Semi= '337;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCA						= 'ESCA',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCB						= 'ESCB',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCC						= 'ESCC',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCc						= 'ESCc',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCD						= 'ESCD',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCE						= 'ESCE',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCF						= 'ESCF',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCG						= 'ESCG',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCH						= 'ESCH',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCI						= 'ESCI',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCJ						= 'ESCJ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCK						= 'ESCK',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCM						= 'ESCM',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCP						= 'ESCP',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCn						= 'ESCn',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCo						= 'ESCo',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCY						= 'ESCY',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCZ						= 'ESCZ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESC7						= 'ESC7',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESC8						= 'ESC8',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound					= 'ESC#',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound3				= 'ES#3',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound4				= 'ES#4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound5				= 'ES#5',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound6				= 'ES#6',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound8				= 'ES#8',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercent				= 'ESC%',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercentG				= 'ES%G',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercentAt				= 'ES%@',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercentSlash			= 'ES%/',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercentSlashG			= 'E%/G',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercentSlashH			= 'E%/H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPercentSlashI			= 'E%/I',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCEquals				= 'ESC=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCTilde					= 'ESC~',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLessThan				= 'ESC<',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCGreaterThan			= 'ESC>',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightBrace			= 'ESC}',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCVerticalBar			= 'ESC|',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCSpace					= 'ESC ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCSpaceF				= 'ES F',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCSpaceG				= 'ES G',	//!< generic state used to define emulator-specific states, below
	// note that a real backslash is a very common escape character, and
	// since state codes tend to be printed, it would screw up the output;
	// so the convention is broken in this case, and "B/" is used instead
	kMy_ParserStateSeenESCBackslash				= 'ESB/',	//!< generic state used to define emulator-specific states, below
};

char const		kMy_TabSet		= 'x';	//!< in "tabSettings" field of terminal structure, characters with this value mark tab stops
char const		kMy_TabClear	= ' ';	//!< in "tabSettings" field of terminal structure, all characters not marking tab stops have this value
UInt8 const		kMy_TabStop		= 8;	//!< number of characters between normal tab stops

enum My_AttributeRule
{
	kMy_AttributeRuleInitialize				= 0,	//!< newly-created lines have cleared attributes
	kMy_AttributeRuleCopyLast				= 1,	//!< newly-created lines copy all of the attributes of the line that used to be at the end (or, for
													//!  insertions of characters on a single line, new characters copy the attributes of the last character);
													//!  special exceptions are made to NOT copy certain attributes such as bitmap IDs
	kMy_AttributeRuleCopyLatentBackground	= 2		//!< newly-created lines or characters have cleared attributes EXCEPT they copy the latent background color
};

typedef UInt32 My_BufferChanges;
enum
{
	kMy_BufferChangesEraseAllText				= (1 << 0),		//!< erase every character (otherwise, only erasable text from selective erase is removed)
	kMy_BufferChangesResetCharacterAttributes	= (1 << 1),		//!< every character is set to normal (otherwise, SGR-defined attributes are untouched);
																//!  if this flag is provided alongside other flags that specify new attributes, such as
																//!  "kMy_BufferChangesKeepBackgroundColor", then this reset flag shall be applied FIRST
																//!  and the attributes added by other flags shall be applied SECOND
	kMy_BufferChangesResetLineAttributes		= (1 << 2),		//!< any completely-erased line resets line attributes, such as double-sized text, to normal;
																//!  if you use this option then you must also use kMy_BufferChangesResetCharacterAttributes
	kMy_BufferChangesKeepBackgroundColor		= (1 << 3)		//!< copies the background color that was most recently set, to ALL reset cells; this is a
																//!  relatively efficient way to preserve background color across a large area (e.g. without
																//!  requiring spaces), though it is technically non-standard behavior in a lot of terminals;
																//!  note that the foreground color is not affected
};

enum My_CharacterSet
{
	kMy_CharacterSetVT100UnitedKingdom		= 0,	//!< ASCII character set except '#' is '£' (British pounds currency symbol)
	kMy_CharacterSetVT100UnitedStates		= 1		//!< ASCII character set
};

enum My_CharacterROM
{
	kMy_CharacterROMNormal		= 0,	//!< regular source
	kMy_CharacterROMAlternate	= 1		//!< alternate ROM (e.g. Unix programs like "top" will try to switch out)
};

enum My_GraphicsMode
{
	kMy_GraphicsModeOff			= 0,	//!< no graphics glyphs will appear
	kMy_GraphicsModeOn			= 1		//!< non-printable ASCII will become VT graphics characters
};

typedef UInt32 My_LEDBits;
enum
{
	kMy_LEDBitsLight1			= (1 << 0),		//!< set if LED 1 is on
	kMy_LEDBitsLight2			= (1 << 1),		//!< set if LED 2 is on
	kMy_LEDBitsLight3			= (1 << 2),		//!< set if LED 3 is on
	kMy_LEDBitsLight4			= (1 << 3),		//!< set if LED 4 is on
	kMy_LEDBitsAllOff			= 0L			//!< cleared if all LEDs are off
};

/*!
In VT102 terminals and beyond, there are multiple
printing modes that can be active.  In order to
ensure that as few printing dialogs appear as
possible, all possible modes must be off before
printing will occur.
*/
typedef UInt8 My_PrintingMode;
enum
{
	kMy_PrintingModeAutoPrint			= (1 << 0),		//!< MC private (VT102): true only if terminal-rendered lines are also sent to the printer
	kMy_PrintingModePrintController		= (1 << 1),		//!< MC (VT102): true only if received data is being copied to the printer, verbatim
};

enum
{
	kMy_MaximumANSIParameters = 16		// when <ESC>'['<param>[;<param>][;<param>]... sequences are encountered,
										// this will be the maximum <param>s allowed
};

} // anonymous namespace

#pragma mark Callbacks
namespace {

struct My_Emulator;			// declared here because the callback declarations use it (defined later)
struct My_ScreenBuffer;		// declared here because the callback declarations use it (defined later)

/*!
Emulator Echo-Data Routine

Translates the given data from the specified screen’s input
text encoding, and sends it to the screen (probably using the
utility function echoCFString()).

This is a separate emulator function because it is occasionally
important for emulators to customize it, e.g. a “dumb” terminal.

WARNING:	The specified buffer is a limited slice of the
			overall data stream.  It is not guaranteed to
			terminate at the end of a full sequence of
			characters that you are interested in, so naïve
			translation will not work.  You must be prepared
			to “backtrack” and ignore zero or more bytes at
			the end of the buffer, to find a valid segment.
			TextTranslation_PersistentCFStringCreate() is
			very useful for this!
*/
typedef UInt32 (*My_EmulatorEchoDataProcPtr)	(My_ScreenBuffer*	inDataPtr,
												 UInt8 const*		inBuffer,
												 UInt32				inLength);
inline UInt32
invokeEmulatorEchoDataProc	(My_EmulatorEchoDataProcPtr		inProc,
							 My_ScreenBuffer*				inDataPtr,
							 UInt8 const*					inBuffer,
							 UInt32							inLength)
{
	return (*inProc)(inDataPtr, inBuffer, inLength);
}

/*!
Emulator Reset Routine

Performs whatever emulator-specific actions are appropriate
for a hard reset or (if the given parameter is true) a soft
reset.  Note that if a terminal does not distinguish between
soft and hard resets it should ignore this parameter.

IMPORTANT:	The terminal screen buffer handles a number of
			reset actions by itself.  An emulator reset does
			not generally require a lot of code.
*/
typedef void (*My_EmulatorResetProcPtr)		(My_Emulator*	inDataPtr,
											 Boolean		inIsSoftReset);
inline void
invokeEmulatorResetProc		(My_EmulatorResetProcPtr	inProc,
							 My_Emulator*				inDataPtr,
							 Boolean					inIsSoftReset)
{
	(*inProc)(inDataPtr, inIsSoftReset);
}

/*!
Emulator State Determinant Routine

Specifies the next state for the parser, based on the given
data and/or current state.  The number of code points read is
returned; this is generally 1, but may be zero or more if you
used look-ahead to set a more precise next state or do not
wish to consume any code points.

The emulator can be queried for the most recent code point,
which is often just one byte.  When the UTF-8 encoding is
active, the original byte stream will have already been
decoded and the recent code point reflects the last valid
code point that was decoded.

WARNING:	The specified buffer is a limited slice of the
			overall data stream.  It is not guaranteed to
			terminate at the end of a full sequence of
			characters that you are interested in, so naïve
			look-ahead generally does not work.  Instead,
			move the parser to a custom scanner state until
			you have found the complete sequence you want
			(you can then continue accumulating characters
			as long as the current state is your scanner
			state, even across multiple calls).  The only
			time look-ahead is reliable in a single call is
			when you do not care about the specific data
			in the sequence (e.g. the standard echo state).

Sometimes, a state is an interrupt: entered and exited
immediately and restoring the parser to a state as if the
interrupt never occurred.  When this applies to your
suggested new state, also set "outInterrupt" to true (the
default is false).  This causes the caller to perform the
state transition, but then restore the current state and
seek another next state (with a buffer pointer that has
since advanced).

You do not need to set a new state; the default is the
current state.  However, for clarity it is recommended
that you always set the state precisely on each call.

The "outHandled" flag is used to indicate that a handler
is taking full responsibility for determining the state
for this input (even if the state is unchanged).  When
invokeEmulatorStateDeterminantProc() is invoked, the flag
is ALWAYS initialized to true, but can be cleared by the
callback.
*/
typedef UInt32 (*My_EmulatorStateDeterminantProcPtr)	(My_Emulator*			inDataPtr,
														 My_ParserStatePair&	inNowOutNext,
														 Boolean&				outInterrupt,
														 Boolean&				outHandled);
inline UInt32
invokeEmulatorStateDeterminantProc	(My_EmulatorStateDeterminantProcPtr		inProc,
									 My_Emulator*							inEmulatorPtr,
									 My_ParserStatePair&					inNowOutNext,
									 Boolean&								outInterrupt,
									 Boolean&								outHandled)
{
	outHandled = true; // initially...
	return (*inProc)(inEmulatorPtr, inNowOutNext, outInterrupt, outHandled);
}

/*!
Emulator State Transition Routine

Unlike a "My_EmulatorStateDeterminantProcPtr", which determines
what the next state should be, this routine *performs an
action* based on the *current* state.  Some context is given
(like the previous state), in case this matters to you.

The screen buffer’s emulator can be queried for the most
recent code point, in case this is needed to help determine the
appropriate course of action.  Note that if a sequence of code
points is significant, they should generally be given separate
states.

The number of bytes read is returned; this is generally 0, but
may be more if you use look-ahead to absorb data (e.g. the echo
state).

The "outHandled" flag is used to indicate that a handler is
taking full responsibility for transitioning between the given
states.  When invokeEmulatorStateTransitionProc() is invoked,
the flag is ALWAYS initialized to true, but can be cleared by
the callback.
*/
typedef UInt32 (*My_EmulatorStateTransitionProcPtr)	(My_ScreenBuffer*			inDataPtr,
													 My_ParserStatePair const&	inOldNew,
													 Boolean&					outHandled);
inline UInt32
invokeEmulatorStateTransitionProc	(My_EmulatorStateTransitionProcPtr	inProc,
									 My_ScreenBuffer*					inDataPtr,
									 My_ParserStatePair const&			inOldNew,
									 Boolean&							outHandled);

} // anonymous namespace

#pragma mark Types
namespace {

typedef TerminalLine_Object		My_ScreenBufferLine;
typedef TerminalLine_Handle		My_ScreenBufferLinePtr; // see createLinePtr(), deleteLinePtr()

typedef std::list< My_ScreenBufferLinePtr >		My_ScreenBufferLineList;
typedef std::list< My_ScreenBufferLinePtr >		My_ScrollbackBufferLineList;
typedef My_ScreenBufferLineList::size_type		My_ScreenRowIndex;

typedef std::basic_string< UInt8 >				My_ByteString;

typedef std::map< UniChar, CFRetainRelease >	My_PrintableByUniChar;

typedef std::vector< UInt8 >					My_RGBComponentList;

typedef std::map< void const*, char const* >	My_StringByPointer;

typedef std::vector< char >						My_TabStopList;

typedef std::map< UInt32, TextAttributes_TrueColorID >		My_TrueColorIDByComponentKey;

typedef std::list< void const* >				My_VoidPtrList;

/*!
All the information associated with either the G0 or G1
character sets in a VT terminal.
*/
struct My_CharacterSetInfo
{
public:
	My_CharacterSetInfo		(My_CharacterSet	inTranslationTable,
							 My_CharacterROM	inSource,
							 My_GraphicsMode	inGraphicsMode)
	: translationTable(inTranslationTable), source(inSource), graphicsMode(inGraphicsMode)
	{
	}
	
	My_CharacterSet		translationTable;	//!< what character code translation rules are used?
	My_CharacterROM		source;				//!< which ROM are characters coming from?
	My_GraphicsMode		graphicsMode;		//!< can graphics glyphs appear?
};
typedef My_CharacterSetInfo*			My_CharacterSetInfoPtr;
typedef My_CharacterSetInfo const*		My_CharacterSetInfoConstPtr;

/*!
A pair of rows that define the start and end rows,
inclusive, of a range in the buffer.
*/
struct My_RowBoundary
{
public:
	My_RowBoundary	(My_ScreenRowIndex		inFirstRow,
					 My_ScreenRowIndex		inLastRow)
	: firstRow(inFirstRow), lastRow(inLastRow)
	{
	}
	
	bool
	operator ==		(My_RowBoundary const&		inOther)
	{
		return ((inOther.firstRow == this->firstRow) &&
				(inOther.lastRow == this->lastRow));
	}
	
	My_ScreenRowIndex	firstRow;   //!< zero-based row number where range occurs
	My_ScreenRowIndex	lastRow;	//!< zero-based row number of the final row included in the range; 0 is topmost
									//!  main screen line, a negative line number is in the scrollback buffer
};

struct My_RowColumnBoundary
{
public:
	My_RowColumnBoundary	(UInt16				inFirstColumn,
							 My_ScreenRowIndex	inFirstRow,
							 UInt16				inLastColumn,
							 My_ScreenRowIndex	inLastRow)
	: firstColumn(inFirstColumn), lastColumn(inLastColumn), rows(inFirstRow, inLastRow)
	{
	}
	
	UInt16				firstColumn;	//!< zero-based column number where range begins
	UInt16				lastColumn;		//!< zero-based column number of the final column included in the range
	My_RowBoundary		rows;			//!< zero-based row numbers where range occurs (inclusive)
};

/*!
Represents a line of the terminal buffer, either in the
scrollback or one of the main screen (“visible”) lines.

A line iterator can traverse from the oldest scrollback
line (the beginning) all the way to the bottommost main
screen line (the end), as if all terminal lines were
stored sequentially in memory.

IMPORTANT:	Since the scrollback size is cached, it is
			important that the scrollback buffer NOT be
			changed without updating the cache.  The
			iterator holds a non-constant reference to
			the buffer, but should have no legitimate
			reason to resize the buffer.
*/
struct My_LineIterator
{
public:
	enum BufferTarget
	{
		kBufferTargetScreen = 0,
		kBufferTargetScrollback = 1
	};
	
	enum ScreenBufferDesignator
	{
		kScreenBufferDesignator = 0
	};
	
	// the "inDesignateScreen" is used only to provide a unique
	// constructor signature in the event that the two buffers
	// are set to exactly the same underlying container type
	My_LineIterator		(Boolean							isHeapStorage,
						 My_ScreenBufferLineList&			inScreenBuffer,
						 My_ScrollbackBufferLineList&		inScrollbackBuffer,
						 My_ScreenBufferLineList::iterator	inRowIterator,
						 ScreenBufferDesignator				UNUSED_ARGUMENT(inDesignateScreen))
	:
	screenBuffer(inScreenBuffer),
	scrollbackBuffer(inScrollbackBuffer),
	screenRowIterator(inRowIterator),
	scrollbackRowIterator(),
	currentBufferType(kBufferTargetScreen),
	heapAllocated(isHeapStorage)
	{
	}
	
	// this version constructs iterators starting in the scrollback
	My_LineIterator		(Boolean								isHeapStorage,
						 My_ScreenBufferLineList&				inScreenBuffer,
						 My_ScrollbackBufferLineList&			inScrollbackBuffer,
						 My_ScrollbackBufferLineList::iterator	inRowIterator)
	:
	screenBuffer(inScreenBuffer),
	scrollbackBuffer(inScrollbackBuffer),
	screenRowIterator(),
	scrollbackRowIterator(inRowIterator),
	currentBufferType(kBufferTargetScrollback),
	heapAllocated(isHeapStorage)
	{
	}
	
	My_ScreenBufferLine&
	currentLine ()
	{
		// these dereferences will crash if past the end, which is expected (STL-like) behavior
		return (currentBufferType == kBufferTargetScreen)
				? **screenRowIterator
				: **scrollbackRowIterator;
	}
	
	//! Returns either the oldest scrollback line, or the topmost
	//! main screen line when the scrollback is empty.
	My_ScreenBufferLine&
	firstLine ()
	{
		// the back of the scrollback is used because its first line is the one
		// that is closest to the main screen, and the front (from the caller’s
		// perspective) must be the oldest scrollback line
		return (scrollbackBuffer.empty() || (currentBufferType == kBufferTargetScreen))
				? *(screenBuffer.front())
				: *(scrollbackBuffer.back());
	}
	
	//! Increments the internal line pointer so that currentLine() now
	//! refers to the next line in the scrollback or screen buffer, as
	//! appropriate.  Returns a reference to the new currentLine().
	//! If the returned line is the last valid line, "isEnd" is set to
	//! true; otherwise, it is set to false.  The initial value of
	//! "isEnd" is ignored.
	My_ScreenBufferLine&
	goToNextLine	(Boolean&	isEnd)
	{
		// IMPORTANT:	The scrollback buffer is stored in the opposite
		//			sense that it is returned by this iterator: the
		//			beginning of the scrollback is the line nearest
		//			the main screen.  So, the scrollback buffer is
		//			traversed backward to reach “next lines”.
		isEnd = false;
		if ((currentBufferType == kBufferTargetScrollback) &&
			(scrollbackBuffer.empty() || (scrollbackBuffer.front() == &currentLine())))
		{
			// change the iterator to look at the screen buffer instead
			currentBufferType = kBufferTargetScreen;
			screenRowIterator = screenBuffer.begin();
		}
		else
		{
			if ((currentBufferType == kBufferTargetScrollback) &&
				(scrollbackBuffer.begin() != scrollbackRowIterator))
			{
				--scrollbackRowIterator; // scrollback is inverted
			}
			else if ((currentBufferType == kBufferTargetScreen) &&
						(*screenRowIterator != &lastLine()))
			{
				++screenRowIterator;
			}
			else
			{
				isEnd = true;
			}
		}
		return currentLine();
	}
	
	//! Decrements the internal line pointer so that currentLine() now
	//! refers to the previous line in the scrollback or screen buffer,
	//! as appropriate.  Returns a reference to the new currentLine().
	//! If the returned line is the first valid line, "isEnd" is set to
	//! true; otherwise, it is set to false.  The initial value of
	//! "isEnd" is ignored.
	My_ScreenBufferLine&
	goToPreviousLine	(Boolean&	isEnd)
	{
		// IMPORTANT:	The scrollback buffer is stored in the opposite
		//			sense that it is returned by this iterator: the
		//			beginning of the scrollback is the line nearest
		//			the main screen.  So, the scrollback buffer is
		//			traversed forward to reach “previous lines”.
		isEnd = false;
		if ((currentBufferType == kBufferTargetScreen) &&
			(screenBuffer.front() == &currentLine()))
		{
			// change the iterator to look at the scrollback buffer instead
			currentBufferType = kBufferTargetScrollback;
			scrollbackRowIterator = scrollbackBuffer.begin();
		}
		else
		{
			if ((currentBufferType == kBufferTargetScreen) &&
				(screenBuffer.begin() != screenRowIterator))
			{
				--screenRowIterator;
			}
			else if ((currentBufferType == kBufferTargetScrollback) &&
						(*scrollbackRowIterator != &firstLine()))
			{
				++scrollbackRowIterator; // scrollback is inverted
			}
			else
			{
				isEnd = true;
			}
		}
		return currentLine();
	}
	
	//! Returns false if the constructor claims to have initialized this
	//! data in a block of memory that is on the stack.
	Boolean
	isHeapAllocated ()
	{
		return heapAllocated;
	}
	
	//! Returns the bottommost main screen line.
	My_ScreenBufferLine&
	lastLine ()
	{
		return *(screenBuffer.back());
	}

private:
	My_ScreenBufferLineList&				screenBuffer;			//!< the other possible source for the current line
	My_ScrollbackBufferLineList&			scrollbackBuffer;		//!< one possible source for the current line
	My_ScreenBufferLineList::iterator		screenRowIterator;		//!< the current line when "kBufferTargetScreen"
	My_ScrollbackBufferLineList::iterator	scrollbackRowIterator;	//!< the current line when "kBufferTargetScrollback"
	BufferTarget							currentBufferType : 2;	//!< whether or not the screen is being targeted
	Boolean									heapAllocated : 1;		//!< an annoying extra use of memory for this flag...
};
STATIC_ASSERT(sizeof(Terminal_LineStackStorage) == sizeof(My_LineIterator),
				assert_My_LineIterator_size_equals_Terminal_LineRefStackStorage);

typedef My_LineIterator*	My_LineIteratorPtr;

/*!
Represents the state of a terminal emulator, such as any
parameters collected and any pending operations.
*/
struct My_Emulator
{
public:
	//! Variants allow emulators to be tweaked so that the
	//! behavior is non-standard in some way.  They can also
	//! allow memory or runtime optimizations when disabled.
	typedef UInt8		VariantFlags;
	enum
	{
		kVariantFlagsNone				= 0,
		kVariantFlag24BitColor			= (1 << 0),		//!< corresponds to kPreferences_TagTerminal24BitColorEnabled
		kVariantFlagXTerm256Color		= (1 << 1),		//!< corresponds to kPreferences_TagXTerm256ColorsEnabled
		kVariantFlagXTermAlterWindow	= (1 << 2),		//!< corresponds to kPreferences_TagXTermWindowAlterationEnabled
		kVariantFlagXTermBCE			= (1 << 3),		//!< corresponds to kPreferences_TagXTermBackgroundColorEraseEnabled
		kVariantFlagSixelGraphics		= (1 << 4),		//!< corresponds to kPreferences_TagSixelGraphicsEnabled
		kVariantFlagITermGraphics		= (1 << 5),		//!< corresponds to kPreferences_TagITermGraphicsEnabled
	};
	
	struct Callbacks
	{
	public:
		Callbacks	();
		Callbacks	(My_EmulatorEchoDataProcPtr,
					 My_EmulatorStateDeterminantProcPtr,
					 My_EmulatorStateTransitionProcPtr,
					 My_EmulatorResetProcPtr);
		
		Boolean
		exist () const;
		
		My_EmulatorEchoDataProcPtr			dataWriter;				//!< translates data and dumps it to the screen
		My_EmulatorResetProcPtr				resetHandler;			//!< handles a soft or hard reset; varies based on the emulator
		My_EmulatorStateDeterminantProcPtr	stateDeterminant;		//!< figures out what the next state should be
		My_EmulatorStateTransitionProcPtr	transitionHandler;		//!< handles new parser states, driving the terminal; varies based on the emulator
	};
	
	typedef std::vector< Callbacks >	VariantChain;
	typedef std::vector< SInt16 >		ParameterList;
	
	My_Emulator		(Emulation_FullType, CFStringRef, CFStringEncoding);
	
	~My_Emulator ();
	
	void
	clearEscapeSequenceParameters ();
	
	void
	initializeParserStateStack ();
	
	Boolean
	is8BitReceiver ();
	
	Boolean
	is8BitTransmitter ();
	
	UInt32
	recentCodePoint ();
	
	void
	reset	(Boolean);
	
	My_BufferChanges
	returnEraseEffectsForNormalUse ();
	
	My_BufferChanges
	returnEraseEffectsForSelectiveUse ();
	
	void
	sendEscape	(SessionRef, void const*, size_t);
	
	void
	set8BitReceive		(Boolean);
	
	void
	set8BitTransmit		(Boolean);
	
	void
	set8BitTransmitNever	();
	
	Boolean
	supportsVariant		(VariantFlags);
	
	Emulation_FullType					primaryType;			//!< VT100, VT220, etc.
	Boolean								isUTF8Encoding;			//!< the emulator is using the UTF-8 encoding, and should ignore any specific sequences
																//!  that would change character sets in other ways (such as those defined for a VT220); in
																//!  addition, the data stream is decoded as UTF-8 BEFORE it is searched for any control
																//!  sequences, instead of being decoded in the echo-data phase; WARNING: do not assign this
																//!  flag casually, it is usually only correct to use Session APIs to change the text encoding
	Boolean								lockUTF8;				//!< set if UTF-8 should be set permanently, ignoring any sequences to switch it back until
																//!  a full reset occurs
	Boolean								disableShifts;			//!< prevents shift-in, shift-out, and any related old-style character set mechanism from working;
																//!  usually this should be set only when a Unicode encoding is present (which is large enough to
																//!  support all characters, making alternate sets unnecessary), but it is a separate state anyway
	CFStringEncoding					inputTextEncoding;		//!< specifies the encoding used by the input data stream
	CFRetainRelease						answerBackCFString;		//!< similar to "primaryType", but can be an arbitrary string
	UInt64								stateRepetitions;		//!< to guard against looping; counts repetitions of same state
	My_ParserState						currentState;			//!< state the terminal input parser is in now
	My_ParserState						stringAccumulatorState;	//!< state that was in effect when the "stringAccumulator" was recently cleared
	std::basic_string< UInt8 >			stringAccumulator;		//!< used to gather characters for such things as XTerm window changes
	UTF8Decoder_StateMachine			multiByteDecoder;		//!< as individual bytes are processed, this tracks complete or invalid sequences
	UInt8								recentCodePointByte;	//!< for 8-bit encodings, the most recent byte read; see also "multiByteDecoder"
	Boolean								addedITerm;				//!< keep track of previous requests to install the same variant
	Boolean								addedSixel;				//!< keep track of previous requests to install the same variant
	Boolean								addedXTerm;				//!< since multiple variants reuse these callbacks, only insert them once
	Boolean								allowSixelScrolling;	//!< whether or not the screen scrolls when a Sixel cursor goes past the bottom
	SInt16								argLastIndex;			//!< zero-based last parameter position in the "values" array
	ParameterList						argList;				//!< all values provided for the current escape sequence
	ParameterList						parameterMarkList;		//!< this list has the same size as "argList"; in parallel with "argList", it sets only two values:
																//!  -1 to indicate that a parameter is finished (the typical case) or 0 to indicate that it is just
																//!  a colon-separated sub-parameter; in cases like SGR, this is used to track related values
	VariantChain						preCallbackSet;			//!< state determinant/transition callbacks invoked ahead of normal callbacks, to allow tweaks;
																//!  a pre-callback set’s echo and reset callbacks are never used and should be nullptr; in
																//!  addition, “normal” emulator callbacks CANNOT be used as pre-callbacks because they will
																//!  absorb too many actions, so every pre-callback must be ITS OWN CLASS and be lightweight
	Callbacks							currentCallbacks;		//!< emulator-type-specific handlers to drive the state machine
	Callbacks							pushedCallbacks;		//!< for emulators that can switch modes, the previous set of callbacks
	VariantFlags						supportedVariants;		//!< tags identifying minor features, e.g. 256-color support
	My_TrueColorIDByComponentKey		trueColorIDsByRGB;		//!< maps RGB elements to their table index value (ID), if any
	My_RGBComponentList*				trueColorTableReds;		//!< red components for all 24-bit colors; allocated only for supporting terminals
	My_RGBComponentList*				trueColorTableGreens;	//!< green components for all 24-bit colors; allocated only for supporting terminals
	My_RGBComponentList*				trueColorTableBlues;	//!< blue components for all 24-bit colors; allocated only for supporting terminals
	TextAttributes_TrueColorID			trueColorTableNextID;	//!< basis for new IDs; current entry for storing new colors in true-color table
	TextAttributes_BitmapID				bitmapTableNextID;		//!< basis for new IDs; current entry for storing new bitmaps in bitmap table
	NSMutableArray*						bitmapImageTable;		//!< NSArray of NSImage*; shared (“whole image”) bitmap representations by index (ID)
	NSMutableArray*						bitmapSegmentTable;		//!< NSArray of NSValue* (holding NSRect); single-cell bitmap sub-rectangles by index (ID)

protected:
	My_EmulatorEchoDataProcPtr
	returnDataWriter	(Emulation_FullType);
	
	My_EmulatorResetProcPtr
	returnResetHandler		(Emulation_FullType);
	
	My_EmulatorStateDeterminantProcPtr
	returnStateDeterminant		(Emulation_FullType);
	
	My_EmulatorStateTransitionProcPtr
	returnStateTransitionHandler	(Emulation_FullType);

private:
	Boolean								eightBitReceiver;		//!< if true, equivalent 8-bit codes should be recognized by the emulator’s state determinant
																//!  (for example, in a VT100, the 7-bit ESC-[ sequence is equivalent to the 8-bit code 155)
	Boolean								eightBitTransmitter;	//!< if true, any sequences returned to the listening session should use 8-bit codes only
	Boolean								lockSevenBitTransmit;	//!< if true, the emulator can never be set to 8-bit; useful in weird emulation corner cases
};
typedef My_Emulator*	My_EmulatorPtr;

typedef MemoryBlockReferenceTracker< TerminalScreenRef >	My_RefTracker;
typedef Registrar< TerminalScreenRef, My_RefTracker >		My_RefRegistrar;

struct My_ScreenBuffer
{
public:
	My_ScreenBuffer	(Preferences_ContextRef, Preferences_ContextRef);
	~My_ScreenBuffer ();
	
	static void
	preferenceChanged	(ListenerModel_Ref, ListenerModel_Event, void*, void*);
	
	void
	printingEnd		(Boolean = true);
	
	void
	printingReset ();
	
	Boolean
	return24BitColor	(Preferences_ContextRef);
	
	CFStringRef
	returnAnswerBackMessage		(Preferences_ContextRef);
	
	Emulation_FullType
	returnEmulator		(Preferences_ContextRef);
	
	Boolean
	returnForceSave		(Preferences_ContextRef);
	
	Boolean
	returnITermGraphics		(Preferences_ContextRef);
	
	Session_LineEnding
	returnLineEndings ();
	
	UInt16
	returnScreenColumns		(Preferences_ContextRef, Boolean = true);
	
	UInt16
	returnScreenRows		(Preferences_ContextRef, Boolean = true);
	
	UInt32
	returnScrollbackRows	(Preferences_ContextRef, Boolean = true);
	
	Boolean
	returnSixelGraphics		(Preferences_ContextRef);
	
	CFStringEncoding
	returnTextEncoding		(Preferences_ContextRef);
	
	Boolean
	returnXTerm256	(Preferences_ContextRef);
	
	Boolean
	returnXTermBackgroundColorErase		(Preferences_ContextRef);
	
	UInt16
	returnXTermPatchLevel	(Preferences_ContextRef, Boolean = true);
	
	Boolean
	returnXTermWindowAlteration		(Preferences_ContextRef);
	
	My_RefRegistrar						refValidator;				//!< ensures this reference is recognized as a valid one
	Preferences_ContextWrap				configuration;
	My_Emulator							emulator;					//!< handles all parsing of the data stream
	SessionRef							listeningSession;			//!< may be nullptr; the currently attached session, where certain terminal reports are sent
	
	TerminalSpeaker_Ref					speaker;					//!< object that emits sound based on this terminal data;
																	//!  TEMPORARY: the speaker REALLY shouldn’t be part of the terminal data model!
	CFRetainRelease						windowTitleCFString;		//!< stores the string that the terminal considers its window title
	CFRetainRelease						iconTitleCFString;			//!< stores the string that the terminal considers its icon title
	
	ListenerModel_Ref					changeListenerModel;		//!< registry of listeners for various terminal events
	ListenerModel_ListenerWrap			preferenceMonitor;			//!< listener for changes to preferences that affect a particular screen
	
	My_ScrollbackBufferLineList::size_type	scrollbackBufferCachedSize;	//!< linked list size is sometimes needed, but is VERY expensive to calculate;
																	//!  therefore, it is cached, and ALL code that changes "scrollbackBuffer" must
																	//!  be aware of this cached size and update it accordingly!
	My_ScrollbackBufferLineList			scrollbackBuffer;			//!< all of the scrollback text for the terminal; "scrollbackBufferCachedSize"
																	//!  is a redundant copy of the (expensively-computed) full size of the buffer;
																	//!  IMPORTANT: the FRONT is the scrollback line CLOSEST to the top (FRONT) of
																	//!  the screen buffer; imagine both buffers starting at the home line and
																	//!  growing away from one another
	My_ScreenBufferLineList				screenBuffer;				//!< all of the visible text for the terminal;
																	//!  IMPORTANT: ONLY modify the screen buffer using screen...() routines!
	My_ByteString						bytesToEcho;				//!< captures contiguous blocks of text to be translated and echoed
	My_VoidPtrList						debugStateHandlerSequence;	//!< used only when logging is enabled; cleared at each state transition, tracks
																	//!  handlers that are invoked during the processing of the transition
	
	// Error Counts
	//
	// Certain classes of error are exceptional, but when they occur
	// they may occur very often (e.g. extremely bad input data that
	// continues to cause errors until it is cleared out).
	//
	// To avoid the spam and resulting slowdown of logging these
	// cases, error counts are used: incremented as errors occur,
	// and decremented when a periodic handler discovers new errors
	// (except for the total, which is never reset).
	UInt32								echoErrorCount;				//!< echo errors occur when a stream of exceptionally bad data arrives (e.g.
																	//!  someone dumps binary data)
	UInt32								translationErrorCount;		//!< translation errors typically occur when the text encoding assumed either
																	//!  by the user or the running process is not actually used by some data; if
																	//!  enough of these are accumulated, the user could actually be prompted to
																	//!  reconsider the chosen translation table for the session
	UInt32								errorCountTotal;			//!< used to eventually fire "kTerminal_ChangeExcessiveErrors", if things have
																	//!  become just ridiculous
	
	My_TabStopList						tabSettings;				//!< array of characters representing tab stops; values are either kMy_TabClear
																	//!  (for most columns), or kMy_TabSet at tab columns
	
	StreamCapture_Ref					captureStream;				//!< used to stream data to a file chosen by the user
	StreamCapture_Ref					printingStream;				//!< used to stream data to a temporary file for later printing
	CFRetainRelease						printingFileURL;			//!< URL of the temporary printing file
	UInt8								printingModes;				//!< MC private (VT102): true only if terminal-rendered lines are also sent to the printer
	Boolean								bellDisabled;				//!< if true, all bell signals are completely ignored (no audio or visual)
	Terminal_CursorType					cursorType;					//!< cursor shape (from viewpoint of program running in terminal)
	Boolean								cursorBlinking;				//!< if true, cursor is set to blink (from viewpoint of program running in terminal)
	Boolean								cursorVisible;				//!< if true, cursor state is visible (as opposed to invisible)
	Boolean								reverseVideo;				//!< if true, foreground and background colors are swapped when rendering
	Boolean								windowMinimized;			//!< if true, the window has been *flagged* as being iconified; since this is only
																	//!  a data model, this only means that a terminal sequence has told the window to
																	//!  become iconified; synchronizing this flag with the actual window state is the
																	//!  responsibility of the Terminal Window module
	
	My_CharacterSetInfo					vtG0;						//!< the G0 character set
	My_CharacterSetInfo					vtG1;						//!< the G1 character set
	My_CharacterSetInfo					vtG2;						//!< the G2 character set
	My_CharacterSetInfo					vtG3;						//!< the G3 character set
	
	My_RowColumnBoundary				visibleBoundary;			//!< rows and columns forming the perimeter of the buffer’s “visible” region
	My_RowBoundary						customScrollingRegion;		//!< region that is in effect when margin set sequence is received; updated only
																	//!  when a terminal sequence to change margins is received
	
	struct
	{
		struct
		{
			My_ScreenRowIndex	numberOfRowsPermitted;  //!< maximum lines of scrollback specified by the user
														//!  (NOTE: this is inflexible; cooler scrollback handling schemes are limited by it...)
			Boolean				enabled;				//!< true if scrolled lines should be appended to the buffer automatically
		} scrollback;
		
		struct
		{
			UInt16				numberOfColumnsAllocated;	//!< how many characters per line are currently taking up memory; it follows
															//!  that this is the maximum possible value for "numberOfColumnsPermitted"
			UInt16				numberOfColumnsPermitted;	//!< maximum columns per line specified by the user; but see current.returnNumberOfColumnsPermitted()
		} visibleScreen;
	} text;
	
	My_LEDBits							litLEDs;					//!< highlighted states of terminal LEDs (lights)
	
	Boolean								passwordMode;				//!< when last checked, terminal device of process was not echoing (password prompt)
	Boolean								mayNeedToSaveToScrollback;	//!< if true, the cursor has moved to the home position, and therefore
																	//!  a subsequent attempt to erase to the end of the line or screen
																	//!  should cause the data to be copied to the scrollback first (that is,
																	//!  of course, if the force-save flag is also set)
	Boolean								saveToScrollbackOnClear;	//!< if true, the entire screen contents are saved to the scrollback
																	//!  buffer just before being explicitly erased with a clear command
	
	Boolean								reportOnlyOnRequest;		//!< DECREQTPARM mode: determines response format and timing
	Boolean								wrapPending;				//!< set only when a character is echoed in final column
	
	Boolean								modeANSIEnabled;			//!< DECANM mode: true only if in ANSI mode (as opposed to VT52 compatibility
																	//!  mode); if the former, parameters are only recognized following the CSI
																	//!  sequence, but if the latter, ESC-character sequences are allowed instead
	Boolean								modeApplicationKeys;		//!< DECKPAM mode: true only if the keypad is in application mode
	Boolean								modeAutoWrap;				//!< DECAWM mode: true only if line wrapping is automatic
	Boolean								modeCursorKeysForApp;		//!< DECCKM mode: true only if the keypad should not act as cursor movement arrows
																	//!  (note also that the VT100 manual states this setting has no effect unless
																	//!  the terminal is also in ANSI mode and the keypad is in application mode
	Boolean								modeInsertNotReplace;		//!< IRM mode: if true, new characters overwrite the cursor position, otherwise
																	//!  they first shift existing text over by one before appearing
	Boolean								modeNewLineOption;			//!< LNM mode: if true, a line feed causes the cursor to move to the start of
																	//!  the next line, and a Return generates a CR LF sequence; if false, the
																	//!  line feed is just vertical movement of the cursor, and Return is just CR
	Boolean								modeOriginRedefined;		//!< DECOM mode: true only if the origin has been moved from its default place
																	//!  (in which case, various subsequent terminal operations must be relative to
																	//!  the origin instead of the home position)
	UInt16								reportedPatchLevel;			//!< for XTerm; how to respond to Secondary Device Attributes
	My_RowBoundary*						originRegionPtr;			//!< automatically set to the boundaries appropriate for the current origin mode;
																	//!  this should always be preferred when restricting the cursor, and as an offset
																	//!  when returning column or row numbers
	
    struct
	{
		Terminal_SpeechMode		mode;	//!< restrictions on when the computer may speak
	} speech;
	
	struct CurrentLineCache
	{
		CurrentLineCache	(struct My_ScreenBuffer const&		inParent)
		:
		parent(inParent),
		cursorAttributes(),
		drawingAttributes(),
		cursorX(0),
		cursorY(0),
		characterSetInfoPtr(nullptr)
		{
		}
		
		UInt16
		returnNumberOfColumnsPermitted ()
		const
		{
			UInt16		result = parent.text.visibleScreen.numberOfColumnsPermitted;
			
			
			if (this->cursorAttributes.hasDoubleAny())
			{
				result = STATIC_CAST(INTEGER_DIV_2(result), UInt16);
			}
			return result;
		}
		
		struct My_ScreenBuffer const&	parent;
		TextAttributes_Object			cursorAttributes;		//!< text attributes for the position under the cursor; useful for rendering the cursor
																//!  in a way that does not clash with these attributes (e.g. by choosing opposite colors)
		TextAttributes_Object			drawingAttributes;		//!< text attributes for the line the cursor is on; these should be updated as the cursor
																//!  moves, or as terminal sequences are processed by the emulator
		TextAttributes_Object			latentAttributes;		//!< used to implement Background Color Erase; records the most recent attribute settings
																//!  (currently for background color bits ONLY, this is NOT properly updated for other
																//!  attribute changes)
		SInt16							cursorX;				//!< column number of current cursor position;
																//!  WARNING: do not change this value except with a moveCursor...() routine
		My_ScreenRowIndex				cursorY;				//!< row number of current cursor position;
																//!  WARNING: do not change this value except with a moveCursor...() routine
		My_CharacterSetInfoPtr			characterSetInfoPtr;	//!< pointer to either G0 or G1 character rules, whichever is active
	} current;
	
	struct
	{
		TextAttributes_Object		drawingAttributes;	//!< previous bits of corresponding bits in "current" structure
		SInt16						cursorX;			//!< previous value of corresponding value in "current" structure
		My_ScreenRowIndex			cursorY;			//!< previous value of corresponding value in "current" structure
	} previous;
	
	TerminalScreenRef		selfRef;					//!< opaque reference that would resolve to a pointer to this structure
};
typedef My_ScreenBuffer*			My_ScreenBufferPtr;
typedef My_ScreenBuffer const*		My_ScreenBufferConstPtr;

typedef MemoryBlockReferenceLocker< TerminalScreenRef, My_ScreenBuffer >	My_ScreenReferenceLocker;

/*!
Manages state determination and transition for conditions
that no emulator knows how to deal with.  Also used to
handle extremely common things like echoing printable
characters.
*/
class My_DefaultEmulator
{
public:
	static UInt32	echoData			(My_ScreenBufferPtr, UInt8 const*, UInt32);
	static void		hardSoftReset		(My_EmulatorPtr, Boolean);
	static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
};

/*!
Manages state determination and transition for a terminal
that does nothing but echo human-readable versions of every
character it receives.
*/
class My_DumbTerminal
{
public:
	static UInt32	echoData			(My_ScreenBufferPtr, UInt8 const*, UInt32);
	static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
};

/*!
A lightweight terminal implementation, suitable ONLY as a
pre-callback.  Implements custom iTerm2 sequences.
*/
class My_ITermCore
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
	
	enum State
	{
		// Ideally these are "protected", but loop evasion code requires them.
		kStateITermCustomBegin		= kMy_ParserStateSeenESCRightSqBracket1337Semi,	//!< saw custom iTerm2 ESC]1337; sequence
		kStateITermAcquireStr		= 'iAcS',							//!< continuously copy iTerm2 data string
		kStateITermStringTerminator	= kMy_ParserStateSeenControlG		//!< end of custom data stream
	};
};

/*!
A lightweight terminal implementation, suitable ONLY as a
pre-callback.  Implements the Sixel graphics sequences.
*/
class My_SixelCore
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
};

/*!
A lightweight terminal implementation, suitable ONLY as a
pre-callback.  Implements sequences that are unique to the
UTF-8 encoding (Unicode).
*/
class My_UTF8Core
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);

protected:
	enum State
	{
		// Ideally these are "protected", but loop evasion code requires them.
		kStateUTF8OnUnspecifiedImpl		= kMy_ParserStateSeenESCPercentG,		//!< switch to UTF-8, allow return to normal mode; unspecified behavior
		kStateUTF8Off					= kMy_ParserStateSeenESCPercentAt,		//!< disable UTF-8, return to ordinary emulator character set behavior
		kStateToUTF8Level1				= kMy_ParserStateSeenESCPercentSlashG,	//!< switch to UTF-8 level 1; cannot switch back
		kStateToUTF8Level2				= kMy_ParserStateSeenESCPercentSlashH,	//!< switch to UTF-8 level 2; cannot switch back
		kStateToUTF8Level3				= kMy_ParserStateSeenESCPercentSlashI,	//!< switch to UTF-8 level 3; cannot switch back
	};
};

/*!
Manages state determination and transition for the VT100
terminal emulator while in ANSI mode.  The VT52 subclass
handles sequences specific to VT52 mode.
*/
class My_VT100
{
public:
	static void				hardSoftReset		(My_EmulatorPtr, Boolean);
	static My_ParserState	returnCSINextState	(My_ParserState, UnicodeScalarValue, Boolean&);
	static UInt32			stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32			stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
	
	static void		alignmentDisplay			(My_ScreenBufferPtr);
	static void		ansiMode					(My_ScreenBufferPtr);
	static void		cursorBackward				(My_ScreenBufferPtr);
	static void		cursorDown					(My_ScreenBufferPtr);
	static void		cursorForward				(My_ScreenBufferPtr);
	static void		cursorUp					(My_ScreenBufferPtr);
	static void		deviceAttributes			(My_ScreenBufferPtr);
	static void		deviceStatusReport			(My_ScreenBufferPtr);
	static void		eraseInDisplay				(My_ScreenBufferPtr);
	static void		eraseInLine					(My_ScreenBufferPtr);
	static void		loadLEDs					(My_ScreenBufferPtr);
	static void		modeSetReset				(My_ScreenBufferPtr, Boolean);
	static void		reportTerminalParameters	(My_ScreenBufferPtr);
	static void		setTopAndBottomMargins		(My_ScreenBufferPtr);
	static void		vt52Mode					(My_ScreenBufferPtr);
	
	class VT52
	{
		friend class My_VT100;
		
	public:
		static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
		static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
		
		static void		cursorBackward		(My_ScreenBufferPtr);
		static void		cursorDown			(My_ScreenBufferPtr);
		static void		cursorForward		(My_ScreenBufferPtr);
		static void		cursorUp			(My_ScreenBufferPtr);
		static void		identify			(My_ScreenBufferPtr);
	
	protected:
		// The names of these constants use the same mnemonics from
		// the programming manual of the original terminal.
		enum State
		{
			// VT100 sequences in VT52 compatibility mode (in the order they appear in the manual) - see VT100 manual for full details
			kStateCU		= kMy_ParserStateSeenESCA,				//!< cursor up
			kStateCD		= kMy_ParserStateSeenESCB,				//!< cursor down
			kStateCR 		= kMy_ParserStateSeenESCC,				//!< cursor right
			kStateCL 		= kMy_ParserStateSeenESCD,				//!< cursor left
			kStateNGM		= kMy_ParserStateSeenESCF,				//!< enter graphics mode
			kStateXGM		= kMy_ParserStateSeenESCG,				//!< exit graphics mode
			kStateCH		= kMy_ParserStateSeenESCH,				//!< cursor home
			kStateRLF		= kMy_ParserStateSeenESCI,				//!< reverse line feed
			kStateEES		= kMy_ParserStateSeenESCJ,				//!< erase to end of screen
			kStateEEL		= kMy_ParserStateSeenESCK,				//!< erase to end of line
			kStateDCA		= kMy_ParserStateSeenESCY,				//!< direct cursor address
			kStateDCAY		= 'DCAY',								//!< direct cursor address, seen first follow-up character (Y)
			kStateDCAX		= 'DCAX',								//!< direct cursor address, seen second follow-up character (X)
			kStateID		= kMy_ParserStateSeenESCZ,				//!< identify terminal
			kStateNAKM		= kMy_ParserStateSeenESCEquals,			//!< enter alternate keypad mode
			kStateXAKM		= kMy_ParserStateSeenESCGreaterThan,	//!< exit alternate keypad mode
			kStateANSI		= kMy_ParserStateSeenESCLessThan,		//!< enter ANSI mode
		};
	};
	
	// The names of these constants use the same mnemonics from
	// the programming manual of the original terminal.
	enum State
	{
		// VT100 immediates (in order of the corresponding control character’s ASCII code)
		kStateControlENQ		= 'VAns',				//!< transmit answerback message
		kStateControlBEL		= 'VBel',				//!< audio event
		kStateControlBS			= 'VBks',				//!< move cursor left if possible
		kStateControlHT			= 'VTab',				//!< move cursor right to tab stop, or margin
		kStateControlLFVTFF		= 'VLnF',				//!< newline or line feed, depending on LNM
		kStateControlCR			= 'VCRt',				//!< move cursor down and to left margin
		kStateControlSO			= 'VShO',				//!< shift out to G1 character set
		kStateControlSI			= 'VShI',				//!< shift in to G0 character set
		kStateControlXON		= 'VXON',				//!< resume transmission
		kStateControlXOFF		= 'VXOF',				//!< suspend transmission
		kStateControlCANSUB		= 'VCAN',				//!< terminate control seq. with error char.
		
		// VT100 sequences (in the order they appear in the manual) - see VT100 manual for full details
		kStateCSI				= kMy_ParserStateSeenESCLeftSqBracket,	//!< control sequence inducer
		kStateCSIParamDigit0	= 'PNm0',				//!< digit has been encountered; define or extend current numerical parameter value;
		kStateCSIParamDigit1	= 'PNm1',				//!  IMPORTANT: the 10 digit state values must be defined consecutively, as the code uses a
		kStateCSIParamDigit2	= 'PNm2',				//!  short-cut that relies on arithmetic...
		kStateCSIParamDigit3	= 'PNm3',
		kStateCSIParamDigit4	= 'PNm4',
		kStateCSIParamDigit5	= 'PNm5',
		kStateCSIParamDigit6	= 'PNm6',
		kStateCSIParamDigit7	= 'PNm7',
		kStateCSIParamDigit8	= 'PNm8',
		kStateCSIParamDigit9	= 'PNm9',
		kStateCSIParamDigitSub	= 'PNm:',				//!< end of sub-parameter (colons can appear more than once)
		kStateCSIParameterEnd	= kMy_ParserStateSeenESCLeftSqBracketSemicolon,		//!< end of parameter (semicolons can appear more than once)
		kStateCSIPrivate		= kMy_ParserStateSeenESCLeftSqBracketQuestionMark,	//!< parameter list prefixed by '?' to indicate a private sequence
		kStateCUB				= kMy_ParserStateSeenESCLeftSqBracketParamsD,	//!< cursor backward
		kStateCUD				= kMy_ParserStateSeenESCLeftSqBracketParamsB,	//!< cursor down
		kStateCUF				= kMy_ParserStateSeenESCLeftSqBracketParamsC,	//!< cursor forward
		kStateCUP				= kMy_ParserStateSeenESCLeftSqBracketParamsH,	//!< cursor position
		kStateCUU				= kMy_ParserStateSeenESCLeftSqBracketParamsA,	//!< cursor up
		kStateDA				= kMy_ParserStateSeenESCLeftSqBracketParamsc,	//!< device attributes
		kStateDECALN			= kMy_ParserStateSeenESCPound8,			//!< screen alignment display
		kStateDECANM			= 'VANM',				//!< ANSI/VT52 mode
		kStateDECARM			= 'VARM',				//!< auto repeat mode
		kStateDECAWM			= 'VAWM',				//!< auto wrap mode
		kStateDECCKM			= 'VCKM',				//!< cursor keys mode
		kStateDECCOLM			= 'VCLM',				//!< column mode
		kStateDECDHLT			= kMy_ParserStateSeenESCPound3,			//!< double height line, top half
		kStateDECDHLB			= kMy_ParserStateSeenESCPound4,			//!< double height line, bottom half
		kStateDECDWL			= kMy_ParserStateSeenESCPound6,			//!< double width line
		kStateDECID				= kMy_ParserStateSeenESCZ,				//!< identify terminal
		kStateDECKPAM			= kMy_ParserStateSeenESCEquals,			//!< keypad application mode
		kStateDECKPNM			= kMy_ParserStateSeenESCGreaterThan,	//!< keypad numeric mode
		kStateDECLL				= kMy_ParserStateSeenESCLeftSqBracketParamsq,	//!< load LEDs (keyboard lights)
		kStateDECOM				= 'VOM ',				//!< origin mode
		kStateDECRC				= kMy_ParserStateSeenESC8,				//!< restore cursor
		kStateDECREPTPARM		= 'VRPM',				//!< report terminal parameters
		kStateDECREQTPARM		= kMy_ParserStateSeenESCLeftSqBracketParamsx,	//!< request terminal parameters
		kStateDECSC				= kMy_ParserStateSeenESC7,				//!< save cursor
		kStateDECSTBM			= kMy_ParserStateSeenESCLeftSqBracketParamsr,	//!< set top and bottom margins
		kStateDECSWL			= kMy_ParserStateSeenESCPound5,			//!< single width line
		kStateDECTST			= 'VTST',				//!< invoke confidence test
		kStateDSR				= kMy_ParserStateSeenESCLeftSqBracketParamsn,	//!< device status report
		kStateED 				= kMy_ParserStateSeenESCLeftSqBracketParamsJ,	//!< erase in display
		kStateEL 				= kMy_ParserStateSeenESCLeftSqBracketParamsK,	//!< erase in line
		kStateHTS				= kMy_ParserStateSeenESCH,				//!< horizontal tabulation set
		kStateHVP				= kMy_ParserStateSeenESCLeftSqBracketParamsf,	//!< horizontal and vertical position
		kStateIND				= kMy_ParserStateSeenESCD,				//!< index
		kStateNEL				= kMy_ParserStateSeenESCE,				//!< next line
		kStateRI				= kMy_ParserStateSeenESCM,				//!< reverse index
		kStateRIS				= kMy_ParserStateSeenESCc,				//!< reset to initial state
		kStateRM				= kMy_ParserStateSeenESCLeftSqBracketParamsl,	//!< reset mode
		kStateSCSG0UK			= kMy_ParserStateSeenESCLeftParenA,		//!< select character set for G0, U.K.
		kStateSCSG0ASCII		= kMy_ParserStateSeenESCLeftParenB,		//!< select character set for G0, ASCII
		kStateSCSG0SG			= kMy_ParserStateSeenESCLeftParen0,		//!< select character set for G0, special graphics
		kStateSCSG0ACRStd		= kMy_ParserStateSeenESCLeftParen1,		//!< select character set for G0, alternate character ROM standard set
		kStateSCSG0ACRSG		= kMy_ParserStateSeenESCLeftParen2,		//!< select character set for G0, alternate character ROM special graphics
		kStateSCSG1UK			= kMy_ParserStateSeenESCRightParenA,	//!< select character set for G0, U.K.
		kStateSCSG1ASCII		= kMy_ParserStateSeenESCRightParenB,	//!< select character set for G0, ASCII
		kStateSCSG1SG			= kMy_ParserStateSeenESCRightParen0,	//!< select character set for G0, special graphics
		kStateSCSG1ACRStd		= kMy_ParserStateSeenESCRightParen1,	//!< select character set for G0, alternate character ROM standard set
		kStateSCSG1ACRSG		= kMy_ParserStateSeenESCRightParen2,	//!< select character set for G0, alternate character ROM special graphics
		kStateSGR				= kMy_ParserStateSeenESCLeftSqBracketParamsm,	//!< select graphic rendition
		kStateSM				= kMy_ParserStateSeenESCLeftSqBracketParamsh,	//!< set mode
		kStateTBC				= kMy_ParserStateSeenESCLeftSqBracketParamsg,	//!< tabulation clear
		
		// ANSI hacks that should be in their own emulator, but for now are not
		kStateANSISC			= kMy_ParserStateSeenESCLeftSqBracketParamss,	//!< save cursor
		kStateANSIRC			= kMy_ParserStateSeenESCLeftSqBracketParamsu,	//!< restore cursor
	};
};

/*!
Manages state determination and transition for the VT102
terminal emulator.
*/
class My_VT102:
public My_VT100
{
public:
	static void		deleteCharacters	(My_ScreenBufferPtr);
	static void		deleteLines			(My_ScreenBufferPtr);
	static void		deviceAttributes	(My_ScreenBufferPtr);
	static void		insertLines			(My_ScreenBufferPtr);
	static void		loadLEDs			(My_ScreenBufferPtr);
	
	static My_ParserState	returnCSINextState	(My_ParserState, UnicodeScalarValue, Boolean&);
	static UInt32			stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32			stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);

protected:
	// The names of these constants use the same mnemonics from
	// the programming manual of the original terminal.
	enum State
	{
		kStateDCH		= kMy_ParserStateSeenESCLeftSqBracketParamsP,	//!< delete characters on the cursor line
		kStateDL		= kMy_ParserStateSeenESCLeftSqBracketParamsM,	//!< delete lines, including cursor line
		kStateIL		= kMy_ParserStateSeenESCLeftSqBracketParamsL,	//!< insert lines below cursor line
		kStateMC		= kMy_ParserStateSeenESCLeftSqBracketParamsi,	//!< media copy (printer access)
	};
};

/*!
Manages state determination and transition for the VT220
terminal emulator.
*/
class My_VT220
{
public:
	static void				hardSoftReset		(My_EmulatorPtr, Boolean);
	static My_ParserState	returnCSINextState	(My_ParserState, UnicodeScalarValue, Boolean&);
	static UInt32			stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32			stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
	
	static void		compatibilityLevel					(My_ScreenBufferPtr);
	static void		deviceStatusReportKeyboardLanguage	(My_ScreenBufferPtr);
	static void		deviceStatusReportPrinterPort		(My_ScreenBufferPtr);
	static void		deviceStatusReportUserDefinedKeys	(My_ScreenBufferPtr);
	static void		eraseCharacters						(My_ScreenBufferPtr);
	static void		insertBlankCharacters				(My_ScreenBufferPtr);
	static void		primaryDeviceAttributes				(My_ScreenBufferPtr);
	static void		requestDECPrivateMode				(My_ScreenBufferPtr);
	static void		secondaryDeviceAttributes			(My_ScreenBufferPtr);
	static void		selectCharacterAttributes			(My_ScreenBufferPtr);
	static void		selectCursorStyle					(My_ScreenBufferPtr);
	static void		selectiveEraseInDisplay				(My_ScreenBufferPtr);
	static void		selectiveEraseInLine				(My_ScreenBufferPtr);

	// The names of these constants use the same mnemonics from
	// the programming manual of the original terminal.
	enum State
	{
		// Ideally these are "protected", but XTerm may require them.
		kStateCSISecondaryDA	= kMy_ParserStateSeenESCLeftSqBracketGreaterThan,	//!< parameter list indicates secondary device attributes
		kStateDCS				= kMy_ParserStateSeenESCP,				//!< device control string
		kStateDCSAcquireStr		= 'VACS',								//!< state of reading bytes into string accumulator
		kStateDECRQM			= 'VRQM',								//!< request DEC private mode
		kStateDECSCA			= 'VSCA',								//!< select character attributes
		kStateDECSCL			= 'VSCL',								//!< compatibility level
		kStateDECSTR			= kMy_ParserStateSeenESCLeftSqBracketExPointp,	//!< soft terminal reset
		kStateDECSCUSR			= 'VSCU',								//!< select cursor style
		kStateECH				= kMy_ParserStateSeenESCLeftSqBracketParamsX,	//!< erase characters without insertion
		kStateICH				= kMy_ParserStateSeenESCLeftSqBracketParamsAt,	//!< insert blank characters
		kStateLS1R				= kMy_ParserStateSeenESCTilde,			//!< lock shift G1, right side
		kStateLS2				= kMy_ParserStateSeenESCn,				//!< lock shift G2, left side
		kStateLS2R				= kMy_ParserStateSeenESCRightBrace,		//!< lock shift G2, right side
		kStateLS3				= kMy_ParserStateSeenESCo,				//!< lock shift G3, left side
		kStateLS3R				= kMy_ParserStateSeenESCVerticalBar,	//!< lock shift G3, right side
		kStateS7C1T				= kMy_ParserStateSeenESCSpaceF,			//!< direct terminal to emit 7-bit sequences only (CSI = "ESC[")
		kStateS8C1T				= kMy_ParserStateSeenESCSpaceG,			//!< direct terminal to emit 8-bit sequences only (CSI = the CSI C1 code)
		kStateSCSG0DECSupplemental	= kMy_ParserStateSeenESCLeftParenLessThan,	//!< select character set for G0, DEC supplemental
		kStateSCSG0Dutch		= kMy_ParserStateSeenESCLeftParen4,		//!< select character set for G0, Dutch
		kStateSCSG0Finnish1		= kMy_ParserStateSeenESCLeftParenC,		//!< select character set for G0, Finnish
		kStateSCSG0Finnish2		= kMy_ParserStateSeenESCLeftParen5,		//!< select character set for G0, Finnish (alternate)
		kStateSCSG0French		= kMy_ParserStateSeenESCLeftParenR,		//!< select character set for G0, French
		kStateSCSG0FrenchCdn	= kMy_ParserStateSeenESCLeftParenQ,		//!< select character set for G0, French Canadian
		kStateSCSG0German		= kMy_ParserStateSeenESCLeftParenK,		//!< select character set for G0, German
		kStateSCSG0Italian		= kMy_ParserStateSeenESCLeftParenY,		//!< select character set for G0, Italian
		kStateSCSG0Norwegian1	= kMy_ParserStateSeenESCLeftParenE,		//!< select character set for G0, Norwegian and Danish
		kStateSCSG0Norwegian2	= kMy_ParserStateSeenESCLeftParen6,		//!< select character set for G0, Norwegian and Danish (alternate)
		kStateSCSG0Spanish		= kMy_ParserStateSeenESCLeftParenZ,		//!< select character set for G0, Spanish
		kStateSCSG0Swedish1		= kMy_ParserStateSeenESCLeftParenH,		//!< select character set for G0, Swedish
		kStateSCSG0Swedish2		= kMy_ParserStateSeenESCLeftParen7,		//!< select character set for G0, Swedish (alternate)
		kStateSCSG0Swiss		= kMy_ParserStateSeenESCLeftParenEquals,	//!< select character set for G0, Swiss
		kStateSCSG1DECSupplemental	= kMy_ParserStateSeenESCRightParenLessThan,	//!< select character set for G1, DEC supplemental
		kStateSCSG1Dutch		= kMy_ParserStateSeenESCRightParen4,	//!< select character set for G1, Dutch
		kStateSCSG1Finnish1		= kMy_ParserStateSeenESCRightParenC,	//!< select character set for G1, Finnish
		kStateSCSG1Finnish2		= kMy_ParserStateSeenESCRightParen5,	//!< select character set for G1, Finnish (alternate)
		kStateSCSG1French		= kMy_ParserStateSeenESCRightParenR,	//!< select character set for G1, French
		kStateSCSG1FrenchCdn	= kMy_ParserStateSeenESCRightParenQ,	//!< select character set for G1, French Canadian
		kStateSCSG1German		= kMy_ParserStateSeenESCRightParenK,	//!< select character set for G1, German
		kStateSCSG1Italian		= kMy_ParserStateSeenESCRightParenY,	//!< select character set for G1, Italian
		kStateSCSG1Norwegian1	= kMy_ParserStateSeenESCRightParenE,	//!< select character set for G1, Norwegian and Danish
		kStateSCSG1Norwegian2	= kMy_ParserStateSeenESCRightParen6,	//!< select character set for G1, Norwegian and Danish (alternate)
		kStateSCSG1Spanish		= kMy_ParserStateSeenESCRightParenZ,	//!< select character set for G1, Spanish
		kStateSCSG1Swedish1		= kMy_ParserStateSeenESCRightParenH,	//!< select character set for G1, Swedish
		kStateSCSG1Swedish2		= kMy_ParserStateSeenESCRightParen7,	//!< select character set for G1, Swedish (alternate)
		kStateSCSG1Swiss		= kMy_ParserStateSeenESCRightParenEquals,	//!< select character set for G1, Swiss
		kStateSCSG2UK			= kMy_ParserStateSeenESCAsteriskA,		//!< select character set for G2, U.K.
		kStateSCSG2ASCII		= kMy_ParserStateSeenESCAsteriskB,		//!< select character set for G2, ASCII
		kStateSCSG2SG			= kMy_ParserStateSeenESCAsterisk0,		//!< select character set for G2, special graphics
		kStateSCSG2DECSupplemental	= kMy_ParserStateSeenESCAsteriskLessThan,	//!< select character set for G2, DEC supplemental
		kStateSCSG2Dutch		= kMy_ParserStateSeenESCAsterisk4,		//!< select character set for G2, Dutch
		kStateSCSG2Finnish1		= kMy_ParserStateSeenESCAsteriskC,		//!< select character set for G2, Finnish
		kStateSCSG2Finnish2		= kMy_ParserStateSeenESCAsterisk5,		//!< select character set for G2, Finnish (alternate)
		kStateSCSG2French		= kMy_ParserStateSeenESCAsteriskR,		//!< select character set for G2, French
		kStateSCSG2FrenchCdn	= kMy_ParserStateSeenESCAsteriskQ,		//!< select character set for G2, French Canadian
		kStateSCSG2German		= kMy_ParserStateSeenESCAsteriskK,		//!< select character set for G2, German
		kStateSCSG2Italian		= kMy_ParserStateSeenESCAsteriskY,		//!< select character set for G2, Italian
		kStateSCSG2Norwegian1	= kMy_ParserStateSeenESCAsteriskE,		//!< select character set for G2, Norwegian and Danish
		kStateSCSG2Norwegian2	= kMy_ParserStateSeenESCAsterisk6,		//!< select character set for G2, Norwegian and Danish (alternate)
		kStateSCSG2Spanish		= kMy_ParserStateSeenESCAsteriskZ,		//!< select character set for G2, Spanish
		kStateSCSG2Swedish1		= kMy_ParserStateSeenESCAsteriskH,		//!< select character set for G2, Swedish
		kStateSCSG2Swedish2		= kMy_ParserStateSeenESCAsterisk7,		//!< select character set for G2, Swedish (alternate)
		kStateSCSG2Swiss		= kMy_ParserStateSeenESCAsteriskEquals,	//!< select character set for G2, Swiss
		kStateSCSG3UK			= kMy_ParserStateSeenESCPlusA,			//!< select character set for G3, U.K.
		kStateSCSG3ASCII		= kMy_ParserStateSeenESCPlusB,			//!< select character set for G3, ASCII
		kStateSCSG3SG			= kMy_ParserStateSeenESCPlus0,			//!< select character set for G3, special graphics
		kStateSCSG3DECSupplemental	= kMy_ParserStateSeenESCPlusLessThan,	//!< select character set for G3, DEC supplemental
		kStateSCSG3Dutch		= kMy_ParserStateSeenESCPlus4,			//!< select character set for G3, Dutch
		kStateSCSG3Finnish1		= kMy_ParserStateSeenESCPlusC,			//!< select character set for G3, Finnish
		kStateSCSG3Finnish2		= kMy_ParserStateSeenESCPlus5,			//!< select character set for G3, Finnish (alternate)
		kStateSCSG3French		= kMy_ParserStateSeenESCPlusR,			//!< select character set for G3, French
		kStateSCSG3FrenchCdn	= kMy_ParserStateSeenESCPlusQ,			//!< select character set for G3, French Canadian
		kStateSCSG3German		= kMy_ParserStateSeenESCPlusK,			//!< select character set for G3, German
		kStateSCSG3Italian		= kMy_ParserStateSeenESCPlusY,			//!< select character set for G3, Italian
		kStateSCSG3Norwegian1	= kMy_ParserStateSeenESCPlusE,			//!< select character set for G3, Norwegian and Danish
		kStateSCSG3Norwegian2	= kMy_ParserStateSeenESCPlus6,			//!< select character set for G3, Norwegian and Danish (alternate)
		kStateSCSG3Spanish		= kMy_ParserStateSeenESCPlusZ,			//!< select character set for G3, Spanish
		kStateSCSG3Swedish1		= kMy_ParserStateSeenESCPlusH,			//!< select character set for G3, Swedish
		kStateSCSG3Swedish2		= kMy_ParserStateSeenESCPlus7,			//!< select character set for G3, Swedish (alternate)
		kStateSCSG3Swiss		= kMy_ParserStateSeenESCPlusEquals,		//!< select character set for G3, Swiss
		kStateST				= kMy_ParserStateSeenESCBackslash,		//!< string terminator
	};
};

/*!
Manages state determination and transition for the XTerm
terminal emulator.  Automatically uses My_XTermCore for
various basic features, but also implements the rest of
the XTerm terminal type.
*/
class My_XTerm
{
public:
	static My_ParserState	returnCSINextState	(My_ParserState, UnicodeScalarValue, Boolean&);
	static UInt32			stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32			stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
	
	static void		cursorBackwardTabulation		(My_ScreenBufferPtr);
	static void		cursorCharacterAbsolute			(My_ScreenBufferPtr);
	static void		cursorForwardTabulation			(My_ScreenBufferPtr);
	static void		cursorNextLine					(My_ScreenBufferPtr);
	static void		cursorPreviousLine				(My_ScreenBufferPtr);
	static void		horizontalPositionAbsolute		(My_ScreenBufferPtr);
	static void		reportCursorStyle				(My_ScreenBufferPtr);
	static void		reportSelectionSettingNotUsed	(My_ScreenBufferPtr);
	static void		reportTopBottomMargins			(My_ScreenBufferPtr);
	static void		scrollDown						(My_ScreenBufferPtr);
	static void		scrollUp						(My_ScreenBufferPtr);
	static void		secondaryDeviceAttributes		(My_ScreenBufferPtr);
	static void		verticalPositionAbsolute		(My_ScreenBufferPtr);
	
	enum State
	{
		// Ideally these are "protected", but loop evasion code requires them.
		kStateCSITertiaryDA		= kMy_ParserStateSeenESCLeftSqBracketEquals,			//!< parameter list indicates tertiary device attributes (VT420)
		kStateCBT				= kMy_ParserStateSeenESCLeftSqBracketParamsZ,			//!< cursor backward tabulation
		kStateCHA				= kMy_ParserStateSeenESCLeftSqBracketParamsG,			//!< cursor character absolute
		kStateCHT				= kMy_ParserStateSeenESCLeftSqBracketParamsI,			//!< cursor forward tabulation
		kStateCNL				= kMy_ParserStateSeenESCLeftSqBracketParamsE,			//!< cursor next line
		kStateCPL				= kMy_ParserStateSeenESCLeftSqBracketParamsF,			//!< cursor previous line
		kStateHPA				= kMy_ParserStateSeenESCLeftSqBracketParamsBackquote,	//!< horizontal (character) position absolute
		kStateSD				= kMy_ParserStateSeenESCLeftSqBracketParamsT,			//!< scroll down
		kStateSU				= kMy_ParserStateSeenESCLeftSqBracketParamsS,			//!< scroll up
		kStateVPA				= kMy_ParserStateSeenESCLeftSqBracketParamsd,			//!< vertical position absolute
	};
};

/*!
A lightweight terminal implementation, suitable ONLY as a
pre-callback.  Implements certain basic features of XTerm,
such as colors and window title changes, that can be safely
superimposed onto other emulators like the VT100.  The state
determinant and transition routines automatically use the
My_Emulator class’ supportsVariant() method to determine if
a particular XTerm sequence should be handled; otherwise,
the sequences are ignored.  In addition, any more advanced
sequences from XTerm are always ignored.

NOTE:	Almost all of XTerm is ignored by this class.  See
		the class "My_XTerm" for the full emulator.
*/
class My_XTermCore
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, My_ParserStatePair const&, Boolean&);
	
	enum State
	{
		// Ideally these are "protected", but loop evasion code requires them.
		kStateSWIT				= kMy_ParserStateSeenESCRightSqBracket0,				//!< subsequent string is for window title and icon title
		kStateSWITAcquireStr	= kMy_ParserStateSeenESCRightSqBracket0Semi,			//!< seen ESC]0, gathering characters of string
		kStateSIT				= kMy_ParserStateSeenESCRightSqBracket1,				//!< subsequent string is for icon title only
		kStateSITAcquireStr		= kMy_ParserStateSeenESCRightSqBracket1Semi,			//!< seen ESC]1, gathering characters of string
		kStateSWT				= kMy_ParserStateSeenESCRightSqBracket2,				//!< subsequent string is for window title only
		kStateSWTAcquireStr		= kMy_ParserStateSeenESCRightSqBracket2Semi,			//!< seen ESC]2, gathering characters of string
		kStateSetColor			= kMy_ParserStateSeenESCRightSqBracket4,				//!< subsequent string is a color specification
		kStateColorAcquireStr	= kMy_ParserStateSeenESCRightSqBracket4Semi,			//!< seen ESC]4, gathering characters of string
	};
};

/*!
Thread context passed to threadForTerminalSearch().
*/
struct My_SearchThreadContext
{
	My_ScreenBufferPtr							screenBufferPtr; // the terminal being searched
	std::vector< Terminal_RangeDescription >*	matchesVectorPtr;
	CFStringRef									queryCFString;
	CFOptionFlags								searchFlags; // for CFStringCreateArrayWithFindResults()
	NSRegularExpressionOptions					regExOptions; // for NSRegularExpression calls
	Boolean										isRegularExpression; // perform non-literal pattern match
	UInt16										threadNumber; // thread 0 searches the screen, thread N searches every Nth scrollback line
	My_ScreenBufferLineList::const_iterator		rangeStart; // buffer offset (into screen if thread 0, otherwise scrollback)
	SInt32										startRowIndex; // index of the line pointed to by "rangeStart"
	UInt32										rowCount; // number of lines from buffer offset to search
};
typedef My_SearchThreadContext*			My_SearchThreadContextPtr;
typedef My_SearchThreadContext const*	My_SearchThreadContextConstPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void						assertScrollingRegion					(My_ScreenBufferPtr);
void						bufferEraseCursorLine					(My_ScreenBufferPtr, My_BufferChanges);
void						bufferEraseFromCursorColumn				(My_ScreenBufferPtr, My_BufferChanges, UInt16);
void						bufferEraseFromCursorColumnToLineEnd	(My_ScreenBufferPtr, My_BufferChanges);
void						bufferEraseFromCursorToEnd				(My_ScreenBufferPtr, My_BufferChanges);
void						bufferEraseFromHomeToCursor				(My_ScreenBufferPtr, My_BufferChanges);
void						bufferEraseFromLineBeginToCursorColumn  (My_ScreenBufferPtr, My_BufferChanges);
void						bufferEraseLineWithoutUpdate			(My_ScreenBufferPtr, My_BufferChanges, My_ScreenBufferLine&);
void						bufferEraseVisibleScreen				(My_ScreenBufferPtr, My_BufferChanges);
void						bufferInsertBlankLines					(My_ScreenBufferPtr, UInt16,
																	 My_ScreenBufferLineList::iterator&,
																	 My_AttributeRule);
void						bufferInsertBlanksAtCursorColumnWithoutUpdate	(My_ScreenBufferPtr, SInt16, My_AttributeRule);
void						bufferInsertInlineImageWithoutUpdate	(My_ScreenBufferPtr, NSImage*, UInt16, UInt16, UInt16, UInt16, Boolean, Boolean);
void						bufferLineFill							(My_ScreenBufferPtr, My_ScreenBufferLine&, UInt8,
																	 TextAttributes_Object = TextAttributes_Object(),
																	 Boolean = true);
void						bufferRemoveCharactersAtCursorColumn	(My_ScreenBufferPtr, SInt16, My_AttributeRule);
void						bufferRemoveLines						(My_ScreenBufferPtr, UInt16,
																	 My_ScreenBufferLineList::iterator&,
																	 My_AttributeRule);
void						changeLineAttributes					(My_ScreenBufferPtr, My_ScreenBufferLine&,
																	 TextAttributes_Object, TextAttributes_Object);
void						changeLineGlobalAttributes				(My_ScreenBufferPtr, My_ScreenBufferLine&,
																	 TextAttributes_Object, TextAttributes_Object);
void						changeLineRangeAttributes				(My_ScreenBufferPtr, My_ScreenBufferLine&, UInt16,
																	 SInt16, TextAttributes_Object, TextAttributes_Object);
void						changeNotifyForTerminal					(My_ScreenBufferConstPtr, Terminal_Change, void*);
My_ScreenBufferLinePtr		createLinePtr							();
void						cursorRestore							(My_ScreenBufferPtr);
void						cursorSave								(My_ScreenBufferPtr);
void						cursorWrapIfNecessaryGetLocation		(My_ScreenBufferPtr, SInt16*, My_ScreenRowIndex*);
Boolean						defineBitmap							(My_ScreenBufferPtr, NSImage*, NSRect, TextAttributes_BitmapID&);
Boolean						defineTrueColor							(My_ScreenBufferPtr, UInt8, UInt8, UInt8, TextAttributes_TrueColorID&);
void						deleteLinePtr							(My_ScreenBufferLinePtr&);
void						echoCFString							(My_ScreenBufferPtr, CFStringRef);
void						eraseRightHalfOfLine					(My_ScreenBufferPtr, My_ScreenBufferLine&);
inline My_LineIteratorPtr	getLineIterator							(Terminal_LineRef);
void						getParametersFromStringAccumulator		(My_ScreenBufferPtr, ParameterDecoder_StateMachine&,
																	 std::basic_string< UInt8 >::const_iterator&);
void						getParametersFromStringRange			(std::basic_string< UInt8 >::const_iterator, std::basic_string< UInt8 >::const_iterator,
																	 ParameterDecoder_StateMachine&, std::basic_string< UInt8 >::const_iterator&);
My_ScreenBufferPtr			getVirtualScreenData					(TerminalScreenRef);
void						highlightLED							(My_ScreenBufferPtr, SInt16);
My_StringByPointer			initCallbackIDsByFuncPtr				();
void						locateCursorLine						(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&);
void						locateScrollingRegion					(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&,
																	 My_ScreenBufferLineList::iterator&);
void						locateScrollingRegionTop				(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&);
void						moveCursor								(My_ScreenBufferPtr, SInt16, My_ScreenRowIndex);
void						moveCursorDown							(My_ScreenBufferPtr);
void						moveCursorDownOrScroll					(My_ScreenBufferPtr);
void						moveCursorDownToEdge					(My_ScreenBufferPtr);
void						moveCursorLeft							(My_ScreenBufferPtr);
void						moveCursorLeftToEdge					(My_ScreenBufferPtr);
void						moveCursorLeftToHalf					(My_ScreenBufferPtr);
void						moveCursorLeftToNextTabStop				(My_ScreenBufferPtr);
void						moveCursorRight							(My_ScreenBufferPtr);
void						moveCursorRightToEdge					(My_ScreenBufferPtr);
void						moveCursorRightToNextTabStop			(My_ScreenBufferPtr);
void						moveCursorUpToEdge						(My_ScreenBufferPtr);
void						moveCursorUp							(My_ScreenBufferPtr);
void						moveCursorUpOrScroll					(My_ScreenBufferPtr);
void						moveCursorX								(My_ScreenBufferPtr, SInt16);
void						moveCursorY								(My_ScreenBufferPtr, My_ScreenRowIndex);
void						resetTerminal							(My_ScreenBufferPtr, Boolean = false);
SessionRef					returnListeningSession					(My_ScreenBufferPtr);
Boolean						screenCopyLinesToScrollback				(My_ScreenBufferPtr);
Boolean						screenInsertNewLines					(My_ScreenBufferPtr, My_ScreenBufferLineList::size_type);
Boolean						screenMoveLinesToScrollback				(My_ScreenBufferPtr, My_ScreenBufferLineList::size_type);
void						screenScroll							(My_ScreenBufferPtr, SInt16 = 1);
void						setCursorVisible						(My_ScreenBufferPtr, Boolean);
void						setScrollbackSize						(My_ScreenBufferPtr, UInt32);
Terminal_Result				setVisibleColumnCount					(My_ScreenBufferPtr, UInt16);
Terminal_Result				setVisibleRowCount						(My_ScreenBufferPtr, UInt16);
CFStringRef					stringByStrippingEndWhitespace			(CFStringRef);
// IMPORTANT: Attribute bit manipulation is fully described in "TextAttributes.h".
//            Changes must be kept consistent everywhere.  See below, for usage.
inline TextAttributes_Object	styleOfVTParameter					(UInt16	inPs)
{
	return TextAttributes_Object(1 << (inPs - 1));
}
void						tabStopClearAll							(My_ScreenBufferPtr);
UInt16						tabStopGetDistanceFromCursor			(My_ScreenBufferConstPtr, Boolean);
void						tabStopInitialize						(My_ScreenBufferPtr);
void*						threadForTerminalSearch					(void*);
UniChar						translateCharacter						(My_ScreenBufferPtr, UnicodeScalarValue, TextAttributes_Object,
																	 TextAttributes_Object&);

} // anonymous namespace

#pragma mark Variables
namespace {

My_StringByPointer&				gCallbackIDsByFuncPtr ()	{ static My_StringByPointer x = initCallbackIDsByFuncPtr(); return x; }
My_PrintableByUniChar&			gDumbTerminalRenderings ()	{ static My_PrintableByUniChar x; return x; }
My_ScreenReferenceLocker&		gScreenRefLocks ()			{ static My_ScreenReferenceLocker x; return x; }
My_RefTracker&					gTerminalScreenValidRefs ()	{ static My_RefTracker x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new terminal screen and initial view according
to the given specifications.

Version 3.0 is designed so that a terminal screen buffer
can have multiple views, or vice-versa (a view can display
the contents of multiple buffers).  Therefore, although a
single buffer and view is created by this routine, more
could be constructed later.

\retval kTerminal_ResultOK
if the screen was created successfully

\retval kTerminal_ResultParameterError
if any of the given storage pointers are nullptr

\retval kTerminal_ResultNotEnoughMemory
if there is a serious problem creating the screen

(3.0)
*/
Terminal_Result
Terminal_NewScreen	(Preferences_ContextRef		inTerminalConfig,
					 Preferences_ContextRef		inTranslationConfig,
					 TerminalScreenRef*			outScreenPtr)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (outScreenPtr == nullptr) result = kTerminal_ResultParameterError;
	else
	{
		*outScreenPtr = nullptr;
		
		try
		{
			*outScreenPtr = REINTERPRET_CAST(new My_ScreenBuffer(inTerminalConfig, inTranslationConfig), TerminalScreenRef);
		}
		catch (std::bad_alloc)
		{
			result = kTerminal_ResultNotEnoughMemory;
			*outScreenPtr = nullptr;
		}
		catch (Terminal_Result		inConstructionError)
		{
			result = inConstructionError;
			*outScreenPtr = nullptr;
		}
		
		if (*outScreenPtr != nullptr)
		{
			// successfully created; set initial retain-count to 1
			Terminal_RetainScreen(*outScreenPtr);
		}
	}
	
	return result;
}// NewScreen


/*!
Adds a lock on the specified reference.  This indicates you
are using the screen, so attempts by anyone else to delete
the screen with Terminal_ReleaseScreen() will fail until you
release your lock (and everyone else releases locks they
may have).

(4.1)
*/
void
Terminal_RetainScreen	(TerminalScreenRef	inRef)
{
	gScreenRefLocks().acquireLock(inRef);
}// RetainScreen


/*!
Releases one lock on the specified screen created with
Terminal_NewScreen() and deletes the screen *if* no other
locks remain.  Your copy of the reference is set to
nullptr.

(4.1)
*/
void
Terminal_ReleaseScreen	(TerminalScreenRef*		inoutRefPtr)
{
	gScreenRefLocks().releaseLock(*inoutRefPtr);
	unless (gScreenRefLocks().isLocked(*inoutRefPtr))
	{
		My_ScreenBufferPtr		ptr = getVirtualScreenData(*inoutRefPtr);
		
		
		delete ptr;
	}
	*inoutRefPtr = nullptr;
}// ReleaseScreen


/*!
Creates a special reference to the given line of the given
terminal screen’s VISIBLE SCREEN buffer, or returns nullptr
if the line is out of range or the iterator cannot be created
for some other reason.

Pass 0 to indicate you want the very topmost line (that is,
the one that would be added to the scrollback buffer first if
the screen scrolls), or larger values to ask for lines below
it.  Currently, this routine takes linearly more time to
figure out where the bottommost lines are.

A main line iterator can be advanced using negative numbers
to go backwards, and if this is done often enough, it will
automatically enter the scrollback buffer (as if the iterator
were constructed with Terminal_NewScrollbackLineIterator()).

IMPORTANT:	An iterator is completely invalid once the
			screen it was created for has been destroyed.

(3.0)
*/
Terminal_LineRef
Terminal_NewMainScreenLineIterator	(TerminalScreenRef				inRef,
									 UInt16							inLineNumberZeroForTop,
									 Terminal_LineStackStorage*		inStackAllocationOrNull)
{
	Terminal_LineRef		result = nullptr;
	My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
	
	
	if (false == Terminal_IsValid(inRef))
	{
		Console_Warning(Console_WriteValueAddress, "attempt to construct iterator from invalid reference", inRef);
	}
	else if (nullptr != ptr)
	{
		// ensure the specified row is in range
		if (inLineNumberZeroForTop < ptr->screenBuffer.size())
		{
			auto	startIterator = ptr->screenBuffer.begin();
			
			
			std::advance(startIterator, STATIC_CAST(inLineNumberZeroForTop,
													My_ScreenBufferLineList::difference_type));
			if (nullptr != inStackAllocationOrNull)
			{
				new (inStackAllocationOrNull) My_LineIterator(false/* heap allocated */,
																ptr->screenBuffer, ptr->scrollbackBuffer,
																startIterator,
																My_LineIterator::kScreenBufferDesignator);
				result = REINTERPRET_CAST(inStackAllocationOrNull, Terminal_LineRef);
			}
			else
			{
				try
				{
					My_LineIteratorPtr		iteratorPtr = new My_LineIterator
																(true/* heap allocated */,
																	ptr->screenBuffer, ptr->scrollbackBuffer,
																	startIterator,
																	My_LineIterator::kScreenBufferDesignator);
					
					
					if (nullptr != iteratorPtr)
					{
						result = REINTERPRET_CAST(iteratorPtr, Terminal_LineRef);
					}
				}
				catch (std::bad_alloc)
				{
					result = nullptr;
				}
			}
		}
	}
	return result;
}// NewMainScreenLineIterator


/*!
Creates a special reference to the given line of the given
terminal screen’s SCROLLBACK buffer, or returns nullptr if
the line is out of range or the iterator cannot be created
for some other reason.

Pass 0 to indicate you want the very newest line (that is,
the one that most recently scrolled off the top of the main
screen), or larger values to ask for older lines.  Currently,
this routine takes linearly more time to figure out where
old scrollback lines are.

A scrollback line iterator can be advanced often enough to
automatically enter the main screen buffer (as if the iterator
were constructed with NewMainScreenLineIterator()).

IMPORTANT:	An iterator is completely invalid once the screen
			it was created for has been destroyed.  It can
			also be invalidated in other ways, e.g. a call to
			Terminal_DeleteAllSavedLines().

(3.0)
*/
Terminal_LineRef
Terminal_NewScrollbackLineIterator	(TerminalScreenRef				inRef,
									 UInt64							inLineNumberZeroForNewest,
									 Terminal_LineStackStorage*		inStackAllocationOrNull)
{
	Terminal_LineRef		result = nullptr;
	My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
	
	
	if ((nullptr != ptr) && (ptr->scrollbackBuffer.begin() != ptr->scrollbackBuffer.end()))
	{
		My_ScrollbackBufferLineList::difference_type const		kLineEnd = inLineNumberZeroForNewest;
		auto		startIterator = ptr->scrollbackBuffer.begin();
		Boolean		validIterator = true;
		
		
		// ensure the specified row is in range; since the scrollback buffer is
		// a linked list, it would be prohibitively expensive to ask for its size,
		// so instead the iterator is incremented while watching for the end
		for (My_ScrollbackBufferLineList::difference_type i = 0; i < kLineEnd; ++i)
		{
			++startIterator;
			if (startIterator == ptr->scrollbackBuffer.end())
			{
				validIterator = false;
				break;
			}
		}
		
		if (validIterator)
		{
			if (nullptr != inStackAllocationOrNull)
			{
				new (inStackAllocationOrNull) My_LineIterator(false/* heap allocated */,
																ptr->screenBuffer, ptr->scrollbackBuffer,
																startIterator);
				result = REINTERPRET_CAST(inStackAllocationOrNull, Terminal_LineRef);
			}
			else
			{
				try
				{
					My_LineIteratorPtr		iteratorPtr = new My_LineIterator
																(true/* heap allocated */,
																	ptr->screenBuffer, ptr->scrollbackBuffer,
																	startIterator);
					
					
					if (nullptr != iteratorPtr)
					{
						result = REINTERPRET_CAST(iteratorPtr, Terminal_LineRef);
					}
				}
				catch (std::bad_alloc)
				{
					result = nullptr;
				}
			}
		}
	}
	return result;
}// NewScrollbackLineIterator


/*!
Destroys an iterator created with Terminal_NewLineIterator(),
and sets your copy of the reference to nullptr.

Note that no actual memory is deallocated by this call if
stack storage was used to construct the iterator, although
your copy of the reference is still nullified.

(3.0)
*/
void
Terminal_DisposeLineIterator	(Terminal_LineRef*		inoutRefPtr)
{
	if (nullptr != inoutRefPtr)
	{
		My_LineIteratorPtr		ptr = getLineIterator(*inoutRefPtr);
		
		
		if (nullptr != ptr)
		{
			if (ptr->isHeapAllocated())
			{
				delete ptr;
			}
			*inoutRefPtr = nullptr;
		}
	}
}// DisposeLineIterator


/*!
Returns "true" only if the given terminal’s bell is
active.  An inactive bell completely ignores all
bell signals - without giving any audible or visible
indication that a bell has occurred.  This can be
useful if you know you’ve just triggered a long string
of bells and don’t want to be annoyed by a series of
beeps or flashes.

(3.0)
*/
Boolean
Terminal_BellIsEnabled		(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (dataPtr != nullptr)
	{
		result = !dataPtr->bellDisabled;
	}
	return result;
}// BellIsEnabled


/*!
Returns the bitmap image representation for the given ID,
which is defined (usually along with several other bitmaps)
if bitmap terminal sequences are received.  The ID may be
used in more than one place.

The bitmap is meant to have a resolution suitable for
rendering within a single default-sized character cell,
with no surrounding space except any defined by the pixels
of the image itself.  It is considered read-only at this
stage; anything that normally overwrites a terminal cell’s
attributes may clear the cell’s bitmaps however.

The returned image can be shared by many bitmap IDs but a
particular sub-rectangle of the image is also given to
specify the portion of the image that the ID represents.

Note that since the entire image is available from any ID
associated with any part of that image, user operations
that depend on a whole image are easier to implement (for
example, a contextual menu click or drag on a particular
terminal cell).

The TextAttributes_Object type can be used to extract the
TextAttributes_BitmapID value for a section of text, and
to determine what a bitmap is for.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultParameterError
if the specified ID is not valid

\retval kTerminal_ResultUnsupported
if the specified terminal was not configured for bitmaps

(2017.11)
*/
Terminal_Result
Terminal_BitmapGetFromID	(TerminalScreenRef			inRef,
							 TextAttributes_BitmapID	inID,
							 CGRect&					outImageSubRect,
							 NSImage*&					outCompleteImage)
{
	Terminal_Result		result = kTerminal_ResultOK;
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	outImageSubRect = CGRectZero; // initially...
	outCompleteImage = nil; // initially...
	
	if (nullptr == dataPtr)
	{
		result = kTerminal_ResultInvalidID;
	}
	else
	{
		if ((nil == dataPtr->emulator.bitmapSegmentTable) ||
			(inID >= dataPtr->emulator.bitmapSegmentTable.count))
		{
			result = kTerminal_ResultParameterError;
		}
		else
		{
			id			anObject = [dataPtr->emulator.bitmapSegmentTable objectAtIndex:inID];
			assert([anObject isKindOfClass:NSValue.class]); 
			NSValue*	rectValue = STATIC_CAST(anObject, NSValue*);
			NSRect		aRect = [rectValue rectValue];
			
			
			outImageSubRect = CGRectMake(aRect.origin.x, aRect.origin.y, NSWidth(aRect), NSHeight(aRect));
		}
		
		if ((nil == dataPtr->emulator.bitmapImageTable) ||
			(inID >= dataPtr->emulator.bitmapImageTable.count))
		{
			result = kTerminal_ResultParameterError;
		}
		else
		{
			outCompleteImage = [dataPtr->emulator.bitmapImageTable objectAtIndex:inID];
		}
	}
	
	return result;
}// BitmapGetFromID


/*!
Applies the specified changes to every single attribute
for a single line of the screen buffer (no effect on
the display until the next redraw). Also updates the
“current” attributes, if the cursor happens to be on
the specified line.

The row value may be negative, signifying a scrollback
buffer row.

Equivalent to invoking Terminal_ChangeLineRangeAttributes()
with a range from first column to last column.

You can use this to apply effects intended to be global
to a line, like double-sized text.

\retval kTerminal_ResultOK
if the line attributes were changed successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_Result
Terminal_ChangeLineAttributes	(TerminalScreenRef			inRef,
								 Terminal_LineRef			inRow,
								 TextAttributes_Object		inSetTheseAttributes,
								 TextAttributes_Object		inClearTheseAttributes)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultInvalidIterator;
	else
	{
		changeLineAttributes(dataPtr, iteratorPtr->currentLine(), inSetTheseAttributes, inClearTheseAttributes);
	}
	
	return result;
}// ChangeLineAttributes


/*!
Applies the specified changes to every single attribute
in the specified column range of the specified line
screen buffer (no effect on the display until the next
redraw).  Also updates the “current” attributes, if the
cursor happens to be in the specified range.

The row value may be negative, signifying a scrollback
buffer row.

Equivalent to invoking Terminal_ChangeRangeAttributes()
with the same start and end rows.

You can use this to apply effects intended to be applied
to a series of characters on a line, like text coloring.

\retval kTerminal_ResultOK
if the line range attributes were changed successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_Result
Terminal_ChangeLineRangeAttributes	(TerminalScreenRef			inRef,
									 Terminal_LineRef			inRow,
									 UInt16						inZeroBasedStartColumn,
									 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
									 TextAttributes_Object		inSetTheseAttributes,
									 TextAttributes_Object		inClearTheseAttributes)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultInvalidIterator;
	else
	{
		changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), inZeroBasedStartColumn,
									inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
									inSetTheseAttributes, inClearTheseAttributes);
	}
	
	return result;
}// ChangeLineRangeAttributes


/*!
Applies the specified changes to every single attribute
within the specified range of the screen buffer (no
effect on the display until the next redraw).  Also
updates the “current” attributes, if the cursor happens
to be in the specified range.

The anchor points are automatically “sorted”, so despite
their names it does not matter which point really marks
the start or end of the range.

If "inConstrainToRectangle" is true, the range is
considered to be exact so no columns outside the range
are filled in (normally, all lines in between the start
and end points are filled completely, from column 0 to
the last column).

You can use this to apply effects to a large number of
characters at once (e.g. when highlighting).

\retval kTerminal_ResultOK
if the range attributes were changed successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_Result
Terminal_ChangeRangeAttributes	(TerminalScreenRef			inRef,
								 Terminal_LineRef			inStartRow,
								 UInt32						inNumberOfRowsToConsider,
								 UInt16						inZeroBasedStartColumn,
								 UInt16						inZeroBasedPastTheEndColumn,
								 Boolean					inConstrainToRectangle,
								 TextAttributes_Object		inSetTheseAttributes,
								 TextAttributes_Object		inClearTheseAttributes)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	// force anchors to be within valid screen boundaries
	if (inZeroBasedStartColumn >= dataPtr->text.visibleScreen.numberOfColumnsPermitted)
	{
		inZeroBasedStartColumn = dataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
	}
	if (inZeroBasedPastTheEndColumn > dataPtr->text.visibleScreen.numberOfColumnsPermitted)
	{
		inZeroBasedPastTheEndColumn = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
	}
	
	if (inNumberOfRowsToConsider > 0)
	{
		// now apply changes to attributes of all specified cells, and to
		// the “current” attributes (if the cursor is anywhere in the range)
		if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
		else if (nullptr == iteratorPtr) result = kTerminal_ResultInvalidIterator;
		else
		{
			Boolean const		kSingleLineRange = (1 == inNumberOfRowsToConsider);
			
			
			// figure out how to treat the first line
			if (kSingleLineRange)
			{
				// same line - iterate only over certain columns
				changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), inZeroBasedStartColumn, inZeroBasedPastTheEndColumn,
											inSetTheseAttributes, inClearTheseAttributes);
			}
			else
			{
				// multi-line range; start by creating some convenient anchors that
				// are set correctly based on the rectangular/non-rectangular mode
				UInt16		lineStartColumn = (inConstrainToRectangle)
												? inZeroBasedStartColumn
												: 0;
				SInt16		linePastTheEndColumn = (inConstrainToRectangle)
													? inZeroBasedPastTheEndColumn
													: dataPtr->text.visibleScreen.numberOfColumnsPermitted;
				Boolean		isEnd = false;
				
				
				// fill in first line
				changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), inZeroBasedStartColumn, linePastTheEndColumn,
											inSetTheseAttributes, inClearTheseAttributes);
				iteratorPtr->goToNextLine(isEnd);
				
				if (false == isEnd)
				{
					// fill in remaining lines; the last line is special
					// because it will end “early” at the end anchor
					{
						SInt16			i = 0;
						SInt32 const	kLineEnd = (inNumberOfRowsToConsider - 1);
						
						
						for (i = 0; i < kLineEnd; ++i, iteratorPtr->goToNextLine(isEnd))
						{
							if (isEnd)
							{
								//Console_Warning(Console_WriteLine, "exceeded row boundaries when changing attributes of a range");
								break;
							}
							
							// intermediate lines occupy the whole range; the last line
							// only occupies up to the final anchor point
							if (i < (kLineEnd - 1))
							{
								// fill in intermediate line
								changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), lineStartColumn, linePastTheEndColumn,
															inSetTheseAttributes, inClearTheseAttributes);
							}
							else
							{
								// fill in last line
								changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), lineStartColumn, inZeroBasedPastTheEndColumn,
															inSetTheseAttributes, inClearTheseAttributes);
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}// ChangeRangeAttributes


/*!
Returns the title assigned to the iconified version of this
terminal.  In MacTerm this is symbolic, as no assumption is
made that a terminal corresponds to a single minimized window;
the exact usage of the “icon title” is not decided by this
module.

IMPORTANT:	You must eventually use CFRelease() on the returned
			title string.

(3.0)
*/
void
Terminal_CopyTitleForIcon	(TerminalScreenRef	inRef,
							 CFStringRef&		outTitle)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	outTitle = dataPtr->iconTitleCFString.returnCFStringRef();
	if (outTitle != nullptr)
	{
		CFRetain(outTitle);
	}
}// CopyTitleForIcon


/*!
Returns the title assigned to the window of this terminal.  In
MacTerm this is symbolic, as no assumption is made that a
terminal corresponds to a single window; the exact usage of the
“window title” is not decided by this module.

IMPORTANT:	You must eventually use CFRelease() on the returned
			title string.

(3.0)
*/
void
Terminal_CopyTitleForWindow		(TerminalScreenRef	inRef,
								 CFStringRef&		outTitle)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	outTitle = dataPtr->windowTitleCFString.returnCFStringRef();
	if (outTitle != nullptr)
	{
		CFRetain(outTitle);
	}
}// CopyTitleForWindow


/*!
Returns the zero-based column and line numbers of the
terminal cursor in the given buffer.  Since the cursor
is not allowed to enter the scrollback buffer, negative
return values are not possible.

Pass nullptr for one of the values if you do not want it.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_CursorGetLocation	(TerminalScreenRef		inRef,
							 UInt16*				outZeroBasedColumnPtr,
							 UInt16*				outZeroBasedRowPtr)
{
	Terminal_Result				result = kTerminal_ResultOK;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else
	{
		if (outZeroBasedColumnPtr != nullptr) *outZeroBasedColumnPtr = dataPtr->current.cursorX;
		if (outZeroBasedRowPtr != nullptr) *outZeroBasedRowPtr = STATIC_CAST(dataPtr->current.cursorY, UInt16);
	}
	
	return result;
}// CursorGetLocation


/*!
Returns "true" only if the terminal cursor is supposed
to be visible in the specified screen buffer.  This is
a logical state ONLY, and is set by interaction with the
terminal emulator.  It is only guaranteed to correspond
to a rendered cursor by virtue of the callback framework
that links Terminal Views to screen buffers.

(3.0)
*/
Boolean
Terminal_CursorIsVisible	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->cursorVisible;
	}
	return result;
}// CursorIsVisible


/*!
Provides all the text attributes of the position currently
occupied by the cursor.

NOTE:	Admittedly this is a bit of a weird exception (since
		all other style information is returned as part of a
		Terminal_ForEachLikeAttributeRun() loop) but the
		cursor is rendered at unusual times and has a special
		appearance.

(4.0)
*/
TextAttributes_Object
Terminal_CursorReturnAttributes		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr			dataPtr = getVirtualScreenData(inRef);
	TextAttributes_Object		result = dataPtr->current.cursorAttributes;
	
	
	return result;
}// CursorReturnAttributes


/*!
Writes arbitrary debugging information to the console for the
specified terminal screen.

(4.0)
*/
void
Terminal_DebugDumpDetailedSnapshot		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	Console_WriteValuePair("Visible boundary first and last columns", dataPtr->visibleBoundary.firstColumn,
																		dataPtr->visibleBoundary.lastColumn);
	Console_WriteValuePair("Visible boundary first and last rows", dataPtr->visibleBoundary.rows.firstRow,
																	dataPtr->visibleBoundary.rows.lastRow);
	Console_WriteValuePair("Defined scrolling region first and last rows", dataPtr->customScrollingRegion.firstRow,
																			dataPtr->customScrollingRegion.lastRow);
	Console_WriteValuePair("Current scrolling region first and last rows", dataPtr->originRegionPtr->firstRow,
																			dataPtr->originRegionPtr->lastRow);
	Console_WriteValue("Mode: auto-wrap", dataPtr->modeAutoWrap);
	Console_WriteValue("Mode: cursor keys for application", dataPtr->modeCursorKeysForApp);
	Console_WriteValue("Mode: application keys", dataPtr->modeApplicationKeys);
	Console_WriteValue("Mode: origin redefined", dataPtr->modeOriginRedefined);
	Console_WriteValue("Mode: insert, not replace", dataPtr->modeInsertNotReplace);
	Console_WriteValue("Mode: new-line option", dataPtr->modeNewLineOption);
	Console_WriteValue("Emulator UTF-8 mode", dataPtr->emulator.isUTF8Encoding);
	Console_WriteValue("Emulator ignores requests to shift character sets", dataPtr->emulator.disableShifts);
	Console_WriteValue("Emulator accepts 8-bit and 7-bit codes", dataPtr->emulator.is8BitReceiver());
	Console_WriteValue("Emulator transmits 8-bit codes", dataPtr->emulator.is8BitTransmitter());
	// INCOMPLETE - could put just about anything here, whatever is interesting to know
}// DebugDumpDetailedSnapshot


/*!
Destroys all scrollback buffer lines.  The “visible”
lines are not affected.  This obviously invalidates any
iterators pointing to scrollback lines.

It follows that after invoking this routine, the return
value of Terminal_ReturnInvisibleRowCount() should be zero.

This triggers two events: "kTerminal_ChangeScrollActivity"
to indicate that data has been removed, and also
"kkTerminal_ChangeTextRemoved" (given a range consisting of
all previous scrollback lines).

(3.1)
*/
void
Terminal_DeleteAllSavedLines	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		SInt16 const	kPreviousScrollbackCount = STATIC_CAST(dataPtr->scrollbackBufferCachedSize, SInt16);
		
		
		dataPtr->scrollbackBuffer.clear();
		dataPtr->scrollbackBufferCachedSize = 0;
		
		// notify listeners of the range of text that has gone away
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = dataPtr->selfRef;
			range.firstRow = -kPreviousScrollbackCount;
			range.firstColumn = 0;
			range.columnCount = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kPreviousScrollbackCount;
			
			changeNotifyForTerminal(dataPtr, kTerminal_ChangeTextRemoved, &range/* context */);
		}
		
		// notify listeners that scroll activity has taken place,
		// though technically no remaining lines have been affected
		{
			Terminal_ScrollDescription	scrollInfo;
			
			
			bzero(&scrollInfo, sizeof(scrollInfo));
			scrollInfo.screen = dataPtr->selfRef;
			scrollInfo.rowDelta = 0;
			changeNotifyForTerminal(dataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
		}
	}
}// DeleteAllSavedLines


/*!
Sends a stream of characters originating in a
C-style string to the specified screen’s terminal
emulator, interpreting escape sequences, etc.
which may affect the terminal display.

USE WITH CARE.  Generally, you send data to this
routine that comes directly from a program that
knows what it’s doing.  There are more elegant
ways to have specific effects on terminal screens
in an emulator-independent fashion; you should
use those routines before hacking up a string as
input to this routine.

\retval kTerminal_ResultOK
if the text is processed without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_EmulatorProcessCString		(TerminalScreenRef	inRef,
									 char const*		inCString)
{
	return Terminal_EmulatorProcessData(inRef, REINTERPRET_CAST(inCString, UInt8 const*), CPP_STD::strlen(inCString));
}// EmulatorProcessCString


/*!
Sends a stream of characters to the specified screen’s terminal
emulator, interpreting escape sequences, etc. which may affect
the terminal display.

The given buffer is considered a fragment of an infinite stream
that continues from text sent by previous calls.  So, do not
assume that "inBuffer" will be the start or end of anything in
particular!  For example, if the previous byte stream ended with
an incomplete multi-byte code point then the first few bytes of
"inBuffer" may act as the terminator of the previous sequence.

If the encoding, as set by Terminal_SetTextEncoding(), is UTF-8
then the specified buffer is decoded BEFORE any bytes are
examined for things like control characters.  (Text in the 7-bit
range of ASCII is considered valid UTF-8 and acts the same in
any case.)  If you intend to include special sequences such as
8-bit control characters, you MUST ensure they are encoded as
proper multi-byte sequences in UTF-8 terminals.

USE WITH CARE.  Generally, you send data to this routine that
comes directly from a program that knows what it’s doing.  There
are more elegant ways to have specific effects on terminal
screens in an emulator-independent fashion; you should use those
approaches before hacking up a string as input to this routine.

\retval kTerminal_ResultOK
if the text is processed without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_EmulatorProcessData	(TerminalScreenRef	inRef,
								 UInt8 const*		inBuffer,
								 size_t				inLength)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (0 != inLength)
	{
		My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
		
		
		if (nullptr == dataPtr)
		{
			result = kTerminal_ResultInvalidID;
		}
		else
		{
			Boolean const	kIsUTF8 = (kCFStringEncodingUTF8 == dataPtr->emulator.inputTextEncoding);
			UInt8 const*	ptr = inBuffer;
			UInt32			countRead = 0;
			
			
			// hide cursor momentarily
			setCursorVisible(dataPtr, false);
			
			// interpret the character stream, one character at a time; NOTE that
			// since this is just a slice of a continuous and infinite stream,
			// all state information is kept in the data structure (e.g. the last
			// character in this particular buffer may form the first character
			// in a sequence, and this scan must be continued the next time this
			// function is called)
			for (size_t i = inLength; i > 0; )
			{
				Boolean		skipEmulators = false;
				
				
				dataPtr->emulator.recentCodePointByte = *ptr;
				
				// when UTF-8 is in use, the stream is decoded BEFORE anything processes
				// the bytes further (including emulators), and a separate set of states
				// prevents emulators from even seeing bytes until complete sequences
				// are discovered
				if (kIsUTF8)
				{
					UInt32		errorCount = 0;
					
					
					dataPtr->emulator.multiByteDecoder.nextState(*ptr, errorCount);
					
					// errors might be detected, although not necessarily in the current byte itself
					// (for instance, the byte may start a new sequence while the previous sequence
					// has not yet seen enough continuation bytes); if both the previous sequence
					// and the current byte indicate separate errors, more than one character can
					// possibly be printed here; but the normal case hopefully is zero!
					for (UInt32 j = 0; j < errorCount; ++j)
					{
						UTF8Decoder_StateMachine::appendErrorCharacter(dataPtr->bytesToEcho);
					}
					
					if (UTF8Decoder_StateMachine::kStateUTF8ValidSequence != dataPtr->emulator.multiByteDecoder.returnState())
					{
						// sequence is incomplete; only allow emulators to process complete code points
						skipEmulators = true;
					}
					
					// skip over the byte that was read
					--i;
					++ptr;
				}
				
				if (skipEmulators)
				{
					// debugging only
					if (DebugInterface_LogsTerminalInputChar())
					{
						Console_WriteValueCharacter("terminal loop: next byte of incomplete sequence", *ptr);
					}
				}
				else
				{
					My_ParserStatePair	states = std::make_pair(dataPtr->emulator.currentState,
																dataPtr->emulator.currentState);
					void const*			handlingProcPtr = nullptr; // see below
					Boolean				isHandled = false;
					Boolean				isInterrupt = false;
					
					
					// debugging only
					if (DebugInterface_LogsTerminalInputChar())
					{
						Console_WriteValueUnicodePoint("input code point", dataPtr->emulator.recentCodePoint());
					}
					
					// find a new state, which may or may not interrupt the state that is
					// currently forming
					isHandled = false;
					for (auto callbackInfo : dataPtr->emulator.preCallbackSet)
					{
						countRead = invokeEmulatorStateDeterminantProc
									(callbackInfo.stateDeterminant, &dataPtr->emulator,
										states, isInterrupt, isHandled);
						if (isHandled)
						{
							break;
						}
					}
					unless (isHandled)
					{
						// most of the time, the ordinary emulator is used
						countRead = invokeEmulatorStateDeterminantProc
									(dataPtr->emulator.currentCallbacks.stateDeterminant, &dataPtr->emulator,
										states, isInterrupt, isHandled);
					}
					unless (kIsUTF8)
					{
						assert(countRead <= i);
						//Console_WriteValue("number of characters absorbed by state determinant", countRead); // debug
						i -= countRead; // could be zero (no-op)
						ptr += countRead; // could be zero (no-op)
					}
					
					// interrupts are never captured for echo
					if (false == isInterrupt)
					{
						// DEBUG:
						//Console_WriteValueFourChars("transition to state", states.second);
						
						// LOOPING GUARD: whenever the proposed next state matches the
						// current state, a counter is incremented; if this count
						// exceeds an arbitrary value, the next state is FORCED to
						// return to the initial state (flagging a console error) so
						// that this does not hang the application in an infinite loop
						if (states.first == states.second)
						{
							// exclude the echo data, because this is the most likely state
							// to last awhile (e.g. long strings of printable text)
							if (states.second != kMy_ParserStateAccumulateForEcho)
							{
								Boolean		interrupt = (dataPtr->emulator.stateRepetitions > 100/* arbitrary */);
								
								
								++dataPtr->emulator.stateRepetitions;
								
								if (interrupt)
								{
									// some states allow a bit more leeway before panicking,
									// because there could be good reasons for long data streams
									// TEMPORARY; it may make the most sense to also defer loop
									// evasion to an emulator method, so that each emulator type
									// can handle its own custom states (and only when that
									// emulator is actually in use!)
									if ((states.second == My_XTermCore::kStateSITAcquireStr) ||
										(states.second == My_XTermCore::kStateSWTAcquireStr) ||
										(states.second == My_XTermCore::kStateSWITAcquireStr))
									{
										interrupt = (dataPtr->emulator.stateRepetitions > 255/* arbitrary */);
									}
									else if (states.second == My_VT220::kStateDCSAcquireStr)
									{
										// arbitrarily allow massive image data sizes (TEMPORARY; make user-configurable?)
										static UInt64 const		kMaxBytesPerImage = (20 * 1024 * 1024);
										
										
										interrupt = (dataPtr->emulator.stateRepetitions > kMaxBytesPerImage);
									}
									else if (states.second == My_ITermCore::kStateITermAcquireStr)
									{
										// arbitrarily allow massive image data sizes (TEMPORARY; make user-configurable?)
										static UInt64 const		kMaxBytesITerm2 = (20 * 1024 * 1024);
										
										
										interrupt = (dataPtr->emulator.stateRepetitions > kMaxBytesITerm2);
									}
								}
								
								if (interrupt)
								{
									Boolean const	kLogThis = DebugInterface_LogsTerminalState();
									
									
									if (kLogThis)
									{
										Console_WriteHorizontalRule();
										Console_WriteValueFourChars("SERIOUS PARSER ERROR: appears to be stuck, state", states.first);
									}
									
									if (kMy_ParserStateInitial == states.first)
									{
										unless (kIsUTF8)
										{
											// if somehow stuck oddly in the initial state, assume
											// the trigger character is responsible and simply
											// ignore the troublesome sequence of characters
											if (kLogThis)
											{
												Console_WriteValueCharacter("FORCING step-over of byte", *ptr);
											}
											--i;
											++ptr;
										}
									}
									else
									{
										if (kLogThis)
										{
											Console_WriteLine("FORCING a return to the initial state");
										}
										states.second = kMy_ParserStateInitial;
									}
									
									if (kLogThis)
									{
										Console_WriteHorizontalRule();
									}
									
									dataPtr->emulator.stateRepetitions = 0;
								}
							}
						}
						else
						{
							dataPtr->emulator.stateRepetitions = 0;
						}
						
						if (kMy_ParserStateAccumulateForEcho == states.second)
						{
							// gather a byte for later use in display, but do not display yet;
							// while it would be nice to feed the raw data stream into the
							// translation APIs, translators might not stop when they see
							// special characters such as control characters; so, the terminal
							// emulator (above) has the first crack at the byte to see if there
							// is any special meaning, and the byte is cached only if the
							// terminal allows the byte to be echoed; the echo does not actually
							// occur until a future byte triggers a non-echo state; at that
							// time, the cached array of bytes is sent to the translator and
							// converted into human-readable text
							if (kIsUTF8)
							{
								// in UTF-8 mode, it is certainly possible to have illegal sequences,
								// so only capture bytes for echo when a sequence is complete; also,
								// it is not necessary to offset "i" or "ptr" in this mode because
								// the current byte was already skipped by the initial decoding
								if (UTF8Decoder_StateMachine::kStateUTF8ValidSequence == dataPtr->emulator.multiByteDecoder.returnState())
								{
									if (false == dataPtr->emulator.multiByteDecoder.multiByteAccumulator.empty())
									{
										std::copy(dataPtr->emulator.multiByteDecoder.multiByteAccumulator.begin(),
													dataPtr->emulator.multiByteDecoder.multiByteAccumulator.end(),
													std::back_inserter(dataPtr->bytesToEcho));
										dataPtr->emulator.multiByteDecoder.reset();
									}
									else
									{
										dataPtr->bytesToEcho.push_back(*ptr);
									}
								}
							}
							else
							{
								dataPtr->bytesToEcho.push_back(*ptr);
								--i;
								++ptr;
							}
						}
					}
					
					// if the new state is no longer echo accumulation, or this chunk of the
					// infinite stream ended with echo-ready data, flush as much as possible
					if ((kMy_ParserStateAccumulateForEcho != states.second) || (0 == i))
					{
						if (false == dataPtr->bytesToEcho.empty())
						{
							std::string::size_type const	kOldSize = dataPtr->bytesToEcho.size();
							UInt32							bytesUsed = invokeEmulatorEchoDataProc
																		(dataPtr->emulator.currentCallbacks.dataWriter, dataPtr,
																			dataPtr->bytesToEcho.data(), STATIC_CAST(kOldSize, UInt32));
							
							
							// it is possible for this chunk of the stream to terminate with
							// an incomplete encoding of bytes, so preserve anything that
							// could not be echoed this time around
							assert(bytesUsed <= kOldSize);
							if (0 == bytesUsed)
							{
								// special case...some kind of error, no bytes were translated;
								// dump the buffer, which LOSES DATA, but this is a spin control
								++(dataPtr->echoErrorCount);
								dataPtr->bytesToEcho.clear();
							}
							else if (bytesUsed > 0)
							{
								dataPtr->bytesToEcho.erase(0/* starting position */, bytesUsed/* count */);
							}
						}
					}
					
					// perform whatever action is appropriate to enter this state
					Boolean const	kIsLogged = ((DebugInterface_LogsTerminalEcho() && (kMy_ParserStateAccumulateForEcho == states.second)) ||
													(DebugInterface_LogsTerminalState() && (kMy_ParserStateAccumulateForEcho != states.second)));
					if (kIsLogged)
					{
						// clear handler sequence (see logging of sequence a bit later, below);
						// note that invokeEmulatorStateTransitionProc() will update the sequence
						dataPtr->debugStateHandlerSequence.clear();
					}
					isHandled = false;
					for (auto callbackInfo : dataPtr->emulator.preCallbackSet)
					{
						countRead = invokeEmulatorStateTransitionProc
									(callbackInfo.transitionHandler, dataPtr, states, isHandled);
						if (isHandled)
						{
							// when debugging, capture the callback that deals with the new state
							handlingProcPtr = REINTERPRET_CAST(callbackInfo.transitionHandler, void const*);
							
							break;
						}
					}
					unless (isHandled)
					{
						countRead = invokeEmulatorStateTransitionProc
									(dataPtr->emulator.currentCallbacks.transitionHandler,
										dataPtr, states, isHandled);
						if (isHandled)
						{
							// when debugging, capture the callback that deals with the new state
							handlingProcPtr = REINTERPRET_CAST(dataPtr->emulator.currentCallbacks.transitionHandler, void const*);
						}
					}
					unless (kIsUTF8)
					{
						assert(countRead <= i);
						//Console_WriteValue("number of characters absorbed by transition handler", countRead); // debug
						i -= countRead; // could be zero (no-op)
						ptr += countRead; // could be zero (no-op)
					}
					
					if ((isHandled) && (kIsLogged))
					{
						// show the handler that determined the new state
						std::ostringstream	debugInfoSS;
						std::string			debugStr;
						CFRetainRelease		newStateDescCFString(UTCreateStringForOSType(states.second),
																	CFRetainRelease::kAlreadyRetained);
						char const*			newStateCStringOrNull = (newStateDescCFString.exists()
																		? CFStringGetCStringPtr(newStateDescCFString.returnCFStringRef(),
																								CFStringGetFastestEncoding
																								(newStateDescCFString.returnCFStringRef()))
																		: nullptr);
						
						
						debugInfoSS << "new state '";
						if (nullptr == newStateCStringOrNull)
						{
							debugInfoSS << "<" << states.second << ">"; // failed to convert; show integer value
						}
						else
						{
							debugInfoSS << newStateCStringOrNull;
						}
						if (false == dataPtr->debugStateHandlerSequence.empty())
						{
							debugInfoSS << "'; handler path:";
							for (auto aProcPtr : dataPtr->debugStateHandlerSequence)
							{
								auto			procIter = gCallbackIDsByFuncPtr().find(aProcPtr);
								char const*		procID = ((procIter == gCallbackIDsByFuncPtr().end())
															? "?"
															: procIter->second);
								
								
								debugInfoSS << " -> " << procID;
							}
						}
						debugStr = debugInfoSS.str();
						Console_WriteLine(debugStr.c_str());
					}
					
					if (false == isInterrupt)
					{
						// remember this new state
						dataPtr->emulator.currentState = states.second;
					}
					
					// if a complete sequence has been read, throw those cached bytes away
					unless (dataPtr->emulator.multiByteDecoder.incompleteSequence())
					{
						dataPtr->emulator.multiByteDecoder.reset();
					}
				}
			}
			
			// restore cursor
			setCursorVisible(dataPtr, true);
		}
		
		// to minimize spam, count certain classes of data error in
		// the loop above, and only report them at the end
		if ((dataPtr->echoErrorCount != 0) ||
			(dataPtr->translationErrorCount != 0))
		{
			Boolean const	kDebugExcessiveErrors = false;
			
			
			++(dataPtr->errorCountTotal);
			if (dataPtr->errorCountTotal == 10/* arbitrary; equality is used to ensure that this event can only fire once */)
			{
				changeNotifyForTerminal(dataPtr, kTerminal_ChangeExcessiveErrors, inRef);
			}
			
			if (kDebugExcessiveErrors)
			{
				Console_Warning(Console_WriteValue, "at least some characters were SKIPPED due to the following errors; original buffer length", inLength);
			}
			if (dataPtr->echoErrorCount != 0)
			{
				if (kDebugExcessiveErrors)
				{
					Console_Warning(Console_WriteValue, "number of times that echoing unexpectedly failed", dataPtr->echoErrorCount);
				}
				dataPtr->echoErrorCount = 0;
			}
			if (dataPtr->translationErrorCount != 0)
			{
				if (kDebugExcessiveErrors)
				{
					Console_WriteValue("current terminal text encoding", dataPtr->emulator.inputTextEncoding);
					Console_Warning(Console_WriteValue, "number of times that translation unexpectedly failed", dataPtr->translationErrorCount);
				}
				dataPtr->translationErrorCount = 0;
			}
		}
	}
	
	return result;
}// EmulatorProcessData


/*!
Returns the default name for the given emulation type,
suitable for use in a TERM environment variable or
answer-back message.  For example, "vt100" is the name
of kEmulation_FullTypeVT100.

The string is not retained, so do not release it.

See also Terminal_EmulatorReturnForName(), the reverse
of this routine, and Terminal_EmulatorReturnName(), which
returns whatever a screen is using.

(3.1)
*/
CFStringRef
Terminal_EmulatorReturnDefaultName		(Emulation_FullType		inEmulationType)
{
	CFStringRef		result = nullptr;
	
	
	// IMPORTANT: This should be the inverse of Terminal_EmulatorReturnForName().
	switch (inEmulationType)
	{
	case kEmulation_FullTypeANSIBBS:
		result = CFSTR("ansi-bbs");
		break;
	
	case kEmulation_FullTypeANSISCO:
		result = CFSTR("ansi-sco");
		break;
	
	case kEmulation_FullTypeDumb:
		result = CFSTR("dumb");
		break;
	
	case kEmulation_FullTypeVT100:
		result = CFSTR("vt100");
		break;
	
	case kEmulation_FullTypeVT102:
		result = CFSTR("vt102");
		break;
	
	case kEmulation_FullTypeVT220:
		result = CFSTR("vt220");
		break;
	
	case kEmulation_FullTypeVT320:
		result = CFSTR("vt320");
		break;
	
	case kEmulation_FullTypeVT420:
		result = CFSTR("vt420");
		break;
	
	case kEmulation_FullTypeXTermColor:
		result = CFSTR("xterm-color");
		break;
	
	case kEmulation_FullTypeXTerm256Color:
		result = CFSTR("xterm-256color");
		break;
	
	case kEmulation_FullTypeXTermOriginal:
		result = CFSTR("xterm");
		break;
	
	default:
		// ???
		assert(false);
		result = CFSTR("UNKNOWN");
		break;
	}
	
	return result;
}// EmulatorReturnDefaultName


/*!
Returns the emulation type for the given name, if any
(otherwise, chooses a reasonable value).  For example,
"vt100" corresponds to kEmulation_FullTypeVT100.

See also EmulatorReturnDefaultName().

(3.1)
*/
Emulation_FullType
Terminal_EmulatorReturnForName		(CFStringRef	inName)
{
	Emulation_FullType	result = kEmulation_FullTypeVT100;
	
	
	// IMPORTANT: This should be the inverse of EmulatorReturnDefaultName().
	if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("ansi-bbs"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeANSIBBS;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("ansi-sco"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeANSISCO;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("dumb"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeDumb;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt100"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeVT100;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt102"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeVT102;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt220"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeVT220;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt320"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeVT320;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt420"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeVT420;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("xterm"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeXTermOriginal;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("xterm-color"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeXTermColor;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("xterm-256color"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kEmulation_FullTypeXTerm256Color;
	}
	else
	{
		// ???
		assert(false);
	}
	
	return result;
}// EmulatorReturnForName


/*!
Returns the current name (often called the answer-back
message) for the emulator being used by the given screen.
This value really could be anything; it is typically used
to set a TERM environment variable for communication
about terminal type with a running process.  If the
process does not directly support a MacTerm emulator,
but supports something mostly compatible, this routine
may return the name of the compatible terminal.

The string is not retained, so do not release it.

See also EmulatorReturnDefaultName(), which always
returns a specific name for a supported emulator.

(3.1)
*/
CFStringRef
Terminal_EmulatorReturnName		(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	CFStringRef				result = nullptr;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->emulator.answerBackCFString.returnCFStringRef();
	}
	
	return result;
}// EmulatorReturnName


/*!
Initiates a capture to a file for the specified screen,
notifying listeners of "kTerminal_ChangeFileCaptureBegun"
events if successful.  The callbacks are invoked after
internal file capture state has been set up.

If "inAutoClose" is set to true, then you NO LONGER OWN
the given File Manager file, and you should not close it
yourself; it will be closed for you, whenever the capture
ends (through Terminal_FileCaptureEnd(), or the destruction
of this terminal screen).  Otherwise, YOU must close the
file, after you have finished with the terminal that is
writing to it.  See the "kTerminal_ChangeFileCaptureEnding"
event, which will tell you exactly when the capture is
finished.

If the capture begins successfully, "true" is returned;
otherwise, "false" is returned.

(3.0)
*/
Boolean
Terminal_FileCaptureBegin	(TerminalScreenRef	inRef,
							 CFURLRef			inFileToOverwrite)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Boolean					result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = StreamCapture_Begin(dataPtr->captureStream, inFileToOverwrite);
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeFileCaptureBegun, inRef);
	}
	return result;
}// FileCaptureBegin


/*!
Terminates the file capture for the specified screen,
notifying listeners of "kTerminal_ChangeFileCaptureEnding"
events just prior to clearing internal file capture state.

Since the Terminal module does not open the capture file,
you’d normally close it yourself using FSCloseFork() after
this routine returns.  HOWEVER, if you set the auto-close
flag in Terminal_FileCaptureBegin(), then FSCloseFork()
WILL be called for you, and you lose ownership of the file.

(3.0)
*/
void
Terminal_FileCaptureEnd		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeFileCaptureEnding, inRef);
		StreamCapture_End(dataPtr->captureStream);
	}
}// FileCaptureEnd


/*!
Returns "true" only if there is a file capture in
progress for the specified screen.

(2.6)
*/
Boolean
Terminal_FileCaptureInProgress	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = StreamCapture_InProgress(dataPtr->captureStream);
	}
	return result;
}// FileCaptureInProgress


/*!
Iterates over the specified row of the given terminal screen,
invoking a block on each chunk of text for which the associated
attributes are all IDENTICAL.  The upper bound on the number of
iterations is the number of cells in the row (reached only if
every single cell has different attributes than its neighbor).

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultParameterError
if the screen run operation function is invalid

\retval kTerminal_ResultNotEnoughMemory
if any line buffers are unexpectedly empty

(2017.12)
*/
Terminal_Result
Terminal_ForEachLikeAttributeRun	(TerminalScreenRef			inRef,
									 Terminal_LineRef			inStartRow,
									 Terminal_ScreenRunBlock	inDoWhat)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		screenPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if ((nullptr == inDoWhat) || (nullptr == screenPtr) || (nullptr == iteratorPtr))
	{
		result = kTerminal_ResultParameterError;
	}
	else
	{
		// need to search line for style chunks
		My_ScreenBufferLine&						currentLine = iteratorPtr->currentLine();
		TerminalLine_TextIterator					textIterator = nullptr;
		TerminalLine_TextAttributesList const&		currentAttributeVector = currentLine.returnAttributeVector();
		auto										attrIterator = currentAttributeVector.begin();
		TextAttributes_Object						previousAttributes;
		TextAttributes_Object						currentAttributes;
		SInt16										runStartCharacterIndex = 0;
		SInt16										characterIndex = 0;
		size_t										styleRunLength = 0;
		
		
	#if 0
		// DEBUGGING ONLY: if you suspect a bug in the incremental loop below,
		// try asking the entire line to be drawn without formatting, first
		inDoWhat(currentLine.textVectorSize/* length */,
					currentLine.textCFString.returnCFStringRef(),
					inStartRow,
					0/* zero-based start column */,
					currentLine.returnGlobalAttributes());
	#endif
		
		// TEMPORARY - HIGHLY inefficient to search here, need to change this into a cache
		//             (in fact, attribute bit arrays can probably be completely replaced by
		//             style runs at some point in the future)
		assert(nullptr != currentLine.textVectorBegin);
		for (textIterator = currentLine.textVectorBegin,
					attrIterator = currentAttributeVector.begin();
				(textIterator != currentLine.textVectorEnd) &&
					(attrIterator != currentAttributeVector.end());
				++textIterator, ++attrIterator, ++characterIndex)
		{
			currentAttributes = *attrIterator;
			if ((currentAttributes != previousAttributes) ||
				(characterIndex == STATIC_CAST(currentLine.textVectorSize - 1, SInt16)) ||
				(characterIndex == STATIC_CAST(currentAttributeVector.size() - 1, SInt16)))
			{
				styleRunLength = characterIndex - runStartCharacterIndex;
				
				// found new style run; so handle the previous run
				if (styleRunLength > 0)
				{
					TextAttributes_Object		rangeAttributes = previousAttributes;
					NSRange						runRange = NSMakeRange(runStartCharacterIndex, styleRunLength);
					CFStringRef					lineAsCFString = currentLine.textCFString.returnCFStringRef();
					NSString*					lineAsNSString = BRIDGE_CAST(lineAsCFString, NSString*);
					NSString*					styleRunSubstring = [lineAsNSString substringWithRange:runRange];
					
					
					rangeAttributes.addAttributes(currentLine.returnGlobalAttributes());
					inDoWhat(STATIC_CAST(styleRunLength, UInt16)/* length */,
								BRIDGE_CAST(styleRunSubstring, CFStringRef),
								inStartRow,
								runStartCharacterIndex/* zero-based start column */,
								rangeAttributes);
				}
				
				// reset for next run
				previousAttributes = currentAttributes;
				runStartCharacterIndex = characterIndex;
			}
		}
		
		// ask that the remainder of the line be handled as if it were blank
		if (textIterator != currentLine.textVectorEnd)
		{
			styleRunLength = std::distance(textIterator, currentLine.textVectorEnd);
			
			// found new style run; so handle the previous run
			if (styleRunLength > 0)
			{
				TextAttributes_Object		attributesForRemainder = currentLine.returnGlobalAttributes();
				
				
				// the “selected” attribute is special; it persists regardless
				if (previousAttributes.hasAttributes(kTextAttributes_Selected))
				{
					attributesForRemainder.addAttributes(kTextAttributes_Selected);
				}
				
				inDoWhat(STATIC_CAST(styleRunLength, UInt16)/* length */,
							nullptr/* text buffer, for non-blank space */,
							inStartRow,
							runStartCharacterIndex/* zero-based start column */,
							attributesForRemainder);
			}
		}
	}
	return result;
}// ForEachLikeAttributeRun


/*!
Returns ONLY the attributes that were assigned to the
specified line in a global manner.  For example, double-sized
text is applied to an entire line, never to a single column.

IMPORTANT:	To properly render a line, its global attributes
			must be mixed with the attributes of the current
			cursor location.

(3.0)
*/
Terminal_Result
Terminal_GetLineGlobalAttributes	(TerminalScreenRef			UNUSED_ARGUMENT(inScreen),
									 Terminal_LineRef			inRow,
									 TextAttributes_Object*		outAttributesPtr)
{
	Terminal_Result			result = kTerminal_ResultOK;
	//My_ScreenBufferConstPtr	dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if ((iteratorPtr == nullptr) || (outAttributesPtr == nullptr)) result = kTerminal_ResultParameterError;
	else
	{
		*outAttributesPtr = iteratorPtr->currentLine().returnGlobalAttributes();
	}
	
	return result;
}// GetLineGlobalAttributes


/*!
Like Terminal_GetLineRange(), but automatically pulls in the
entire line (from the first column to past the end column).

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.1)
*/
Terminal_Result
Terminal_GetLine	(TerminalScreenRef			inScreen,
					 Terminal_LineRef			inRow,
					 UniChar const*&			outPossibleReferenceStart,
					 UniChar const*&			outPossibleReferencePastEnd,
					 Terminal_TextFilterFlags	inFlags)
{
	return Terminal_GetLineRange(inScreen, inRow, 0/* first column */, -1/* last column; negative means “very end” */,
									outPossibleReferenceStart, outPossibleReferencePastEnd, inFlags);
}// GetLine


/*!
Allows read-only access to a single line of text - everything
from the specified start column, inclusive, of the given row to
the specified end column, exclusive.

Pass -1 for the end column to conveniently refer to the end of
the line.  Otherwise, pass a nonnegative number to index a
column, where 0 is the first column.

The pointers must be immediately used to access data; they could
become invalid (for instance, if the line is about to be
scrolled into oblivion from the oldest part of the scrollback
buffer).

As their names imply, the range parameters are inclusive at the
beginning and exclusive at the end, such that the pointer
difference is zero if the range is empty.  (Like the STL.)

NOTE:	This API is somewhat implementation dependent.  So this
		API could change in the future, and any code that calls
		it would have to change too.

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultParameterError
if the specified column is out of range and nonnegative

(3.1)
*/
Terminal_Result
Terminal_GetLineRange	(TerminalScreenRef			inScreen,
						 Terminal_LineRef			inRow,
						 UInt16						inZeroBasedStartColumn,
						 SInt16						inZeroBasedPastEndColumnOrNegativeForLastColumn,
						 UniChar const*&			outReferenceStart,
						 UniChar const*&			outReferencePastEnd,
						 Terminal_TextFilterFlags	inFlags)
{
	Terminal_Result			result = kTerminal_ResultParameterError;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	outReferenceStart = nullptr;
	outReferencePastEnd = nullptr;
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == iteratorPtr) result = kTerminal_ResultInvalidIterator;
	else
	{
		UInt16 const	kPastEndColumn = (inZeroBasedPastEndColumnOrNegativeForLastColumn < 0)
											? dataPtr->text.visibleScreen.numberOfColumnsPermitted
											: inZeroBasedPastEndColumnOrNegativeForLastColumn;
		
		
		if (kPastEndColumn > iteratorPtr->currentLine().textVectorSize)
		{
			result = kTerminal_ResultParameterError;
		}
		else
		{
			outReferenceStart = iteratorPtr->currentLine().textVectorBegin + inZeroBasedStartColumn;
			outReferencePastEnd = iteratorPtr->currentLine().textVectorBegin + kPastEndColumn;
			if (inFlags & kTerminal_TextFilterFlagsNoEndWhitespace)
			{
				UniChar const*		lastCharPtr = outReferencePastEnd - 1;
				
				
				// LOCALIZE THIS
				while ((lastCharPtr != outReferenceStart) && std::isspace(*lastCharPtr))
				{
					--lastCharPtr;
				}
				outReferencePastEnd = lastCharPtr + 1;
			}
			result = kTerminal_ResultOK;
		}
	}
	
	return result;
}// GetLineRange


/*!
Returns true only if the most recent check of the raw
terminal device showed that it was not echoing (e.g.
to display a password prompt).

This is currently checked every time the cursor moves
to a different line.

(4.1)
*/
Boolean
Terminal_IsInPasswordMode	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->passwordMode;
	}
	return result;
}// IsInPasswordMode


/*!
Returns "true" only if the specified terminal screen has
not been destroyed with Terminal_ReleaseScreen(), and is
not in the process of being destroyed.

Most of the time, checking for a null reference is enough,
and efficient; this check may be slower, but is important
if you are handling something indirectly or asynchronously
(where a terminal could have been destroyed at any time).

(4.1)
*/
Boolean
Terminal_IsValid        (TerminalScreenRef      inRef)
{
	Boolean		result = ((nullptr != inRef) && (gTerminalScreenValidRefs().find(inRef) != gTerminalScreenValidRefs().end()));
	
	
	return result;
}// IsValid


/*!
Returns "true" only if the LED with the specified
number is currently on.  The meaning of an LED with
a specific number is left to the caller.

Currently, only 4 LEDs are defined.  Providing an
LED number of 5 or greater will result in a "false"
return value.

Numbers less than zero are reserved.  Probably, they
will one day be used to determine if specific LEDs
are OFF.

(3.0)
*/
Boolean
Terminal_LEDIsOn	(TerminalScreenRef	inRef,
					 SInt16				inOneBasedLEDNumber)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		switch (inOneBasedLEDNumber)
		{
		case 1:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight1));
			break;
		
		case 2:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight2));
			break;
		
		case 3:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight3));
			break;
		
		case 4:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight4));
			break;
		
		case 0:
		default:
			// ???
			break;
		}
	}
	return result;
}// LEDIsOn


/*!
Sets an LED to on or off.  Values less than or equal to
zero are reserved.

Note that LEDs are ordinarily entirely under the control
of the terminal, and the terminal could later change the
LED state anyway.

(3.1)
*/
void
Terminal_LEDSetState	(TerminalScreenRef	inRef,
						 SInt16				inOneBasedLEDNumber,
						 Boolean			inIsHighlighted)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		switch (inOneBasedLEDNumber)
		{
		case 1:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight1;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight1;
			break;
		
		case 2:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight2;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight2;
			break;
		
		case 3:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight3;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight3;
			break;
		
		case 4:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight4;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight4;
			break;
		
		case 0:
		default:
			// ???
			break;
		}
		
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeNewLEDState, dataPtr->selfRef/* context */);
	}
}// LEDSetState


/*!
Changes an iterator to point to a different line, one that is
the specified number of rows later than or earlier than the
row that the iterator currently points to.  Pass a value less
than zero to find a previous row, otherwise positive values
find following rows.

\retval kTerminal_ResultOK
if the iterator is advanced or backed-up successfully

\retval kTerminal_ResultParameterError
if the iterator is completely invalid

\retval kTerminal_ResultIteratorCannotAdvance
if the iterator cannot move in the requested direction (already
at oldest scrollback or screen line, or already at bottommost
screen line)

(3.0)
*/
Terminal_Result
Terminal_LineIteratorAdvance	(TerminalScreenRef		UNUSED_ARGUMENT(inRef),
								 Terminal_LineRef		inRow,
								 SInt16					inHowManyRowsForwardOrNegativeForBackward)
{
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (iteratorPtr == nullptr) result = kTerminal_ResultParameterError;
	else
	{
		Boolean		isEnd = false;
		
		
		if (inHowManyRowsForwardOrNegativeForBackward > 0)
		{
			SInt16		i = 0;
			
			
			for (i = 0; i < inHowManyRowsForwardOrNegativeForBackward; ++i)
			{
				iteratorPtr->goToNextLine(isEnd);
				if (isEnd) break;
			}
			if (inHowManyRowsForwardOrNegativeForBackward != i)
			{
				result = kTerminal_ResultIteratorCannotAdvance;
			}
		}
		else
		{
			SInt16		i = 0;
			
			
			for (i = inHowManyRowsForwardOrNegativeForBackward; i < 0; ++i)
			{
				iteratorPtr->goToPreviousLine(isEnd);
				if (isEnd) break;
			}
			if (0 != i)
			{
				result = kTerminal_ResultIteratorCannotAdvance;
			}
		}
	}
	return result;
}// LineIteratorAdvance


/*!
Returns "true" only if the given terminal has mapped Return
key presses and line feeds to the “carriage return, line feed”
behavior.  If "false", a Return sends only a carriage return
and a line feed only moves the cursor vertically.

Valid in VT terminals; may not be defined for others.

(3.1)
*/
Boolean
Terminal_LineFeedNewLineMode	(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->modeNewLineOption;
	}
	return result;
}// LineFeedNewLineMode


/*!
Returns "true" only if the given terminal automatically moves
the cursor to the beginning of the next line (and inserts
text there) when an attempt to write past the limit of the
current line is made.

(3.0)
*/
Boolean
Terminal_LineWrapIsEnabled		(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->modeAutoWrap;
	}
	return result;
}// LineWrapIsEnabled


/*!
Resets the indicated terminal settings to their default
states.  Pass "kTerminal_ResetFlagsAll" to do a standard
terminal reset, which among other things clears the screen
and homes the cursor.

(3.0)
*/
void
Terminal_Reset		(TerminalScreenRef		inRef,
					 Terminal_ResetFlags	inFlags)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		// TEMPORARY - currently, this is not broken down into parts,
		//             but if more flag bits are defined in the future
		//             this should probably do exactly what is requested
		//             and nothing more
		if (inFlags == kTerminal_ResetFlagsAll)
		{
			setCursorVisible(dataPtr, false);
			resetTerminal(dataPtr); // homes cursor, among other things
			setCursorVisible(dataPtr, true);
			// ensure cursor is in visible screen area in all views - UNIMPLEMENTED
		}
	}
}// Reset


/*!
Returns the maximum number of columns allowed
(useful in order to limit a text field in a
dialog box, for example).

(3.0)
*/
UInt16
Terminal_ReturnAllocatedColumnCount ()
{
	return kTerminalLine_MaximumCharacterCount;
}// ReturnAllocatedColumnCount


/*!
Returns the number of characters wide the specified
terminal screen is (regardless of however many may
be visible in the window that contains it).

(3.0)
*/
UInt16
Terminal_ReturnColumnCount		(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
	}
	return result;
}// ReturnColumnCount


/*!
Returns a variety of preferences unique to this screen.

You can make changes to this context ONLY if you do it in “batch
mode” with Preferences_ContextCopy().  In other words, even to
make a single change, you must first add the change to a new
temporary context, then use Preferences_ContextCopy() to read
the temporary settings into the context returned by this routine.
Batch mode changes are detected by the Terminal Screen and used
to automatically update the emulator and internal caches.

Note that you cannot expect all possible tags to be present;
be prepared to not find what you look for.  In addition, tags
that are present in one screen may be absent in another.

(3.1)
*/
Preferences_ContextRef
Terminal_ReturnConfiguration	(TerminalScreenRef		inRef)
{
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef		result = dataPtr->configuration.returnRef();
	
	
	// since many settings are represented internally, this context
	// will not contain the latest information; update the context
	// based on current settings
	
	// INCOMPLETE
	
	{
		UInt16		dimension = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalScreenColumns,
													sizeof(dimension), &dimension);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		UInt16		dimension = STATIC_CAST(dataPtr->screenBuffer.size(), UInt16);
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalScreenRows,
													sizeof(dimension), &dimension);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		UInt32		dimension = STATIC_CAST(dataPtr->text.scrollback.numberOfRowsPermitted, UInt32);
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalScreenScrollbackRows,
													sizeof(dimension), &dimension);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	return result;
}// ReturnConfiguration


/*!
Returns the number of saved lines that have scrolled
off the top of the screen.

(3.0)
*/
UInt32
Terminal_ReturnInvisibleRowCount	(TerminalScreenRef		inRef)
{
	UInt32						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = STATIC_CAST(dataPtr->scrollbackBufferCachedSize, UInt32);
	}
	return result;
}// ReturnInvisibleRowCount


/*!
Returns the number of lines long the specified
terminal screen’s main screen area is (minus any
scrollback lines).

(3.0)
*/
UInt16
Terminal_ReturnRowCount		(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = STATIC_CAST(dataPtr->screenBuffer.size(), UInt16);
	}
	return result;
}// ReturnRowCount


/*!
Returns the Terminal Speaker object that handles
audio for the given terminal.

IMPORTANT:	This API is under evaluation.  Returning
			a delegate object is usually a sign of
			design weakness, maybe what should happen
			is the speaker should be controlled by a
			larger entity such as Terminal Window.
			Hmmm...

(3.0)
*/
TerminalSpeaker_Ref
Terminal_ReturnSpeaker		(TerminalScreenRef	inRef)
{
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	TerminalSpeaker_Ref			result = nullptr;
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->speaker;
	}
	return result;
}// ReturnSpeaker


/*!
Returns the encoding of text streams read by the terminal
emulator.  This does *not* indicate the internal buffer
encoding, which might be Unicode regardless.

Returns "kCFStringEncodingInvalidId" if for any reason the
encoding cannot be found.

(4.0)
*/
CFStringEncoding
Terminal_ReturnTextEncoding		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	CFStringEncoding	result = kCFStringEncodingInvalidId;
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->emulator.inputTextEncoding;
	}
	return result;
}// ReturnTextEncoding


/*!
Returns "true" only if the specified screen should
be rendered in reverse video mode - that is, with
the foreground and background colors swapped prior
to rendering.

(3.0)
*/
Boolean
Terminal_ReverseVideoIsEnabled	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->reverseVideo;
	}
	return result;
}// ReverseVideoIsEnabled


/*!
Returns "true" only if the lines of the terminal
screen are scrolled prior to a clearing of the
visible screen area.

(3.0)
*/
Boolean
Terminal_SaveLinesOnClearIsEnabled		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->saveToScrollbackOnClear;
	}
	return result;
}// SaveLinesOnClearIsEnabled


/*!
Searches the specified terminal screen buffer using the given
query and flags as a guide, and returns zero or more matches.
All matching ranges are returned.

Multiple threads may be spawned to perform searches of the
terminal buffer but they will all terminate before this routine
returns.

This is not guaranteed to be perfectly efficient; in particular,
currently the internal implementation needs to copy data in order
to pass it to the scanner.  In the future, if the internal
storage format is made to match that of the search engine, the
search may be faster.

\retval kTerminal_ResultOK
if no error occurs

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the query string is invalid or an unrecognized flag is given

\retval kTerminal_ResultNotEnoughMemory
if the buffer is too large to search

(3.1)
*/
Terminal_Result
Terminal_Search		(TerminalScreenRef							inRef,
					 CFStringRef								inQuery,
					 Terminal_SearchFlags						inFlags,
					 std::vector< Terminal_RangeDescription >&	outMatches)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == inQuery) result = kTerminal_ResultParameterError;
	else if (kTerminal_ResultOK == result)
	{
		typedef std::vector< Terminal_RangeDescription >	RangeDescriptionVector;
		CFStringRef					actualQuery = inQuery; // usually the same but could change below
		CFOptionFlags				searchFlags = 0;
		NSRegularExpressionOptions	regExOptions = 0;
		Boolean						isRegEx = (0 != (inFlags & kTerminal_SearchFlagsRegularExpression));
		pthread_attr_t				threadAttributes;
		pthread_t					threadList[] =
									{
										// size according to maximum number of scrollback-search threads
										nullptr,
										nullptr,
										nullptr,
										nullptr,
										nullptr
									};
		size_t const				kMaxThreads = sizeof(threadList) / sizeof(pthread_t);
		RangeDescriptionVector*		matchVectorsList[] =
									{
										// one per thread, same size as "threadList"
										nullptr,
										nullptr,
										nullptr,
										nullptr,
										nullptr
									};
		int							threadResult = 0;
		Boolean						threadOK = true;
		
		
		// translate given flags to Core Foundation String search flags
		if (0 == (inFlags & kTerminal_SearchFlagsCaseSensitive))
		{
			searchFlags |= kCFCompareCaseInsensitive;
			regExOptions |= NSRegularExpressionCaseInsensitive;
		}
		if (inFlags & kTerminal_SearchFlagsSearchBackwards)
		{
			searchFlags |= kCFCompareBackwards;
		}
		if (inFlags & kTerminal_SearchFlagsMatchOnlyAtLineEnd)
		{
			NSCharacterSet*		newlineSet = [NSCharacterSet characterSetWithCharactersInString:@"\n\r\0"];
			NSString*			asNSString = BRIDGE_CAST(actualQuery, NSString*);
			
			
			searchFlags |= (kCFCompareAnchored | kCFCompareBackwards);
			
			// also strip any new-lines that may be at the end of the query string
			// (technically this call strips the characters from ANYWHERE in the
			// query but realistically they would only appear at the end and this
			// is simpler than manually creating a copy of the string and stripping
			// only ending characters that match)
			actualQuery = BRIDGE_CAST([asNSString stringByTrimmingCharactersInSet:newlineSet], CFStringRef);
			//NSLog(@"actual query changed to “%@”", (NSString*)actualQuery); // debug
		}
		
		threadResult = pthread_attr_init(&threadAttributes);
		if (-1 == threadResult)
		{
			Console_Warning(Console_WriteValue, "failed to initialize thread attributes, errno", errno);
			threadOK = false;
		}
		else
		{
			threadResult = pthread_attr_setdetachstate(&threadAttributes, PTHREAD_CREATE_JOINABLE);
			if (-1 == threadResult)
			{
				Console_Warning(Console_WriteValue, "failed to mark thread as 'joinable', errno", errno);
				threadOK = false;
			}
			threadResult = pthread_attr_setstacksize(&threadAttributes, 1024/* arbitrary but needs to suffice! */);
			if (-1 == threadResult)
			{
				Console_Warning(Console_WriteValue, "failed to set thread stack size, errno", errno);
				threadOK = false;
			}
		}
		
		// threads are spawned to perform search in parallel; the first thread
		// searches the main screen, and all subsequent threads search every
		// Nth line of the scrollback buffer (e.g. if there are 3 scrollback
		// threads, the first searches scrollback line 0, 3, 6, ..., the 2nd
		// searches line 1, 4, 7, and so on); in this way it is relatively
		// easy to experiment with additional threads to see where the best
		// performance lies, as the search results should always be OK
		if (threadOK)
		{
			My_SearchThreadContextPtr	threadContextPtr = nullptr;
			
			
			matchVectorsList[0] = new RangeDescriptionVector();
			
			threadContextPtr = new My_SearchThreadContext; // contains C++ classes, must use "new"
			threadContextPtr->screenBufferPtr = dataPtr;
			threadContextPtr->matchesVectorPtr = matchVectorsList[0];
			threadContextPtr->queryCFString = actualQuery;
			threadContextPtr->searchFlags = searchFlags;
			threadContextPtr->regExOptions = regExOptions;
			threadContextPtr->isRegularExpression = isRegEx;
			threadContextPtr->threadNumber = 0;
			threadContextPtr->rangeStart = dataPtr->screenBuffer.begin();
			threadContextPtr->startRowIndex = 0;
			threadContextPtr->rowCount = STATIC_CAST(dataPtr->screenBuffer.size(), UInt32);
			
			threadResult = pthread_create(&threadList[0], &threadAttributes,
											threadForTerminalSearch, threadContextPtr);
			if (-1 == threadResult)
			{
				Console_Warning(Console_WriteValue, "failed to create thread 0, errno", errno);
				threadOK = false;
			}
		}
		if (false == dataPtr->scrollbackBuffer.empty())
		{
			size_t const	kScrollbackSize = dataPtr->scrollbackBufferCachedSize;
			UInt16			scrollbackThreadCount = 1;
			
			
			// set arbitrary thread allocations based on how big the search space is
			if (kScrollbackSize > 10000/* arbitrary */)
			{
				++scrollbackThreadCount;
			}
			if (kScrollbackSize > 30000/* arbitrary */)
			{
				++scrollbackThreadCount;
			}
			if (kScrollbackSize > 50000/* arbitrary */)
			{
				++scrollbackThreadCount;
			}
			
			if (scrollbackThreadCount >= kMaxThreads)
			{
				scrollbackThreadCount = kMaxThreads - 1;
			}
			UInt32		averageLinesPerThread = STATIC_CAST(kScrollbackSize, UInt32) / scrollbackThreadCount;
			for (UInt16 i = 1; i <= scrollbackThreadCount; ++i)
			{
				if (threadOK)
				{
					My_SearchThreadContextPtr	threadContextPtr = nullptr;
					
					
					matchVectorsList[i] = new RangeDescriptionVector();
					
					threadContextPtr = new My_SearchThreadContext; // contains C++ classes, must use "new"
					threadContextPtr->screenBufferPtr = dataPtr;
					threadContextPtr->matchesVectorPtr = matchVectorsList[i];
					threadContextPtr->queryCFString = actualQuery;
					threadContextPtr->searchFlags = searchFlags;
					threadContextPtr->regExOptions = regExOptions;
					threadContextPtr->isRegularExpression = isRegEx;
					threadContextPtr->threadNumber = i;
					threadContextPtr->rangeStart = dataPtr->scrollbackBuffer.begin();
					threadContextPtr->startRowIndex = (i - 1) * averageLinesPerThread;
					if (i > 1)
					{
						std::advance(threadContextPtr->rangeStart, threadContextPtr->startRowIndex);
					}
					threadContextPtr->rowCount = averageLinesPerThread;
					if (scrollbackThreadCount == i)
					{
						// since the scrollback size is unlikely to divide perfectly between
						// threads, the last thread picks up the slack
						threadContextPtr->rowCount = STATIC_CAST(kScrollbackSize, UInt32) - threadContextPtr->startRowIndex;
					}
					
					threadResult = pthread_create(&threadList[i], &threadAttributes,
													threadForTerminalSearch, threadContextPtr);
					if (-1 == threadResult)
					{
						Console_Warning(Console_WriteValue, "failed to create thread, errno", errno);
						threadOK = false;
					}
				}
			}
		}
		
		// wait for all threads to complete
		for (UInt16 i = 0; i < kMaxThreads; ++i)
		{
			if (nullptr == threadList[i])
			{
				break;
			}
			UNUSED_RETURN(int)pthread_join(threadList[i], nullptr);
		}
		
		// process search results
		if (false == threadOK)
		{
			result = kTerminal_ResultNotEnoughMemory;
		}
		else
		{
			result = kTerminal_ResultOK;
			
			// since all threads had their own vectors to avoid locking delays,
			// it is now necessary to combine each thread’s results into one list
			// (in the future it might make sense to expose this fact to the
			// caller just to avoid pointless copies; the caller will most likely
			// just iterate over it all anyway)
			size_t	totalSize = 0;
			for (UInt16 i = 0; i < kMaxThreads; ++i)
			{
				if (nullptr == matchVectorsList[i])
				{
					break;
				}
				totalSize += matchVectorsList[i]->size();
			}
			outMatches.reserve(totalSize);
			for (UInt16 i = 0; i < kMaxThreads; ++i)
			{
				if (nullptr == matchVectorsList[i])
				{
					break;
				}
				outMatches.insert(outMatches.end(), matchVectorsList[i]->begin(), matchVectorsList[i]->end());
			}
		}
		
		// free space
		for (UInt16 i = 0; i < kMaxThreads; ++i)
		{
			if (nullptr != matchVectorsList[i])
			{
				delete matchVectorsList[i], matchVectorsList[i] = nullptr;
			}
		}
	}
	
	return result;
}// Search


/*!
Specifies whether the given terminal’s bell is active.
An inactive bell completely ignores all bell signals -
without giving any audible or visible indication that
a bell has occurred.  This can be useful if you know
you’ve just triggered a long string of bells and don’t
want to be annoyed by a series of beeps or flashes.

(3.0)
*/
void
Terminal_SetBellEnabled		(TerminalScreenRef	inRef,
							 Boolean			inIsEnabled)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		dataPtr->bellDisabled = !inIsEnabled;
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeAudioState, dataPtr->selfRef/* context */);
	}
}// SetBellEnabled


/*!
Sets the string that will be printed by a dumb terminal
(kEmulation_FullTypeDumb) when the specified character is
to be displayed.  The description must be in UTF-8 encoding.

Normally, any character that is considered “printable”
should be echoed as-is, so this is the default behavior
if no mapping has been given for a printable character.

(3.1)
*/
void
Terminal_SetDumbTerminalRendering	(UniChar		inCharacter,
									 char const*	inDescription)
{
	CFRetainRelease		descriptionCFString(CFStringCreateWithCString
											(kCFAllocatorDefault, inDescription, kCFStringEncodingUTF8),
											CFRetainRelease::kAlreadyRetained);
	
	
	if (false == descriptionCFString.exists())
	{
		Console_Warning(Console_WriteLine, "unexpected error creating UTF-8 string for description");
	}
	else
	{
		gDumbTerminalRenderings()[inCharacter].setWithRetain(descriptionCFString.returnCFStringRef());
	}
}// SetDumbTerminalRendering


/*!
Specifies whether the given terminal automatically moves
the cursor to the beginning of the next line (and inserts
text there) when an attempt to write past the limit of the
current line is made.

(3.0)
*/
void
Terminal_SetLineWrapEnabled		(TerminalScreenRef	inRef,
								 Boolean			inIsEnabled)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		dataPtr->modeAutoWrap = inIsEnabled;
	}
}// SetLineWrapEnabled


/*!
Attaches a session to this terminal, so that certain
features (such as VT100 Device Attributes) can return
their report data somewhere.  If you want to disable
this, pass a nullptr session.

(3.1)
*/
Terminal_Result
Terminal_SetListeningSession	(TerminalScreenRef	inRef,
								 SessionRef			inSession)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else
	{
		dataPtr->listeningSession = inSession;
	}
	
	return result;
}// SetListeningSession


/*!
Specifies whether or not the lines of the terminal
screen are scrolled prior to a clearing of the
visible screen area.

(2.6)
*/
void
Terminal_SetSaveLinesOnClear	(TerminalScreenRef	inRef,
								 Boolean			inClearScreenSavesLines)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) dataPtr->saveToScrollbackOnClear = inClearScreenSavesLines;
}// SetSaveLinesOnClear


/*!
Specifies whether the given terminal’s text may be spoken
by the computer.  If you disable speech, any remaining text
in the buffer will continue to be spoken; you may want to
invoke Terminal_SpeechPause() ahead of time to force speech
to end immediately, as well.

(3.0)
*/
void
Terminal_SetSpeechEnabled	(TerminalScreenRef	inRef,
							 Boolean			inIsEnabled)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		// TEMPORARY; other speech modes are not implemented yet
		dataPtr->speech.mode = (inIsEnabled) ? kTerminal_SpeechModeSpeakAlways : kTerminal_SpeechModeSpeakNever;
	}
}// SetSpeechEnabled


/*!
Specifies the encoding of text streams read by the terminal
emulator.  This does *not* indicate the internal buffer
encoding, which might be Unicode regardless.

WARNING:	This should only be called by the Session module.
			Use Session APIs to change encoding information,
			because other things may need to be in sync with
			that change.

\retval kTerminal_ResultOK
if the encoding is set successfully

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(4.0)
*/
Terminal_Result
Terminal_SetTextEncoding	(TerminalScreenRef		inRef,
							 CFStringEncoding		inNewEncoding)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Terminal_Result		result = kTerminal_ResultInvalidID;
	
	
	if (nullptr != dataPtr)
	{
		dataPtr->emulator.inputTextEncoding = inNewEncoding;
		if (kCFStringEncodingUTF8 == inNewEncoding)
		{
			// this mode is not normally set by the user, but the exception
			// is here, when the encoding is assigned; if the input is
			// explicitly UTF-8, it can be assumed that everything is UTF-8
			dataPtr->emulator.isUTF8Encoding = true;
		}
		else
		{
			dataPtr->emulator.isUTF8Encoding = false;
			dataPtr->emulator.lockUTF8 = false;
		}
		result = kTerminal_ResultOK;
	}
	return result;
}// SetTextEncoding


/*!
Calls both of the internal setVisibleColumnCount() and
setVisibleRowCount() routines, but generates only a single
notification to observers of screen size changes.

\retval kTerminal_ResultOK
if the terminal is resized without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the given number of columns or rows is too small or too large

\retval kTerminal_ResultNotEnoughMemory
not currently returned because this routine does no memory
reallocation; however a future implementation might decide to
reallocate, and if such reallocation fails, this error should be
returned

(4.0)
*/
Terminal_Result
Terminal_SetVisibleScreenDimensions		(TerminalScreenRef	inRef,
										 UInt16				inNewNumberOfCharactersWide,
										 UInt16				inNewNumberOfLinesHigh)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else
	{
		UNUSED_RETURN(Terminal_Result)setVisibleColumnCount(dataPtr, inNewNumberOfCharactersWide);
		result = setVisibleRowCount(dataPtr, inNewNumberOfLinesHigh);
		
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeScreenSize, dataPtr->selfRef/* context */);
	}
	return result;
}// SetVisibleScreenDimensions


/*!
Returns "true" only if speech is enabled for the specified
session.  Use the SpeechBusy() system call to determine if
the computer is actually speaking something at the moment.

(3.0)
*/
Boolean
Terminal_SpeechIsEnabled	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = (kTerminal_SpeechModeSpeakNever != dataPtr->speech.mode);
	}
	return result;
}// SpeechIsEnabled


/*!
Immediately interrupts any speaking the computer is doing
on behalf of the specified terminal, or does nothing if
no speech is in progress.

You can resume speaking with Terminal_SpeechResume().

(3.0)
*/
void
Terminal_SpeechPause	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		TerminalSpeaker_SetPaused(dataPtr->speaker, true);
	}
}// SpeechPause


/*!
Immediately interrupts any speaking the computer is doing
on behalf of the specified terminal, or does nothing if
no speech is in progress.

You can resume speaking with Terminal_SpeechResume().

(3.0)
*/
void
Terminal_SpeechResume	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		TerminalSpeaker_SetPaused(dataPtr->speaker, false);
	}
}// SpeechResume


/*!
Arranges for a callback to be invoked whenever a setting
changes for a terminal (such as the lit state of one or
more terminal LEDs).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "Terminal.h" for comments
			on what the context means for each type of
			change.

(3.0)
*/
void
Terminal_StartMonitoring	(TerminalScreenRef			inRef,
							 Terminal_Change			inForWhatChange,
							 ListenerModel_ListenerRef	inListener)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		Boolean		addOK = false;
		
		
		// add a listener to the specified target’s listener model for the given setting change
		addOK = ListenerModel_AddListenerForEvent(dataPtr->changeListenerModel, inForWhatChange, inListener);
		if (false == addOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to set up monitor for terminal setting", inForWhatChange);
		}
	}
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
a setting changes for a terminal (such as the lit state
of one or more terminal LEDs).

IMPORTANT:	This routine cancels the effects of a previous
			call to Terminal_StartMonitoring() - your
			parameters must match the previous start-call,
			or the stop will fail.

(3.0)
*/
void
Terminal_StopMonitoring		(TerminalScreenRef			inRef,
							 Terminal_Change			inForWhatChange,
							 ListenerModel_ListenerRef	inListener)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		// remove a listener from the specified target’s listener model for the given setting change
		ListenerModel_RemoveListenerForEvent(dataPtr->changeListenerModel, inForWhatChange, inListener);
	}
}// StopMonitoring


/*!
Returns true only if this type of terminal can be sent
function key sequences (VT220 or above, and XTerm).

This is a base property of the terminal; it does not
reflect other sources of settings, such as system
function key overrides.

(2020.04)
*/
Boolean
Terminal_SupportsFunctionKeys	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		auto const		emulationType = dataPtr->emulator.primaryType;
		auto const		baseType = (emulationType & kEmulation_BaseTypeMask);
		auto const		variantType = (emulationType & kEmulation_VariantMask);
		
		
		switch (baseType)
		{
		case kEmulation_BaseTypeVT:
			result = ((kEmulation_VariantVT100 != variantType) && (kEmulation_VariantVT102 != variantType));
			break;
		
		case kEmulation_BaseTypeXTerm:
			result = true;
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// SupportsFunctionKeys


/*!
Returns true only if this type of terminal can be sent
page-up, page-down, home or end sequences (VT102 or above,
and XTerm).

This is a base property of the terminal; it does not
consider other sources of settings, such as a user
preference for “local” page keys.

(2020.04)
*/
Boolean
Terminal_SupportsPageKeys	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		auto const		emulationType = dataPtr->emulator.primaryType;
		auto const		baseType = (emulationType & kEmulation_BaseTypeMask);
		auto const		variantType = (emulationType & kEmulation_VariantMask);
		
		
		switch (baseType)
		{
		case kEmulation_BaseTypeVT:
			result = (kEmulation_VariantVT100 != variantType);
			break;
		
		case kEmulation_BaseTypeXTerm:
			result = true;
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// SupportsPageKeys


/*!
Returns the red, green and blue intensity fractions for
the given “true” color, which is defined whenever a
24-bit color terminal sequence is received.

The TextAttributes_Object type can be used to extract the
TextAttributes_TrueColorID value for a section of text.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultParameterError
if the specified index is out of range

\retval kTerminal_ResultUnsupported
if the specified terminal was not configured for true-color values

(4.1)
*/
Terminal_Result
Terminal_TrueColorGetFromID		(TerminalScreenRef				inRef,
								 TextAttributes_TrueColorID		inIndex,
								 CGFloat&						outRedComponentFraction,
								 CGFloat&						outGreenComponentFraction,
								 CGFloat&						outBlueComponentFraction)
{
	Terminal_Result		result = kTerminal_ResultOK;
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr == dataPtr)
	{
		result = kTerminal_ResultInvalidID;
	}
	else
	{
		auto	redVectorPtr = dataPtr->emulator.trueColorTableReds;
		auto	greenVectorPtr = dataPtr->emulator.trueColorTableGreens;
		auto	blueVectorPtr = dataPtr->emulator.trueColorTableBlues;
		
		
		if ((nullptr == redVectorPtr) || (nullptr == greenVectorPtr) || (nullptr == blueVectorPtr))
		{
			result = kTerminal_ResultUnsupported;
		}
		else if ((inIndex >= (*redVectorPtr).size()) ||
					(inIndex >= (*greenVectorPtr).size()) ||
					(inIndex >= (*blueVectorPtr).size()))
		{
			result = kTerminal_ResultParameterError;
		}
		else
		{
			// convert the RGB components (each from 0 to 255) into fractions of 1.0
			outRedComponentFraction = STATIC_CAST((*redVectorPtr)[inIndex], Float32) / 255.0f;
			outGreenComponentFraction = STATIC_CAST((*greenVectorPtr)[inIndex], Float32) / 255.0f;
			outBlueComponentFraction = STATIC_CAST((*blueVectorPtr)[inIndex], Float32) / 255.0f;
		}
	}
	
	return result;
}// TrueColorGetFromID


/*!
Attempts to reposition the cursor by sending enough arrow key
sequences in the specified directions.  (A delta of zero in
either direction means the cursor does not move on that axis.)

This could fail, if for instance there is no session listening
to input for this terminal.

IMPORTANT:	This could fail in ways that cannot be detected by
			this function; for instance, if the user is currently
			running a process that does not interpret arrow keys
			properly.

IMPORTANT:	User input routines at the terminal level are rare,
			and generally only used to handle sequences that
			depend very directly on terminal state.  Look in the
			Session module for preferred user input routines.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultNoListeningSession
if keys cannot be sent because no session is listening

(3.1)
*/
Terminal_Result
Terminal_UserInputOffsetCursor	(TerminalScreenRef		inRef,
								 SInt16					inColumnDelta,
								 SInt16					inRowDelta)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == dataPtr->listeningSession) result = kTerminal_ResultNoListeningSession;
	else
	{
		Session_Result		keyResult = kSession_ResultOK;
		
		
		// horizontal offset
		if (inColumnDelta < 0)
		{
			for (SInt16 i = 0; i > inColumnDelta; --i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSLT);
			}
		}
		else
		{
			for (SInt16 i = 0; i < inColumnDelta; ++i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSRT);
			}
		}
		
		// vertical offset
		if (inRowDelta < 0)
		{
			for (SInt16 i = 0; i > inRowDelta; --i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSUP);
			}
		}
		else
		{
			for (SInt16 i = 0; i < inRowDelta; ++i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSDN);
			}
		}
	}
	return result;
}// UserInputOffsetCursor


/*!
Sends a function key sequence.  All possible key codes are
defined in the "VTKeys.h" header.

User input of any kind should take local echoing behavior into
account, but note that this routine does not do echoing.

This could fail, if for instance there is no session listening
to input for this terminal.

IMPORTANT:	User input routines at the terminal level are rare,
			and generally only used to handle sequences that
			depend very directly on terminal state.  Look in the
			Session module for preferred user input routines.

See also Terminal_UserInputVTKey().

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference or key code are invalid

\retval kTerminal_ResultNoListeningSession
if keys cannot be sent because no session is listening

(4.0)
*/
Terminal_Result
Terminal_UserInputVTFunctionKey		(TerminalScreenRef		inRef,
									 VTKeys_FKey			inFunctionKey)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == dataPtr->listeningSession) result = kTerminal_ResultNoListeningSession;
	else
	{
		char const*		seqPtr = nullptr;
		
		
		// now the fun part...determine which strange sequence corresponds
		// to the requested function key and layout
		switch (inFunctionKey)
		{
		case kVTKeys_FKeyVT100PF1:
			seqPtr = "\033OP";
			break;
		
		case kVTKeys_FKeyVT100PF2:
			seqPtr = "\033OQ";
			break;
		
		case kVTKeys_FKeyVT100PF3:
			seqPtr = "\033OR";
			break;
		
		case kVTKeys_FKeyVT100PF4:
			seqPtr = "\033OS";
			break;
		
		case kVTKeys_FKeyVT220F6:
			seqPtr = "\033[17~";
			break;
		
		case kVTKeys_FKeyVT220F7:
			seqPtr = "\033[18~";
			break;
		
		case kVTKeys_FKeyVT220F8:
			seqPtr = "\033[19~";
			break;
		
		case kVTKeys_FKeyVT220F9:
			seqPtr = "\033[20~";
			break;
		
		case kVTKeys_FKeyVT220F10:
			seqPtr = "\033[21~";
			break;
		
		case kVTKeys_FKeyVT220F11:
			seqPtr = "\033[23~";
			break;
		
		case kVTKeys_FKeyVT220F12:
			seqPtr = "\033[24~";
			break;
		
		case kVTKeys_FKeyVT220F13:
			seqPtr = "\033[25~";
			break;
		
		case kVTKeys_FKeyVT220F14:
			seqPtr = "\033[26~";
			break;
		
		case kVTKeys_FKeyVT220F15:
			seqPtr = "\033[28~";
			break;
		
		case kVTKeys_FKeyVT220F16:
			seqPtr = "\033[29~";
			break;
		
		case kVTKeys_FKeyVT220F17:
			seqPtr = "\033[31~";
			break;
		
		case kVTKeys_FKeyVT220F18:
			seqPtr = "\033[32~";
			break;
		
		case kVTKeys_FKeyVT220F19:
			seqPtr = "\033[33~";
			break;
		
		case kVTKeys_FKeyVT220F20:
			seqPtr = "\033[34~";
			break;
		
		case kVTKeys_FKeyXTermX11F1:
			seqPtr = "\033[11~";
			break;
		
		case kVTKeys_FKeyXTermX11F2:
			seqPtr = "\033[12~";
			break;
		
		case kVTKeys_FKeyXTermX11F3:
			seqPtr = "\033[13~";
			break;
		
		case kVTKeys_FKeyXTermX11F4:
			seqPtr = "\033[14~";
			break;
		
		case kVTKeys_FKeyXTermX11F5:
			seqPtr = "\033[15~";
			break;
		
		case kVTKeys_FKeyXTermX11F13:
			seqPtr = "\033[11;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F14:
			seqPtr = "\033[12;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F15:
			seqPtr = "\033[13;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F16:
			seqPtr = "\033[14;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F17:
			seqPtr = "\033[15;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F18:
			seqPtr = "\033[17;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F19:
			seqPtr = "\033[18;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F20:
			seqPtr = "\033[19;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F21:
			seqPtr = "\033[20;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F22:
			seqPtr = "\033[21;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F23:
			seqPtr = "\033[23;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F24:
			seqPtr = "\033[24;2~";
			break;
		
		case kVTKeys_FKeyXTermX11F25:
			seqPtr = "\033[11;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F26:
			seqPtr = "\033[12;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F27:
			seqPtr = "\033[13;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F28:
			seqPtr = "\033[14;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F29:
			seqPtr = "\033[15;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F30:
			seqPtr = "\033[17;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F31:
			seqPtr = "\033[18;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F32:
			seqPtr = "\033[19;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F33:
			seqPtr = "\033[20;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F34:
			seqPtr = "\033[21;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F35:
			seqPtr = "\033[23;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F36:
			seqPtr = "\033[24;5~";
			break;
		
		case kVTKeys_FKeyXTermX11F37:
			seqPtr = "\033[11;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F38:
			seqPtr = "\033[12;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F39:
			seqPtr = "\033[13;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F40:
			seqPtr = "\033[14;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F41:
			seqPtr = "\033[15;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F42:
			seqPtr = "\033[17;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F43:
			seqPtr = "\033[18;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F44:
			seqPtr = "\033[19;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F45:
			seqPtr = "\033[20;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F46:
			seqPtr = "\033[21;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F47:
			seqPtr = "\033[23;6~";
			break;
		
		case kVTKeys_FKeyXTermX11F48:
			seqPtr = "\033[24;6~";
			break;
		
		case kVTKeys_FKeyXFree86F13:
			seqPtr = "\033O2P";
			break;
		
		case kVTKeys_FKeyXFree86F14:
			seqPtr = "\033O2Q";
			break;
		
		case kVTKeys_FKeyXFree86F15:
			seqPtr = "\033O2R";
			break;
		
		case kVTKeys_FKeyXFree86F16:
			seqPtr = "\033O2S";
			break;
		
		case kVTKeys_FKeyXFree86F25:
			seqPtr = "\033O5P";
			break;
		
		case kVTKeys_FKeyXFree86F26:
			seqPtr = "\033O5Q";
			break;
		
		case kVTKeys_FKeyXFree86F27:
			seqPtr = "\033O5R";
			break;
		
		case kVTKeys_FKeyXFree86F28:
			seqPtr = "\033O5S";
			break;
		
		case kVTKeys_FKeyXFree86F37:
			seqPtr = "\033O6P";
			break;
		
		case kVTKeys_FKeyXFree86F38:
			seqPtr = "\033O6Q";
			break;
		
		case kVTKeys_FKeyXFree86F39:
			seqPtr = "\033O6R";
			break;
		
		case kVTKeys_FKeyXFree86F40:
			seqPtr = "\033O6S";
			break;
		
		case kVTKeys_FKeyRxvtF21:
			seqPtr = "\033[23$";
			break;
		
		case kVTKeys_FKeyRxvtF22:
			seqPtr = "\033[24$";
			break;
		
		case kVTKeys_FKeyRxvtF23:
			seqPtr = "\033[11^";
			break;
		
		case kVTKeys_FKeyRxvtF24:
			seqPtr = "\033[12^";
			break;
		
		case kVTKeys_FKeyRxvtF25:
			seqPtr = "\033[13^";
			break;
		
		case kVTKeys_FKeyRxvtF26:
			seqPtr = "\033[14^";
			break;
		
		case kVTKeys_FKeyRxvtF27:
			seqPtr = "\033[15^";
			break;
		
		case kVTKeys_FKeyRxvtF28:
			seqPtr = "\033[17^";
			break;
		
		case kVTKeys_FKeyRxvtF29:
			seqPtr = "\033[18^";
			break;
		
		case kVTKeys_FKeyRxvtF30:
			seqPtr = "\033[19^";
			break;
		
		case kVTKeys_FKeyRxvtF31:
			seqPtr = "\033[20^";
			break;
		
		case kVTKeys_FKeyRxvtF32:
			seqPtr = "\033[21^";
			break;
		
		case kVTKeys_FKeyRxvtF33:
			seqPtr = "\033[23^";
			break;
		
		case kVTKeys_FKeyRxvtF34:
			seqPtr = "\033[24^";
			break;
		
		case kVTKeys_FKeyRxvtF35:
			seqPtr = "\033[25^";
			break;
		
		case kVTKeys_FKeyRxvtF36:
			seqPtr = "\033[26^";
			break;
		
		case kVTKeys_FKeyRxvtF37:
			seqPtr = "\033[28^";
			break;
		
		case kVTKeys_FKeyRxvtF38:
			seqPtr = "\033[29^";
			break;
		
		case kVTKeys_FKeyRxvtF39:
			seqPtr = "\033[31^";
			break;
		
		case kVTKeys_FKeyRxvtF40:
			seqPtr = "\033[32^";
			break;
		
		case kVTKeys_FKeyRxvtF41:
			seqPtr = "\033[33^";
			break;
		
		case kVTKeys_FKeyRxvtF42:
			seqPtr = "\033[34^";
			break;
		
		case kVTKeys_FKeyRxvtF43:
			seqPtr = "\033[23@";
			break;
		
		case kVTKeys_FKeyRxvtF44:
			seqPtr = "\033[24@";
			break;
		
		default:
			// ???
			result = kTerminal_ResultInvalidID;
			break;
		}
		
		if (nullptr != seqPtr)
		{
			// send the key sequence (note, escape sequences like ESC-[ will
			// be automatically converted into single-byte codes if the
			// emulator is in a mode that requires them)
			dataPtr->emulator.sendEscape(dataPtr->listeningSession, seqPtr, CPP_STD::strlen(seqPtr));
		}
	}
	return result;
}// UserInputVTFunctionKey


/*!
Sends a special key sequence, such as a page-down command.  All
possible key codes are defined in the "VTKeys.h" header.

NOTE:	It is possible for historical reasons to use certain
		function key codes as an argument to this routine, and
		for the time being this is allowed.  A better way to
		handle function keys however is with the newer routine
		Terminal_UserInputVTFunctionKey(), which can handle a
		much wider variety of codes.

User input of any kind should take local echoing behavior into
account, but note that this routine does not do echoing.

This could fail, if for instance there is no session listening
to input for this terminal.

IMPORTANT:	User input routines at the terminal level are rare,
			and generally only used to handle sequences that
			depend very directly on terminal state.  Look in the
			Session module for preferred user input routines.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultNoListeningSession
if keys cannot be sent because no session is listening

(3.1)
*/
Terminal_Result
Terminal_UserInputVTKey		(TerminalScreenRef		inRef,
							 UInt8					inVTKey)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == dataPtr->listeningSession) result = kTerminal_ResultNoListeningSession;
	else
	{
		if ((inVTKey >= VSK0) && (inVTKey <= VSKE) && (false == dataPtr->modeApplicationKeys))
		{
			// VT SPECIFIC:
			// keypad key in numeric mode (as opposed to application key mode)
			UInt8 const		kVTNumericModeTranslation[] =
			{
				// numbers, symbols, Enter, PF1-PF4
				"0123456789,-.\015"
			};
			
			
			dataPtr->emulator.sendEscape(dataPtr->listeningSession, &kVTNumericModeTranslation[inVTKey - VSK0], 1);
		}
		else
		{
			// VT SPECIFIC:
			// keypad key in application mode (as opposed to numeric mode),
			// or an arrow key or PF-key in either mode
			char const		kVTApplicationModeTranslation[] =
			{
				// arrows, numbers, symbols, Enter, PF1-PF4
				"ABCDpqrstuvwxylmnMPQRS"
			};
			char*		seqPtr = nullptr;
			size_t		seqLength = 0;
			
			
			if (inVTKey < VSUP)
			{
				// construct key code sequences starting from VSF10 (see VTKeys.h);
				// each sequence is defined starting with the template, below, and
				// then substituting 1 or 2 characters into the sequence template;
				// the order must exactly match what is in VTKeys.h, because of the
				// subtraction used to derive the array index
				static char			seqVT220Keys[] = "\033[  ~";
				static char const	kArrayIndex2Translation[] = "222122?2?3?3?2?3?3123425161";
				static char const	kArrayIndex3Translation[] = "134956?9?2?3?8?1?4~~~~0~8~7";
				
				
				seqVT220Keys[2] = kArrayIndex2Translation[inVTKey - VSF10];
				seqVT220Keys[3] = kArrayIndex3Translation[inVTKey - VSF10];
				seqPtr = seqVT220Keys;
				seqLength = CPP_STD::strlen(seqVT220Keys);
				if ('~' == seqPtr[3]) --seqLength; // a few of the sequences are shorter
			}
			else if (inVTKey < VSF1)
			{
				// arrows or most keypad keys
				if (dataPtr->modeANSIEnabled)
				{
					// non-VT52
					static char		seqKeypadApp[] = "\033O ";
					static char		seqKeypadCursor[] = "\033[ ";
					
					
					if (inVTKey < VSK0)
					{
						// arrows
						if (dataPtr->modeCursorKeysForApp)
						{
							seqPtr = seqKeypadApp;
							seqLength = CPP_STD::strlen(seqKeypadApp);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
						else
						{
							seqPtr = seqKeypadCursor;
							seqLength = CPP_STD::strlen(seqKeypadCursor);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
					}
					else
					{
						if (dataPtr->modeApplicationKeys)
						{
							// keypad keys have special application behavior, unless
							// the cursor keys are hit and cursor mode is enabled
							seqPtr = seqKeypadApp;
							seqLength = CPP_STD::strlen(seqKeypadApp);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
						else
						{
							// numerical mode
							seqPtr = seqKeypadCursor;
							seqLength = CPP_STD::strlen(seqKeypadCursor);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
					}
				}
				else
				{
					// VT52
					static char		seqKeypadVT52[] = "\033? ";
					static char		seqArrowsVT52[] = "\033 ";
					
					
					if (inVTKey > VSLT)
					{
						// non-arrows
						seqPtr = seqKeypadVT52;
						seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						seqLength = CPP_STD::strlen(seqKeypadVT52);
					}
					else
					{
						// arrows
						seqPtr = seqArrowsVT52;
						seqLength = CPP_STD::strlen(seqArrowsVT52);
						seqPtr[1] = kVTApplicationModeTranslation[inVTKey - VSUP];
					}
				}
			}
			else
			{
				// PF1 through PF4
				static char		seqFunctionKeysNormal[] = "\033O ";
				static char		seqFunctionKeysVT52[] = "\033 ";
				
				
				if (dataPtr->modeANSIEnabled)
				{
					seqPtr = seqFunctionKeysNormal;
					seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
					seqLength = CPP_STD::strlen(seqFunctionKeysNormal);
				}
				else
				{
					seqPtr = seqFunctionKeysVT52;
					seqPtr[1] = kVTApplicationModeTranslation[inVTKey - VSUP];
					seqLength = CPP_STD::strlen(seqFunctionKeysVT52);
				}
			}
			
			// finally, send the key sequence
			dataPtr->emulator.sendEscape(dataPtr->listeningSession, seqPtr, seqLength);
		}
	}
	return result;
}// UserInputVTKey


/*!
Returns "true" only if the specified terminal has been
*told* to iconify its window.  In order for the state of
a real window to be in sync with this, a window handler
(such as the Terminal Window module) must register a
handler for the "kTerminal_ChangeWindowMinimization"
event using Terminal_StartMonitoring().

(3.0)
*/
Boolean
Terminal_WindowIsToBeMinimized	(TerminalScreenRef	inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->windowMinimized;
	}
	return result;
}// WindowIsToBeMinimized


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_Emulator class instance.  See also reset().

(3.1)
*/
My_Emulator::
My_Emulator		(Emulation_FullType		inPrimaryEmulation,
				 CFStringRef			inAnswerBack,
				 CFStringEncoding		inInputTextEncoding)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
primaryType(inPrimaryEmulation),
isUTF8Encoding(kCFStringEncodingUTF8 == inInputTextEncoding),
lockUTF8(false),
disableShifts(false),
inputTextEncoding(inInputTextEncoding),
answerBackCFString(inAnswerBack, CFRetainRelease::kNotYetRetained),
stateRepetitions(0),
currentState(kMy_ParserStateInitial),
stringAccumulatorState(kMy_ParserStateInitial),
stringAccumulator(),
multiByteDecoder(),
recentCodePointByte('\0'),
addedITerm(false),
addedSixel(false),
addedXTerm(false),
allowSixelScrolling(true),
argLastIndex(0),
argList(kMy_MaximumANSIParameters),
parameterMarkList(kMy_MaximumANSIParameters),
preCallbackSet(),
currentCallbacks(returnDataWriter(inPrimaryEmulation),
					returnStateDeterminant(inPrimaryEmulation),
					returnStateTransitionHandler(inPrimaryEmulation),
					returnResetHandler(inPrimaryEmulation)),
pushedCallbacks(),
supportedVariants(kVariantFlagsNone),
trueColorTableReds(nullptr),
trueColorTableGreens(nullptr),
trueColorTableBlues(nullptr),
trueColorTableNextID(0),
bitmapTableNextID(0),
bitmapImageTable(nil),
bitmapSegmentTable(nil),
eightBitReceiver(false),
eightBitTransmitter(false),
lockSevenBitTransmit(false)
{
	initializeParserStateStack();
}// My_Emulator default constructor


/*!
Destructor.

(4.1)
*/
My_Emulator::~My_Emulator()
{
	// NOTE: constructor initializes these to nullptr and it is
	// safe to do an unchecked "delete" of nullptr
	delete trueColorTableReds;
	delete trueColorTableGreens;
	delete trueColorTableBlues;
	[bitmapImageTable release];
	[bitmapSegmentTable release];
}// My_Emulator destructor


/*!
Responds to a CSI (control sequence inducer) by reinitializing
all parameter values and resetting the current parameter index
to 0.

(3.0)
*/
void
My_Emulator::
clearEscapeSequenceParameters ()
{
	SInt16		parameterIndex = STATIC_CAST(this->argList.size(), SInt16);
	
	
	while (--parameterIndex >= 0)
	{
		this->argList[parameterIndex] = kMy_ParamUndefined;
	}
	
	parameterIndex = STATIC_CAST(this->parameterMarkList.size(), SInt16);
	while (--parameterIndex >= 0)
	{
		this->parameterMarkList[parameterIndex] = -1;
	}
	
	this->argLastIndex = 0;
}// clearEscapeSequenceParameters


/*!
Discards all state history in the screen’s parser and creates
a single, initial state.

This is obviously done when a screen is first created, but
should also be done whenever the state determinant and state
transition routines are changed (e.g. to change the emulation
type).

(3.1)
*/
void
My_Emulator::
initializeParserStateStack ()
{
	this->currentState = kMy_ParserStateInitial;
	this->stringAccumulatorState = kMy_ParserStateInitial;
}// initializeParserStateStack


/*!
Returns true only if the emulator is in 8-bit receive mode,
which permits characters like CSI to be considered equivalent
to multi-byte 7-bit sequences (in the case of CSI, ESC-"[").

(4.0)
*/
inline Boolean
My_Emulator::
is8BitReceiver ()
{
	return this->eightBitReceiver;
}// is8BitReceiver


/*!
Returns true only if the emulator is in 8-bit transmit mode,
which means that any reports will use single bytes for control
codes instead of multi-byte escape sequences.

(4.0)
*/
inline Boolean
My_Emulator::
is8BitTransmitter ()
{
	return this->eightBitTransmitter;
}// is8BitTransmitter


/*!
If the emulator is currently using a Unicode encoding, this
routine returns the most recent complete (valid) code point
that was represented; for example, in UTF-8 this comes from
the last 0-6 bytes.  Since UTF-8 is always decoded prior to
calling emulators, the code point is exact and requires no
further translation.  Note that the return value can be
considered of type "UnicodeScalarValue" when UTF-8 is in use.

For “normal” cases of single-byte encodings however, the text
translation only occurs AFTER bytes have passed into emulators.
In these cases, the return value is going to contain only one
byte, and its meaning depends entirely on the encoding.  Note
that low ASCII values are the same across many encodings, so
in almost all cases an emulator can read values as-is and only
worry about translation for things that compose strings (like
echoes or XTerm window titles).

Note that this function is intended to prevent emulators from
having to know anything about UTF-8!  It allows emulators to
handle things like 8-bit control sequences without caring how
that sequence might have been originally encoded.

The value "kUTF8Decoder_InvalidUnicodeCodePoint" is returned
if there is no apparent value available in the buffer.

(4.0)
*/
UInt32
My_Emulator::
recentCodePoint ()
{
	UInt32		result = this->recentCodePointByte;
	
	
	if (kCFStringEncodingUTF8 == this->inputTextEncoding)
	{
		if (this->multiByteDecoder.incompleteSequence())
		{
			result = kUTF8Decoder_InvalidUnicodeCodePoint;
		}
		else
		{
			size_t		bytesUsed = 0;
			
			
			result = UTF8Decoder_StateMachine::byteSequenceTotalValue(this->multiByteDecoder.multiByteAccumulator, 0/* offset */,
																		this->multiByteDecoder.multiByteAccumulator.size(), &bytesUsed);
		}
	}
	return result;
}// recentCodePoint


/*!
Responds to a terminal reset by returning certain emulator
settings to defaults.  This does NOT reinitialize settings in
the instance that are not directly attributed to terminal state.

If "inIsSoftReset" is true, then only parameters that should be
affected by a soft reset are changed (for example, there are
differences in a VT220).

(4.0)
*/
void
My_Emulator::
reset	(Boolean	UNUSED_ARGUMENT(inIsSoftReset))
{
	clearEscapeSequenceParameters();
	initializeParserStateStack();
	this->eightBitReceiver = false;
	this->eightBitTransmitter = false;
	this->lockSevenBitTransmit = false;
}// reset


/*!
Returns the entry point for determining how to echo data,
for the specified terminal type.

(4.0)
*/
My_EmulatorEchoDataProcPtr
My_Emulator::
returnDataWriter	(Emulation_FullType		inPrimaryEmulation)
{
	My_EmulatorEchoDataProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kEmulation_FullTypeDumb:
		result = My_DumbTerminal::echoData;
		break;
	
	case kEmulation_FullTypeVT100:
	case kEmulation_FullTypeXTermOriginal:
	case kEmulation_FullTypeXTermColor:
	case kEmulation_FullTypeXTerm256Color:
	case kEmulation_FullTypeANSIBBS:
	case kEmulation_FullTypeANSISCO:
	case kEmulation_FullTypeVT102:
	case kEmulation_FullTypeVT220:
	case kEmulation_FullTypeVT320:
	case kEmulation_FullTypeVT420:
	default:
		// Echoing data with correct translation, etc. is not trivial and
		// it is not recommended that most emulators try to do this any
		// differently than the default emulator.
		result = My_DefaultEmulator::echoData;
		break;
	}
	return result;
}// returnDataWriter


/*!
When implementing a type of erase that is not selective, this
method should be called to determine the proper effects on the
screen buffer.

A normal erase will clear all text (to whitespace) and reset
all attributes that are set on individual characters.  It will
also set a flag to reset line-global attributes, but note that
the internal routines that erase parts of the buffer will
ignore this flag when only part of a line is being erased.

If an emulator is configured for background-color-erase, then
the returned attributes will specify that behavior.

(4.0)
*/
My_BufferChanges
My_Emulator::
returnEraseEffectsForNormalUse ()
{
	My_BufferChanges	result = (kMy_BufferChangesEraseAllText |
									kMy_BufferChangesResetCharacterAttributes |
									kMy_BufferChangesResetLineAttributes);
	
	
	if (supportsVariant(kVariantFlagXTermBCE))
	{
		result |= kMy_BufferChangesKeepBackgroundColor;
	}
	
	return result;
}// returnEraseEffectsForNormalUse


/*!
When implementing a type of erase that is selective, this method
should be called to determine the proper effects on the screen
buffer.

A selective erase will not ordinarily touch any attributes and
will not clear any text.

If an emulator is configured for background-color-erase, then
the returned attributes will specify that behavior.

(4.0)
*/
My_BufferChanges
My_Emulator::
returnEraseEffectsForSelectiveUse ()
{
	My_BufferChanges	result = 0;
	
	
	if (supportsVariant(kVariantFlagXTermBCE))
	{
		result |= kMy_BufferChangesKeepBackgroundColor;
	}
	
	return result;
}// returnEraseEffectsForSelectiveUse


/*!
Returns the entry point for resetting the emulator, for the
specified terminal type.

(4.0)
*/
My_EmulatorResetProcPtr
My_Emulator::
returnResetHandler		(Emulation_FullType		inPrimaryEmulation)
{
	My_EmulatorResetProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kEmulation_FullTypeVT100:
	case kEmulation_FullTypeVT102:
	case kEmulation_FullTypeANSIBBS: // TEMPORARY
	case kEmulation_FullTypeANSISCO: // TEMPORARY
		result = My_VT100::hardSoftReset;
		break;
	
	case kEmulation_FullTypeVT220:
	case kEmulation_FullTypeVT320: // TEMPORARY
	case kEmulation_FullTypeVT420: // TEMPORARY
		result = My_VT220::hardSoftReset;
		break;
	
	case kEmulation_FullTypeXTermOriginal: // TEMPORARY
	case kEmulation_FullTypeXTermColor: // TEMPORARY
	case kEmulation_FullTypeXTerm256Color: // TEMPORARY
		// no XTerm-specific reset yet
		result = My_VT220::hardSoftReset;
		break;
	
	case kEmulation_FullTypeDumb:
	default:
		// ???
		result = My_DefaultEmulator::hardSoftReset;
		break;
	}
	return result;
}// returnResetHandler


/*!
Returns the entry point for determining emulator state,
for the specified terminal type.

(4.0)
*/
My_EmulatorStateDeterminantProcPtr
My_Emulator::
returnStateDeterminant		(Emulation_FullType		inPrimaryEmulation)
{
	My_EmulatorStateDeterminantProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kEmulation_FullTypeVT100:
	case kEmulation_FullTypeANSIBBS: // TEMPORARY
	case kEmulation_FullTypeANSISCO: // TEMPORARY
		result = My_VT100::stateDeterminant;
		break;
	
	case kEmulation_FullTypeVT102:
		result = My_VT102::stateDeterminant;
		break;
	
	case kEmulation_FullTypeVT220:
	case kEmulation_FullTypeVT320: // TEMPORARY
	case kEmulation_FullTypeVT420: // TEMPORARY
		result = My_VT220::stateDeterminant;
		break;
	
	case kEmulation_FullTypeXTermOriginal:
	case kEmulation_FullTypeXTermColor:
	case kEmulation_FullTypeXTerm256Color:
		result = My_XTerm::stateDeterminant;
		break;
	
	case kEmulation_FullTypeDumb:
		result = My_DumbTerminal::stateDeterminant;
		break;
	
	default:
		// ???
		result = My_DefaultEmulator::stateDeterminant;
		break;
	}
	return result;
}// returnStateDeterminant


/*!
Returns the entry point for moving between emulator states,
for the specified terminal type.

(4.0)
*/
My_EmulatorStateTransitionProcPtr
My_Emulator::
returnStateTransitionHandler	(Emulation_FullType		inPrimaryEmulation)
{
	My_EmulatorStateTransitionProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kEmulation_FullTypeVT100:
	case kEmulation_FullTypeANSIBBS: // TEMPORARY
	case kEmulation_FullTypeANSISCO: // TEMPORARY
		result = My_VT100::stateTransition;
		break;
	
	case kEmulation_FullTypeVT102:
		result = My_VT102::stateTransition;
		break;
	
	case kEmulation_FullTypeVT220:
	case kEmulation_FullTypeVT320: // TEMPORARY
	case kEmulation_FullTypeVT420: // TEMPORARY
		result = My_VT220::stateTransition;
		break;
	
	case kEmulation_FullTypeXTermOriginal:
	case kEmulation_FullTypeXTermColor:
	case kEmulation_FullTypeXTerm256Color:
		result = My_XTerm::stateTransition;
		break;
	
	case kEmulation_FullTypeDumb:
		result = My_DumbTerminal::stateTransition;
		break;
	
	default:
		// ???
		result = My_DefaultEmulator::stateTransition;
		break;
	}
	return result;
}// returnStateTransitionHandler


/*!
Transmits a string that starts with an escape sequence to the
given session.  If the emulator is in "eightBitTransmitter"
mode, the sequence is automatically converted into a single
byte (e.g. "\033[" becomes CSI) before transmitting the rest
of the string.

This allows you to express all responses as if they must be
7-bit sequences, and have 8-bit versions used automatically
when appropriate.

IMPORTANT:	When a terminal has to respond to its listening
			session, it is probably a good idea to always use
			this method instead of calling Session_SendData()
			directly.  That way, if any escape sequence is
			needed now or in the future, your response will
			handle 8-bit mode properly.

(4.0)
*/
void
My_Emulator::
sendEscape	(SessionRef		inSession,
			 void const*	in7BitSequence,
			 size_t			inSequenceLength)
{
	if (inSequenceLength > 0)
	{
		UInt8 const*	asCharPtr = REINTERPRET_CAST(in7BitSequence, UInt8 const*);
		
		
		if ((false == this->eightBitTransmitter) || ('\033' != asCharPtr[0]) || (1 == inSequenceLength) ||
			(asCharPtr[1] < 0x40/* @ */) || (asCharPtr[1] > 0x5F/* _ */))
		{
			// only an ESC, not an ESC sequence, or not possible to translate
			// into an 8-bit sequence; emit normally
			Session_SendData(inSession, asCharPtr, inSequenceLength);
		}
		else
		{
			UInt8	eightBit[] = { '\0' };
			
			
			eightBit[0] = (asCharPtr[1] + 0x40); // translate 7-bit to 8-bit, ignore the ESC
			Session_SendData(inSession, eightBit, 1/* count */);
			if (inSequenceLength > 2)
			{
				size_t const	kESCSeqLength = 2;
				
				
				Session_SendData(inSession, asCharPtr + kESCSeqLength, inSequenceLength - kESCSeqLength);
			}
		}
	}
}// sendEscape


/*!
Specifies whether or not the terminal will recognize 8-bit
sequences as being equivalent to multi-byte 7-bit sequences,
for the purposes of its state machine.  For example, in a
VT100 this means that code 155 is equivalent to (but one fewer
byte than) the sequence ESC-[.

(4.0)
*/
void
My_Emulator::
set8BitReceive		(Boolean	inIs8Bit)
{
	this->eightBitReceiver = inIs8Bit;
}// set8BitReceive


/*!
Specifies whether or not transmissions from the terminal to the
listening session use a single byte to represent certain escape
sequences.  Has no effect if set8BitTransmitNever() has been
used.

(4.0)
*/
void
My_Emulator::
set8BitTransmit		(Boolean	inIs8Bit)
{
	if (false == inIs8Bit)
	{
		// it can always be turned off...
		this->eightBitTransmitter = inIs8Bit;
	}
	else
	{
		// if locked in a 7-bit mode then refuse to return to 8-bit mode
		this->eightBitTransmitter = (false == this->lockSevenBitTransmit);
	}
}// set8BitTransmit


/*!
Fuses the emulator into 7-bit mode.  This is only useful in odd
corner cases with strict emulation of hardware, where it may be
expected that the terminal stop respecting 8-bit mode requests.

(4.0)
*/
void
My_Emulator::
set8BitTransmitNever ()
{
	this->lockSevenBitTransmit = true;
}// set8BitTransmitNever


/*!
Returns true only if this terminal emulator has been configured
to support the specified variant.

(4.0)
*/
Boolean
My_Emulator::
supportsVariant		(VariantFlags	inFlag)
{
	Boolean		result = (0 != (this->supportedVariants & inFlag));
	
	
	return result;
}// supportsVariant


/*!
Initializes a My_Emulator::Callbacks class instance with
null pointers.

(4.0)
*/
My_Emulator::Callbacks::
Callbacks ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
dataWriter(nullptr),
resetHandler(nullptr),
stateDeterminant(nullptr),
transitionHandler(nullptr)
{
}// My_Emulator::Callbacks default constructor


/*!
Initializes a My_Emulator::Callbacks class instance.

(4.0)
*/
My_Emulator::Callbacks::
Callbacks	(My_EmulatorEchoDataProcPtr				inDataWriter,
			 My_EmulatorStateDeterminantProcPtr		inStateDeterminant,
			 My_EmulatorStateTransitionProcPtr		inTransitionHandler,
			 My_EmulatorResetProcPtr				inResetHandler)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
dataWriter(inDataWriter),
resetHandler(inResetHandler),
stateDeterminant(inStateDeterminant),
transitionHandler(inTransitionHandler)
{
}// My_Emulator::Callbacks 2-argument constructor


/*!
Returns true only if the callbacks are all defined.

(4.0)
*/
Boolean
My_Emulator::Callbacks::
exist ()
const
{
	return ((nullptr != dataWriter) && (nullptr != stateDeterminant) && (nullptr != transitionHandler));
}// My_Emulator::Callbacks::exist


/*!
Constructor.  See Terminal_NewScreen().

Throws a Terminal_Result if any problems occur.

(3.0)
*/
My_ScreenBuffer::
My_ScreenBuffer	(Preferences_ContextRef		inTerminalConfig,
				 Preferences_ContextRef		inTranslationConfig)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
refValidator(REINTERPRET_CAST(this, TerminalScreenRef), gTerminalScreenValidRefs()),
configuration(Preferences_NewCloneContext(inTerminalConfig, true/* detach */),
				Preferences_ContextWrap::kAlreadyRetained),
emulator(returnEmulator(inTerminalConfig), returnAnswerBackMessage(inTerminalConfig), returnTextEncoding(inTranslationConfig)),
listeningSession(nullptr),
speaker(nullptr),
windowTitleCFString(),
iconTitleCFString(),
changeListenerModel(ListenerModel_New(kListenerModel_StyleStandard, kConstantsRegistry_ListenerModelDescriptorTerminalChanges)),
preferenceMonitor(ListenerModel_NewStandardListener(preferenceChanged, this/* context */),
					ListenerModel_ListenerWrap::kAlreadyRetained),
scrollbackBufferCachedSize(0),
scrollbackBuffer(),
screenBuffer(),
bytesToEcho(),
debugStateHandlerSequence(),
echoErrorCount(0),
translationErrorCount(0),
errorCountTotal(0),
tabSettings(),
captureStream(StreamCapture_New(returnLineEndings())),
printingStream(nullptr),
printingFileURL(),
printingModes(0),
bellDisabled(false),
cursorType(kTerminal_CursorTypeBlock),
cursorBlinking(true),
cursorVisible(true),
reverseVideo(false),
windowMinimized(false),
vtG0(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOff),
vtG1(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOn),
vtG2(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOff),
vtG3(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOff),
visibleBoundary(0, 0, returnScreenColumns(inTerminalConfig) - 1, returnScreenRows(inTerminalConfig) - 1),
customScrollingRegion(0, 0), // reset below...
// text elements - not initialized
litLEDs(kMy_LEDBitsAllOff),
passwordMode(false),
mayNeedToSaveToScrollback(false),
saveToScrollbackOnClear(true),
reportOnlyOnRequest(false),
wrapPending(false),
modeANSIEnabled(true),
modeApplicationKeys(false),
modeAutoWrap(false),
modeCursorKeysForApp(false),
modeInsertNotReplace(false),
modeNewLineOption(false),
modeOriginRedefined(false),
reportedPatchLevel(returnXTermPatchLevel(inTerminalConfig)),
originRegionPtr(&visibleBoundary.rows),
// speech elements - not initialized
current(*this),
// previous elements - not initialized
selfRef(REINTERPRET_CAST(this, TerminalScreenRef))
// TEMPORARY: initialize other members here...
{
	this->text.visibleScreen.numberOfColumnsAllocated = Terminal_ReturnAllocatedColumnCount(); // always allocate max columns
	
	this->current.cursorX = 0; // initialized because moveCursor() depends on prior values...
	this->current.cursorY = 0; // initialized because moveCursor() depends on prior values...
	
	// now “append” the desired number of main screen lines, which will have
	// the effect of allocating a screen buffer of the right size
	unless (screenInsertNewLines(this, returnScreenRows(inTerminalConfig)))
	{
		throw kTerminal_ResultNotEnoughMemory;
	}
	assert(!this->screenBuffer.empty());
	
	// arbitrary pre-allocation to avoid dynamic allocation;
	// the destructor can log the actual size in debug mode,
	// which will help to tune this for optimal performance
	this->bytesToEcho.reserve(256);
	
	try
	{
		// it is important to make the list a multiple of the tab stop distance;
		// see tabStopInitialize() to see why this is the case
		this->tabSettings.resize(this->text.visibleScreen.numberOfColumnsAllocated +
									(this->text.visibleScreen.numberOfColumnsAllocated % kMy_TabStop));
		tabStopInitialize(this);
	}
	catch (std::bad_alloc)
 	{
		throw kTerminal_ResultNotEnoughMemory;
	}
	
	this->current.characterSetInfoPtr = &this->vtG0; // by definition, G0 is active initially
	setScrollbackSize(this, returnScrollbackRows(inTerminalConfig));
	this->text.scrollback.enabled = (this->text.scrollback.enabled && returnForceSave(inTerminalConfig));
	this->text.visibleScreen.numberOfColumnsPermitted = returnScreenColumns(inTerminalConfig);
	this->current.cursorAttributes.clear();
	this->current.drawingAttributes.clear();
	this->current.latentAttributes.clear();
	this->previous.drawingAttributes = kTextAttributes_Invalid; // initially no saved attribute
	
	// speech setup
	this->speech.mode = kTerminal_SpeechModeSpeakNever;
	
	// set up optional terminal emulator features
	if (return24BitColor(inTerminalConfig))
	{
		this->emulator.supportedVariants |= My_Emulator::kVariantFlag24BitColor;
		this->emulator.trueColorTableReds = new My_RGBComponentList();
		this->emulator.trueColorTableGreens = new My_RGBComponentList();
		this->emulator.trueColorTableBlues = new My_RGBComponentList();
	}
	if (returnITermGraphics(inTerminalConfig))
	{
		this->emulator.supportedVariants |= My_Emulator::kVariantFlagITermGraphics;
		if (false == this->emulator.addedITerm)
		{
			this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
													My_Emulator::Callbacks(nullptr/* echo - override is not allowed in a pre-callback */,
																			My_ITermCore::stateDeterminant,
																			My_ITermCore::stateTransition,
																			nullptr/* reset - override is not allowed in a pre-callback */));
			this->emulator.addedITerm = true;
		}
	}
	if (returnXTerm256(inTerminalConfig))
	{
		this->emulator.supportedVariants |= My_Emulator::kVariantFlagXTerm256Color;
		if (false == this->emulator.addedXTerm)
		{
			this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
													My_Emulator::Callbacks(nullptr/* echo - override is not allowed in a pre-callback */,
																			My_XTermCore::stateDeterminant,
																			My_XTermCore::stateTransition,
																			nullptr/* reset - override is not allowed in a pre-callback */));
			this->emulator.addedXTerm = true;
		}
	}
	if (returnXTermBackgroundColorErase(inTerminalConfig))
	{
		// although named for XTerm, this mode is technically handled at a very low level
		// and is therefore supported by any kind of emulator; so, it is not necessary to
		// install any extra callbacks (even though other variants install callbacks)
		this->emulator.supportedVariants |= My_Emulator::kVariantFlagXTermBCE;
	}
	if (returnXTermWindowAlteration(inTerminalConfig))
	{
		this->emulator.supportedVariants |= My_Emulator::kVariantFlagXTermAlterWindow;
		if (false == this->emulator.addedXTerm)
		{
			this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
													My_Emulator::Callbacks(nullptr/* echo - override is not allowed in a pre-callback */,
																			My_XTermCore::stateDeterminant,
																			My_XTermCore::stateTransition,
																			nullptr/* reset - override is not allowed in a pre-callback */));
			this->emulator.addedXTerm = true;
		}
	}
	if (returnSixelGraphics(inTerminalConfig))
	{
		this->emulator.supportedVariants |= My_Emulator::kVariantFlagSixelGraphics;
		if (false == this->emulator.addedSixel)
		{
			this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
													My_Emulator::Callbacks(nullptr/* echo - override is not allowed in a pre-callback */,
																			My_SixelCore::stateDeterminant,
																			My_SixelCore::stateTransition,
																			nullptr/* reset - override is not allowed in a pre-callback */));
			this->emulator.addedSixel = true;
		}
	}
	this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
											My_Emulator::Callbacks(nullptr/* echo - override is not allowed in a pre-callback */,
																	My_UTF8Core::stateDeterminant,
																	My_UTF8Core::stateTransition,
																	nullptr/* reset - override is not allowed in a pre-callback */));
	
	// IMPORTANT: Within constructors, calls to routines expecting a *self reference* should be
	//            *last*; otherwise, there’s no telling whether or not the data that the routine
	//            requires will have been properly initialized yet.
	
	moveCursor(this, 0, 0);
	
	this->customScrollingRegion = this->visibleBoundary.rows; // initially...
	assertScrollingRegion(this);
	
	this->speaker = TerminalSpeaker_New(REINTERPRET_CAST(this, TerminalScreenRef));
	
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextStartMonitoring(this->configuration.returnRef(), this->preferenceMonitor.returnRef(),
															kPreferences_ChangeContextBatchMode);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "screen buffer failed to set up monitor for batch-mode changes to configuration, error", prefsResult);
		}
	}
	
	assert(Terminal_IsValid(this->selfRef));
	//Console_WriteValueAddress("validated screen", this);
}// My_ScreenBuffer 1-argument constructor


/*!
Destructor.  See Terminal_ReleaseScreen().

(3.1)
*/
My_ScreenBuffer::
~My_ScreenBuffer ()
{
	if (DebugInterface_LogsTerminalState())
	{
		// write some statistics on the terminal when it is finished,
		// for things like memory profiling; by knowing how large
		// dynamic structures typically grow, the pre-allocation
		// arbitrations can be more appropriate
		Console_WriteLine("statistics for disposed screen buffer object:");
		Console_WriteValue("final echo buffer size", this->bytesToEcho.capacity());
	}
	
	this->printingModes = 0; // clear so that printingEnd() will clean up
	printingEnd();
	StreamCapture_Release(&this->captureStream);
	UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(this->configuration.returnRef(), this->preferenceMonitor.returnRef(),
																		kPreferences_ChangeContextBatchMode);
	TerminalSpeaker_Dispose(&this->speaker);
	ListenerModel_Dispose(&this->changeListenerModel);
	
	for (My_ScreenBufferLinePtr& linePtrRef : this->scrollbackBuffer)
	{
		deleteLinePtr(linePtrRef);
	}
	for (My_ScreenBufferLinePtr& linePtrRef : this->screenBuffer)
	{
		deleteLinePtr(linePtrRef);
	}
	
	//Console_WriteValueAddress("invalidated screen", this);
}// My_ScreenBuffer destructor


/*!
Invoked whenever a monitored preference value is changed for
a particular screen (see the constructor for the calls that
arrange to monitor preferences).  This routine responds by
updating internal caches.

(4.0)
*/
void
My_ScreenBuffer::
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inPreferencesContext,
					 void*					inMyScreenBufferPtr)
{
	// WARNING: The context is only defined for the preferences monitored in a
	// context-specific way through Preferences_ContextStartMonitoring() calls.
	// Otherwise, the data type of the input is "Preferences_ChangeContext*".
	Preferences_ContextRef		prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	My_ScreenBufferPtr			ptr = REINTERPRET_CAST(inMyScreenBufferPtr, My_ScreenBufferPtr);
	
	
	if (kPreferences_ChangeContextBatchMode == inPreferenceTagThatChanged)
	{
		// batch mode; multiple things have changed; this should basically mirror
		// what is copied by Terminal_ReturnConfiguration()
		Terminal_SetVisibleScreenDimensions(ptr->selfRef, ptr->returnScreenColumns(prefsContext),
											ptr->returnScreenRows(prefsContext));
		setScrollbackSize(ptr, ptr->returnScrollbackRows(prefsContext));
	}
	else
	{
		// ???
	}
}// preferenceChanged


/*!
Conditionally terminates a printout for the specified terminal,
closing the temporary file used for print data.  If requested,
all the text cached since the last printingReset() is then sent
to the printer (displaying a preview dialog first).  The cache
file is deleted, so this is your only chance to print.

This has no effect if ANY printing mode is still active.  The
typical approach is to first disable the desired printing mode
bit (in the "printingModes" member) when a mode ends, and to
then call this routine.  If the disabled mode was the last
active mode, then there will be no active mode, and printing
will occur automatically.

(4.0)
*/
void
My_ScreenBuffer::
printingEnd		(Boolean	inSendRemainderToPrinter)
{
	if ((nullptr != this->printingStream) && (0 == this->printingModes))
	{
		StreamCapture_End(this->printingStream);
		StreamCapture_Release(&this->printingStream);
		
		// to print the outstanding text, determine the location of the temporary file
		// and then print from that file
		if (inSendRemainderToPrinter)
		{
			// print the captured text using the print dialog
			CFRetainRelease		jobTitle(UIStrings_ReturnCopy(kUIStrings_TerminalPrintFromTerminalJobTitle),
											CFRetainRelease::kAlreadyRetained);
			
			
			if (jobTitle.exists())
			{
				TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(this->listeningSession);
				PrintTerminal_JobRef	printJob = PrintTerminal_NewJobFromFile
													(CFUtilities_URLCast(this->printingFileURL.returnCFTypeRef()),
														TerminalWindow_ReturnViewWithFocus(terminalWindow),
														jobTitle.returnCFStringRef());
				
				
				if (nullptr != printJob)
				{
					UNUSED_RETURN(PrintTerminal_Result)PrintTerminal_JobSendToPrinter
														(printJob, TerminalWindow_ReturnNSWindow(terminalWindow));
					PrintTerminal_ReleaseJob(&printJob);
				}
			}
		}
		
		// delete temporary file
		{
			NSError* /*__autoreleasing*/	error = nil;
			
			
			if (NO == [[NSFileManager defaultManager] removeItemAtURL:BRIDGE_CAST(this->printingFileURL.returnCFURLRef(), NSURL*) error:&error])
			{
				if (nil != error)
				{
					Console_Warning(Console_WriteValueCFString, "failed to delete temporary printing file, error", BRIDGE_CAST([error localizedDescription], CFStringRef));
				}
			}
		}
		
		this->printingFileURL.clear();
	}
}// printingEnd


/*!
Conditionally starts printing by opening a new temporary file
for streaming.  If ANY printing mode is currently active (such
as auto-print or print controller), this has no effect.

Call this whenever you start a new printing mode.

To terminate all active prints, use printingEnd() (also called
automatically by the destructor of the class).

(4.0)
*/
void
My_ScreenBuffer::
printingReset ()
{
	if (0 == this->printingModes)
	{
		NSString*	temporaryFilePath = [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSUUID UUID] UUIDString]];
		
		
		// create a new, empty file that is open for write
		this->printingFileURL.setWithRetain(BRIDGE_CAST([NSURL fileURLWithPath:temporaryFilePath isDirectory:NO], CFURLRef));
		if (false == this->printingFileURL.exists())
		{
			Console_Warning(Console_WriteLine, "failed to find URL for temporary file for printing");
		}
		else
		{
			this->printingStream = StreamCapture_New(kSession_LineEndingLF);
			if (false == StreamCapture_Begin(this->printingStream, this->printingFileURL.returnCFURLRef()))
			{
				Console_Warning(Console_WriteLine, "failed to initiate capture to temporary file for printing");
			}
		}
	}
}// printingReset


/*!
Reads "kPreferences_TagTerminal24BitColorEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(4.0)
*/
Boolean
My_ScreenBuffer::
return24BitColor	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminal24BitColorEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// return24BitColor


/*!
Reads "kPreferences_TagTerminalAnswerBackMessage" from a
Preferences context, and returns either that value or the
default answer-back for returnEmulatorType().

(3.1)
*/
CFStringRef
My_ScreenBuffer::
returnAnswerBackMessage		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFStringRef				result = nullptr;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalAnswerBackMessage,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = Terminal_EmulatorReturnDefaultName(returnEmulator(inTerminalConfig));
	
	return result;
}// returnAnswerBackMessage


/*!
Reads "kPreferences_TagTerminalEmulatorType" from a Preferences
context, and returns either that value or the default VT100
type if none was found.

(3.1)
*/
Emulation_FullType
My_ScreenBuffer::
returnEmulator	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Emulation_FullType		result = kEmulation_FullTypeVT100;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalEmulatorType,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = kEmulation_FullTypeVT100;
	
	return result;
}// returnEmulator


/*!
Reads "kPreferences_TagTerminalClearSavesLines" from a
Preferences context, and returns either that value or the
default of true if none was found.

(3.1)
*/
Boolean
My_ScreenBuffer::
returnForceSave		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalClearSavesLines,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnForceSave


/*!
Reads "kPreferences_TagITermGraphicsEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(2017.12)
*/
Boolean
My_ScreenBuffer::
returnITermGraphics		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagITermGraphicsEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnITermGraphics


/*!
Reads "kPreferences_TagCaptureFileLineEndings" from a Preferences
context, and returns either that value or the default value if
none was found.

(4.0)
*/
Session_LineEnding
My_ScreenBuffer::
returnLineEndings ()
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Session_LineEnding		result = kSession_LineEndingLF; // arbitrary default
	
	
	// TEMPORARY - perhaps this routine should take a specific preferences context
	prefsResult = Preferences_GetData(kPreferences_TagCaptureFileLineEndings,
										sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "screen buffer failed to read capture file line endings from preferences, error", prefsResult);
	}
	return result;
}// returnLineEndings


/*!
Reads "kPreferences_TagTerminalScreenColumns" from a Preferences
context, and returns either that value or the default value of
80 if none was found.

(3.1)
*/
UInt16
My_ScreenBuffer::
returnScreenColumns		(Preferences_ContextRef		inTerminalConfig,
						 Boolean					inFallBackToDefaults)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	UInt16					result = 80; // arbitrary default
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenColumns,
												sizeof(result), &result, inFallBackToDefaults);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "screen buffer failed to read column count from preferences, error", prefsResult);
	}
	return result;
}// returnScreenColumns


/*!
Reads "kPreferences_TagTerminalScreenRows" from a Preferences
context, and returns either that value or the default value of
24 if none was found.

(3.1)
*/
UInt16
My_ScreenBuffer::
returnScreenRows	(Preferences_ContextRef		inTerminalConfig,
					 Boolean					inFallBackToDefaults)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	UInt16					result = 24; // arbitrary default
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenRows,
												sizeof(result), &result, inFallBackToDefaults);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "screen buffer failed to read row count from preferences, error", prefsResult);
	}
	return result;
}// returnScreenRows


/*!
Reads all scrollback-related settings from a Preferences context,
and returns an appropriate value for scrollback size.

(3.1)
*/
UInt32
My_ScreenBuffer::
returnScrollbackRows	(Preferences_ContextRef		inTerminalConfig,
						 Boolean					inFallBackToDefaults)
{
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Terminal_ScrollbackType		scrollbackType = kTerminal_ScrollbackTypeFixed;
	UInt32						result = 200; // arbitrary default
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenScrollbackType,
												sizeof(scrollbackType), &scrollbackType, inFallBackToDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		if (kTerminal_ScrollbackTypeDisabled == scrollbackType)
		{
			result = 0;
		}
		else if (kTerminal_ScrollbackTypeUnlimited == scrollbackType)
		{
			result = USHRT_MAX; // TEMPORARY (warning: every screen allocates this amount!)
		}
		else if (kTerminal_ScrollbackTypeDistributed == scrollbackType)
		{
			// UNIMPLEMENTED
		}
	}
	
	if (kTerminal_ScrollbackTypeFixed == scrollbackType)
	{
		prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenScrollbackRows,
													sizeof(result), &result);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "screen buffer failed to read scrollback row count from preferences, error", prefsResult);
		}
	}
	
	return result;
}// returnScrollbackRows


/*!
Reads "kPreferences_TagSixelGraphicsEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(2017.11)
*/
Boolean
My_ScreenBuffer::
returnSixelGraphics		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagSixelGraphicsEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnSixelGraphics


/*!
Reads "kPreferences_TagTextEncodingIANAName" or
"kPreferences_TagTextEncodingID" from a Preferences context,
and returns either that value or the default of UTF-8 if none
was found.

(4.0)
*/
CFStringEncoding
My_ScreenBuffer::
returnTextEncoding		(Preferences_ContextRef		inTranslationConfig)
{
	CFStringEncoding	result = kCFStringEncodingUTF8;
	
	
	result = TextTranslation_ContextReturnEncoding(inTranslationConfig, result/* default */);
	return result;
}// returnTextEncoding


/*!
Reads "kPreferences_TagXTerm256ColorsEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(4.0)
*/
Boolean
My_ScreenBuffer::
returnXTerm256	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagXTerm256ColorsEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnXTerm256


/*!
Reads "kPreferences_TagXTermBackgroundColorEraseEnabled" from
a Preferences context, and returns either that value or the
default of true if none was found.

(4.0)
*/
Boolean
My_ScreenBuffer::
returnXTermBackgroundColorErase		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagXTermBackgroundColorEraseEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnXTermBackgroundColorErase


/*!
Reads "kPreferences_TagXTermReportedPatchLevel" from a Preferences
context, and returns either that value or the default value if
none was found.

(2018.01)
*/
UInt16
My_ScreenBuffer::
returnXTermPatchLevel	(Preferences_ContextRef		inTerminalConfig,
						 Boolean					inFallBackToDefaults)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	UInt16					result = 95; // arbitrary default (minimum defined by XTerm)
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagXTermReportedPatchLevel,
												sizeof(result), &result, inFallBackToDefaults);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "screen buffer failed to read XTerm patch level from preferences, error", prefsResult);
	}
	return result;
}// returnXTermPatchLevel


/*!
Reads "kPreferences_TagXTermWindowAlterationEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(4.0)
*/
Boolean
My_ScreenBuffer::
returnXTermWindowAlteration		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagXTermWindowAlterationEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnXTermWindowAlteration


/*!
Translates the specified buffer into Unicode (from the input
text encoding of the terminal), and echoes it to the screen.
Also pipes the data to additional targets, such as spoken voice,
if so enabled in the terminal.

This implementation expects to be used with terminals whose
state determinants will pre-filter the data stream to not have
control characters, etc.  It will ignore text that it cannot
translate correctly!

Returns the number of characters successfully echoed.

(4.0)
*/
UInt32
My_DefaultEmulator::
echoData	(My_ScreenBufferPtr		inDataPtr,
			 UInt8 const*			inBuffer,
			 UInt32					inLength)
{
	UInt32		result = inLength;
	
	
	if (inLength > 0)
	{
		CFIndex				bytesRequired = 0;
		CFRetainRelease		bufferAsCFString(TextTranslation_PersistentCFStringCreate
												(kCFAllocatorDefault, inBuffer, inLength, inDataPtr->emulator.inputTextEncoding,
													false/* is external representation */, bytesRequired, inLength/* maximum trim/repeat */),
												CFRetainRelease::kAlreadyRetained);
		
		
		if (false == bufferAsCFString.exists())
		{
			// TEMPORARY: this should probably be handled better
			++(inDataPtr->translationErrorCount);
			result = 0;
		}
		else
		{
			// send the data wherever it needs to go
			echoCFString(inDataPtr, bufferAsCFString.returnCFStringRef());
			
			// speech implementation; TEMPORARY: handled here because the spoken text must
			// have access to a post-translation string, free of any meta-characters that
			// might have been in the original text stream; but text typically arrives too
			// irregularly and too quickly to allow a useful sentence to be spoken, so it
			// may be necessary to capture lines and speak them via a timer in a more
			// orderly fashion
			unless (TerminalSpeaker_IsMuted(inDataPtr->speaker) || TerminalSpeaker_IsGloballyMuted())
			{
				Boolean		doSpeak = false;
				
				
				switch (inDataPtr->speech.mode)
				{
				case kTerminal_SpeechModeSpeakAlways:
					doSpeak = true;
					break;
				
				case kTerminal_SpeechModeSpeakWhenActive:
					//doSpeak = IsWindowHilited(the screen window);
					break;
				
				case kTerminal_SpeechModeSpeakWhenInactive:
					//doSpeak = !IsWindowHilited(the screen window);
					break;
				
				case kTerminal_SpeechModeSpeakNever:
				default:
					doSpeak = false;
					break;
				}
				
				if (doSpeak)
				{
					TerminalSpeaker_Result		speakerResult = kTerminalSpeaker_ResultOK;
					
					
					// TEMPORARY - queue this, to keep asynchronous speech from jumbling or interrupting multi-line text
					// (and to improve performance as a result)
					speakerResult = TerminalSpeaker_SynthesizeSpeechFromCFString(inDataPtr->speaker, bufferAsCFString.returnCFStringRef());
					if (kTerminalSpeaker_ResultSpeechSynthesisTryAgain == speakerResult)
					{
						// error...
					}
				}
			}
		}
	}
	return result;
}// My_DefaultEmulator::echoData


/*!
A standard "My_EmulatorResetProcPtr" to reset shared settings.
Should generally be invoked by the reset routines of all other
terminals.

(4.0)
*/
void
My_DefaultEmulator::
hardSoftReset	(My_EmulatorPtr		UNUSED_ARGUMENT(inEmulatorPtr),
				 Boolean			UNUSED_ARGUMENT(inIsSoftReset))
{
	// debug
	//Console_WriteValue("    <<< default emulator has reset, soft", inIsSoftReset);
}// hardSoftReset


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
default states based on the characters of the given buffer.

This routine can be used to automatically handle a huge
variety of state transitions for your specific terminal
type.  It understands a lot of common encoding schemes,
so you need only define your emulator-specific states to
have generic values (like "kMy_ParserStateSeenESCA") and
let your emulator-specific state determinant use this
routine as a fallback for MOST input values!

When this routine - based on the data stream alone - sees
a pattern it recognizes, it chooses the next generic
state that matches the pattern.  If your emulator-specific
state matches that generic state, you will see this in
your code as a transition to your emulator-specific state
and can therefore react in an emulator-specific way within
your state transition callback.

IMPORTANT:	Even if this routine can handle a sequence
			applicable to your terminal type, it will do
			so entirely on a raw data basis.  This will
			sometimes not be enough.  Ensure this routine
			is the *fallback*, not the primary, in your
			emulator-specific state determinant so that
			you can override how any state is handled.

(3.1)
*/
UInt32
My_DefaultEmulator::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				UNUSED_ARGUMENT(outHandled))
{
	UInt32 const			kTriggerChar = inEmulatorPtr->recentCodePoint(); // for convenience; usually only first character matters
	// if no specific next state seems appropriate, the character will either
	// be printed (if possible) or be re-evaluated from the initial state
	My_ParserState const	kDefaultNextState = kMy_ParserStateAccumulateForEcho;
	UInt32					result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// by default, the state does not change
	inNowOutNext.second = inNowOutNext.first;
	
	// some characters are independent of the current state by default
	switch (kTriggerChar)
	{
	case '\007':
		inNowOutNext.second = kMy_ParserStateSeenControlG;
		break;
	
	case '\033':
		inNowOutNext.second = kMy_ParserStateSeenESC;
		break;
	
	default:
		// most characters however are not sufficient by themselves to
		// determine a new state; the current state must be checked too
		switch (inNowOutNext.first)
		{
		case kMy_ParserStateInitial:
		case kMy_ParserStateAccumulateForEcho:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		
		case kMy_ParserStateSeenESC:
			switch (kTriggerChar)
			{
			case '[':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracket;
				break;
			
			case ']':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket;
				break;
			
			case '(':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen;
				break;
			
			case ')':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen;
				break;
			
			case '}':
				inNowOutNext.second = kMy_ParserStateSeenESCRightBrace;
				break;
			
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCB;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCC;
				break;
			
			case 'c':
				inNowOutNext.second = kMy_ParserStateSeenESCc;
				break;
			
			case 'D':
				inNowOutNext.second = kMy_ParserStateSeenESCD;
				break;
			
			case 'E':
				inNowOutNext.second = kMy_ParserStateSeenESCE;
				break;
			
			case 'F':
				inNowOutNext.second = kMy_ParserStateSeenESCF;
				break;
			
			case 'G':
				inNowOutNext.second = kMy_ParserStateSeenESCG;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCH;
				break;
			
			case 'I':
				inNowOutNext.second = kMy_ParserStateSeenESCI;
				break;
			
			case 'J':
				inNowOutNext.second = kMy_ParserStateSeenESCJ;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCK;
				break;
			
			case 'M':
				inNowOutNext.second = kMy_ParserStateSeenESCM;
				break;
			
			case 'n':
				inNowOutNext.second = kMy_ParserStateSeenESCn;
				break;
			
			case 'o':
				inNowOutNext.second = kMy_ParserStateSeenESCo;
				break;
			
			case 'P':
				inNowOutNext.second = kMy_ParserStateSeenESCP;
				break;
			
			case 'Y':
				inNowOutNext.second = kMy_ParserStateSeenESCY;
				break;
			
			case 'Z':
				inNowOutNext.second = kMy_ParserStateSeenESCZ;
				break;
			
			case '7':
				inNowOutNext.second = kMy_ParserStateSeenESC7;
				break;
			
			case '8':
				inNowOutNext.second = kMy_ParserStateSeenESC8;
				break;
			
			case '*':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk;
				break;
			
			case '+':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus;
				break;
			
			case '#':
				inNowOutNext.second = kMy_ParserStateSeenESCPound;
				break;
			
			case '%':
				inNowOutNext.second = kMy_ParserStateSeenESCPercent;
				break;
			
			case '~':
				inNowOutNext.second = kMy_ParserStateSeenESCTilde;
				break;
			
			case '=':
				inNowOutNext.second = kMy_ParserStateSeenESCEquals;
				break;
			
			case '<':
				inNowOutNext.second = kMy_ParserStateSeenESCLessThan;
				break;
			
			case '>':
				inNowOutNext.second = kMy_ParserStateSeenESCGreaterThan;
				break;
			
			case '\\':
				inNowOutNext.second = kMy_ParserStateSeenESCBackslash;
				break;
			
			case '|':
				inNowOutNext.second = kMy_ParserStateSeenESCVerticalBar;
				break;
			
			case ' ':
				inNowOutNext.second = kMy_ParserStateSeenESCSpace;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCLeftSqBracket:
			// these cases are only fallbacks; in general, more specific emulators should
			// already be catching the terminator types that they consider to be valid
			switch (kTriggerChar)
			{
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsB;
				break;
			
			case 'c':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsc;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsC;
				break;
			
			case 'd':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsd;
				break;
			
			case 'D':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsD;
				break;
			
			case 'E':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsE;
				break;
			
			case 'f':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsf;
				break;
			
			case 'F':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsF;
				break;
			
			case 'g':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsg;
				break;
			
			case 'G':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsG;
				break;
			
			case 'h':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsh;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsH;
				break;
			
			case 'i':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsi;
				break;
			
			case 'I':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsI;
				break;
			
			case 'J':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsJ;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsK;
				break;
			
			case 'l':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsl;
				break;
			
			case 'L':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsL;
				break;
			
			case 'm':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsm;
				break;
			
			case 'M':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsM;
				break;
			
			case 'n':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsn;
				break;
			
			case 'P':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsP;
				break;
			
			case 'q':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsq;
				break;
			
			case 'r':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsr;
				break;
			
			case 's':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamss;
				break;
			
			case 'u':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsu;
				break;
			
			case 'x':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsx;
				break;
			
			case 'X':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsX;
				break;
			
			case 'Z':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsZ;
				break;
			
			case '@':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsAt;
				break;
			
			case '`':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsBackquote;
				break;
			
			case '$':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsDollarSign;
				break;
			
			case '\"':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsQuotes;
				break;
			
			case '>':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketGreaterThan;
				break;
			
			case '=':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketEquals;
				break;
			
			case '?':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketQuestionMark;
				break;
			
			case '!':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketExPoint;
				break;
			
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketSemicolon;
				break;
			
			case ' ':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsSpace;
				break;
			
			default:
				// more specific emulators should detect parameter terminators that they care about
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-[", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCLeftSqBracketExPoint:
			switch (kTriggerChar)
			{
			case 'p':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketExPointp;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-!", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCAsterisk:
			switch (kTriggerChar)
			{
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskB;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskC;
				break;
			
			case 'E':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskE;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskH;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskK;
				break;
			
			case 'Q':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskQ;
				break;
			
			case 'R':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskR;
				break;
			
			case 'Y':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskY;
				break;
			
			case 'Z':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskZ;
				break;
			
			case '0':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk0;
				break;
			
			case '1':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk1;
				break;
			
			case '2':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk2;
				break;
			
			case '4':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk4;
				break;
			
			case '5':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk5;
				break;
			
			case '6':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk6;
				break;
			
			case '7':
				inNowOutNext.second = kMy_ParserStateSeenESCAsterisk7;
				break;
			
			case '<':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskLessThan;
				break;
			
			case '=':
				inNowOutNext.second = kMy_ParserStateSeenESCAsteriskEquals;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-*", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCPlus:
			switch (kTriggerChar)
			{
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusB;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusC;
				break;
			
			case 'E':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusE;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusH;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusK;
				break;
			
			case 'Q':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusQ;
				break;
			
			case 'R':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusR;
				break;
			
			case 'Y':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusY;
				break;
			
			case 'Z':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusZ;
				break;
			
			case '0':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus0;
				break;
			
			case '1':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus1;
				break;
			
			case '2':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus2;
				break;
			
			case '4':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus4;
				break;
			
			case '5':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus5;
				break;
			
			case '6':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus6;
				break;
			
			case '7':
				inNowOutNext.second = kMy_ParserStateSeenESCPlus7;
				break;
			
			case '<':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusLessThan;
				break;
			
			case '=':
				inNowOutNext.second = kMy_ParserStateSeenESCPlusEquals;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-+", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCLeftParen:
			switch (kTriggerChar)
			{
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenB;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenC;
				break;
			
			case 'E':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenE;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenH;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenK;
				break;
			
			case 'Q':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenQ;
				break;
			
			case 'R':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenR;
				break;
			
			case 'Y':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenY;
				break;
			
			case 'Z':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenZ;
				break;
			
			case '0':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen0;
				break;
			
			case '1':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen1;
				break;
			
			case '2':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen2;
				break;
			
			case '4':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen4;
				break;
			
			case '5':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen5;
				break;
			
			case '6':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen6;
				break;
			
			case '7':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParen7;
				break;
			
			case '<':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenLessThan;
				break;
			
			case '=':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftParenEquals;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-(", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightParen:
			switch (kTriggerChar)
			{
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenB;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenC;
				break;
			
			case 'E':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenE;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenH;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenK;
				break;
			
			case 'Q':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenQ;
				break;
			
			case 'R':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenR;
				break;
			
			case 'Y':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenY;
				break;
			
			case 'Z':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenZ;
				break;
			
			case '0':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen0;
				break;
			
			case '1':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen1;
				break;
			
			case '2':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen2;
				break;
			
			case '4':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen4;
				break;
			
			case '5':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen5;
				break;
			
			case '6':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen6;
				break;
			
			case '7':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParen7;
				break;
			
			case '<':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenLessThan;
				break;
			
			case '=':
				inNowOutNext.second = kMy_ParserStateSeenESCRightParenEquals;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-)", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket:
			switch (kTriggerChar)
			{
			case '0':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket0;
				break;
			
			case '1':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket1;
				break;
			
			case '2':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket2;
				break;
			
			case '3':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket3;
				break;
			
			case '4':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket4;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket0:
			switch (kTriggerChar)
			{
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket0Semi;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket1:
			switch (kTriggerChar)
			{
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket1Semi;
				break;
			
			case '3':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket13;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket13:
			switch (kTriggerChar)
			{
			case '3':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket133;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket133:
			switch (kTriggerChar)
			{
			case '7':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket1337;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket1337:
			switch (kTriggerChar)
			{
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket1337Semi;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket2:
			switch (kTriggerChar)
			{
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket2Semi;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket3:
			switch (kTriggerChar)
			{
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket3Semi;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCRightSqBracket4:
			switch (kTriggerChar)
			{
			case ';':
				inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket4Semi;
				break;
			
			default:
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCPound:
			switch (kTriggerChar)
			{
			case '3':
				inNowOutNext.second = kMy_ParserStateSeenESCPound3;
				break;
			
			case '4':
				inNowOutNext.second = kMy_ParserStateSeenESCPound4;
				break;
			
			case '5':
				inNowOutNext.second = kMy_ParserStateSeenESCPound5;
				break;
			
			case '6':
				inNowOutNext.second = kMy_ParserStateSeenESCPound6;
				break;
			
			case '8':
				inNowOutNext.second = kMy_ParserStateSeenESCPound8;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-#", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCPercent:
			switch (kTriggerChar)
			{
			case 'G':
				inNowOutNext.second = kMy_ParserStateSeenESCPercentG;
				break;
			
			case '@':
				inNowOutNext.second = kMy_ParserStateSeenESCPercentAt;
				break;
			
			case '/':
				inNowOutNext.second = kMy_ParserStateSeenESCPercentSlash;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-%", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCPercentSlash:
			switch (kTriggerChar)
			{
			case 'G':
				inNowOutNext.second = kMy_ParserStateSeenESCPercentSlashG;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCPercentSlashH;
				break;
			
			case 'I':
				inNowOutNext.second = kMy_ParserStateSeenESCPercentSlashI;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-%", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		case kMy_ParserStateSeenESCSpace:
			switch (kTriggerChar)
			{
			case 'F':
				inNowOutNext.second = kMy_ParserStateSeenESCSpaceF;
				break;
			
			case 'G':
				inNowOutNext.second = kMy_ParserStateSeenESCSpaceG;
				break;
			
			default:
				//Console_Warning(Console_WriteValueUnicodePoint, "terminal received unknown character following escape-space", kTriggerChar);
				inNowOutNext.second = kDefaultNextState;
				result = 0; // do not absorb the unknown
				break;
			}
			break;
		
		default:
			// unknown state!
			//Console_Warning(Console_WriteValueUnicodePoint, "terminal entered unknown state; choosing a valid state based on character", kTriggerChar);
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< default in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     default proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        default bases this at least on character", kTriggerChar);
	
	return result;
}// My_DefaultEmulator::stateDeterminant


/*!
Every "My_EmulatorStateTransitionProcPtr" callback should
default to the result of invoking this routine with its
arguments.

(3.1)
*/
UInt32
My_DefaultEmulator::
stateTransition		(My_ScreenBufferPtr			UNUSED_ARGUMENT(inDataPtr),
					 My_ParserStatePair const&	UNUSED_ARGUMENT(inOldNew),
					 Boolean&					UNUSED_ARGUMENT(outHandled))
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	//Console_Warning(Console_WriteValueFourChars, "no known actions associated with new terminal state", inOldNew.second);
	// the trigger character would also be skipped in this case
	
	return result;
}// My_DefaultEmulator::stateTransition


/*!
Translates the specified buffer into Unicode (from the input
text encoding of the terminal), and echoes a representation of
those bytes to the stream.

By definition, a dumb terminal can render any byte, as nothing
is “special” or invisible.

Returns the number of characters successfully echoed.

(4.0)
*/
UInt32
My_DumbTerminal::
echoData	(My_ScreenBufferPtr		inDataPtr,
			 UInt8 const*			inBuffer,
			 UInt32					inLength)
{
	UInt32		result = inLength;
	
	
	if (inLength > 0)
	{
		CFIndex				bytesRequired = 0;
		CFRetainRelease		bufferAsCFString(TextTranslation_PersistentCFStringCreate
												(kCFAllocatorDefault, inBuffer, inLength, inDataPtr->emulator.inputTextEncoding,
													false/* is external representation */, bytesRequired, inLength/* maximum trim/repeat */),
												CFRetainRelease::kAlreadyRetained);
		CFRetainRelease		humanReadableCFString(CFStringCreateMutable(kCFAllocatorDefault, 0/* maximum length or 0 for no limit */),
													CFRetainRelease::kAlreadyRetained);
		UniChar*			deletedBufferPtr = nullptr;
		
		
		if (false == bufferAsCFString.exists())
		{
			// TEMPORARY: this should probably be handled better
			++(inDataPtr->translationErrorCount);
			
			// echo a single byte so that it will be skipped next time
			CFStringAppendFormat(humanReadableCFString.returnCFMutableStringRef(), nullptr/* format options */,
									CFSTR("<!%u>"), STATIC_CAST(*inBuffer, unsigned int));
			result = 1;
		}
		else
		{
			CFIndex const		kLength = CFStringGetLength(bufferAsCFString.returnCFStringRef());
			UniChar const*		bufferIterator = nullptr;
			UniChar const*		bufferPtr = CFStringGetCharactersPtr(bufferAsCFString.returnCFStringRef());
			CFIndex				characterIndex = 0;
			
			
			if (nullptr == bufferPtr)
			{
				// not ideal, but if the internal buffer is not a Unicode array,
				// it must be copied before it can be interpreted that way
				deletedBufferPtr = new UniChar[kLength];
				CFStringGetCharacters(bufferAsCFString.returnCFStringRef(), CFRangeMake(0, kLength), deletedBufferPtr);
				bufferPtr = deletedBufferPtr;
			}
			
			// create a printable interpretation of every character
			bufferIterator = bufferPtr;
			while (characterIndex < kLength)
			{
				if (gDumbTerminalRenderings().end() != gDumbTerminalRenderings().find(*bufferIterator))
				{
					// print whatever was registered as the proper rendering
					CFStringAppend(humanReadableCFString.returnCFMutableStringRef(),
									gDumbTerminalRenderings()[*bufferIterator].returnCFStringRef());
				}
				else
				{
					// print the numerical value, e.g. 200 becomes "<200>"
					CFStringAppendFormat(humanReadableCFString.returnCFMutableStringRef(), nullptr/* format options */,
											CFSTR("<%u>"), STATIC_CAST(*bufferIterator, unsigned int));
				}
				++bufferIterator;
				++characterIndex;
			}
		}
		
		// send the data wherever it needs to go
		echoCFString(inDataPtr, humanReadableCFString.returnCFStringRef());
		
		if (nullptr != deletedBufferPtr)
		{
			delete [] deletedBufferPtr, deletedBufferPtr = nullptr;
		}
	}
	return result;
}// My_DumbTerminal::echoData


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
dumb-terminal states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_DumbTerminal::
stateDeterminant	(My_EmulatorPtr			UNUSED_ARGUMENT(inEmulatorPtr),
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				UNUSED_ARGUMENT(outHandled))
{
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// dumb terminals echo everything
	inNowOutNext.second = kMy_ParserStateAccumulateForEcho;
	result = 0; // do not absorb, it will be handled by the emulator loop
	
	// debug
	//Console_WriteValueFourChars("    <<< dumb terminal in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     dumb terminal proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        dumb terminal bases this at least on character", kTriggerChar);
	
	return result;
}// My_DumbTerminal::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds
to dumb terminal state changes.

(3.1)
*/
UInt32
My_DumbTerminal::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	result = invokeEmulatorStateTransitionProc
				(My_DefaultEmulator::stateTransition, inDataPtr,
					inOldNew, outHandled);
	
	return result;
}// My_DumbTerminal::stateTransition


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
iTerm2-specific states based on the characters of the
given buffer.

(2017.12)
*/
UInt32
My_ITermCore::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	switch (inNowOutNext.first)
	{
	case kStateITermCustomBegin:
		inNowOutNext.second = kStateITermAcquireStr;
		result = 0; // do not absorb the unknown
		break;
	
	case kStateITermAcquireStr:
		switch (kTriggerChar)
		{
		case '\007':
			inNowOutNext.second = kStateITermStringTerminator;
			break;
		
		case '\033':
			inNowOutNext.second = kMy_ParserStateSeenESC;
			break;
		
		default:
			// continue extending the string until a known terminator is found
			inNowOutNext.second = kStateITermAcquireStr;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	default:
		// other states are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	// debug
	//Console_WriteValueFourChars("    <<< iTerm handler in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     iTerm handler proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        iTerm handler bases this at least on character", kTriggerChar);
	
	return result;
}// My_ITermCore::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds to
iTerm2-specific state changes.

(2017.12)
*/
UInt32
My_ITermCore::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateITermCustomBegin:
		{
			if (DebugInterface_LogsTerminalState())
			{
				Console_WriteLine("preparing to read iTerm data"); // debug
			}
			
			inDataPtr->emulator.stringAccumulator.clear();
			inDataPtr->emulator.stringAccumulatorState = inOldNew.second;
		}
		break;
	
	case kStateITermAcquireStr:
		{
			if (DebugInterface_LogsTerminalState())
			{
				Console_WriteLine("reading iTerm data"); // debug
			}
			
			if (inDataPtr->emulator.isUTF8Encoding)
			{
				if (UTF8Decoder_StateMachine::kStateUTF8ValidSequence == inDataPtr->emulator.multiByteDecoder.returnState())
				{
					std::copy(inDataPtr->emulator.multiByteDecoder.multiByteAccumulator.begin(),
								inDataPtr->emulator.multiByteDecoder.multiByteAccumulator.end(),
								std::back_inserter(inDataPtr->emulator.stringAccumulator));
					result = 1;
				}
			}
			else
			{
				inDataPtr->emulator.stringAccumulator.push_back(STATIC_CAST(inDataPtr->emulator.recentCodePoint(), UInt8));
				result = 1;
			}
		}
		break;
	
	case kStateITermStringTerminator:
		// when custom data stream is complete, process the contents of the buffer
		switch (inDataPtr->emulator.stringAccumulatorState)
		{
		case kStateITermCustomBegin:
			{
				if (DebugInterface_LogsTerminalState())
				{
					Console_WriteLine("stopped reading iTerm data"); // debug
				}
				
				// this format is documented at: "http://www.iterm2.com/documentation-images.html"
				// INCOMPLETE
				NSString*			asNSString = [[[NSString alloc] initWithBytes:inDataPtr->emulator.stringAccumulator.c_str()
																					length:inDataPtr->emulator.stringAccumulator.size()
																					encoding:NSUTF8StringEncoding] autorelease];
				NSMutableArray*		equalsSeparated = [[[asNSString componentsSeparatedByString:@"="] mutableCopy] autorelease];
				
				
				if (equalsSeparated.count < 2)
				{
					Console_Warning(Console_WriteValue, "unable to parse; expected key=value but fragment count", equalsSeparated.count);
				}
				else
				{
					id			objectKey = [equalsSeparated objectAtIndex:0];
					assert([objectKey isKindOfClass:NSString.class]);
					NSString*	stringKey = STATIC_CAST(objectKey, NSString*);
					NSString*	stringValue = nil; // set below
					
					
					[equalsSeparated removeObjectAtIndex:0];
					stringValue = [equalsSeparated componentsJoinedByString:@"="]; // allow equal signs in the remainder
					
					if ([stringKey isEqualToString:@"File"] || [stringKey isEqualToString:@"file"])
					{
						// custom iTerm2 sequence for downloading file data
						NSArray*	colonSeparated = [stringValue componentsSeparatedByString:@":"];
						
						
						if (colonSeparated.count != 2)
						{
							Console_Warning(Console_WriteValue, "expected two colon-separated fragments but saw count", colonSeparated.count);
						}
						else
						{
							id			objectEntry0 = [colonSeparated objectAtIndex:0];
							id			objectEntry1 = [colonSeparated objectAtIndex:1];
							assert([objectEntry0 isKindOfClass:NSString.class]);
							assert([objectEntry1 isKindOfClass:NSString.class]);
							NSString*	argumentsString = STATIC_CAST(objectEntry0, NSString*);
							NSArray*	semicolonSeparated = [argumentsString componentsSeparatedByString:@";"];
							NSString*	dataString = STATIC_CAST(objectEntry1, NSString*);
							UInt16		totalPixelsH = 0; // reset below
							UInt16		totalPixelsV = 0; // reset below
							UInt16		defaultCellPixelsH = 9; // reset below
							UInt16		defaultCellPixelsV = 12; // reset below
							UInt16		suggestedCellCountH = 0;
							UInt16		suggestedCellCountV = 0;
							Boolean		isBase64 = true; // see below
							Boolean		dumpImage = false;
							Boolean		ignoreAspectRatio = false;
							
							
							for (id argObject in semicolonSeparated)
							{
								assert([argObject isKindOfClass:NSString.class]);
								NSString*			argString = STATIC_CAST(argObject, NSString*);
								NSMutableArray*		argKeyValue = [[[argString componentsSeparatedByString:@"="] mutableCopy] autorelease];
								
								
								if (argKeyValue.count < 2)
								{
									Console_Warning(Console_WriteValueCFString, "expected key=value; unable to process string", BRIDGE_CAST(argumentsString, CFStringRef));
								}
								else
								{
									id			objectArgName = [argKeyValue objectAtIndex:0];
									assert([objectArgName isKindOfClass:NSString.class]);
									NSString*	argName = STATIC_CAST(objectArgName, NSString*);
									NSString*	argValue = nil; // see below
									
									
									[argKeyValue removeObjectAtIndex:0];
									argValue = [argKeyValue componentsJoinedByString:@"="]; // allow equal signs in the value itself
									
									//Console_WriteValueCFString("received argument name", BRIDGE_CAST(argName, CFStringRef)); // debug
									//Console_WriteValueCFString("received argument value", BRIDGE_CAST(argValue, CFStringRef)); // debug
									
									if ([argName isEqualToString:@"name"])
									{
										// the name is actually a string encoded in base64
										NSData*		decodedName = [[[NSData alloc] initWithBase64EncodingOSImplementation:argValue]
																	autorelease];
										NSString*	decodedString = [[NSString alloc] initWithData:decodedName encoding:NSUTF8StringEncoding];
										
										
										if (nil == decodedString)
										{
											Console_Warning(Console_WriteLine, "failed to decode base64 for 'name' argument");
										}
										else
										{
											SessionRef		session = returnListeningSession(inDataPtr);
											
											
											if (nil == session)
											{
												Console_Warning(Console_WriteValueCFString, "no session found for terminal; ignoring file name",
																BRIDGE_CAST(decodedString, CFStringRef));
											}
											else
											{
												Session_DisplayFileDownloadNameUI(session, BRIDGE_CAST(decodedString, CFStringRef));
											}
										}
									}
									else if ([argName isEqualToString:@"inline"])
									{
										if ([argValue isEqualToString:@"1"] || [argValue isEqualToString:@"true"])
										{
											dumpImage = true;
										}
									}
									else if ([argName isEqualToString:@"size"])
									{
										// total image data size in bytes; not used right now
										Console_Warning(Console_WriteLine, "ignored argument 'size'");
									}
									else if ([argName isEqualToString:@"preserveAspectRatio"])
									{
										if ([argValue isEqualToString:@"0"] || [argValue isEqualToString:@"false"])
										{
											ignoreAspectRatio = true;
											Console_WriteLine("ignoring image aspect ratio"); // debug
										}
										else
										{
											ignoreAspectRatio = false;
										}
									}
									else if ([argName isEqualToString:@"width"] || [argName isEqualToString:@"height"])
									{
										Boolean		isWidth = [argName isEqualToString:@"width"]; // otherwise, height
										
										
										if ([argValue isEqualToString:@"auto"])
										{
											// implied for zero values (see below)
											if (isWidth)
											{
												totalPixelsH = 0;
											}
											else
											{
												totalPixelsV = 0;
											}
										}
										else
										{
											if ([argValue hasSuffix:@"px"])
											{
												// number of pixels
												NSScanner*		valueScanner = [NSScanner scannerWithString:argValue];
												NSString*		subValue = nil;
												
												
												if ([valueScanner scanUpToString:@"px" intoString:&subValue])
												{
													UInt16 const	kPixelCount = STATIC_CAST([subValue integerValue], UInt16);
													
													
													if (isWidth)
													{
														totalPixelsH = kPixelCount;
													}
													else
													{
														totalPixelsV = kPixelCount;
													}
													
													//Console_WriteValue("set image pixel width/height", kPixelCount); // debug
												}
											}
											else if ([argValue hasSuffix:@"%"])
											{
												// INCOMPLETE; percent not supported for arbitrary values but it is
												// easy to support some common absolute percentages
												if ([argValue isEqualToString:@"100%"])
												{
													if (isWidth)
													{
														suggestedCellCountH = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
													}
													else
													{
														suggestedCellCountV = inDataPtr->screenBuffer.size();
													}
												}
												else
												{
													Console_Warning(Console_WriteValueCFString, "ignored unusual width/height percentage value (try '100%')", BRIDGE_CAST(argValue, CFStringRef));
												}
											}
											else
											{
												// number of ordinary cells to occupy; this is handled by scaling
												// the “pixels per cell” later, when the image dimensions are final
												UInt16 const	kCellCount = STATIC_CAST([argValue integerValue], UInt16);
												
												
												if (isWidth)
												{
													suggestedCellCountH = kCellCount;
												}
												else
												{
													suggestedCellCountV = kCellCount;
												}
												
												//Console_WriteValue("set image cell count across/down", kCellCount); // debug
											}
										}
									}
									else
									{
										// ???
										Console_Warning(Console_WriteValueCFString, "ignored argument with name", BRIDGE_CAST(argName, CFStringRef));
										// INCOMPLETE; process arguments
									}
								}
							}
							
							if (isBase64)
							{
								NSData*		decodedData = [[[NSData alloc] initWithBase64EncodingOSImplementation:dataString]
															autorelease];
								
								
								if (nil == decodedData)
								{
									Console_Warning(Console_WriteLine, "unable to process given string; expected base64 format");
								}
								else
								{
									if (dumpImage)
									{
										Boolean const	kScrollWithImage = true;
										Boolean const	kRestoreCursor = false;
										NSImage*		decodedImage = [[[NSImage alloc] initWithData:decodedData] autorelease];
										
										
										if (0 == totalPixelsH)
										{
											// automatically determine width
											totalPixelsH = STATIC_CAST(decodedImage.size.width, UInt16);
										}
										
										if (0 == totalPixelsV)
										{
											// automatically determine height
											totalPixelsV = STATIC_CAST(decodedImage.size.height, UInt16);
										}
										
										if (false == ignoreAspectRatio)
										{
											Console_Warning(Console_WriteLine, "preservation of aspect ratio is not implemented");
										}
										
										if (0 != suggestedCellCountH)
										{
											// use given number of cells across
											defaultCellPixelsH = std::max< UInt16 >(1, STATIC_CAST(roundf(STATIC_CAST(totalPixelsH, Float32) /
																											STATIC_CAST(suggestedCellCountH, Float32)),
																									UInt16));
										}
										
										if (0 != suggestedCellCountV)
										{
											// use given number of cells down
											defaultCellPixelsV = std::max< UInt16 >(1, STATIC_CAST(roundf(STATIC_CAST(totalPixelsV, Float32) /
																											STATIC_CAST(suggestedCellCountV, Float32)),
																									UInt16));
										}
										
										bufferInsertInlineImageWithoutUpdate(inDataPtr, decodedImage, totalPixelsH, totalPixelsV,
																				defaultCellPixelsH, defaultCellPixelsV,
																				kScrollWithImage, kRestoreCursor);
									}
									else
									{
										Console_Warning(Console_WriteLine, "recognized and decoded image but file downloading is not implemented; try using 'inline=1'");
									}
								}
							}
						}
					}
					else
					{
						// ???
						Console_Warning(Console_WriteValueCFString, "unable to process given string; unsupported key", BRIDGE_CAST(stringKey, CFStringRef));
					}
				}
				
				inDataPtr->emulator.stringAccumulator.clear();
				inDataPtr->emulator.stringAccumulatorState = kMy_ParserStateInitial;
			}
			break;
		
		default:
			// ignore
			outHandled = false;
			break;
		}
		break;
	
	default:
		// other state transitions are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	return result;
}// My_ITermCore::stateTransition


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
Sixel-specific graphics states based on the characters of
the given buffer.

(2017.11)
*/
UInt32
My_SixelCore::
stateDeterminant	(My_EmulatorPtr			UNUSED_ARGUMENT(inEmulatorPtr),
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				outHandled)
{
	//UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	switch (inNowOutNext.first)
	{
	default:
		// other states are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	// debug
	//Console_WriteValueFourChars("    <<< Sixel handler in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     Sixel handler proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        Sixel handler bases this at least on character", kTriggerChar);
	
	return result;
}// My_SixelCore::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds to
Sixel-specific graphics state changes.

(2017.11)
*/
UInt32
My_SixelCore::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case My_VT100::kStateRM:
	case My_VT100::kStateSM:
		{
			Boolean const	kParameterIsSet = (My_VT100::kStateSM == inOldNew.second);
			
			
			// defer the entire set/reset handler to the actual terminal emulator
			// but check for a Sixel parameter here
			outHandled = false;
			
			switch (inDataPtr->emulator.argList[0])
			{
			case kMy_ParamPrivate: // DEC-private control sequence
				{
					SInt16		i = 0;
					
					
					for (i = 1/* skip the meta-parameter */; i <= inDataPtr->emulator.argLastIndex; ++i)
					{
						switch (inDataPtr->emulator.argList[i])
						{
						case 80:
							// in Sixel mode, request to turn scrolling on/off
							inDataPtr->emulator.allowSixelScrolling = kParameterIsSet;
							if (kParameterIsSet)
							{
								Console_WriteLine("request to allow Sixel scrolling");
							}
							else
							{
								Console_WriteLine("request to prohibit Sixel scrolling");
							}
							break;
						
						default:
							break;
						}
					}
				}
				break;
			
			default:
				break;
			}
		}
		break;
	
	case My_VT220::kStateST:
		// when a Sixel image is complete, process the contents of the buffer
		if (My_VT220::kStateDCS == inDataPtr->emulator.stringAccumulatorState)
		{
			// in order to represent a Sixel image, a device control string must
			// contain the parameter terminator "q" (i.e. ESC P <params> q)
			ParameterDecoder_StateMachine				paramDecoder;
			std::basic_string< UInt8 >::const_iterator	pastEndParams = inDataPtr->emulator.stringAccumulator.end();
			
			
			getParametersFromStringAccumulator(inDataPtr, paramDecoder, pastEndParams);
			if ((inDataPtr->emulator.stringAccumulator.end() != pastEndParams) && ('q' == *pastEndParams))
			{
				typedef decltype(inDataPtr->emulator.stringAccumulator.size())	StringAccumulatorSize;
				decltype(pastEndParams) const	kSixelDataBegin = (pastEndParams + 1/* skip terminating 'q' */);
				decltype(pastEndParams) const	kPastEndSixelData = inDataPtr->emulator.stringAccumulator.end();
				StringAccumulatorSize const		kSixelDataSize = std::distance(kSixelDataBegin, kPastEndSixelData);
				
				
				if (DebugInterface_LogsSixelDecoderState())
				{
					//Console_WriteLine("stopped reading Sixel data"); // debug
				}
				
				SixelDecoder_StateMachine			sixelDecoder;
				SixelDecoder_StateMachine const&	decoderRef = sixelDecoder; // so copied blocks can refer to object without copying object
				UInt16								aspectRatioParameter = (paramDecoder.parameterValues.empty()
																			? 0
																			: paramDecoder.parameterValues[0]);
				UInt16								initialAspectRatioV = 2; // see below
				UInt16								initialAspectRatioH = 1; // see below
				Boolean								zeroValuePixelsKeepColor = false; // see below
				
				
				// the following according to the VT330/VT340 manual
				switch (aspectRatioParameter)
				{
				case 2:
					initialAspectRatioV = 5;
					initialAspectRatioH = 1;
					break;
				
				case 3:
				case 4:
					initialAspectRatioV = 3;
					initialAspectRatioH = 1;
					break;
				
				case 7:
				case 8:
				case 9:
					initialAspectRatioV = 1;
					initialAspectRatioH = 1;
					break;
				
				case 0:
				case 1:
				case 5:
				case 6:
				default:
					// 2:1 (see above)
					break;
				}
				
				if (DebugInterface_LogsSixelDecoderSummary())
				{
					Console_WriteHorizontalRule();
					Console_WriteValue("default pan (Sixel aspect ratio height) set to", initialAspectRatioV);
					Console_WriteValue("default pad (Sixel aspect ratio width) set to", initialAspectRatioH);
				}
				
				// determine if “off” bits use the current color or revert to the background color
				zeroValuePixelsKeepColor = ((paramDecoder.parameterValues.size() > 0)
											? (1 == paramDecoder.parameterValues[1])
											: false);
				if (zeroValuePixelsKeepColor)
				{
					if (DebugInterface_LogsSixelDecoderSummary())
					{
						Console_WriteLine("Sixel image is configured to not change the color of zero-value pixels"); // debug
					}
				}
				
				if (paramDecoder.parameterValues.size() > 2)
				{
					if (DebugInterface_LogsSixelDecoderErrors())
					{
						Console_Warning(Console_WriteValue, "ignoring Sixel grid size parameter", paramDecoder.parameterValues[2]);
					}
				}
				
				// process the Sixel data
				// (TEMPORARY; may need to have a background thread that separates terminal
				// cell processing from image processing so that control can return to the
				// terminal while very large images are being constructed)
				{
					// NOTE: the dimensions and ratio values chosen by default are
					// based on what the VT300 series uses for 80-column mode; it
					// may eventually be good to vary these values based on the
					// purpose of input Sixel data (for instance, 132-column mode
					// apparently uses 9 pixels wide instead of 15, and if custom
					// character sets are ever supported then their default pixel
					// height is 3:1 over the width)  
					__block NSColor*				currentColor = nil; // if "nil", use background color
					__block NSMutableDictionary*	colorsByIndex = [[[NSMutableDictionary alloc] init] autorelease];
					NSImage*						completeImage = nil;
					CGContextRef					drawingContext = nullptr;
					UInt16							defaultCellPixelsH = 9; // number of dots across to define a terminal cell at normal width
					UInt16							defaultCellPixelsV = 12; // number of dots down to define a terminal cell at normal height
					UInt16							totalPixelsH = 0;
					UInt16							totalPixelsV = 0;
					UInt16							sixelSizeH = 1; // number of normal pixels occupied horizontally for each “sixel”
					UInt16							sixelSizeV = 1; // number of normal pixels occupied vertically for each “sixel”
					size_t							processedBytes = 0;
					
					
					if (DebugInterface_LogsSixelInput())
					{
						std::basic_string< UInt8 >		dataString(kSixelDataBegin, kPastEndSixelData);
						
						
						Console_WriteValueCString("accumulated string", REINTERPRET_CAST(dataString.c_str(), char const*));
					}
					
					// the exact size of the image cannot be determined without parsing
					// all of the sixel data; therefore, this is done in two passes
					// (at the end of the first pass, the configuration of the image
					// and final graphics cursor position are used to find the size)
					sixelDecoder.reset();
					sixelDecoder.aspectRatioV = initialAspectRatioV; // default only; can change if raster attributes are parsed
					sixelDecoder.aspectRatioH = initialAspectRatioH;
					for (auto toByte = kSixelDataBegin; toByte != kPastEndSixelData; ++toByte)
					{
						SixelDecoder_StateMachine::State const	kOriginalState = sixelDecoder.returnState();
						SInt16									loopGuard = 0;
						Boolean									byteNotUsed = true;
						
						
						while (byteNotUsed)
						{
							sixelDecoder.goNextState(*toByte, byteNotUsed);
							if ((byteNotUsed) && (kOriginalState == sixelDecoder.returnState()))
							{
								// no way to proceed
								break;
							}
							++loopGuard;
							if (loopGuard > 100/* arbitrary */)
							{
								break;
							}
						}
					}
					
					// now that the maximum movement of the graphics cursor and pixel aspect ratio
					// are known, specify the image dimensions (consider both the locations that
					// the graphics cursor has moved to, and any configured image size parameters;
					// the real canvas size will be whichever is larger!)
					sixelDecoder.getSixelSize(sixelSizeV, sixelSizeH);
					totalPixelsH = ((1 + sixelDecoder.graphicsCursorMaxX) * sixelSizeH);
					totalPixelsV = ((1 + sixelDecoder.graphicsCursorMaxY) * sixelSizeV * 6/* sixel has 6 bits, one per vertical plot */);
					//totalPixelsH = std::max< UInt16 >(totalPixelsH, sixelDecoder.suggestedImageWidth);
					//totalPixelsV = std::max< UInt16 >(totalPixelsV, sixelDecoder.suggestedImageHeight);
					
					// allocate a large enough bitmap image
					{
						NSInteger const		kBitsPerSample = 8; // currently, 0-255 values defined
						NSInteger const		kSamplesPerPixel = 4; // red, green, blue, alpha
						NSInteger const		kBitsPerPixel = (kBitsPerSample * kSamplesPerPixel);
						NSInteger const		kBytesPerRow = ((kBitsPerPixel / 8) * totalPixelsH);
						CGColorSpaceRef		colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
						
						
						drawingContext = CGBitmapContextCreate(nullptr/* data or nullptr to auto-allocate */, totalPixelsH, totalPixelsV, kBitsPerSample, kBytesPerRow,
																colorSpace, (kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast));
						assert(nullptr != drawingContext);
						//NSLog(@"Sixel bitmap %lf x %lf, context@%p, colorspace@%p", (double)totalPixelsH, (double)totalPixelsV, drawingContext, colorSpace); // debug
						CGColorSpaceRelease(colorSpace); colorSpace = nullptr;
						
						// transform to expected coordinate system
						CGContextTranslateCTM(drawingContext, 0, totalPixelsV); // anchor at top
						CGContextScaleCTM(drawingContext, 1.0, -1.0); // flip vertically
						
						// configure for bitmap drawing (otherwise, a line drawn at the same
						// start and end coordinates will have no rendering)
						CGContextSetLineWidth(drawingContext, 1.0);
						CGContextSetLineCap(drawingContext, kCGLineCapSquare);
						CGContextSetAllowsAntialiasing(drawingContext, false);
					}
					
					// now reset the decoder and repeat the parsing sequence,
					// this time with an appropriately-allocated image and
					// blocks of code defined; this will cause the parser to
					// call the blocks as key states are entered and ultimately
					// set colored pixels in the bitmap buffer
					sixelDecoder.reset();
					sixelDecoder.aspectRatioV = initialAspectRatioV; // default only; can change if raster attributes are parsed
					sixelDecoder.aspectRatioH = initialAspectRatioH;
					sixelDecoder.setColorCreator(^(UInt16 colorIndex, SixelDecoder_ColorType colorType,
													UInt16 component1, UInt16 component2, UInt16 component3)
					{
						// create and select non-default color
						NSNumber*	colorNumber = [NSNumber numberWithInteger:colorIndex];
						NSColor*	newColor = nil;
						
						
						switch (colorType)
						{
						case kSixelDecoder_ColorTypeHLS:
							// note: given in HLS order, not HSB order (flipping last two values)
							newColor = [NSColor colorWithCalibratedHue:(component1 / 360.0) saturation:(component3 / 100.0)
																		brightness:(component2 / 100.0) alpha:1.0];
							break;
						
						case kSixelDecoder_ColorTypeRGB:
							newColor = [NSColor colorWithCalibratedRed:(component1 / 100.0) green:(component2 / 100.0)
																		blue:(component3 / 100.0) alpha:1.0];
							break;
						
						default:
							// ???
							break;
						}
						if ((nil == newColor) || (nil == colorNumber))
						{
							if (DebugInterface_LogsSixelDecoderErrors())
							{
								Console_Warning(Console_WriteValue, "failed to define Sixel color, index", colorIndex);
							}
						}
						else
						{
							// the VT330/340 manual says that a VT300 would only fill the width/height
							// when zero-pixels are set to use the current background (as opposed to
							// keeping their previous color); this isn’t practical though because image
							// conversion utilities typically provide only the sixel data string and
							// not the introductory terminal sequence that would specify a color mode,
							// causing images to have large gaps if no background fill is performed
							//Boolean const	kAutoFillBackground = (false == zeroValuePixelsKeepColor);
							Boolean const	kAutoFillBackground = true;
							Boolean const	kFirstColor = (0 == colorsByIndex.count);
							
							
							newColor = [newColor colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
							[colorsByIndex setObject:newColor forKey:colorNumber];
							
							if (DebugInterface_LogsSixelDecoderSummary())
							{
								Console_WriteValue("Sixel color defined with index", colorIndex);
								Console_WriteValueFloat4("Sixel color components", newColor.redComponent, newColor.greenComponent, newColor.blueComponent, newColor.alphaComponent);
							}
							
							CGContextSetRGBFillColor(drawingContext, newColor.redComponent, newColor.greenComponent, newColor.blueComponent, 1.0/* alpha */);
							
							if ((kAutoFillBackground) && (kFirstColor) &&
								(decoderRef.suggestedImageWidth > 0) && (decoderRef.suggestedImageHeight > 0))
							{
								// the first time a color is defined, fill the image background
								// (the VT300 manual specifies only the region that is filled, as
								// the width/height from raster attributes; it isn’t clear what
								// “background color” should be; this interpretation is somewhat
								// arbitrary but it seems compatible with known image utilities)
								if (DebugInterface_LogsSixelDecoderSummary())
								{
									Console_WriteLine("Sixel color definition will be used for background");
								}
								
							#if 1
								// can clear suggested area (relying on Sixel-provided dimensions)
								// or erase the actual area traversed by graphics commands, which
								// is more accurate overall
								//CGContextFillRect(drawingContext, CGRectMake(0, 0, decoderRef.suggestedImageWidth * sixelSizeH, decoderRef.suggestedImageHeight * 6 * sixelSizeV));
								CGContextFillRect(drawingContext, CGRectMake(0, 0, totalPixelsH, totalPixelsV));
							#else
								// debug: provide a way to tweak every pixel (at performance cost)
								for (UInt16 i = 0; i < decoderRef.suggestedImageWidth; ++i)
								{
									auto const	kPixelX = (sixelSizeH * i);
									
									
									for (UInt16 j = 0; j < decoderRef.suggestedImageHeight; ++j)
									{
										auto const	kPixelY = (sixelSizeV * (6/* bits per sixel */ * j));
										
										
										CGContextFillRect(drawingContext, CGRectMake(kPixelX, kPixelY, sixelSizeH, 6 * sixelSizeV));
									}
								}
							#endif
							}
						}
					});
					sixelDecoder.setColorChooser(^(UInt16 index)
					{
						// select color (see also the color creator above)
						NSNumber*	colorNumber = [NSNumber numberWithInteger:index];
						
						
						currentColor = [colorsByIndex objectForKey:colorNumber];
					});
					sixelDecoder.setSixelHandler(^(UInt8 rawChar, UInt16 repeatCount)
					{
						// process a sixel (render up to 6 points in a vertical line,
						// where each position’s real pixel size depends on the aspect
						// ratio that was defined)
						std::bitset<6>	topToBottomOnOffFlags;
						
						
						SixelDecoder_StateMachine::getSixelBits(rawChar, topToBottomOnOffFlags);
						if (topToBottomOnOffFlags.none() && (zeroValuePixelsKeepColor))
						{
							// in this configuration, no pixels in the line of 6 are changing
						}
						else
						{
							// at least one pixel will change
							auto			kSixelBitCount = topToBottomOnOffFlags.size();
							CGFloat const	kForegroundRed = currentColor.redComponent;
							CGFloat const	kForegroundGreen = currentColor.greenComponent;
							CGFloat const	kForegroundBlue = currentColor.blueComponent;
							
							
							for (UInt16 i = 0; i < repeatCount; ++i)
							{
								auto const	kPixelX = (sixelSizeH * (decoderRef.graphicsCursorX + i));
								
								
								// see earlier definition; "nil" used to mean “use background” (also,
								// there is no erase phase because off-pixels simply rely on the
								// pre-filled background and/or any previous pixel at that location)
								if (nil != currentColor)
								{
									CGContextSetRGBFillColor(drawingContext, kForegroundRed, kForegroundGreen, kForegroundBlue, 1.0/* alpha */);
									if (topToBottomOnOffFlags.all())
									{
										// all 6 pixels are being drawn
										CGContextFillRect(drawingContext, CGRectMake(kPixelX, (sixelSizeV * (decoderRef.graphicsCursorY * kSixelBitCount)),
																						sixelSizeH, sixelSizeV * kSixelBitCount));
									}
									else
									{
										// some pixels are being drawn
										for (decltype(kSixelBitCount) j = 0; j < kSixelBitCount; ++j)
										{
											auto const	kPixelY = (sixelSizeV * (decoderRef.graphicsCursorY * kSixelBitCount + j));
											
											
											if (topToBottomOnOffFlags.test(j))
											{
												// draw
											#if 0
												CGContextFillRect(drawingContext, CGRectMake(kPixelX, kPixelY, sixelSizeH, sixelSizeV));
											#else
												// debug: provide a way to tweak every pixel (at performance cost)
												for (auto k = 0; k < sixelSizeV; ++k)
												{
													for (auto l = 0; l < sixelSizeH; ++l)
													{
														CGContextFillRect(drawingContext, CGRectMake((kPixelX + l), (kPixelY + k), 1, 1));
													}
												}
											#endif
											}
										}
									}
								}
							}
						}
					});
					for (auto toByte = kSixelDataBegin; toByte != kPastEndSixelData; ++toByte)
					{
						SixelDecoder_StateMachine::State const	kOriginalState = sixelDecoder.returnState();
						SInt16									loopGuard = 0;
						Boolean									byteNotUsed = true;
						
						
						while (byteNotUsed)
						{
							sixelDecoder.goNextState(*toByte, byteNotUsed);
							if (DebugInterface_LogsSixelInput())
							{
								//Console_WriteValueFourChars("Sixel parser state", sixelDecoder.returnState());
							}
							
							if ((byteNotUsed) && (kOriginalState == sixelDecoder.returnState()))
							{
								// no way to proceed
								break;
							}
							
							++loopGuard;
							if (loopGuard > 100/* arbitrary */)
							{
								Sound_StandardAlert();
								Console_Warning(Console_WriteLine, "Sixel decoder forced to break after unexpected tight loop");
								break;
							}
						}
						++processedBytes;
					}
					if (processedBytes < kSixelDataSize)
					{
						Console_Warning(Console_WriteValue, "Sixel decoder did not handle entire data string; bytes left",
										kSixelDataSize - processedBytes);
					}
					
					if (DebugInterface_LogsSixelDecoderSummary())
					{
						Console_WriteValue("final cursor position relative to Sixel image: x", sixelDecoder.graphicsCursorX);
						Console_WriteValue("final cursor position relative to Sixel image: y", sixelDecoder.graphicsCursorY);
						Console_WriteValue("apparent width in “sixels”", 1 + sixelDecoder.graphicsCursorMaxX);
						Console_WriteValue("apparent height in “sixels”", 1 + sixelDecoder.graphicsCursorMaxY);
						Console_WriteValue("final pan (aspect ratio, vertical)", sixelDecoder.aspectRatioV);
						Console_WriteValue("final pad (aspect ratio, horizontal)", sixelDecoder.aspectRatioH);
						Console_WriteValue("calculated “sixel” width (in screen pixels)", sixelSizeH);
						Console_WriteValue("calculated “sixel” height (in screen pixels)", sixelSizeV);
					}
					
					// convert image for rendering
					{
						CGImageRef	snapshotCGImage = CGBitmapContextCreateImage(drawingContext);
						
						
						completeImage = [[NSImage alloc] initWithCGImage:snapshotCGImage size:NSZeroSize/* if NSZeroSize, CGImage size is used */];
						CGImageRelease(snapshotCGImage); snapshotCGImage = nullptr;
					}
					
					// write complete image for debugging (helps to separate possible issues
					// with terminal display from issues with raw image content)
					if (DebugInterface_LogsSixelDecoderState())
					{
						NSData*		imageData = [completeImage TIFFRepresentation];
						NSString*	filePath = @"/tmp/macterm_sixel_image.tiff";
						
						
						if (NO == [[NSFileManager defaultManager] createFileAtPath:filePath contents:imageData attributes:nil])
						{
							Console_Warning(Console_WriteValueCFString, "failed to write debugging image, path", BRIDGE_CAST(filePath, CFStringRef));
						}
					}
					
					// determine the terminal cells that will need to have a bitmap association
					// scrolling is technically controlled by a terminal parameter sequence
					// but in the future it may be good to let the user force image scrolling
					Boolean const	kScrollWithImage = inDataPtr->emulator.allowSixelScrolling;
					// according to VT300 series documentation, the text cursor does not move
					// from its original position if scrolling is disabled
					Boolean const	kRestoreCursor = (false == kScrollWithImage);
					bufferInsertInlineImageWithoutUpdate(inDataPtr, completeImage, totalPixelsH, totalPixelsV,
															defaultCellPixelsH, defaultCellPixelsV,
															kScrollWithImage, kRestoreCursor);
					[completeImage release]; completeImage = nil;
				}
				
				inDataPtr->emulator.stringAccumulator.clear();
				inDataPtr->emulator.stringAccumulatorState = kMy_ParserStateInitial;
			}
			else
			{
				// not apparently Sixel data
				outHandled = false;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	default:
		// other state transitions are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	return result;
}// My_SixelCore::stateTransition


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
UTF-8-specific window states based on the characters of the
given buffer.

(4.0)
*/
UInt32
My_UTF8Core::
stateDeterminant	(My_EmulatorPtr			UNUSED_ARGUMENT(inEmulatorPtr),
					 My_ParserStatePair&	UNUSED_ARGUMENT(inNowOutNext),
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				outHandled)
{
	//UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	outHandled = false;
	
	// debug
	//Console_WriteValueFourChars("    <<< UTF-8 handler in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     UTF-8 handler proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        UTF-8 handler bases this at least on character", kTriggerChar);
	
	return result;
}// My_UTF8Core::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds to
UTF-8-specific window state changes.

(4.0)
*/
UInt32
My_UTF8Core::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition; note that
	// although certain UTF-8 state is maintained in the terminal as well,
	// character set changes are in the domain of the Session and should
	// only occur at that level (the Session API may then trigger changes
	// in the terminal emulator state anyway)
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateUTF8Off:
	case kStateUTF8OnUnspecifiedImpl:
	case kStateToUTF8Level1:
	case kStateToUTF8Level2:
	case kStateToUTF8Level3:
		// collect all of these states together so that the relatively-expensive
		// lookup of translation configuration will occur when one of these
		// states is actually entered
		{
			SessionRef				listeningSession = returnListeningSession(inDataPtr);
			Preferences_ContextRef	translationConfig = Session_IsValid(listeningSession)
														? Session_ReturnTranslationConfiguration(listeningSession)
														: nullptr;
			
			
			switch (inOldNew.second)
			{
			case kStateUTF8Off:
				if ((false == inDataPtr->emulator.lockUTF8) && (nullptr != translationConfig))
				{
					UNUSED_RETURN(Boolean)TextTranslation_ContextSetEncoding(translationConfig, kCFStringEncodingASCII/* arbitrary */, true/* copy only */);
					inDataPtr->emulator.disableShifts = false;
				}
				break;
			
			case kStateUTF8OnUnspecifiedImpl:
				if (nullptr != translationConfig)
				{
					UNUSED_RETURN(Boolean)TextTranslation_ContextSetEncoding(translationConfig, kCFStringEncodingUTF8, true/* copy only */);
					inDataPtr->emulator.disableShifts = true;
				}
				break;
			
			case kStateToUTF8Level1:
				if (nullptr != translationConfig)
				{
					UNUSED_RETURN(Boolean)TextTranslation_ContextSetEncoding(translationConfig, kCFStringEncodingUTF8, true/* copy only */);
					inDataPtr->emulator.disableShifts = true;
				}
				inDataPtr->emulator.lockUTF8 = true;
				// INCOMPLETE
				break;
			
			case kStateToUTF8Level2:
				if (nullptr != translationConfig)
				{
					UNUSED_RETURN(Boolean)TextTranslation_ContextSetEncoding(translationConfig, kCFStringEncodingUTF8, true/* copy only */);
					inDataPtr->emulator.disableShifts = true;
				}
				inDataPtr->emulator.lockUTF8 = true;
				// INCOMPLETE
				break;
			
			case kStateToUTF8Level3:
				if (nullptr != translationConfig)
				{
					UNUSED_RETURN(Boolean)TextTranslation_ContextSetEncoding(translationConfig, kCFStringEncodingUTF8, true/* copy only */);
					inDataPtr->emulator.disableShifts = true;
				}
				inDataPtr->emulator.lockUTF8 = true;
				// INCOMPLETE
				break;
			
			default:
				assert(false); // this should be impossible to reach
				break;
			}
		}
		break;
	
	default:
		// other state transitions are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	return result;
}// My_UTF8Core::stateTransition


/*!
Handles the VT100 'DECALN' sequence.  See the VT100
manual for complete details.

(3.0)
*/
void
My_VT100::
alignmentDisplay	(My_ScreenBufferPtr		inDataPtr)
{
	// first clear the buffer, saving it to scrollback if appropriate
	bufferEraseVisibleScreen(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
	
	// now fill the lines with letter-E characters; technically this
	// also will reset all attributes and this may not be part of the
	// VT100 specification (but it seems reasonable to get rid of any
	// special colors or oversized text when doing screen alignment)
	for (auto& lineInfo : inDataPtr->screenBuffer)
	{
		bufferLineFill(inDataPtr, *lineInfo, 'E', TextAttributes_Object(), true/* change line global attributes to match */);
	}
	
	// update the display - UNIMPLEMENTED
}// My_VT100::alignmentDisplay


/*!
Switches a VT100-compatible terminal to ANSI mode, which means
it no longer accepts VT52 sequences.

(3.1)
*/
void
My_VT100::
ansiMode	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->modeANSIEnabled = true;
	if (inDataPtr->emulator.pushedCallbacks.exist())
	{
		inDataPtr->emulator.currentCallbacks = inDataPtr->emulator.pushedCallbacks;
		inDataPtr->emulator.pushedCallbacks = My_Emulator::Callbacks();
	}
	inDataPtr->emulator.initializeParserStateStack();
	
	// a peculiarity about a real VT200 terminal is that returning to
	// ANSI mode from VT52 mode will cause 8-bit mode to be unselectable
	// (TEMPORARY; may want an option to not be so sticky about the
	// accuracy of the emulation in this case)
	inDataPtr->emulator.set8BitTransmitNever();
}// My_VT100::ansiMode


/*!
Handles the VT100 'CUB' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorBackward	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.argList[0] < 1)
	{
		if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
		else moveCursorLeftToEdge(inDataPtr);
	}
	else
	{
		SInt16		newValue = inDataPtr->current.cursorX - inDataPtr->emulator.argList[0];
		
		
		if (newValue < 0) newValue = 0;
		moveCursorX(inDataPtr, newValue);
	}
}// My_VT100::cursorBackward


/*!
Handles the VT100 'CUD' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorDown	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.argList[0] < 1)
	{
		if (inDataPtr->current.cursorY < inDataPtr->originRegionPtr->lastRow) moveCursorDown(inDataPtr);
		else moveCursorDownToEdge(inDataPtr);
	}
	else
	{
		My_ScreenRowIndex	newValue = inDataPtr->current.cursorY +
										inDataPtr->emulator.argList[0];
		
		
		if (newValue > inDataPtr->originRegionPtr->lastRow)
		{
			newValue = inDataPtr->originRegionPtr->lastRow;
		}
		// NOTE: the check below may not be necessary
		if (newValue >= inDataPtr->screenBuffer.size())
		{
			newValue = inDataPtr->screenBuffer.size() - 1;
		}
		moveCursorY(inDataPtr, newValue);
	}
}// My_VT100::cursorDown


/*!
Handles the VT100 'CUF' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorForward	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		rightLimit = inDataPtr->current.returnNumberOfColumnsPermitted() - ((inDataPtr->modeAutoWrap) ? 0 : 1);
	
	
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.argList[0] < 1)
	{
		if (inDataPtr->current.cursorX < rightLimit) moveCursorRight(inDataPtr);
		else moveCursorRightToEdge(inDataPtr);
	}
	else
	{
		SInt16		newValue = inDataPtr->current.cursorX + inDataPtr->emulator.argList[0];
		
		
		if (newValue > rightLimit) newValue = rightLimit;
		moveCursorX(inDataPtr, newValue);
	}
}// My_VT100::cursorForward


/*!
Handles the VT100 'CUU' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.argList[0] < 1)
	{
		if (inDataPtr->current.cursorY > inDataPtr->originRegionPtr->firstRow)
		{
			moveCursorUp(inDataPtr);
		}
		else
		{
			moveCursorUpToEdge(inDataPtr);
		}
	}
	else
	{
		SInt16				newValue = STATIC_CAST(inDataPtr->current.cursorY - inDataPtr->emulator.argList[0], SInt16);
		My_ScreenRowIndex	rowIndex = 0;
		
		
		if (newValue < 0)
		{
			newValue = 0;
		}
		
		rowIndex = STATIC_CAST(newValue, My_ScreenRowIndex);
		if (rowIndex < inDataPtr->originRegionPtr->firstRow)
		{
			rowIndex = inDataPtr->originRegionPtr->firstRow;
		}
		moveCursorY(inDataPtr, rowIndex);
	}
}// My_VT100::cursorUp


/*!
Handles the VT100 'DA' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
deviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// support GPO (graphics processor option) and AVO (advanced video option)
		inDataPtr->emulator.sendEscape(session, "\033[?1;6c", 7);
	}
}// My_VT100::deviceAttributes


/*!
Handles the VT100 'DSR' sequence.  See the VT100
manual for complete details.

(3.0)
*/
void
My_VT100::
deviceStatusReport		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->emulator.argList[0])
	{
	case 5:
		// report status using a DSR control sequence
		{
			SessionRef	session = returnListeningSession(inDataPtr);
			
			
			if (nullptr != session)
			{
				inDataPtr->emulator.sendEscape(session, "\033[0n"/* 0 means “ready, no malfunctions detected”; see VT100 manual on DSR for details */,
												4/* length of the string */);
			}
		}
		break;
	
	case 6:
		// report active (cursor) position using a CPR control sequence
		{
			SInt16				reportedCursorX = inDataPtr->current.cursorX;
			My_ScreenRowIndex	reportedCursorY = inDataPtr->current.cursorY;
			
			
			// determine imminent cursor position
			if (reportedCursorX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
			{
				// auto-wrap pending
				reportedCursorX = 0;
				++reportedCursorY;
			}
			if (reportedCursorY >= inDataPtr->screenBuffer.size())
			{
				// scroll pending (because of auto-wrap)
				reportedCursorY = inDataPtr->screenBuffer.size() - 1;
			}
			
			// report relative to the scroll region if in origin mode
			reportedCursorY -= inDataPtr->originRegionPtr->firstRow;
			
			// the reported numbers are one-based, not zero-based
			++reportedCursorX;
			++reportedCursorY;
			
			// send response
			{
				SessionRef	session = returnListeningSession(inDataPtr);
				
				
				if (nullptr != session)
				{
					std::ostringstream		reportBuffer;
					
					
					reportBuffer
					<< "\033["
					<< reportedCursorY
					<< ";"
					<< reportedCursorX
					<< "R"
					;
					std::string		reportBufferString = reportBuffer.str();
					inDataPtr->emulator.sendEscape(session, reportBufferString.c_str(), reportBufferString.size());
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// My_VT100::deviceStatusReport


/*!
Handles the VT100 'ED' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
eraseInDisplay		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->emulator.argList[0])
	{
	case kMy_ParamUndefined: // when nothing is given, the default value is 0
	case 0:
		bufferEraseFromCursorToEnd(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
		break;
	
	case 1:
		bufferEraseFromHomeToCursor(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
		break;
	
	case 2:
		bufferEraseVisibleScreen(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
		break;
	
	default:
		// ???
		break;
	}
}// My_VT100::eraseInDisplay


/*!
Handles the VT100 'EL' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
eraseInLine		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->emulator.argList[0])
	{
	case kMy_ParamUndefined: // when nothing is given, the default value is 0
	case 0:
		bufferEraseFromCursorColumnToLineEnd(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
		break;
	
	case 1:
		bufferEraseFromLineBeginToCursorColumn(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
		break;
	
	case 2:
		bufferEraseCursorLine(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
		break;
	
	default:
		// ???
		break;
	}
}// My_VT100::eraseInLine


/*!
A standard "My_EmulatorResetProcPtr" to reset VT100-specific
settings.

(4.0)
*/
void
My_VT100::
hardSoftReset	(My_EmulatorPtr		inEmulatorPtr,
				 Boolean			inIsSoftReset)
{
	My_DefaultEmulator::hardSoftReset(inEmulatorPtr, inIsSoftReset);
	
	// debug
	//Console_WriteValue("    <<< VT100 has reset, soft", inIsSoftReset);
}// hardSoftReset


/*!
Handles the VT100 'DECLL' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
loadLEDs	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		i = 0;
	
	
	for (i = 0; i <= inDataPtr->emulator.argLastIndex; ++i)
	{
		if (kMy_ParamUndefined == inDataPtr->emulator.argList[i])
		{
			// no value; default is “all off”
			highlightLED(inDataPtr, 0);
		}
		else if (inDataPtr->emulator.argList[i] == 137)
		{
			// could decide to emulate the “ludicrous repeat rate” bug
			// of the VT100, here; in combination with key click, this
			// should basically make each key press play a musical note :)
		}
		else
		{
			// a parameter of 1 means “LED 1 on”, 2 means “LED 2 on”,
			// 3 means “LED 3 on”, 4 means “LED 4 on”; 0 means “all off”
			highlightLED(inDataPtr, inDataPtr->emulator.argList[i]/* LED # */);
		}
	}
}// My_VT100::loadLEDs


/*!
Handles the VT100 'SM' sequence (if "inIsModeEnabled" is true)
or the VT100 'RM' sequence (if "inIsModeEnabled" is false).
See the VT100 manual for complete details.

(2.6)
*/
void
My_VT100::
modeSetReset	(My_ScreenBufferPtr		inDataPtr,
				 Boolean				inIsModeEnabled)
{
	switch (inDataPtr->emulator.argList[0])
	{
	case kMy_ParamPrivate: // DEC-private control sequence
		{
			SInt16		i = 0;
			Boolean		emulateDECOMBug = false;
			
			
			for (i = 1/* skip the meta-parameter */; i <= inDataPtr->emulator.argLastIndex; ++i)
			{
				switch (inDataPtr->emulator.argList[i])
				{
				case 1:
					// DECCKM (cursor-key mode)
					inDataPtr->modeCursorKeysForApp = inIsModeEnabled;
					break;
				
				case 2:
					// DECANM (ANSI/VT52 mode); this is only possible to reset, not set
					// (the set is accomplished in a different way)
					if (false == inIsModeEnabled)
					{
						My_VT100::vt52Mode(inDataPtr);
					}
					break;
				
				case 3:
					// DECCOLM (80/132 column switch)
					{
						// TEMPORARY; first responder is used for now but this may need
						// to be sent directly to a particular view or window (in case
						// the command appears when another window is active)
						UNUSED_RETURN(Boolean)Commands_ViaFirstResponderPerformSelector((inIsModeEnabled)
																						? @selector(performScreenResizeWide:)
																						: @selector(performScreenResizeStandard:),
																						nil/* object parameter */);
					}
					break;
				
				case 5:
					// DECSCNM (screen mode)
					inDataPtr->reverseVideo = inIsModeEnabled;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeVideoMode, inDataPtr->selfRef);
					break;
				
				case 6: // DECOM (origin mode)
					{
					#if 0
						// the original VT100 has a bug where as soon as DECOM is set or cleared,
						// ALL subsequent mode changes are ignored; since technically MacTerm is
						// emulating a *real* VT100 and not the ideal manual, you could enable
						// this flag to replicate VT100 behavior instead of what the manual says
						emulateDECOMBug = true;
					#endif
						
						inDataPtr->modeOriginRedefined = inIsModeEnabled;
						if (inIsModeEnabled)
						{
							// restrict cursor movements to the defined margins, and
							// ensure that reported cursor row/column use these offsets
							inDataPtr->originRegionPtr = &inDataPtr->customScrollingRegion;
						}
						else
						{
							// no restrictions
							inDataPtr->originRegionPtr = &inDataPtr->visibleBoundary.rows;
						}
						
						// home the cursor, but relative to the new top margin
						// (automatically restricted by cursor movement routines)
						moveCursor(inDataPtr, 0, 0);
					}
					break;
				
				case 7: // DECAWM (auto-wrap mode)
					inDataPtr->modeAutoWrap = inIsModeEnabled;
					if (false == inIsModeEnabled) inDataPtr->wrapPending = false;
					break;
				
				case 4: // DECSCLM (if enabled, scrolling is smooth at 6 lines per second; otherwise, instantaneous)
				case 8: // DECARM (auto-repeating)
				case 9: // DECINLM (interlace)
				case 0: // error, ignored
				case kMy_ParamUndefined: // no value given (set and reset do not have default values)
				default:
					// ???
					break;
				}
				
				// see above, where this is defined, for more information on this break
				if (emulateDECOMBug)
				{
					break;
				}
			}
		}
		break;
	
	case 4: // insert/replace character writing mode
		inDataPtr->modeInsertNotReplace = inIsModeEnabled;
		break;
	
	case 20: // LNM (line feed / newline mode)
		inDataPtr->modeNewLineOption = inIsModeEnabled;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeLineFeedNewLineMode, inDataPtr->selfRef);
		break;
	
	default:
		break;
	}
}// My_VT100::modeSetReset


/*!
Handles the VT100 'DECREQT' sequence.  See the VT100
manual for complete details.

(3.1)
*/
void
My_VT100::
reportTerminalParameters	(My_ScreenBufferPtr		inDataPtr)
{
	UInt16 const	kRequestType = (kMy_ParamUndefined != inDataPtr->emulator.argList[0])
									? inDataPtr->emulator.argList[0]
									: 0/* default is a request, unsolicited reports mode */;
	// these variable names are copied directly from the VT100 manual
	// in the DECREQT section, to make it crystal clear what each one is;
	// see the VT100 manual for full descriptions of their values
	UInt16			sol = 0;		// solicitation; what to do
	UInt16			par = 0;		// parity
	UInt16			nbits = 0;		// bits per character
	UInt16			xspeed = 0;		// transmission speed
	UInt16			rspeed = 0;		// reception speed
	UInt16			clkmul = 0;		// clock multiplier
	UInt16			flags = 0;		// four switch values from SETUP-B
	
	
	// determine the type of response based on the request;
	// also remember this mode for future reports
	if (kRequestType == 1)
	{
		inDataPtr->reportOnlyOnRequest = true;
	}
	else
	{
		inDataPtr->reportOnlyOnRequest = false;
	}
	
	// set the report parameters appropriately
	sol = (inDataPtr->reportOnlyOnRequest) ? 3 : 2;
	par = 1; // 1 = no parity is set
	// TEMPORARY: bits setting is fixed, should be fluid
	nbits = 1; // 1 = 8 bits per character, 2 = 7 bits
	// NOTE: These speeds are a hack...technically, this is set in
	// the terminal control code in "Local.cp", but even that is a hack!
	xspeed = 112; // 112 = 9600 baud
	rspeed = 112; // 112 = 9600 baud
	clkmul = 1; // set to 1 if the bit rate multiplier is 16
	flags = 0; // TEMPORARY: these switches are not stored anywhere
	
	// send response
	{
		SessionRef	session = returnListeningSession(inDataPtr);
		
		
		if (nullptr != session)
		{
			std::ostringstream	reportBuffer;
			
			
			reportBuffer
			<< "\033["
			<< sol << ";"
			<< par << ";"
			<< nbits << ";"
			<< xspeed << ";"
			<< rspeed << ";"
			<< clkmul << ";"
			<< flags
			<< "x"
			;
			std::string		reportBufferString = reportBuffer.str();
			inDataPtr->emulator.sendEscape(session, reportBufferString.c_str(), reportBufferString.size());
		}
	}
}// My_VT100::reportTerminalParameters


/*!
Returns the next logical state of CSI parameter processing,
given the current state and the most recent code point.

Parameter sequences can be terminated by many characters, and
this must be checked from multiple states and emulators, so
this routine was created to simplify maintenance.

NOTE:	This is the VT100 version, and later emulators will
		have additional valid terminators.  It is typical for
		other emulators’ methods to invoke the VT100 one as a
		fallback.

(4.0)
*/
My_ParserState
My_VT100::
returnCSINextState		(My_ParserState			inPreviousState,
						 UnicodeScalarValue		inCodePoint,
						 Boolean&				outHandled)
{
	My_ParserState		result = inPreviousState;
	
	
	outHandled = true; // initially...
	
	// currently the VT100 emulator is the lowest terminal type among
	// terminals that share CSI parameter sequences, so the majority of
	// common characters and state transitions are checked here (and
	// only characters recognized by a VT100 should be here); higher
	// terminals such as the VT102 check only their exclusive cases and
	// fall back on the VT100 as needed
	switch (inCodePoint)
	{
	case '0':
		result = kStateCSIParamDigit0;
		break;
	
	case '1':
		result = kStateCSIParamDigit1;
		break;
	
	case '2':
		result = kStateCSIParamDigit2;
		break;
	
	case '3':
		result = kStateCSIParamDigit3;
		break;
	
	case '4':
		result = kStateCSIParamDigit4;
		break;
	
	case '5':
		result = kStateCSIParamDigit5;
		break;
	
	case '6':
		result = kStateCSIParamDigit6;
		break;
	
	case '7':
		result = kStateCSIParamDigit7;
		break;
	
	case '8':
		result = kStateCSIParamDigit8;
		break;
	
	case '9':
		result = kStateCSIParamDigit9;
		break;
	
	case ':':
		result = kStateCSIParamDigitSub;
		break;
	
	case ';':
		result = kStateCSIParameterEnd;
		break;
	
	case 'A':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsA;
		break;
	
	case 'B':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsB;
		break;
	
	case 'c':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsc;
		break;
	
	case 'C':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsC;
		break;
	
	case 'D':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsD;
		break;
	
	case 'f':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsf;
		break;
	
	case 'g':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsg;
		break;
	
	case 'h':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsh;
		break;
	
	case 'H':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsH;
		break;
	
	case 'J':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsJ;
		break;
	
	case 'K':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsK;
		break;
	
	case 'l':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsl;
		break;
	
	case 'm':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsm;
		break;
	
	case 'n':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsn;
		break;
	
	case 'q':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsq;
		break;
	
	case 'r':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsr;
		break;
	
	case 's':
		// TEMPORARY - ANSI compatibility hack, should be a separate terminal type
		result = kMy_ParserStateSeenESCLeftSqBracketParamss;
		break;
	
	case 'u':
		// TEMPORARY - ANSI compatibility hack, should be a separate terminal type
		result = kMy_ParserStateSeenESCLeftSqBracketParamsu;
		break;
	
	case 'x':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsx;
		break;
	
	case '?':
		// this is not a terminator; it may only come first
		if (My_VT100::kStateCSI == inPreviousState)
		{
			// this is identical to "kStateCSIPrivate"
			result = kMy_ParserStateSeenESCLeftSqBracketQuestionMark;
		}
		else
		{
			outHandled = false;
		}
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in parameter state", inPreviousState);
	//Console_WriteValueFourChars(">>>     VT100 proposes parameter state", result);
	//Console_WriteValueUnicodePoint("        VT100 bases this at least on character", inCodePoint);
	
	return result;
}// My_VT100::returnCSINextState


/*!
Handles the VT100 'DECSTBM' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
setTopAndBottomMargins		(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->emulator.argList[0] < 0)
	{
		// no top parameter given; default is top of screen
		inDataPtr->customScrollingRegion.firstRow = inDataPtr->visibleBoundary.rows.firstRow;
	}
	else
	{
		// parameter is the line number of the new first line of the scrolling region; the
		// input is 1-based but internally it is a zero-based array index, so subtract one
		if (0 == inDataPtr->emulator.argList[0]) inDataPtr->emulator.argList[0] = 1;
		inDataPtr->customScrollingRegion.firstRow = inDataPtr->emulator.argList[0] - 1;
	}
	
	if (inDataPtr->emulator.argList[1] < 0)
	{
		// no bottom parameter given; default is bottom of screen
		inDataPtr->customScrollingRegion.lastRow = inDataPtr->visibleBoundary.rows.lastRow;
	}
	else
	{
		// parameter is the line number of the new last line of the scrolling region; the
		// input is 1-based but internally it is a zero-based array index, so subtract one
		UInt16		newValue = 0;
		
		
		if (0 == inDataPtr->emulator.argList[1]) inDataPtr->emulator.argList[1] = 1;
		
		newValue = inDataPtr->emulator.argList[1] - 1;
		if (newValue > inDataPtr->visibleBoundary.rows.lastRow)
		{
			Console_Warning(Console_WriteLine, "emulator was given a scrolling region bottom row that is too large; truncating");
			newValue = STATIC_CAST(inDataPtr->visibleBoundary.rows.lastRow, UInt16);
		}
		inDataPtr->customScrollingRegion.lastRow = newValue;
	}
	
	// VT100 requires that the range be 2 lines minimum
	if (inDataPtr->customScrollingRegion.firstRow >= inDataPtr->customScrollingRegion.lastRow)
	{
		Console_Warning(Console_WriteLine, "emulator was given a scrolling region bottom row that is less than the top; resetting");
		inDataPtr->customScrollingRegion.lastRow = inDataPtr->customScrollingRegion.firstRow + 1;
		if (inDataPtr->customScrollingRegion.lastRow > inDataPtr->visibleBoundary.rows.lastRow)
		{
			inDataPtr->customScrollingRegion.lastRow = inDataPtr->visibleBoundary.rows.lastRow;
		}
		if (inDataPtr->customScrollingRegion.firstRow >= inDataPtr->customScrollingRegion.lastRow)
		{
			inDataPtr->customScrollingRegion.firstRow = inDataPtr->customScrollingRegion.lastRow - 1;
		}
		assertScrollingRegion(inDataPtr);
	}
	
	// home the cursor, but relative to any current top margin
	// (this limit is enforced by moveCursorY())
	moveCursor(inDataPtr, 0, 0);
	
	//Console_WriteValuePair("scrolling region rows are now", inDataPtr->inDataPtr->customScrollingRegion.firstRow,
	//							inDataPtr->customScrollingRegion.lastRow); // debug
	//Console_WriteValuePair("origin mode enable flag and cursor row", inDataPtr->modeOriginRedefined, inDataPtr->current.cursorY); // debug
}// My_VT100::setTopAndBottomMargins


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT100-specific states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_VT100::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	Boolean			isEightBitControl = false;
	
	
	// all control characters are interrupt-class: they should
	// cause actions, but not “corrupt” any partially completed
	// sequence that may have come before them, i.e. the caller
	// should revert to the state preceding the control character
	outInterrupt = true; // initially...
	
	// see if the given character is a control character; if so,
	// it will not contribute to the current sequence and may
	// even reset the parser
	switch (kTriggerChar)
	{
	case '\000':
		// ignore this character for the purposes of sequencing
		break;
	
	case '\005':
		// send answer-back message
		inNowOutNext.second = kStateControlENQ;
		break;
	
	case '\007':
		// audio event
		inNowOutNext.second = kStateControlBEL;
		break;
	
	case '\010':
		// backspace
		inNowOutNext.second = kStateControlBS;
		break;
	
	case '\011':
		// horizontal tab
		inNowOutNext.second = kStateControlHT;
		break;
	
	case '\012':
	case '\013':
	case '\014':
		// line feed
		// all of these are interpreted the same for VT100
		inNowOutNext.second = kStateControlLFVTFF;
		break;
	
	case '\015':
		// carriage return
		inNowOutNext.second = kStateControlCR;
		break;
	
	case '\016':
		// shift out
		inNowOutNext.second = kStateControlSO;
		break;
	
	case '\017':
		// shift in
		inNowOutNext.second = kStateControlSI;
		break;
	
	case '\021':
		// resume transmission
		inNowOutNext.second = kStateControlXON;
		break;
	
	case '\023':
		// suspend transmission
		inNowOutNext.second = kStateControlXOFF;
		break;
	
	case '\030':
	case '\032':
		// abort control sequence (if any) and emit error character
		inNowOutNext.second = kStateControlCANSUB;
		break;
	
	case '\177': // DEL
		// ignore this character for the purposes of sequencing
		break;
	
	default:
		outInterrupt = false;
		if ((inEmulatorPtr->is8BitReceiver()) && (0 == (kTriggerChar & 0xFFFFFF00)))
		{
			// in 8-bit receive mode, single bytes in a certain range are
			// equivalent to an ESC followed by some 7-bit character
			SInt16		secondSevenBitChar = (STATIC_CAST(kTriggerChar, UInt8) - 0x40);
			
			
			if ((secondSevenBitChar >= 0x40/* @ */) && (secondSevenBitChar <= 0x5F/* _ */))
			{
				// WARNING: this takes advantage of how the state codes happen to be defined,
				// and the way arithmetic works with a 4-character code; if any state is not
				// accounted for here, a separate switch statement should be added to fix it!
				inNowOutNext.second = 'ESC\0' + STATIC_CAST(secondSevenBitChar, UInt8);
				isEightBitControl = true;
			}
		}
		break;
	}
	
	// if no interrupt has occurred, use the current state and
	// the available data to determine the next logical state
	if ((false == outInterrupt) && (false == isEightBitControl))
	{
		// first handle various parameter states
		switch (inNowOutNext.first)
		{
		case kStateCSI:
		case kStateCSIParamDigit0:
		case kStateCSIParamDigit1:
		case kStateCSIParamDigit2:
		case kStateCSIParamDigit3:
		case kStateCSIParamDigit4:
		case kStateCSIParamDigit5:
		case kStateCSIParamDigit6:
		case kStateCSIParamDigit7:
		case kStateCSIParamDigit8:
		case kStateCSIParamDigit9:
		case kStateCSIParamDigitSub:
		case kStateCSIParameterEnd:
		case kStateCSIPrivate:
			inNowOutNext.second = My_VT100::returnCSINextState(inNowOutNext.first, kTriggerChar, outHandled);
			break;
		
		default:
			// not in a parameter
			outHandled = false;
			break;
		}
		
		if (false == outHandled)
		{
			// use the current state and the available data to determine the next logical state
			switch (inNowOutNext.first)
			{
			// currently, no special cases are needed (they are handled
			// entirely by the default terminal transitioning between
			// states based on character sequences alone)
			default:
				outHandled = false;
				break;
			}
		}
	}
	
	if ((false == outHandled) && (false == isEightBitControl))
	{
		result = invokeEmulatorStateDeterminantProc
					(My_DefaultEmulator::stateDeterminant, inEmulatorPtr,
						inNowOutNext, outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT100 proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        VT100 bases this at least on character", kTriggerChar);
	
	return result;
}// My_VT100::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds
to VT100-specific (ANSI mode) state changes.

See also My_VT100::VT52::stateTransition(), which should
ONLY be called when the emulator is in VT52 mode.

IMPORTANT:	This emulator should ONLY be called when the
			emulator is in ANSI mode, *or* if the VT52
			emulator has been called first (which,
			incidentally, defaults to calling this one).
			This code filters out VT52 codes, however
			some codes overlap and do DIFFERENT THINGS
			in ANSI mode.

(3.1)
*/
UInt32
My_VT100::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateControlENQ:
		// send answer-back message
		// UNIMPLEMENTED
		//Console_WriteLine("request to send answer-back message; unimplemented");
		break;
	
	case kStateControlBEL:
		// audio event
		unless (inDataPtr->bellDisabled)
		{
			static CFAbsoluteTime	gLastBeep = 0;
			CFAbsoluteTime			now = CFAbsoluteTimeGetCurrent();
			
			
			// do not allow repeating beeps in a short period of time to
			// take over the terminal; automatically ignore events that
			// occur at an arbitrarily high frequency
			if ((now - gLastBeep) > 2/* arbitrary; in seconds */)
			{
				changeNotifyForTerminal(inDataPtr, kTerminal_ChangeAudioEvent, inDataPtr->selfRef/* context */);
			}
			gLastBeep = now;
		}
		break;
	
	case kStateControlBS:
		// backspace
		if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
		else moveCursorLeftToEdge(inDataPtr); // do not extend past margin
		break;
	
	case kStateControlHT:
		// horizontal tab
		moveCursorRightToNextTabStop(inDataPtr);
		{
			UInt8 const		kTabChar = 0x09;
			
			
			StreamCapture_WriteUTF8Data(inDataPtr->captureStream, &kTabChar, 1);
		}
		break;
	
	case kStateControlLFVTFF:
		// line feed
		// all of these are interpreted the same for VT100;
		// if LNM was received, this is a regular line feed,
		// otherwise it is actually a new-line operation
		moveCursorDownOrScroll(inDataPtr);
	#if 0
		if (inDataPtr->modeNewLineOption)
		{
			moveCursorLeftToEdge(inDataPtr);
		}
	#endif
		break;
	
	case kStateControlCR:
		// carriage return
		moveCursorLeftToEdge(inDataPtr);
	#if 0
		if (inDataPtr->modeNewLineOption)
		{
			moveCursorDownOrScroll(inDataPtr);
		}
	#endif
		{
			UInt8 const		kReturnChar = '\015';
			
			
			// the implementation is such that sending any new-line-like character
			// (return, whatever) will translate to the actual new-line sequence
			// defined for the stream
			StreamCapture_WriteUTF8Data(inDataPtr->captureStream, &kReturnChar, 1);
		}
		break;
	
	case kStateControlSO:
		// shift out
		if (false == inDataPtr->emulator.disableShifts)
		{
			inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG1;
			if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
			{
				// set attribute
				inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics);
			}
			else
			{
				// clear attribute
				inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics);
			}
		}
		break;
	
	case kStateControlSI:
		// shift in
		if (false == inDataPtr->emulator.disableShifts)
		{
			inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
			if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
			{
				// set attribute
				inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics);
			}
			else
			{
				// clear attribute
				inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics);
			}
		}
		break;
	
	case kStateControlXON:
		// resume transmission
		// UNIMPLEMENTED
		//Console_WriteLine("request to resume transmission; unimplemented");
		break;
	
	case kStateControlXOFF:
		// suspend transmission
		// UNIMPLEMENTED
		//Console_WriteLine("request to suspend transmission (except for XON/XOFF); unimplemented");
		break;
	
	case kStateControlCANSUB:
		// abort control sequence (if any) and emit error character
		echoCFString(inDataPtr, CFSTR("?"));
		break;
	
	case kStateCSI:
		// each new CSI means a blank slate for parameters
		inDataPtr->emulator.clearEscapeSequenceParameters();
		break;
	
	case kStateCSIParamDigit0:
	case kStateCSIParamDigit1:
	case kStateCSIParamDigit2:
	case kStateCSIParamDigit3:
	case kStateCSIParamDigit4:
	case kStateCSIParamDigit5:
	case kStateCSIParamDigit6:
	case kStateCSIParamDigit7:
	case kStateCSIParamDigit8:
	case kStateCSIParamDigit9:
		{
			SInt16&		valueRef = inDataPtr->emulator.argList[inDataPtr->emulator.argLastIndex];
			
			
			if (valueRef < 0)
			{
				valueRef = 0;
			}
			valueRef *= 10;
			valueRef += (inOldNew.second - kStateCSIParamDigit0); // WARNING: requires states to be defined consecutively
		}
		break;
	
	case kStateCSIParamDigitSub:
		// end of sub-parameter
		inDataPtr->emulator.parameterMarkList[inDataPtr->emulator.argLastIndex] = 0;
		if (inDataPtr->emulator.argLastIndex < kMy_MaximumANSIParameters)
		{
			++(inDataPtr->emulator.argLastIndex);
		}
		break;
	
	case kStateCSIParameterEnd:
		// end of control sequence parameter
		inDataPtr->emulator.parameterMarkList[inDataPtr->emulator.argLastIndex] = -1;
		if (inDataPtr->emulator.argLastIndex < kMy_MaximumANSIParameters)
		{
			++(inDataPtr->emulator.argLastIndex);
		}
		break;
	
	case kStateCSIPrivate:
		// flag to mark the control sequence as private
		inDataPtr->emulator.argList[inDataPtr->emulator.argLastIndex] = kMy_ParamPrivate;
		++(inDataPtr->emulator.argLastIndex);
		break;
	
	case kStateCUB:
		My_VT100::cursorBackward(inDataPtr);
		break;
	
	case kStateCUD:
		My_VT100::cursorDown(inDataPtr);
		break;
	
	case kStateCUF:
		My_VT100::cursorForward(inDataPtr);
		break;
	
	case kStateCUU:
		My_VT100::cursorUp(inDataPtr);
		break;
	
	case kStateCUP:
	case kStateHVP:
		// absolute cursor positioning
		{
			// both 0 and 1 are considered first row/column
			SInt16				newX = (0 == inDataPtr->emulator.argList[1])
										? 0
										: (kMy_ParamUndefined != inDataPtr->emulator.argList[1])
											? inDataPtr->emulator.argList[1] - 1
											: 0/* default is home */;
			My_ScreenRowIndex	newY = (0 == inDataPtr->emulator.argList[0])
										? 0
										: (kMy_ParamUndefined != inDataPtr->emulator.argList[0])
											? inDataPtr->emulator.argList[0] - 1
											: 0/* default is home */;
			
			
			// offset according to the origin mode
			newY += inDataPtr->originRegionPtr->firstRow;
			
			// the new values are not checked for violation of constraints
			// because constraints (including current origin mode) are
			// automatically enforced by moveCursor...() routines
			moveCursor(inDataPtr, newX, newY);
		}
		break;
	
	case kStateDA:
		My_VT100::deviceAttributes(inDataPtr);
		break;
	
	case kStateDECALN:
		My_VT100::alignmentDisplay(inDataPtr);
		break;
	
	case kStateDECANM:
		break;
	
	case kStateDECARM:
		break;
	
	case kStateDECAWM:
		break;
	
	case kStateDECCKM:
		break;
	
	case kStateDECCOLM:
		break;
	
	case kStateDECDHLT:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, **cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, **cursorLineIterator, kTextAttributes_DoubleHeightTop/* set */,
										kTextAttributes_DoubleTextAll/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateDECDHLB:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, **cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, **cursorLineIterator, kTextAttributes_DoubleHeightBottom/* set */,
										kTextAttributes_DoubleTextAll/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateDECDWL:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, **cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, **cursorLineIterator, kTextAttributes_DoubleWidth/* set */,
										kTextAttributes_DoubleTextAll/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateDECID:
		My_VT100::deviceAttributes(inDataPtr);
		break;
	
	case kStateDECKPAM:
		inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (use application key sequences) of VT100
		break;
	
	case kStateDECKPNM:
		inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (restore regular keypad characters) of VT100
		break;
	
	case kStateDECLL:
		My_VT100::loadLEDs(inDataPtr);
		break;
	
	case kStateDECOM:
		break;
	
	case kStateANSIRC:
	case kStateDECRC:
		cursorRestore(inDataPtr);
		break;
	
	case kStateDECREPTPARM:
		// a parameter report has been received
		// IGNORED
		break;
	
	case kStateDECREQTPARM:
		// a request for parameters has been made; send a response
		My_VT100::reportTerminalParameters(inDataPtr);
		break;
	
	case kStateANSISC:
	case kStateDECSC:
		cursorSave(inDataPtr);
		break;
	
	case kStateDECSTBM:
		My_VT100::setTopAndBottomMargins(inDataPtr);
		break;
	
	case kStateDECSWL:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, **cursorLineIterator, TextAttributes_Object()/* set */,
										kTextAttributes_DoubleTextAll/* clear */);
		}
		break;
	
	case kStateDECTST:
		break;
	
	case kStateDSR:
		My_VT100::deviceStatusReport(inDataPtr);
		break;
	
	case kStateED:
		My_VT100::eraseInDisplay(inDataPtr);
		break;
	
	case kStateEL:
		My_VT100::eraseInLine(inDataPtr);
		break;
	
	case kStateHTS:
		// set tab at current position
		inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;
		break;
	
	//case kStateHVP:
	//see above
	
	case kStateIND:
		moveCursorDownOrScroll(inDataPtr);
		break;
	
	case kStateNEL:
		moveCursorLeftToEdge(inDataPtr);
		moveCursorDownOrScroll(inDataPtr);
		break;
	
	case kStateRI:
		moveCursorUpOrScroll(inDataPtr);
		break;
	
	case kStateRIS:
		resetTerminal(inDataPtr);
		break;
	
	case kStateRM:
		My_VT100::modeSetReset(inDataPtr, false/* set */);
		break;
	
	case kStateSCSG0UK:
	case kStateSCSG1UK:
		// U.K. character set, normal ROM, no graphics
		if (false == inDataPtr->emulator.disableShifts)
		{
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1UK == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedKingdom;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG0ASCII:
	case kStateSCSG1ASCII:
		// U.S. character set, normal ROM, no graphics
		if (false == inDataPtr->emulator.disableShifts)
		{
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1ASCII == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedStates;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG0SG:
	case kStateSCSG1SG:
		// normal ROM, graphics mode
		if (false == inDataPtr->emulator.disableShifts)
		{
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1SG == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics); // set graphics attribute
		}
		break;
	
	case kStateSCSG0ACRStd:
	case kStateSCSG1ACRStd:
		// alternate ROM, no graphics
		if (false == inDataPtr->emulator.disableShifts)
		{
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1ACRStd == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMAlternate;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG0ACRSG:
	case kStateSCSG1ACRSG:
		// normal ROM, graphics mode
		if (false == inDataPtr->emulator.disableShifts)
		{
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1ACRSG == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMAlternate;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics); // set graphics attribute
		}
		break;
	
	case kStateSGR:
		// ANSI colors and other character attributes
		{
			SInt16		lastCheckedIndex = 0; // current numerical value (sub-parameter or main parameter)
			UInt16		subParameterBackCount = 0; // number of sub-parameters to check prior to current value (if any)
			
			
			while (lastCheckedIndex <= inDataPtr->emulator.argLastIndex)
			{
				// the marker array identifies values that were given as sub-parameters
				// (most terminal modes do not recognize these)
				Boolean const	kIsSubParam = (0 == inDataPtr->emulator.parameterMarkList[lastCheckedIndex]);
				SInt16 const	kAnchorIndex = (lastCheckedIndex - subParameterBackCount);
				SInt16 const	kArgsLeft = (inDataPtr->emulator.argLastIndex - kAnchorIndex + 1);
				SInt16			skipParameterCount = 0; // force-discard future arguments (ignored if there are sub-parameters)
				SInt16			paramValue = 0;
				
				
				// all unused parameters receive default values
				if (inDataPtr->emulator.argList[lastCheckedIndex] < 0)
				{
					inDataPtr->emulator.argList[lastCheckedIndex] = 0;
				}
				
				if (kIsSubParam)
				{
					// sub-parameters (colon-separated) are not processed immediately;
					// they are processed when a terminating argument is found
					++subParameterBackCount;
					++lastCheckedIndex;
					continue;
				}
				
				// normal parameter processing; if any sub-parameters have
				// been processed, the “anchor” will actually be at the
				// beginning of all sub-parameters (in this way, if some
				// parameter "1" expects to then see "2", "3", and "4",
				// there is no real difference between processing "1:2:3:4"
				// and "1;2;3;4"; if a distinction is necessary, note that
				// "lastCheckedIndex" and "kAnchorIndex" are the same when
				// there are no sub-parameters present)
				paramValue = inDataPtr->emulator.argList[kAnchorIndex];
				
				// Note that a real VT100 will only understand 0-7 here.
				// Other values are basically recognized because they are
				// compatible with VT100 and are very often used (ANSI
				// colors in particular).
				if (0 == paramValue)
				{
					inDataPtr->current.drawingAttributes.removeStyleAndColorRelatedAttributes();
					inDataPtr->current.latentAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.drawingAttributes);
				}
				else if (paramValue < 9)
				{
					inDataPtr->current.drawingAttributes.addAttributes(styleOfVTParameter(paramValue)); // set attribute
				}
				else if (10 == paramValue)
				{
					// set normal font - unsupported
				}
				else if (11 == paramValue)
				{
					// set alternate font - unsupported
				}
				else if (12 == paramValue)
				{
					// set alternate font, shifting by 128 - unsupported
				}
				else if (22 == paramValue)
				{
					inDataPtr->current.drawingAttributes.removeAttributes(styleOfVTParameter(1)); // clear bold (oddball - 22, not 21)
				}
				else if ((paramValue > 22) && (paramValue < 29))
				{
					inDataPtr->current.drawingAttributes.removeAttributes(styleOfVTParameter(paramValue - 20)); // clear attribute
				}
				else
				{
					if ((paramValue >= 30) && (paramValue < 38))
					{
						inDataPtr->current.drawingAttributes.colorIndexForegroundSet(paramValue - 30);
					}
					else if ((paramValue >= 40) && (paramValue < 48))
					{
						inDataPtr->current.drawingAttributes.colorIndexBackgroundSet(paramValue - 40);
						inDataPtr->current.latentAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.drawingAttributes);
					}
					else if ((38 == paramValue) || (48 == paramValue))
					{
						// this implements the “popular” form, which is to abuse SGR-mode parameters
						// themselves to obtain the color specification; colon (:) is NOT recognized
						// as a delimiter here (note that the user can disable certain terminal
						// tweaks, including this one, to restore standard behavior for applications
						// that require it)
						Boolean const	kSetForeground = (38 == paramValue);
						
						
						if (kArgsLeft < 2)
						{
							if (DebugInterface_LogsTerminalInputChar())
							{
								Console_Warning(Console_WriteLine, "expected more parameters for 24-bit-color or 256-color request");
							}
						}
						else if (1 == inDataPtr->emulator.argList[kAnchorIndex + 1])
						{
							// documented as a transparency request; no known applications
							// currently send this option (has no parameters) and it is
							// the default behavior when no background color is set anyway
							if (DebugInterface_LogsTerminalInputChar())
							{
								Console_Warning(Console_WriteLine, "transparency option not currently supported");
							}
						}
						else if (2 == inDataPtr->emulator.argList[kAnchorIndex + 1])
						{
							// 24-bit color (three 8-bit values for R, G, B components)
							if (false == inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlag24BitColor))
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "ignoring 24-bit-color request due to terminal configuration");
								}
							}
							else if (kArgsLeft < 5)
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "expected 5 parameters for 24-bit RGB color request");
								}
							}
							else
							{
								SInt16 const	kRedParam = inDataPtr->emulator.argList[kAnchorIndex + 2];
								SInt16 const	kGreenParam = inDataPtr->emulator.argList[kAnchorIndex + 3];
								SInt16 const	kBlueParam = inDataPtr->emulator.argList[kAnchorIndex + 4];
								
								
								if ((kRedParam > 255) || (kRedParam < 0) ||
									(kGreenParam > 255) || (kGreenParam < 0) ||
									(kBlueParam > 255) || (kBlueParam < 0))
								{
									if (DebugInterface_LogsTerminalInputChar())
									{
										Console_Warning(Console_WriteValueFloat4,
														"illegal range for R, G or B value of 24-bit color (expected 0-255)",
														kRedParam, kGreenParam, kBlueParam, 0);
									}
								}
								else
								{
									TextAttributes_TrueColorID		colorID = 0;
									
									
									//Console_WriteValueFloat4("request to set RGB 24-bit color (R,G,B,fg/bg)",
									//							kRedParam, kGreenParam, kBlueParam, kSetForeground); // debug
									if (false == defineTrueColor(inDataPtr, STATIC_CAST(kRedParam, UInt8),
																	STATIC_CAST(kGreenParam, UInt8),
																	STATIC_CAST(kBlueParam, UInt8), colorID))
									{
										// NOTE: this should not happen because the supportsVariant() method
										// is called earlier, and the color table should always be allocated
										// in terminals that support 24-bit color
										if (DebugInterface_LogsTerminalInputChar())
										{
											Console_Warning(Console_WriteLine, "terminal failed to store new 24-bit color!");
										}
									}
									else
									{
										if (kSetForeground)
										{
											inDataPtr->current.drawingAttributes.colorIDForegroundSet(colorID);
										}
										else
										{
											inDataPtr->current.drawingAttributes.colorIDBackgroundSet(colorID);
										}
									}
								}
								skipParameterCount = 4; // skip parameters (2, 3, 4, 5)
							}
						}
						else if (3 == inDataPtr->emulator.argList[kAnchorIndex + 1])
						{
							if (false == inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlag24BitColor))
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "ignoring 24-bit-color request due to terminal configuration");
								}
							}
							else
							{
								// CMY color request; this is not supported but it is probably
								// important to strip off any “parameters”
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "CMY color type is not supported");
								}
								
								if (kArgsLeft < 5)
								{
									if (DebugInterface_LogsTerminalInputChar())
									{
										Console_Warning(Console_WriteLine, "expected 5 parameters for 24-bit CMY color request");
									}
								}
								else
								{
									skipParameterCount = 4; // skip parameters (2, 3, 4, 5)
								}
							}
						}
						else if (4 == inDataPtr->emulator.argList[kAnchorIndex + 1])
						{
							if (false == inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlag24BitColor))
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "ignoring 24-bit-color request due to terminal configuration");
								}
							}
							else
							{
								// CMYK color request; this is not supported but it is probably
								// important to strip off any “parameters”
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "CMYK color type is not supported");
								}
								
								if (kArgsLeft < 6)
								{
									if (DebugInterface_LogsTerminalInputChar())
									{
										Console_Warning(Console_WriteLine, "expected 6 parameters for 24-bit CMYK color request");
									}
								}
								else
								{
									skipParameterCount = 5; // skip parameters (2, 3, 4, 5, 6)
								}
							}
						}
						else if (5 == inDataPtr->emulator.argList[kAnchorIndex + 1])
						{
							// index into XTerm 256-color table
							if (kArgsLeft < 2)
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "expected 2 parameters for XTerm 256-color request");
								}
							}
							else if (false == inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTerm256Color))
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "ignoring XTerm 256-color request due to terminal configuration");
								}
							}
							else
							{
								// index into XTerm 256-color palette
								SInt16 const	kColorParam = inDataPtr->emulator.argList[kAnchorIndex + 2];
								
								
								if ((kColorParam > 255) || (kColorParam < 0))
								{
									Console_Warning(Console_WriteValue, "illegal range for index of color (expected 0-255)",
													kColorParam);
								}
								else
								{
									if (kSetForeground)
									{
										inDataPtr->current.drawingAttributes.colorIndexForegroundSet(kColorParam);
									}
									else
									{
										inDataPtr->current.drawingAttributes.colorIndexBackgroundSet(kColorParam);
										inDataPtr->current.latentAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.drawingAttributes);
									}
								}
								skipParameterCount = 2; // skip next parameters (2, 3)
							}
						}
						else
						{
							if (DebugInterface_LogsTerminalInputChar())
							{
								Console_Warning(Console_WriteLine, "expected leading digit parameter ('1', '2', '3', '4' or '5')");
							}
						}
					}
					else if (39 == paramValue)
					{
						// generally means “reset foreground”
						inDataPtr->current.drawingAttributes.colorIndexForegroundSet(0);
						inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_EnableForeground);
					}
					else if (49 == paramValue)
					{
						// generally means “reset background”
						inDataPtr->current.drawingAttributes.colorIndexBackgroundSet(0);
						inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_EnableBackground);
						inDataPtr->current.latentAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.drawingAttributes);
					}
					else if ((paramValue >= 90) && (paramValue < 98))
					{
						inDataPtr->current.drawingAttributes.colorIndexForegroundSet(8 + (paramValue - 90));
					}
					else if ((paramValue >= 100) && (paramValue < 108))
					{
						inDataPtr->current.drawingAttributes.colorIndexBackgroundSet(8 + (paramValue - 100));
						inDataPtr->current.latentAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.drawingAttributes);
					}
					else
					{
						if (DebugInterface_LogsTerminalInputChar())
						{
							Console_WriteValue("current terminal in SGR mode does not support parameter", paramValue);
						}
					}
				}
				++lastCheckedIndex;
				if (0 == subParameterBackCount)
				{
					// skip any requested future parameters when processing
					// a series of arguments (if there are sub-parameters,
					// ignore this because sub-parameters are already handled)
					lastCheckedIndex += skipParameterCount;
				}
				subParameterBackCount = 0;
			}
		}
		break;
	
	case kStateSM:
		My_VT100::modeSetReset(inDataPtr, true/* set */);
		break;
	
	case kStateTBC:
		if (3 == inDataPtr->emulator.argList[0])
		{
			// clear all tabs
			tabStopClearAll(inDataPtr);
		}
		else if (0 >= inDataPtr->emulator.argList[0])
		{
			// clear tab at current position
			inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabClear;
		}
		else
		{
			// invalid (do nothing)
		}
		break;
	
	// ignore all VT100/VT52 sequences that are invalid within this ANSI-mode parser
	case My_VT100::VT52::kStateCU:
	case My_VT100::VT52::kStateCD:
	case My_VT100::VT52::kStateCR:
	//case My_VT100::VT52::kStateCL: // this conflicts with a valid VT100 ANSI mode value above
	case My_VT100::VT52::kStateNGM:
	case My_VT100::VT52::kStateXGM:
	//case My_VT100::VT52::kStateCH: // this conflicts with a valid VT100 ANSI mode value above
	case My_VT100::VT52::kStateRLF:
	case My_VT100::VT52::kStateEES:
	case My_VT100::VT52::kStateEEL:
	case My_VT100::VT52::kStateDCA:
	//case My_VT100::VT52::kStateID: // this conflicts with a valid VT100 ANSI mode value above
	//case My_VT100::VT52::kStateNAKM: // this conflicts with a valid VT100 ANSI mode value above
	//case My_VT100::VT52::kStateXAKM: // this conflicts with a valid VT100 ANSI mode value above
	case My_VT100::VT52::kStateANSI:
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateTransitionProc
					(My_DefaultEmulator::stateTransition, inDataPtr,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT100::stateTransition


/*!
Switches a VT100 terminal to VT52 mode, which means it
starts to accept VT52 sequences.

(3.1)
*/
void
My_VT100::
vt52Mode	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->modeANSIEnabled = false;
	inDataPtr->emulator.pushedCallbacks = inDataPtr->emulator.currentCallbacks;
	inDataPtr->emulator.currentCallbacks = My_Emulator::Callbacks
											(My_DefaultEmulator::echoData,
												My_VT100::VT52::stateDeterminant,
												My_VT100::VT52::stateTransition,
												My_VT100::hardSoftReset);
	inDataPtr->emulator.initializeParserStateStack();
}// My_VT100::vt52Mode


/*!
Handles the VT100 VT52-compatibility sequence 'ESC D'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorBackward	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
	else moveCursorLeftToEdge(inDataPtr);
}// My_VT100::VT52::cursorBackward


/*!
Handles the VT100 VT52-compatibility sequence 'ESC B'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorDown	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY < inDataPtr->originRegionPtr->lastRow) moveCursorDown(inDataPtr);
	else moveCursorDownToEdge(inDataPtr);
}// My_VT100::VT52:cursorDown


/*!
Handles the VT100 VT52-compatibility sequence 'ESC C'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorForward	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		rightLimit = inDataPtr->current.returnNumberOfColumnsPermitted() - ((inDataPtr->modeAutoWrap) ? 0 : 1);
	
	
	if (inDataPtr->current.cursorX < rightLimit) moveCursorRight(inDataPtr);
	else moveCursorRightToEdge(inDataPtr);
}// My_VT100::VT52:cursorForward


/*!
Handles the VT100 VT52-compatibility sequence 'ESC A'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY > inDataPtr->originRegionPtr->firstRow)
	{
		moveCursorUp(inDataPtr);
	}
	else
	{
		moveCursorUpToEdge(inDataPtr);
	}
}// My_VT100::VT52:cursorUp


/*!
Handles the VT100 'ESC Z' sequence, which should only
be recognized in VT52 compatibility mode.  See the
VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
identify	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		inDataPtr->emulator.sendEscape(session, "\033/Z", 3);
	}
}// My_VT100::VT52:identify


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT52-specific states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_VT100::VT52::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	switch (inNowOutNext.first)
	{
	case kMy_ParserStateSeenESC:
		switch (kTriggerChar)
		{
		case 'A':
			inNowOutNext.second = kMy_ParserStateSeenESCA;
			break;
		
		case 'B':
			inNowOutNext.second = kMy_ParserStateSeenESCB;
			break;
		
		case 'C':
			inNowOutNext.second = kMy_ParserStateSeenESCC;
			break;
		
		case 'D':
			inNowOutNext.second = kMy_ParserStateSeenESCD;
			break;
		
		case 'F':
			inNowOutNext.second = kMy_ParserStateSeenESCF;
			break;
		
		case 'G':
			inNowOutNext.second = kMy_ParserStateSeenESCG;
			break;
		
		case 'H':
			inNowOutNext.second = kMy_ParserStateSeenESCH;
			break;
		
		case 'I':
			inNowOutNext.second = kMy_ParserStateSeenESCI;
			break;
		
		case 'J':
			inNowOutNext.second = kMy_ParserStateSeenESCJ;
			break;
		
		case 'K':
			inNowOutNext.second = kMy_ParserStateSeenESCK;
			break;
		
		case 'Y':
			inNowOutNext.second = kMy_ParserStateSeenESCY;
			break;
		
		case 'Z':
			inNowOutNext.second = kMy_ParserStateSeenESCZ;
			break;
		
		case '=':
			inNowOutNext.second = kMy_ParserStateSeenESCEquals;
			break;
		
		case '>':
			inNowOutNext.second = kMy_ParserStateSeenESCGreaterThan;
			break;
		
		case '<':
			inNowOutNext.second = kMy_ParserStateSeenESCLessThan;
			break;
		
		default:
			// this is unexpected data; choose a new state
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValueUnicodePoint, "VT52 did not expect an ESC to be followed by character", kTriggerChar);
			}
			outHandled = false;
			break;
		}
		break;
	
	case kStateDCA:
		// the 2 characters after a VT52 DCA are the coordinates (Y first)
		inNowOutNext.second = kStateDCAY;
		result = 0; // absorb nothing
		break;
	
	case kStateDCAY:
		// the 2 characters after a VT52 DCA are the coordinates (Y first)
		inNowOutNext.second = kStateDCAX;
		result = 0; // absorb nothing
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT100::stateDeterminant, inEmulatorPtr,
						inNowOutNext, outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in VT52 mode in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT100 in VT52 mode proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        VT100 in VT52 mode bases this at least on character", kTriggerChar);
	
	return result;
}// My_VT100::VT52::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT100-specific (VT52 mode) state changes.

(3.1)
*/
UInt32
My_VT100::VT52::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateCU:
		My_VT100::VT52::cursorUp(inDataPtr);
		break;
	
	case kStateCD:
		My_VT100::VT52::cursorDown(inDataPtr);
		break;
	
	case kStateCR:
		My_VT100::VT52::cursorForward(inDataPtr);
		break;
	
	case kStateCL:
		My_VT100::VT52::cursorBackward(inDataPtr);
		break;
	
	case kStateNGM:
		// enter graphics mode - unimplemented
		break;
	
	case kStateXGM:
		// exit graphics mode - unimplemented
		break;
	
	case kStateCH:
		moveCursor(inDataPtr, 0, 0); // home cursor in VT52 compatibility mode of VT100
		break;
	
	case kStateRLF:
		moveCursorUpOrScroll(inDataPtr); // reverse line feed in VT52 compatibility mode of VT100
		break;
	
	case kStateEES:
		bufferEraseFromCursorToEnd(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse()); // erase to end of screen, in VT52 compatibility mode of VT100
		break;
	
	case kStateEEL:
		bufferEraseFromCursorColumnToLineEnd(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse()); // erase to end of line, in VT52 compatibility mode of VT100
		break;
	
	case kStateDCA:
		// direct cursor address in VT52 compatibility mode of VT100;
		// new cursor position is encoded as the next two characters
		// (vertical first, then horizontal) offset by the octal
		// value 037 (equal to decimal 31); this is handled by 2
		// other states, "DCAY" and "DCAX" (below)
		break;
	
	case kStateDCAY:
		// VT52 DCA, first character (Y + 31)
		{
			My_ScreenRowIndex	newY = 0;
			
			
			newY = inDataPtr->emulator.recentCodePoint() - 32/* - 31 - 1 to convert from one-based to zero-based */;
			++result;
			
			// constrain the value and then change it safely
			//if (newY < 0) newY = 0;
			if (newY >= inDataPtr->screenBuffer.size())
			{
				newY = inDataPtr->screenBuffer.size() - 1;
			}
			moveCursorY(inDataPtr, newY);
		}
		break;
	
	case kStateDCAX:
		// VT52 DCA, second character (X + 31)
		{
			SInt16		newX = 0;
			
			
			newX = STATIC_CAST(inDataPtr->emulator.recentCodePoint(), SInt16) - 32/* - 31 - 1 to convert from one-based to zero-based */;
			++result;
			
			// constrain the value and then change it safely
			if (newX < 0) newX = 0;
			if (newX >= inDataPtr->current.returnNumberOfColumnsPermitted())
			{
				newX = inDataPtr->current.returnNumberOfColumnsPermitted() - 1;
			}
			moveCursorX(inDataPtr, newX);
		}
		break;
	
	case kStateID:
		My_VT100::VT52::identify(inDataPtr);
		break;
	
	case kStateNAKM:
		inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (use application key sequences) in VT52 compatibility mode of VT100
		break;
	
	case kStateXAKM:
		inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (restore regular keypad characters) in VT52 compatibility mode of VT100
		break;
	
	case kStateANSI:
		My_VT100::ansiMode(inDataPtr);
		break;
	
	// ignore all VT100 sequences that are invalid in VT52 mode
	// NONE?
	//case :
	//	break;
	
	default:
		// other state transitions should still basically be handled as if in VT100 ANSI
		// (NOTE: this switch should filter out any ANSI mode states that are supposed to
		// be ignored while in VT52 mode!)
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateTransitionProc
					(My_VT100::stateTransition, inDataPtr,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT100::VT52::stateTransition


/*!
Handles the VT102 'DCH' sequence.  See the VT102
manual for complete details.

(3.1)
*/
void
My_VT102::
deleteCharacters	(My_ScreenBufferPtr		inDataPtr)
{
	// “one character” is assumed if a parameter is zero, or there are no parameters,
	// even though this is not explicitly stated in VT102 documentation
	SInt16		i = 0;
	UInt16		totalChars = (inDataPtr->emulator.argLastIndex < 0) ? 1 : 0;
	
	
	for (i = 0; i <= inDataPtr->emulator.argLastIndex; ++i)
	{
		if (inDataPtr->emulator.argList[i] > 0)
		{
			totalChars += inDataPtr->emulator.argList[i];
		}
		else
		{
			++totalChars;
		}
	}
	bufferRemoveCharactersAtCursorColumn(inDataPtr, totalChars,
											inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermBCE)
											? kMy_AttributeRuleCopyLatentBackground
											: kMy_AttributeRuleCopyLast);
}// My_VT102::deleteCharacters


/*!
Handles the VT102 'DA' sequence.  See the VT102
manual for complete details.

(4.0)
*/
inline void
My_VT102::
deviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		inDataPtr->emulator.sendEscape(session, "\033[?6c", 5);
	}
}// My_VT102::deviceAttributes


/*!
Handles the VT102 'DL' sequence.  See the VT102
manual for complete details.

(3.1)
*/
void
My_VT102::
deleteLines		(My_ScreenBufferPtr		inDataPtr)
{
	// do nothing if the cursor is outside the scrolling region
	if ((inDataPtr->customScrollingRegion.lastRow >= inDataPtr->current.cursorY) &&
		(inDataPtr->customScrollingRegion.firstRow <= inDataPtr->current.cursorY))
	{
		// “one line” is assumed if a parameter is zero, or there are no parameters,
		// even though this is not explicitly stated in VT102 documentation
		My_ScreenBufferLineList::iterator	lineIterator;
		SInt16								i = 0;
		UInt16								totalLines = (inDataPtr->emulator.argLastIndex < 0) ? 1 : 0;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		for (i = 0; i <= inDataPtr->emulator.argLastIndex; ++i)
		{
			if (inDataPtr->emulator.argList[i] > 0)
			{
				totalLines += inDataPtr->emulator.argList[i];
			}
			else
			{
				++totalLines;
			}
		}
		bufferRemoveLines(inDataPtr, totalLines, lineIterator,
							inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermBCE)
							? kMy_AttributeRuleCopyLatentBackground
							: kMy_AttributeRuleCopyLast);
	}
}// My_VT102::deleteLines


/*!
Handles the VT102 'IL' sequence.  See the VT102
manual for complete details.

(3.1)
*/
void
My_VT102::
insertLines		(My_ScreenBufferPtr		inDataPtr)
{
	// do nothing if the cursor is outside the scrolling region
	if ((inDataPtr->customScrollingRegion.lastRow >= inDataPtr->current.cursorY) &&
		(inDataPtr->customScrollingRegion.firstRow <= inDataPtr->current.cursorY))
	{
		// “one line” is assumed if a parameter is zero, or there are no parameters,
		// even though this is not explicitly stated in VT102 documentation
		My_ScreenBufferLineList::iterator	lineIterator;
		SInt16								i = 0;
		UInt16								totalLines = (inDataPtr->emulator.argLastIndex < 0) ? 1 : 0;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		for (i = 0; i <= inDataPtr->emulator.argLastIndex; ++i)
		{
			if (inDataPtr->emulator.argList[i] > 0)
			{
				totalLines += inDataPtr->emulator.argList[i];
			}
			else
			{
				++totalLines;
			}
		}
		bufferInsertBlankLines(inDataPtr, totalLines, lineIterator,
								inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermBCE)
								? kMy_AttributeRuleCopyLatentBackground
								: kMy_AttributeRuleInitialize);
	}
}// My_VT102::insertLines


/*!
Handles the VT102 'DECLL' sequence.  See the VT102
manual for complete details.  Unlike VT100, the VT102
has only a single LED.

(3.1)
*/
inline void
My_VT102::
loadLEDs	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		i = 0;
	
	
	for (i = 0; i <= inDataPtr->emulator.argLastIndex; ++i)
	{
		// a parameter of 1 means “LED 1 on”, 0 means “LED 1 off”
		if (0 == inDataPtr->emulator.argList[i])
		{
			highlightLED(inDataPtr, 0/* 0 means “all off” */);
		}
		else if (1 == inDataPtr->emulator.argList[i])
		{
			highlightLED(inDataPtr, 1/* LED # */);
		}
	}
}// My_VT102::loadLEDs


/*!
Returns the next logical state of CSI parameter processing,
given the current state and the most recent code point.

Parameter sequences can be terminated by many characters, and
this must be checked from multiple states and emulators, so
this routine was created to simplify maintenance.

(4.0)
*/
My_ParserState
My_VT102::
returnCSINextState		(My_ParserState			inPreviousState,
						 UnicodeScalarValue		inCodePoint,
						 Boolean&				outHandled)
{
	My_ParserState		result = inPreviousState;
	
	
	outHandled = true; // initially...
	
	// there should be an entry here for each parameter list terminator that is
	// valid AT LEAST in a VT102 terminal; any that are also valid in lesser
	// terminals can be omitted, since they will be handled in the VT100 fallback
	switch (inCodePoint)
	{
	case 'i':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsi;
		break;
	
	case 'L':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsL;
		break;
	
	case 'M':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsM;
		break;
	
	case 'P':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsP;
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		// fall back to VT100
		result = My_VT100::returnCSINextState(inPreviousState, inCodePoint, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT102 in parameter state", inPreviousState);
	//Console_WriteValueFourChars(">>>     VT102 proposes parameter state", result);
	//Console_WriteValueUnicodePoint("        VT102 bases this at least on character", inCodePoint);
	
	return result;
}// My_VT102::returnCSINextState


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT102-specific states based on the characters of the given
buffer.

Since a VT102 is very similar to a VT100, the vast majority
of state analysis is done by the VT100 routine.

(3.1)
*/
UInt32
My_VT102::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// first handle various parameter states
	switch (inNowOutNext.first)
	{
	case My_VT100::kStateCSI:
	case My_VT100::kStateCSIParamDigit0:
	case My_VT100::kStateCSIParamDigit1:
	case My_VT100::kStateCSIParamDigit2:
	case My_VT100::kStateCSIParamDigit3:
	case My_VT100::kStateCSIParamDigit4:
	case My_VT100::kStateCSIParamDigit5:
	case My_VT100::kStateCSIParamDigit6:
	case My_VT100::kStateCSIParamDigit7:
	case My_VT100::kStateCSIParamDigit8:
	case My_VT100::kStateCSIParamDigit9:
	case My_VT100::kStateCSIParamDigitSub:
	case My_VT100::kStateCSIParameterEnd:
	case My_VT100::kStateCSIPrivate:
		inNowOutNext.second = My_VT102::returnCSINextState(inNowOutNext.first, kTriggerChar, outHandled);
		break;
	
	default:
		// not in a parameter
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		// use the current state and the available data to determine the next logical state
		switch (inNowOutNext.first)
		{
		// currently, no special cases are needed (they are handled
		// entirely by the default terminal transitioning between
		// states based on character sequences alone)
		default:
			outHandled = false;
			break;
		}
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT100::stateDeterminant, inEmulatorPtr,
						inNowOutNext, outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT102 in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT102 proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        VT102 bases this at least on character", kTriggerChar);
	
	return result;
}// My_VT102::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT102-specific state changes.

(3.1)
*/
UInt32
My_VT102::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case My_VT100::kStateDA:
		My_VT102::deviceAttributes(inDataPtr);
		break;
	
	case kStateControlLFVTFF:
		// line feed
		// all of these are interpreted the same for VT102;
		// when printing, this also forces a line to print
		// (for auto-print, but not printer controller mode)
		moveCursorDownOrScroll(inDataPtr);
		if (0 != inDataPtr->printingModes)
		{
			UInt8 const		kReturnChar = '\015';
			
			
			// the implementation is such that sending any new-line-like character
			// (return, whatever) will translate to the actual new-line sequence
			// defined for the stream
			StreamCapture_WriteUTF8Data(inDataPtr->printingStream, &kReturnChar, 1);
		}
		break;
	
	case kStateDCH:
		deleteCharacters(inDataPtr);
		break;
	
	case kStateDECLL:
		loadLEDs(inDataPtr);
		break;
	
	case kStateDL:
		deleteLines(inDataPtr);
		break;
	
	case kStateIL:
		insertLines(inDataPtr);
		break;
	
	case kStateMC:
		// media copy (automatic printing)
		{
			Boolean		printScreen = false;
			
			
			if (inDataPtr->emulator.argLastIndex >= 0)
			{
				switch (inDataPtr->emulator.argList[0])
				{
				case 5:
					// printer controller (print lines without necessarily displaying them);
					// this can be enabled or disabled while auto-print remains in effect
					inDataPtr->printingReset();
					inDataPtr->printingModes |= kMy_PrintingModePrintController;
					break;
				
				case 4:
					// printer controller off
					inDataPtr->printingModes &= ~kMy_PrintingModePrintController;
					inDataPtr->printingEnd();
					break;
				
				case 0:
					printScreen = true;
					break;
				
				case kMy_ParamPrivate:
					// private parameters (e.g. ESC [ ? 5 i)
					if (inDataPtr->emulator.argLastIndex >= 1)
					{
						switch (inDataPtr->emulator.argList[1])
						{
						case 5:
							// auto-print (print lines that are also displayed)
							inDataPtr->printingReset();
							inDataPtr->printingModes |= kMy_PrintingModeAutoPrint;
							break;
						
						case 4:
							// auto-print off
							inDataPtr->printingModes &= ~kMy_PrintingModeAutoPrint;
							inDataPtr->printingEnd();
							break;
						
						case 1:
							// print cursor line
							// UNIMPLEMENTED
							break;
						
						default:
							// ???
							if (DebugInterface_LogsTerminalInputChar())
							{
								Console_Warning(Console_WriteValue, "VT102 media copy did not recognize ?-parameter",
												inDataPtr->emulator.argList[1]);
							}
							break;
						}
					}
					break;
				
				default:
					if (DebugInterface_LogsTerminalInputChar())
					{
						Console_Warning(Console_WriteLine, "VT102 media copy did not recognize the given parameters");
					}
					break;
				}
			}
			else
			{
				// no parameters; defaults to “print screen”
				printScreen = true;
			}
			
			if (printScreen)
			{
				// print the screen or the scrolling region, based on
				// the most recent use of the DECEXT sequence
				// UNIMPLEMENTED
			}
		}
		break;
	
	default:
		// other state transitions should still basically be handled as if in VT100 ANSI
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateTransitionProc
					(My_VT100::stateTransition, inDataPtr,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT102::stateTransition


/*!
Handles the VT220 'DECSCL' sequence.  See the VT220 manual for
complete details.

(4.0)
*/
inline void
My_VT220::
compatibilityLevel		(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->emulator.argLastIndex >= 0)
	{
		switch (inDataPtr->emulator.argList[0])
		{
		case 61:
			// VT100 mode
			inDataPtr->emulator.set8BitReceive(false);
			inDataPtr->emulator.set8BitTransmit(false);
			// INCOMPLETE
			break;
		
		case 62:
			// VT200 mode
			if (inDataPtr->emulator.argLastIndex >= 1)
			{
				Boolean		isEightBit = true;
				
				
				switch (inDataPtr->emulator.argList[1])
				{
				case 0:
				case 2:
					// 8-bit (also the same as having no 2nd parameter)
					break;
				
				case 1:
					// 7-bit
					isEightBit = false;
					break;
				
				default:
					// ???
					if (DebugInterface_LogsTerminalInputChar())
					{
						Console_Warning(Console_WriteValue, "VT220 in DECSCL/VT200 mode did not expect parameter", inDataPtr->emulator.argList[1]);
					}
					break;
				}
				inDataPtr->emulator.set8BitReceive(isEightBit);
				inDataPtr->emulator.set8BitTransmit(isEightBit);
				// INCOMPLETE
			}
			break;
		
		default:
			// ???
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValue, "VT220 in DECSCL mode did not expect parameter", inDataPtr->emulator.argList[0]);
			}
			break;
		}
	}
}// My_VT220::compatibilityLevel


/*!
Handles the VT220 'DSR' sequence for the keyboard language.
See the VT220 manual for complete details.

INCOMPLETE

(4.0)
*/
inline void
My_VT220::
deviceStatusReportKeyboardLanguage		(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		inDataPtr->emulator.sendEscape(session, "\033[?27", 5);
		// TEMPORARY; the keyboard cannot be changed this way right now,
		// so the response is always “unknown” (0)
		inDataPtr->emulator.sendEscape(session, ";0", 2);
		// insert any other parameters here, with semicolons
		inDataPtr->emulator.sendEscape(session, "n", 1);
	}
}// My_VT220::deviceStatusReportKeyboardLanguage


/*!
Handles the VT220 'DSR' sequence for the printer port.
See the VT220 manual for complete details.

(4.0)
*/
inline void
My_VT220::
deviceStatusReportPrinterPort	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// possible responses are "?13" (no printer), "?10" (ready), "?11" (not ready)
		inDataPtr->emulator.sendEscape(session, "\033[?10n", 6);
	}
}// My_VT220::deviceStatusReportPrinterPort


/*!
Handles the VT220 'DSR' sequence for user-defined keys.
See the VT220 manual for complete details.

INCOMPLETE

(4.0)
*/
inline void
My_VT220::
deviceStatusReportUserDefinedKeys	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// possible responses are "?20" (unlocked), "?21" (locked)
		// UNIMPLEMENTED
		inDataPtr->emulator.sendEscape(session, "\033[?20n", 6);
	}
}// My_VT220::deviceStatusReportUserDefinedKeys


/*!
Handles the VT220 'ECH' sequence.

This should accept zero or one parameters.  With no parameters,
a single character is erased at the character position, without
offsetting anything after that point.  Otherwise, the parameter
refers to the number of characters to erase (and again, without
affecting anything outside that range on the line).

(4.0)
*/
void
My_VT220::
eraseCharacters		(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		characterCount = inDataPtr->emulator.argList[0];
	
	
	if (0 != characterCount)
	{
		if (kMy_ParamUndefined == characterCount)
		{
			characterCount = 1;
		}
		
		bufferEraseFromCursorColumn(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse(), characterCount);
	}
}// My_VT220::eraseCharacters


/*!
A standard "My_EmulatorResetProcPtr" to reset VT220-specific
settings.

A soft reset should return settings to the defaults specified
in the VT220 manual (Table 4-10).

A VT220 hard reset is additionally responsible for:
- Clearing UDKs.  (UNIMPLEMENTED)
- Clearing down-line-loaded character sets.  (UNIMPLEMENTED)
- Clearing the screen.  (Handled by buffer reset.)
- Homing the cursor.  (Handled by buffer reset.)
- Setting SGR (text style) to normal.  (Handled by buffer reset.)
- Setting selective erase to do-not-erase.  (UNIMPLEMENTED)
- Setting character sets to the defaults.  (Handled by buffer reset.)

(4.0)
*/
void
My_VT220::
hardSoftReset	(My_EmulatorPtr		inEmulatorPtr,
				 Boolean			inIsSoftReset)
{
	My_VT100::hardSoftReset(inEmulatorPtr, inIsSoftReset);
	
	// debug
	//Console_WriteValue("    <<< VT220 has reset, soft", inIsSoftReset);
}// hardSoftReset


/*!
Handles the VT220 'ICH' sequence.

This should accept zero or one parameters.  With no parameters,
a single blank character is inserted at the character position,
moving the rest of the line over by one column.  Otherwise, the
parameter refers to the number of blank characters to insert
(shifting the line by that amount).

(4.0)
*/
void
My_VT220::
insertBlankCharacters	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		characterCount = inDataPtr->emulator.argList[0];
	
	
	if (0 != characterCount)
	{
		SInt16				preWriteCursorX = inDataPtr->current.cursorX;
		My_ScreenRowIndex	preWriteCursorY = inDataPtr->current.cursorY;
		
		
		if (kMy_ParamUndefined == characterCount)
		{
			characterCount = 1;
		}
		
		bufferInsertBlanksAtCursorColumnWithoutUpdate(inDataPtr, characterCount,
														inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermBCE)
														? kMy_AttributeRuleCopyLatentBackground
														: kMy_AttributeRuleInitialize);
		
		// add the effects of the insert to the text-change region;
		// this should trigger things like Terminal View updates
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = preWriteCursorY;
			if (preWriteCursorY != inDataPtr->current.cursorY)
			{
				// more than one line; just draw all lines completely
				range.firstColumn = 0;
				range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
				range.rowCount = inDataPtr->current.cursorY - preWriteCursorY + 1;
			}
			else
			{
				// invalidate the entire line starting from the original cursor column
				range.firstColumn = preWriteCursorX;
				range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - preWriteCursorX;
				range.rowCount = 1;
			}
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// My_VT220::insertBlankCharacters


/*!
Handles the VT220 'DA' sequence for primary device attributes.
See the VT220 manual for complete details.

(3.0)
*/
inline void
My_VT220::
primaryDeviceAttributes		(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// possible parameters to return are:
		// (62) service class 2 terminal
		// (1) if 132 columns
		// (2) if printer port
		// (3) if ReGIS graphics
		// (4) if Sixel graphics
		// (6) if selective erase
		// (7) if DRCS
		// (8) if UDK
		// (9) if 7-bit national replacement character sets supported
		// INCOMPLETE
		inDataPtr->emulator.sendEscape(session, "\033[?62", 5);
		if (Terminal_ReturnColumnCount(inDataPtr->selfRef) >= 132)
		{
			inDataPtr->emulator.sendEscape(session, ";1", 2);
		}
		inDataPtr->emulator.sendEscape(session, ";2", 2);
		if (inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagSixelGraphics))
		{
			inDataPtr->emulator.sendEscape(session, ";4", 2);
		}
		inDataPtr->emulator.sendEscape(session, ";6", 2);
		// insert any other parameters here, with semicolons
		inDataPtr->emulator.sendEscape(session, "c", 1);
	}
}// My_VT220::primaryDeviceAttributes


/*!
Returns the next logical state of CSI parameter processing,
given the current state and the most recent code point.

Parameter sequences can be terminated by many characters, and
this must be checked from multiple states and emulators, so
this routine was created to simplify maintenance.

(4.0)
*/
My_ParserState
My_VT220::
returnCSINextState		(My_ParserState			inPreviousState,
						 UnicodeScalarValue		inCodePoint,
						 Boolean&				outHandled)
{
	My_ParserState		result = inPreviousState;
	
	
	outHandled = true; // initially...
	
	if (kMy_ParserStateSeenESCLeftSqBracketParamsQuotes == inPreviousState)
	{
		// the weird double-terminator cases ("p, "q) are handled by using two states
		switch (inCodePoint)
		{
		case 'p':
			result = kStateDECSCL;
			break;
		
		case 'q':
			result = kStateDECSCA;
			break;
		
		default:
			outHandled = false;
			break;
		}
	}
	else if (kMy_ParserStateSeenESCLeftSqBracketParamsDollarSign == inPreviousState)
	{
		// the weird double-terminator cases (like $p) are handled by using two states
		switch (inCodePoint)
		{
		case 'p':
			result = kStateDECRQM;
			break;
		
		default:
			outHandled = false;
			break;
		}
	}
	else if (kMy_ParserStateSeenESCLeftSqBracketParamsSpace == inPreviousState)
	{
		// the weird double-terminator cases (like space-q) are handled by using two states
		switch (inCodePoint)
		{
		case 'q':
			result = kStateDECSCUSR;
			break;
		
		default:
			outHandled = false;
			break;
		}
	}
	else
	{
		// there should be an entry here for each parameter list terminator that is
		// valid AT LEAST in a VT220 terminal; any that are also valid in lesser
		// terminals can be omitted, since they will be handled in the VT102 fallback
		switch (inCodePoint)
		{
		case 'X':
			result = kMy_ParserStateSeenESCLeftSqBracketParamsX;
			break;
		
		case '@':
			result = kMy_ParserStateSeenESCLeftSqBracketParamsAt;
			break;
		
		case '$':
			result = kMy_ParserStateSeenESCLeftSqBracketParamsDollarSign;
			break;
		
		case '\"':
			result = kMy_ParserStateSeenESCLeftSqBracketParamsQuotes;
			break;
		
		case ' ':
			result = kMy_ParserStateSeenESCLeftSqBracketParamsSpace;
			break;
		
		case '>':
			// this is not a terminator; it may only come first
			if (My_VT100::kStateCSI == inPreviousState)
			{
				// this is identical to "kStateCSISecondaryDA"
				result = kMy_ParserStateSeenESCLeftSqBracketGreaterThan;
			}
			else
			{
				outHandled = false;
			}
			break;
		
		default:
			outHandled = false;
			break;
		}
	}
	
	if (false == outHandled)
	{
		// fall back to VT102
		result = My_VT102::returnCSINextState(inPreviousState, inCodePoint, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT220 in parameter state", inPreviousState);
	//Console_WriteValueFourChars(">>>     VT220 proposes parameter state", result);
	//Console_WriteValueUnicodePoint("        VT220 bases this at least on character", inCodePoint);
	
	return result;
}// My_VT220::returnCSINextState


/*!
Handles the VT300 'DECRQM' sequence for requests of DEC private
modes.  See the XTerm manual for complete details.

(2017.12)
*/
inline void
My_VT220::
requestDECPrivateMode	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		if (kMy_ParamPrivate != inDataPtr->emulator.argList[0])
		{
			// sequence apparently had no '?' in it (just CSI, params, '$', 'p')
			// UNDEFINED
		}
		else if (inDataPtr->emulator.argLastIndex >= 1)
		{
			SInt16 const			kModeValueUnrecognized = 0; // see DECRPM in manuals for values
			SInt16 const			kModeValueSet = 1; // see DECRPM in manuals for values
			SInt16 const			kModeValueReset = 2; // see DECRPM in manuals for values
			//SInt16 const			kModeValuePermanentlySet = 3; // see DECRPM in manuals for values
			//SInt16 const			kModeValuePermanentlyReset = 4; // see DECRPM in manuals for values
			SInt16					modeNumber = inDataPtr->emulator.argList[1];
			SInt16					modeValue = kModeValueUnrecognized;
			std::ostringstream		reportBuffer;
			
			
			// see DECSET values in XTerm manual for details
			switch (modeNumber)
			{
			case 12:
				// start blinking cursor
				modeValue = ((inDataPtr->cursorBlinking) ? kModeValueSet : kModeValueReset);
				break;
			
			default:
				// unknown or unsupported
				modeValue = kModeValueUnrecognized;
				break;
			}
			
			reportBuffer
			<< "\033[?" // start of CSI sequence for response
			<< modeNumber
			<< ";"
			<< modeValue
			<< "$y" // end of CSI sequence for response
			;
			std::string		reportBufferString = reportBuffer.str();
			inDataPtr->emulator.sendEscape(session, reportBufferString.c_str(), reportBufferString.size());
		}
	}
}// My_VT220::requestDECPrivateMode


/*!
Handles the VT220 'DA' sequence for secondary device attributes.
See the VT220 manual for complete details.

(4.0)
*/
inline void
My_VT220::
secondaryDeviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// an emulated terminal has no firmware, so this is a bit made-up;
		// it means VT220 version 1.0, no options
		inDataPtr->emulator.sendEscape(session, "\033[>1;10;0c", 10);
	}
}// My_VT220::secondaryDeviceAttributes


/*!
Handles the VT220 'DECSCA' sequence.  See the VT220 manual for
complete details.

(4.0)
*/
inline void
My_VT220::
selectCharacterAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->emulator.argLastIndex >= 0)
	{
		switch (inDataPtr->emulator.argList[0])
		{
		case 0:
			// all non-SGR attributes off (currently equivalent to a "2")
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_CannotErase);
			break;
		
		case 1:
			// character set CANNOT be erased by DECSEL/DECSED sequences
			inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_CannotErase);
			break;
		
		case 2:
			// character set CAN be erased by DECSEL/DECSED sequences
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_CannotErase);
			break;
		
		default:
			// ???
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValue, "VT220 in DECSCA mode did not expect parameter", inDataPtr->emulator.argList[0]);
			}
			break;
		}
	}
}// My_VT220::selectCharacterAttributes


/*!
Handles the VT220 'DECSCUSR' sequence.  See the VT220 manual for
complete details.

(2017.12)
*/
inline void
My_VT220::
selectCursorStyle	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->emulator.argLastIndex >= 0)
	{
		switch (inDataPtr->emulator.argList[0])
		{
		case kMy_ParamUndefined: // when nothing is given, the default value is 1
		case 0:
		case 1:
			inDataPtr->cursorType = kTerminal_CursorTypeBlock;
			inDataPtr->cursorBlinking = true;
			break;
		
		case 2:
			inDataPtr->cursorType = kTerminal_CursorTypeBlock;
			inDataPtr->cursorBlinking = false;
			break;
		
		case 3:
			inDataPtr->cursorType = kTerminal_CursorTypeUnderscore;
			inDataPtr->cursorBlinking = true;
			break;
		
		case 4:
			inDataPtr->cursorType = kTerminal_CursorTypeUnderscore;
			inDataPtr->cursorBlinking = false;
			break;
		
		case 5:
			// technically XTerm-only
			inDataPtr->cursorType = kTerminal_CursorTypeVerticalLine;
			inDataPtr->cursorBlinking = true;
			break;
		
		case 6:
			// technically XTerm-only
			inDataPtr->cursorType = kTerminal_CursorTypeVerticalLine;
			inDataPtr->cursorBlinking = false;
			break;
		
		default:
			// ???
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValue, "VT220 select-cursor-style did not recognize parameter",
								inDataPtr->emulator.argList[0]);
			}
			break;
		}
		
		// UNIMPLEMENTED: notify listeners of change to cursor setting
		// (currently, terminal views use a global preference and do
		// not read the cursor that may have been set from a terminal;
		// the terminal setting only affects later reporting)
	}
}// My_VT220::selectCursorStyle


/*!
Handles the VT220 'DECSED' sequence.  See the VT220
manual for complete details.

(4.0)
*/
inline void
My_VT220::
selectiveEraseInDisplay		(My_ScreenBufferPtr		inDataPtr)
{
	assert(kMy_ParamPrivate == inDataPtr->emulator.argList[0]);
	
	if (inDataPtr->emulator.argLastIndex >= 1)
	{
		switch (inDataPtr->emulator.argList[1])
		{
		case kMy_ParamUndefined: // when nothing is given, the default value is 0
		case 0:
			bufferEraseFromCursorToEnd(inDataPtr, inDataPtr->emulator.returnEraseEffectsForSelectiveUse());
			break;
		
		case 1:
			bufferEraseFromHomeToCursor(inDataPtr, inDataPtr->emulator.returnEraseEffectsForSelectiveUse());
			break;
		
		case 2:
			bufferEraseVisibleScreen(inDataPtr, inDataPtr->emulator.returnEraseEffectsForSelectiveUse());
			break;
		
		default:
			// ???
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValue, "VT220 selective-erase-in-display did not recognize ?-parameter",
								inDataPtr->emulator.argList[1]);
			}
			break;
		}
	}
}// My_VT220::selectiveEraseInDisplay


/*!
Handles the VT220 'DECSEL' sequence.  See the VT220
manual for complete details.

(4.0)
*/
inline void
My_VT220::
selectiveEraseInLine	(My_ScreenBufferPtr		inDataPtr)
{
	assert(kMy_ParamPrivate == inDataPtr->emulator.argList[0]);
	
	assert(kMy_ParamPrivate == inDataPtr->emulator.argList[0]);
	
	if (inDataPtr->emulator.argLastIndex >= 1)
	{
		switch (inDataPtr->emulator.argList[1])
		{
		case kMy_ParamUndefined: // when nothing is given, the default value is 0
		case 0:
			bufferEraseFromCursorColumnToLineEnd(inDataPtr, inDataPtr->emulator.returnEraseEffectsForSelectiveUse());
			break;
		
		case 1:
			bufferEraseFromLineBeginToCursorColumn(inDataPtr, inDataPtr->emulator.returnEraseEffectsForSelectiveUse());
			break;
		
		case 2:
			bufferEraseCursorLine(inDataPtr, inDataPtr->emulator.returnEraseEffectsForSelectiveUse());
			break;
		
		default:
			// ???
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValue, "VT220 selective-erase-in-line did not recognize ?-parameter",
								inDataPtr->emulator.argList[1]);
			}
			break;
		}
	}
}// My_VT220::selectiveEraseInLine


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT220-specific states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_VT220::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// first handle various parameter states
	switch (inNowOutNext.first)
	{
	case My_VT100::kStateCSI:
	case My_VT100::kStateCSIParamDigit0:
	case My_VT100::kStateCSIParamDigit1:
	case My_VT100::kStateCSIParamDigit2:
	case My_VT100::kStateCSIParamDigit3:
	case My_VT100::kStateCSIParamDigit4:
	case My_VT100::kStateCSIParamDigit5:
	case My_VT100::kStateCSIParamDigit6:
	case My_VT100::kStateCSIParamDigit7:
	case My_VT100::kStateCSIParamDigit8:
	case My_VT100::kStateCSIParamDigit9:
	case My_VT100::kStateCSIParamDigitSub:
	case My_VT100::kStateCSIParameterEnd:
	case My_VT100::kStateCSIPrivate:
	case kMy_ParserStateSeenESCLeftSqBracketParamsDollarSign:
	case kMy_ParserStateSeenESCLeftSqBracketParamsQuotes:
	case kMy_ParserStateSeenESCLeftSqBracketParamsSpace:
	case kStateCSISecondaryDA:
		inNowOutNext.second = My_VT220::returnCSINextState(inNowOutNext.first, kTriggerChar, outHandled);
		break;
	
	default:
		// not in a parameter
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		outHandled = true; // initially...
		
		// use the current state and the available data to determine the next logical state
		switch (inNowOutNext.first)
		{
		case kStateDCS:
			inNowOutNext.second = kStateDCSAcquireStr;
			result = 0; // do not absorb the unknown
			break;
		
		case kStateDCSAcquireStr:
			{
				// handle device control strings (DCS) commonly regardless of
				// what is in the string (a variety of different emulator
				// sequences use them); then, once the string is parsed out
				// of the primary state machine, perform a sub-parsing of the
				// string at transition time to determine further actions
				switch (kTriggerChar)
				{
				case '\007':
					inNowOutNext.second = My_VT220::kStateST;
					break;
				
				case '\033':
					inNowOutNext.second = kMy_ParserStateSeenESC;
					break;
				
				default:
					// continue extending the string until a known terminator is found
					inNowOutNext.second = kStateDCSAcquireStr;
					result = 0; // do not absorb the unknown
					break;
				}
			}
			break;
		
		// currently, no special cases are needed (they are handled
		// entirely by the default terminal transitioning between
		// states based on character sequences alone)
		default:
			outHandled = false;
			break;
		}
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT102::stateDeterminant, inEmulatorPtr, inNowOutNext,
						outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT220 in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT220 proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        VT220 bases this at least on character", kTriggerChar);
	
	return result;
}// My_VT220::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT220-specific state changes.

(3.1)
*/
UInt32
My_VT220::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case My_VT100::kStateDA:
		// device attributes (primary or secondary)
		{
			Boolean		isPrimary = false;
			Boolean		isSecondary = false;
			
			
			if (inDataPtr->emulator.argLastIndex >= 0)
			{
				switch (inDataPtr->emulator.argList[0])
				{
				case kMy_ParamSecondaryDA:
					isSecondary = true;
					break;
				
				case 0:
					isPrimary = true;
					break;
				
				default:
					// ???
					break;
				}
			}
			else
			{
				// no parameters; defaults to “primary”
				isPrimary = true;
			}
			
			if (isSecondary)
			{
				My_VT220::secondaryDeviceAttributes(inDataPtr);
			}
			else if (isPrimary)
			{
				My_VT220::primaryDeviceAttributes(inDataPtr);
			}
			else
			{
				// ??? - do nothing
			}
		}
		break;
	
	case My_VT100::kStateDECID:
		My_VT220::primaryDeviceAttributes(inDataPtr);
		break;
	
	case My_VT100::kStateDSR:
		if (inDataPtr->printingModes & kMy_PrintingModePrintController)
		{
			// print controller mode; request is received but not answered
		}
		else
		{
			if (inDataPtr->emulator.argLastIndex >= 0)
			{
				switch (inDataPtr->emulator.argList[0])
				{
				case kMy_ParamPrivate:
					// specific report parameters (e.g. ESC [ ? 1 5 n)
					if (inDataPtr->emulator.argLastIndex >= 1)
					{
						switch (inDataPtr->emulator.argList[1])
						{
						case 15:
							// printer status request
							My_VT220::deviceStatusReportPrinterPort(inDataPtr);
							break;
						
						case 25:
							// user-defined keys request for lock state
							My_VT220::deviceStatusReportUserDefinedKeys(inDataPtr);
							break;
						
						case 26:
							// keyboard language request
							My_VT220::deviceStatusReportKeyboardLanguage(inDataPtr);
							break;
						
						default:
							// ???
							if (DebugInterface_LogsTerminalInputChar())
							{
								Console_Warning(Console_WriteValue, "VT220 device status report did not recognize ?-parameter",
												inDataPtr->emulator.argList[1]);
							}
							break;
						}
					}
					break;
				
				default:
					// earlier models of VT terminal already support the other
					// kinds of device status report; pass to another handler
					outHandled = false;
					break;
				}
			}
			else
			{
				outHandled = false;
			}
		}
		break;
	
	case My_VT100::kStateED:
		if ((inDataPtr->emulator.argLastIndex > 0) &&
			(kMy_ParamPrivate == inDataPtr->emulator.argList[0]))
		{
			selectiveEraseInDisplay(inDataPtr);
		}
		else
		{
			// VT100 will handle this (probably a normal ED sequence)
			outHandled = false;
		}
		break;
	
	case My_VT100::kStateEL:
		if ((inDataPtr->emulator.argLastIndex > 0) &&
			(kMy_ParamPrivate == inDataPtr->emulator.argList[0]))
		{
			selectiveEraseInLine(inDataPtr);
		}
		else
		{
			// VT100 will handle this (probably a normal EL sequence)
			outHandled = false;
		}
		break;
	
	case kStateCSISecondaryDA:
		// flag to mark the control sequence as secondary device attributes
		inDataPtr->emulator.argList[inDataPtr->emulator.argLastIndex] = kMy_ParamSecondaryDA;
		++(inDataPtr->emulator.argLastIndex);
		break;
	
	case kStateDCS:
		if (DebugInterface_LogsTerminalState())
		{
			Console_WriteLine("device control string");
		}
		inDataPtr->emulator.clearEscapeSequenceParameters(); // should not be needed; just avoiding side effects
		inDataPtr->emulator.stringAccumulator.clear();
		inDataPtr->emulator.stringAccumulatorState = inOldNew.second;
		break;
	
	case kStateDCSAcquireStr:
		{
			if (DebugInterface_LogsTerminalState())
			{
				Console_WriteLine("reading DCS data"); // debug
			}
			
			if (inDataPtr->emulator.isUTF8Encoding)
			{
				if (UTF8Decoder_StateMachine::kStateUTF8ValidSequence == inDataPtr->emulator.multiByteDecoder.returnState())
				{
					std::copy(inDataPtr->emulator.multiByteDecoder.multiByteAccumulator.begin(),
								inDataPtr->emulator.multiByteDecoder.multiByteAccumulator.end(),
								std::back_inserter(inDataPtr->emulator.stringAccumulator));
					result = 1;
				}
			}
			else
			{
				inDataPtr->emulator.stringAccumulator.push_back(STATIC_CAST(inDataPtr->emulator.recentCodePoint(), UInt8));
				result = 1;
			}
		}
		break;
	
	case kStateDECRQM:
		// request DEC private mode
		My_VT220::requestDECPrivateMode(inDataPtr);
		break;
	
	case kStateDECSCA:
		// select character attributes (other than those set by SGR)
		My_VT220::selectCharacterAttributes(inDataPtr);
		break;
	
	case kStateDECSCL:
		// set terminal compatibility level
		My_VT220::compatibilityLevel(inDataPtr);
		break;
	
	case kStateDECSTR:
		// soft terminal reset; note that hard reset is handled by My_VT100::kStateRIS
		resetTerminal(inDataPtr, true/* is soft reset */);
		break;
	
	case kStateDECSCUSR:
		// select cursor style
		My_VT220::selectCursorStyle(inDataPtr);
		break;
	
	case kStateECH:
		My_VT220::eraseCharacters(inDataPtr);
		break;
	
	case kStateICH:
		My_VT220::insertBlankCharacters(inDataPtr);
		break;
	
	case kStateLS1R:
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteLine, "request for VT220 to lock shift G1 right side, which is unsupported");
		}
		break;
	
	case kStateLS2:
		// lock shift (left) the G2 setting
		if (false == inDataPtr->emulator.disableShifts)
		{
			inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG2;
			if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
			{
				// set attribute
				inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics);
			}
			else
			{
				// clear attribute
				inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics);
			}
		}
		break;
	
	case kStateLS2R:
		// lock shift (left) the G2 setting, but for the “right” (requiring 8 bits) code section
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteLine, "request for VT220 to lock shift G2 right side, which is unsupported");
		}
		break;
	
	case kStateLS3:
		// lock shift (left) the G3 setting
		if (false == inDataPtr->emulator.disableShifts)
		{
			inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG3;
			if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
			{
				// set attribute
				inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics);
			}
			else
			{
				// clear attribute
				inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics);
			}
		}
		break;
	
	case kStateLS3R:
		// lock shift (left) the G3 setting, but for the “right” (requiring 8 bits) code section
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteLine, "request for VT220 to lock shift G3 right side, which is unsupported");
		}
		break;
	
	case kStateS7C1T:
		inDataPtr->emulator.set8BitTransmit(false);
		break;
	
	case kStateS8C1T:
		inDataPtr->emulator.set8BitTransmit(true);
		break;
	
	case kStateSCSG0DECSupplemental:
	case kStateSCSG0Dutch:
	case kStateSCSG0Finnish1:
	case kStateSCSG0Finnish2:
	case kStateSCSG0French:
	case kStateSCSG0FrenchCdn:
	case kStateSCSG0German:
	case kStateSCSG0Italian:
	case kStateSCSG0Norwegian1:
	case kStateSCSG0Norwegian2:
	case kStateSCSG0Spanish:
	case kStateSCSG0Swedish1:
	case kStateSCSG0Swedish2:
	case kStateSCSG0Swiss:
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteValueFourChars, "request for VT220 to use unsupported G0 character set", inOldNew.second);
		}
		break;
	
	case kStateSCSG1DECSupplemental:
	case kStateSCSG1Dutch:
	case kStateSCSG1Finnish1:
	case kStateSCSG1Finnish2:
	case kStateSCSG1French:
	case kStateSCSG1FrenchCdn:
	case kStateSCSG1German:
	case kStateSCSG1Italian:
	case kStateSCSG1Norwegian1:
	case kStateSCSG1Norwegian2:
	case kStateSCSG1Spanish:
	case kStateSCSG1Swedish1:
	case kStateSCSG1Swedish2:
	case kStateSCSG1Swiss:
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteValueFourChars, "request for VT220 to use unsupported G1 character set", inOldNew.second);
		}
		break;
	
	case kStateSCSG2UK:
	case kStateSCSG3UK:
		if (false == inDataPtr->emulator.disableShifts)
		{
			// U.K. character set, normal ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG2;
			
			
			if (kStateSCSG3UK == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG3;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedKingdom;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG2ASCII:
	case kStateSCSG3ASCII:
		if (false == inDataPtr->emulator.disableShifts)
		{
			// U.S. character set, normal ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG2;
			
			
			if (kStateSCSG3ASCII == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG3;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedStates;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_VTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG2SG:
	case kStateSCSG3SG:
		if (false == inDataPtr->emulator.disableShifts)
		{
			// normal ROM, graphics mode
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG2;
			
			
			if (kStateSCSG3SG == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG3;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			inDataPtr->current.drawingAttributes.addAttributes(kTextAttributes_VTGraphics); // set graphics attribute
		}
		break;
	
	case kStateSCSG2DECSupplemental:
	case kStateSCSG2Dutch:
	case kStateSCSG2Finnish1:
	case kStateSCSG2Finnish2:
	case kStateSCSG2French:
	case kStateSCSG2FrenchCdn:
	case kStateSCSG2German:
	case kStateSCSG2Italian:
	case kStateSCSG2Norwegian1:
	case kStateSCSG2Norwegian2:
	case kStateSCSG2Spanish:
	case kStateSCSG2Swedish1:
	case kStateSCSG2Swedish2:
	case kStateSCSG2Swiss:
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteValueFourChars, "request for VT220 to use unsupported G2 character set", inOldNew.second);
		}
		break;
	
	case kStateSCSG3DECSupplemental:
	case kStateSCSG3Dutch:
	case kStateSCSG3Finnish1:
	case kStateSCSG3Finnish2:
	case kStateSCSG3French:
	case kStateSCSG3FrenchCdn:
	case kStateSCSG3German:
	case kStateSCSG3Italian:
	case kStateSCSG3Norwegian1:
	case kStateSCSG3Norwegian2:
	case kStateSCSG3Spanish:
	case kStateSCSG3Swedish1:
	case kStateSCSG3Swedish2:
	case kStateSCSG3Swiss:
		if (false == inDataPtr->emulator.disableShifts)
		{
			Console_Warning(Console_WriteValueFourChars, "request for VT220 to use unsupported G3 character set", inOldNew.second);
		}
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		// other state transitions should still basically be handled as if in VT100
		result = invokeEmulatorStateTransitionProc
					(My_VT102::stateTransition, inDataPtr,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT220::stateTransition


/*!
Handles the XTerm 'CBT' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved backwards to the first tab stop on the
current line.  Otherwise, the parameter refers to the number of
tab stops to move.

See also the handling of the tab character in the VT terminal,
which tabs in the opposite direction.

(4.0)
*/
void
My_XTerm::
cursorBackwardTabulation	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		tabCount = 1;
	
	
	if (kMy_ParamUndefined != inDataPtr->emulator.argList[0])
	{
		tabCount = inDataPtr->emulator.argList[0];
	}
	
	for (SInt16 i = 0; i < tabCount; ++i)
	{
		moveCursorLeftToNextTabStop(inDataPtr);
	}
}// My_XTerm::cursorBackwardTabulation


/*!
Handles the XTerm 'CHA' sequence.

This should accept up to 2 parameters.  With no parameters, the
cursor is moved to the first position on the current line.  If
there is just one parameter, it is a one-based index for the new
cursor column on the current line.  And with two parameters, the
order changes and the parameters are the one-based indices of the
new cursor row and column, in that order.

NOTE:	It is not clear yet if this should be any different from
		the implementation of horizontalPositionAbsolute().
		Currently they are the same.

(4.0)
*/
void
My_XTerm::
cursorCharacterAbsolute		(My_ScreenBufferPtr		inDataPtr)
{
	horizontalPositionAbsolute(inDataPtr);
}// My_XTerm::cursorCharacterAbsolute


/*!
Handles the XTerm 'CHT' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved forwards to the first tab stop on the
current line.  Otherwise, the parameter refers to the number of
tab stops to move.

See also the handling of the tab character in the VT terminal.

(4.0)
*/
void
My_XTerm::
cursorForwardTabulation		(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		tabCount = 1;
	
	
	if (kMy_ParamUndefined != inDataPtr->emulator.argList[0])
	{
		tabCount = inDataPtr->emulator.argList[0];
	}
	
	for (SInt16 i = 0; i < tabCount; ++i)
	{
		moveCursorRightToNextTabStop(inDataPtr);
	}
}// My_XTerm::cursorForwardTabulation


/*!
Handles the XTerm 'CNL' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved to the next line.  Otherwise, it is moved
the specified number of lines downward, limited by the visible
screen area.  In any case, the cursor is also reset to the
first column of the display.

(4.0)
*/
void
My_XTerm::
cursorNextLine		(My_ScreenBufferPtr		inDataPtr)
{
	My_ScreenRowIndex	newY = inDataPtr->current.cursorY;
	
	
	if (kMy_ParamUndefined == inDataPtr->emulator.argList[0])
	{
		++newY;
	}
	else
	{
		newY += inDataPtr->emulator.argList[0];
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, 0, newY);
}// My_XTerm::cursorNextLine


/*!
Handles the XTerm 'CPL' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved to the previous line.  Otherwise, it is
moved the specified number of lines upward, limited by the
visible screen area.  In any case, the cursor is also reset to
the first column of the display.

(4.0)
*/
void
My_XTerm::
cursorPreviousLine		(My_ScreenBufferPtr		inDataPtr)
{
	My_ScreenRowIndex	newY = inDataPtr->current.cursorY;
	
	
	if (kMy_ParamUndefined == inDataPtr->emulator.argList[0])
	{
		--newY;
	}
	else
	{
		newY -= inDataPtr->emulator.argList[0];
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, 0, newY);
}// My_XTerm::cursorPreviousLine


/*!
Handles the XTerm 'HPA' sequence.

This should accept up to 2 parameters.  With no parameters, the
cursor is moved to the first position on the current line.  If
there is just one parameter, it is a one-based index for the new
cursor column on the current line.  And with two parameters, the
order changes and the parameters are the one-based indices of the
new cursor row and column, in that order.

See also the 'CUP' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
horizontalPositionAbsolute	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16 const		kParam0 = inDataPtr->emulator.argList[0];
	SInt16 const		kParam1 = inDataPtr->emulator.argList[1];
	SInt16				newX = 0;
	My_ScreenRowIndex	newY = inDataPtr->current.cursorY;
	
	
	// in the following definitions, 0 and 1 are considered the same number
	if (kMy_ParamUndefined == kParam1)
	{
		// only one parameter was given, so it is the column and
		// the cursor row does not change
		newX = (0 == kParam0)
				? 0
				: (kMy_ParamUndefined != kParam0)
					? kParam0 - 1
					: 0/* default is column 1 in current row */;
	}
	else
	{
		// two parameters, so the order changes to (row, column)
		newY = (0 == kParam0)
				? 0
				: (kMy_ParamUndefined != kParam0)
					? STATIC_CAST(kParam0 - 1, My_ScreenRowIndex)
					: inDataPtr->current.cursorY/* default is current cursor row */;
		newX = (0 == kParam1)
				? 0
				: kParam1 - 1;
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, newX, newY);
}// My_XTerm::horizontalPositionAbsolute


/*!
Sends the XTerm 'DECRPSS' response for Set Cursor Style (SP q).
See the XTerm manual for complete details.

(2017.12)
*/
void
My_XTerm::
reportCursorStyle	(My_ScreenBufferPtr		inDataPtr)
{
	// send response
	SessionRef	session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		std::ostringstream		reportBuffer;
		Terminal_CursorType		cursorShape = kTerminal_CursorTypeBlock;
		Boolean					cursorBlinks = true;
		
		
	#if 0
		// report cursor shape that is in use (currently corresponds
		// to a global preference setting); the problem with this is
		// that the reported setting will not agree with any previous
		// attempt to change the setting from the terminal 
		{
			Preferences_Result		prefsResult = kPreferences_ResultOK;
			
			
			prefsResult = Preferences_GetData(kPreferences_TagTerminalCursorType, sizeof(cursorShape), &cursorShape);
			if (kPreferences_ResultOK != prefsResult)
			{
				// assume a default if preference can’t be found
				cursorShape = kTerminal_CursorTypeBlock;
			}
			
			prefsResult = Preferences_GetData(kPreferences_TagCursorBlinks, sizeof(cursorBlinks), &cursorBlinks);
			if (kPreferences_ResultOK != prefsResult)
			{
				// assume a default if preference can’t be found
				cursorBlinks = true;
			}
		}
	#else
		// report only default setting and/or whatever might have
		// been previously modified by a DECSCUSR sequence
		cursorShape = inDataPtr->cursorType;
		cursorBlinks = inDataPtr->cursorBlinking;
	#endif
		
		// (see DECSCUSR documentation for possible return values here)
		reportBuffer
		<< "\033[" // start of CSI sequence for cursor setting
		;
		switch (cursorShape)
		{
		case kTerminal_CursorTypeUnderscore:
		case kTerminal_CursorTypeThickUnderscore:
			if (cursorBlinks)
			{
				reportBuffer << "3";
			}
			else
			{
				reportBuffer << "4";
			}
			break;
		
		case kTerminal_CursorTypeVerticalLine:
		case kTerminal_CursorTypeThickVerticalLine:
			if (cursorBlinks)
			{
				reportBuffer << "5";
			}
			else
			{
				reportBuffer << "6";
			}
			break;
		
		case kTerminal_CursorTypeBlock:
		default:
			if (cursorBlinks)
			{
				reportBuffer << "1";
			}
			else
			{
				reportBuffer << "2";
			}
			break;
		}
		reportBuffer
		<< " q" // end of CSI sequence for cursor setting
		;
		inDataPtr->emulator.sendEscape(session, "\033P", 2/* string length */); // DCS (device control string)
		inDataPtr->emulator.sendEscape(session, "1$r", 3/* string length */); // 1 = code for “valid” in XTerm
		std::string		reportBufferString = reportBuffer.str();
		inDataPtr->emulator.sendEscape(session, reportBufferString.c_str(), reportBufferString.size());
		inDataPtr->emulator.sendEscape(session, "\033\\", 2/* string length */); // ST (string terminator)
	}
}// My_XTerm::reportCursorStyle


/*!
Sends the XTerm 'DECRPSS' response for any case that is not
recognized.  See the XTerm manual for complete details.

(2017.12)
*/
void
My_XTerm::
reportSelectionSettingNotUsed	(My_ScreenBufferPtr		inDataPtr)
{
	// send response
	SessionRef	session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		inDataPtr->emulator.sendEscape(session, "\033P", 2/* string length */); // DCS (device control string)
		inDataPtr->emulator.sendEscape(session, "0$r", 3/* string length */); // 0 = code for “invalid” in XTerm
		inDataPtr->emulator.sendEscape(session, "\033\\", 2/* string length */); // ST (string terminator)
	}
}// My_XTerm::reportSelectionSettingNotUsed


/*!
Sends the XTerm 'DECRPSS' response for Set Top/Bottom Margins (r).
See the XTerm manual for complete details.

(2018.01)
*/
void
My_XTerm::
reportTopBottomMargins	(My_ScreenBufferPtr		inDataPtr)
{
	// send response
	SessionRef	session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		std::ostringstream		reportBuffer;
		
		
		// (see DECSTBM documentation for possible return values here)
		reportBuffer
		<< "\033[" // start of CSI sequence for top/bottom margin setting
		<< inDataPtr->customScrollingRegion.firstRow
		<< ";"
		<< inDataPtr->customScrollingRegion.lastRow
		<< "r" // end of CSI sequence for top/bottom margin setting
		;
		inDataPtr->emulator.sendEscape(session, "\033P", 2/* string length */); // DCS (device control string)
		inDataPtr->emulator.sendEscape(session, "1$r", 3/* string length */); // 1 = code for “valid” in XTerm
		std::string		reportBufferString = reportBuffer.str();
		inDataPtr->emulator.sendEscape(session, reportBufferString.c_str(), reportBufferString.size());
		inDataPtr->emulator.sendEscape(session, "\033\\", 2/* string length */); // ST (string terminator)
	}
}// My_XTerm::reportTopBottomMargins


/*!
Returns the next logical state of CSI parameter processing,
given the current state and the most recent code point.

Parameter sequences can be terminated by many characters, and
this must be checked from multiple states and emulators, so
this routine was created to simplify maintenance.

(4.0)
*/
My_ParserState
My_XTerm::
returnCSINextState		(My_ParserState			inPreviousState,
						 UnicodeScalarValue		inCodePoint,
						 Boolean&				outHandled)
{
	My_ParserState		result = inPreviousState;
	
	
	outHandled = true; // initially...
	
	// there should be an entry here for each parameter list terminator that is
	// valid AT LEAST in an XTerm terminal; any that are also valid in lesser
	// terminals can be omitted, since they will be handled in the VT220 fallback
	switch (inCodePoint)
	{
	case 'd':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsd;
		break;
	
	case 'E':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsE;
		break;
	
	case 'F':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsF;
		break;
	
	case 'G':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsG;
		break;
	
	case 'I':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsI;
		break;
	
	case 'S':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsS;
		break;
	
	case 'T':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsT;
		break;
	
	case 'Z':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsZ;
		break;
	
	case '`':
		result = kMy_ParserStateSeenESCLeftSqBracketParamsBackquote;
		break;
	
	case '=':
		// this is not a terminator; it may only come first
		if (My_VT100::kStateCSI == inPreviousState)
		{
			// this is identical to "kStateCSITertiaryDA"
			result = kMy_ParserStateSeenESCLeftSqBracketEquals;
		}
		else
		{
			outHandled = false;
		}
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		// fall back to VT220
		result = My_VT220::returnCSINextState(inPreviousState, inCodePoint, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< XTerm in parameter state", inPreviousState);
	//Console_WriteValueFourChars(">>>     XTerm proposes parameter state", result);
	//Console_WriteValueUnicodePoint("        XTerm bases this at least on character", inCodePoint);
	
	return result;
}// My_XTerm::returnCSINextState


/*!
Handles the XTerm 'SD' sequence.

This should accept zero or one parameters.  With no parameters,
the screen content moves down by one line (which from a user’s
point of view is like clicking an up-arrow in a scroll bar).
If a parameter is given, it is the number of rows to scroll by.

See also the 'IND' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
scrollDown	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		lineCount = inDataPtr->emulator.argList[0];
	
	
	if (kMy_ParamUndefined == lineCount)
	{
		lineCount = 1;
	}
	
	// the following function handles the scrolling region boundaries automatically
	screenScroll(inDataPtr, -lineCount);
}// My_XTerm::scrollDown


/*!
Handles the XTerm 'SU' sequence.

This should accept zero or one parameters.  With no parameters,
the screen content moves up by one line (which from a user’s
point of view is like clicking a down-arrow in a scroll bar).
If a parameter is given, it is the number of rows to scroll by.

See also the 'RI' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
scrollUp	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		lineCount = inDataPtr->emulator.argList[0];
	
	
	if (kMy_ParamUndefined == lineCount)
	{
		lineCount = 1;
	}
	
	// the following function handles the scrolling region boundaries automatically
	screenScroll(inDataPtr, lineCount);
}// My_XTerm::scrollUp


/*!
Handles the VT220 'DA' sequence for secondary device attributes
but with different values for XTerm.

See the VT220 manual for complete details, and note that XTerm
uses its XFree86 “patch level” for the middle value.

(2018.01)
*/
inline void
My_XTerm::
secondaryDeviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		std::ostringstream		reportBuffer;
		
		
		// an emulated terminal has no firmware, so this is a bit made-up;
		// it means VT220, no options supported; the middle value is hacked
		// by XTerm to be the “XFree86 patch number”, which is unfortunate
		// because MacTerm obviously does not have this and applications
		// such as "vim" change behavior based on it; the best that can be
		// done is to return a value that is at least the minimum patch
		// defined (95), and provide a low-level override for anything that
		// needs to hack in a different patch number
		inDataPtr->emulator.sendEscape(session, "\033[>", 3/* string length */); // response header
		reportBuffer
		<< "1;" // identification code; 1 means VT220
		<< inDataPtr->reportedPatchLevel << ";" // XTerm “firmware version” hacked to be “XFree86 patch level”
		<< "0" // options value; always zero (0)
		;
		std::string		reportBufferString = reportBuffer.str();
		inDataPtr->emulator.sendEscape(session, reportBufferString.c_str(), reportBufferString.size());
		inDataPtr->emulator.sendEscape(session, "c", 1/* string length */); // response terminator
	}
}// My_XTerm::secondaryDeviceAttributes


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
XTerm-specific window states based on the characters of the
given buffer.

(4.0)
*/
UInt32
My_XTerm::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// first handle various parameter states
	switch (inNowOutNext.first)
	{
	case My_VT100::kStateCSI:
	case My_VT100::kStateCSIParamDigit0:
	case My_VT100::kStateCSIParamDigit1:
	case My_VT100::kStateCSIParamDigit2:
	case My_VT100::kStateCSIParamDigit3:
	case My_VT100::kStateCSIParamDigit4:
	case My_VT100::kStateCSIParamDigit5:
	case My_VT100::kStateCSIParamDigit6:
	case My_VT100::kStateCSIParamDigit7:
	case My_VT100::kStateCSIParamDigit8:
	case My_VT100::kStateCSIParamDigit9:
	case My_VT100::kStateCSIParamDigitSub:
	case My_VT100::kStateCSIParameterEnd:
	case My_VT100::kStateCSIPrivate:
	case My_VT220::kStateCSISecondaryDA:
	case kStateCSITertiaryDA:
		inNowOutNext.second = My_XTerm::returnCSINextState(inNowOutNext.first, kTriggerChar, outHandled);
		break;
	
	default:
		// not in a parameter
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		// use the current state and the available data to determine the next logical state
		switch (inNowOutNext.first)
		{
		default:
			// call the XTerm “core” to handle the sequence and update "outHandled" appropriately;
			// note that the core IS NOT a complete emulator, which is why this routine falls
			// back to a proper VT220 at the end instead of falling back to the core exclusively
			result = invokeEmulatorStateDeterminantProc
						(My_XTermCore::stateDeterminant, inEmulatorPtr, inNowOutNext,
							outInterrupt, outHandled);
			break;
		}
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT220::stateDeterminant, inEmulatorPtr, inNowOutNext,
						outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< XTerm in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     XTerm proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        XTerm bases this at least on character", kTriggerChar);
	
	return result;
}// My_XTerm::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds to
XTerm-specific window state changes.

(4.0)
*/
UInt32
My_XTerm::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case My_VT100::kStateDA:
		// device attributes (primary or secondary)
		{
			Boolean		isPrimary = false;
			Boolean		isSecondary = false;
			
			
			if (inDataPtr->emulator.argLastIndex >= 0)
			{
				switch (inDataPtr->emulator.argList[0])
				{
				case kMy_ParamSecondaryDA:
					isSecondary = true;
					break;
				
				case 0:
					isPrimary = true;
					break;
				
				default:
					// ???
					break;
				}
			}
			else
			{
				// no parameters; defaults to “primary”
				isPrimary = true;
			}
			
			if (isSecondary)
			{
				My_XTerm::secondaryDeviceAttributes(inDataPtr);
			}
			else if (isPrimary)
			{
				My_VT220::primaryDeviceAttributes(inDataPtr);
			}
			else
			{
				// ??? - do nothing
			}
		}
		break;
	
	case kStateCBT:
		cursorBackwardTabulation(inDataPtr);
		break;
	
	case kStateCHA:
		cursorCharacterAbsolute(inDataPtr);
		break;
	
	case kStateCHT:
		cursorForwardTabulation(inDataPtr);
		break;
	
	case kStateCNL:
		cursorNextLine(inDataPtr);
		break;
	
	case kStateCPL:
		cursorPreviousLine(inDataPtr);
		break;
	
	case kStateCSITertiaryDA:
		// flag to mark the control sequence as tertiary device attributes
		inDataPtr->emulator.argList[inDataPtr->emulator.argLastIndex] = kMy_ParamTertiaryDA;
		++(inDataPtr->emulator.argLastIndex);
		break;
	
	case kStateHPA:
		horizontalPositionAbsolute(inDataPtr);
		break;
	
	case kStateSD:
		scrollDown(inDataPtr);
		break;
	
	case kStateSU:
		scrollUp(inDataPtr);
		break;
	
	case kStateVPA:
		verticalPositionAbsolute(inDataPtr);
		break;
	
	default:
		// call the XTerm “core” to handle the sequence and update "outHandled" appropriately;
		// note that the core IS NOT a complete emulator, which is why this routine falls
		// back to a proper VT220 at the end instead of falling back to the core exclusively
		result = invokeEmulatorStateTransitionProc
					(My_XTermCore::stateTransition, inDataPtr,
						inOldNew, outHandled);
		break;
	}
	
	if (false == outHandled)
	{
		// other state transitions should still basically be handled as if in VT100
		result = invokeEmulatorStateTransitionProc
					(My_VT220::stateTransition, inDataPtr,
						inOldNew, outHandled);
	}
	
	return result;
}// My_XTerm::stateTransition


/*!
Handles the XTerm 'VPA' sequence.

This should accept up to 2 parameters.  With no parameters, the
cursor is moved to the first row, but the same cursor column.  If
there is just one parameter, it is a one-based index for the new
cursor row, using the same column.  And with two parameters, the
parameters are the one-based indices of the new cursor row and
column, in that order.

See also the 'CUP' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
verticalPositionAbsolute	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16 const		kParam0 = inDataPtr->emulator.argList[0];
	SInt16 const		kParam1 = inDataPtr->emulator.argList[1];
	SInt16				newX = inDataPtr->current.cursorX;
	My_ScreenRowIndex	newY = 0;
	
	
	// in the following definitions, 0 and 1 are considered the same number
	if (kMy_ParamUndefined == kParam1)
	{
		// only one parameter was given, so it is the row and
		// the cursor column does not change
		newY = (0 == kParam0)
				? 0
				: (kMy_ParamUndefined != kParam0)
					? kParam0 - 1
					: 0/* default is row 1 in current column */;
	}
	else
	{
		// two parameters, giving (row, column)
		newY = (0 == kParam0)
				? 0
				: (kMy_ParamUndefined != kParam0)
					? kParam0 - 1
					: 0/* default is the home cursor row */;
		newX = (0 == kParam1)
				? 0
				: kParam1 - 1;
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, newX, newY);
}// My_XTerm::verticalPositionAbsolute


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
XTerm-specific window states based on the characters of the
given buffer.

(4.0)
*/
UInt32
My_XTermCore::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				outHandled)
{
	UInt32 const	kTriggerChar = inEmulatorPtr->recentCodePoint();
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	switch (inNowOutNext.first)
	{
	#if 0
	case kMy_ParserStateSeenESC:
	case kMy_ParserStateSeenESCRightSqBracket:
		result = inEmulatorPtr->currentCallbacks.stateDeterminant(inEmulatorPtr, inBuffer, inLength,
																	inNowOutNext, outInterrupt, outHandled);
		break;
	#endif
	
	case kStateSWITAcquireStr:
		if (inEmulatorPtr->supportsVariant(My_Emulator::kVariantFlagXTermAlterWindow))
		{
			switch (kTriggerChar)
			{
			case '\007':
				inNowOutNext.second = My_VT220::kStateST;
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string until a known terminator is found
				inNowOutNext.second = kStateSWITAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateSITAcquireStr:
		if (inEmulatorPtr->supportsVariant(My_Emulator::kVariantFlagXTermAlterWindow))
		{
			switch (kTriggerChar)
			{
			case '\007':
				inNowOutNext.second = My_VT220::kStateST;
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string until a known terminator is found
				inNowOutNext.second = kStateSITAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateSWTAcquireStr:
		if (inEmulatorPtr->supportsVariant(My_Emulator::kVariantFlagXTermAlterWindow))
		{
			switch (kTriggerChar)
			{
			case '\007':
				inNowOutNext.second = My_VT220::kStateST;
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string until a known terminator is found
				inNowOutNext.second = kStateSWTAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateColorAcquireStr:
		if (inEmulatorPtr->supportsVariant(My_Emulator::kVariantFlagXTerm256Color))
		{
			switch (kTriggerChar)
			{
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string; termination is an escape sequence (a different state)
				inNowOutNext.second = kStateColorAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	default:
		// other states are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	// debug
	//Console_WriteValueFourChars("    <<< XTerm in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     XTerm proposes state", inNowOutNext.second);
	//Console_WriteValueUnicodePoint("        XTerm bases this at least on character", kTriggerChar);
	
	return result;
}// My_XTermCore::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds to
XTerm-specific window state changes.

(4.0)
*/
UInt32
My_XTermCore::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateSWIT:
	case kStateSIT:
	case kStateSWT:
	case kStateSetColor:
		inDataPtr->emulator.stringAccumulator.clear();
		inDataPtr->emulator.stringAccumulatorState = inOldNew.second;
		break;
	
	case kStateColorAcquireStr:
	case kStateSWITAcquireStr:
	case kStateSITAcquireStr:
	case kStateSWTAcquireStr:
		// upon first entry to the state, ignore the code point (semicolon)
		// that caused the state to be selected; only accumulate code points
		// that occurred while the previous state was also accumulating
		if (inOldNew.first == inOldNew.second)
		{
			if (inDataPtr->emulator.isUTF8Encoding)
			{
				if (UTF8Decoder_StateMachine::kStateUTF8ValidSequence == inDataPtr->emulator.multiByteDecoder.returnState())
				{
					std::copy(inDataPtr->emulator.multiByteDecoder.multiByteAccumulator.begin(),
								inDataPtr->emulator.multiByteDecoder.multiByteAccumulator.end(),
								std::back_inserter(inDataPtr->emulator.stringAccumulator));
					result = 1;
				}
			}
			else
			{
				inDataPtr->emulator.stringAccumulator.push_back(STATIC_CAST(inDataPtr->emulator.recentCodePoint(), UInt8));
				result = 1;
			}
		}
		break;
	
	case My_VT220::kStateST:
		// a string-based terminal sequence can be terminated in multiple
		// ways; if a new-style XTerm string terminator is seen, then the
		// action must be chosen based on the state that most recently used
		// the string buffer
		switch (inDataPtr->emulator.stringAccumulatorState)
		{
		case My_VT220::kStateDCS:
			{
				// scan the device control string for the expected terminators;
				// otherwise, pass to a parent handler (e.g. might be Sixel data)
				ParameterDecoder_StateMachine				paramDecoder;
				std::basic_string< UInt8 >::const_iterator	pastEndParams = inDataPtr->emulator.stringAccumulator.end();
				
				
				getParametersFromStringAccumulator(inDataPtr, paramDecoder, pastEndParams);
				if ((inDataPtr->emulator.stringAccumulator.end() != pastEndParams) && ('$' == *pastEndParams) &&
					(inDataPtr->emulator.stringAccumulator.end() != (1 + pastEndParams)) && ('q' == *(1 + pastEndParams)))
				{
					// “ESC P $ q” (DECRQSS)
					typedef decltype(inDataPtr->emulator.stringAccumulator.size())	StringAccumulatorSize;
					decltype(pastEndParams) const	kMainStringBegin = (pastEndParams + 2/* skip terminating '$q' */);
					decltype(pastEndParams) const	kPastEndMainString = inDataPtr->emulator.stringAccumulator.end();
					StringAccumulatorSize const		kStringDataSize = std::distance(kMainStringBegin, kPastEndMainString);
					Boolean							requestHandled = true;
					
					
					if (DebugInterface_LogsTerminalState())
					{
						std::basic_string< UInt8 >		requestString(kMainStringBegin, kPastEndMainString);
						
						
						Console_WriteValueCString("received request for selection or setting, string", REINTERPRET_CAST(requestString.c_str(), char const*));
					}
					
					if (1 == kStringDataSize)
					{
						if ('r' == *kMainStringBegin)
						{
							// report value for DECSTBM from XTerm (or VT100)
							My_XTerm::reportTopBottomMargins(inDataPtr);
							requestHandled = true;
						}
						else
						{
							// INCOMPLETE; dozens of other XTerm sequences should be supported here
						}
					}
					else if (2 == kStringDataSize)
					{
						if ((' '/* space */ == *kMainStringBegin) && ('q' == *(1 + kMainStringBegin)))
						{
							// report value for DECSCUSR from XTerm (or VT520)
							My_XTerm::reportCursorStyle(inDataPtr);
							requestHandled = true;
						}
						else
						{
							// INCOMPLETE; dozens of other XTerm sequences should be supported here
						}
					}
					
					if (false == requestHandled)
					{
						// most requests are not recognized right now; give a default response
						My_XTerm::reportSelectionSettingNotUsed(inDataPtr);
					}
					
					inDataPtr->emulator.stringAccumulator.clear();
					inDataPtr->emulator.stringAccumulatorState = kMy_ParserStateInitial;
				}
			}
			break;
		
		case kStateSWIT:
		case kStateSIT:
		case kStateSWT:
			if (inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermAlterWindow))
			{
				CFRetainRelease		titleCFString(CFStringCreateWithBytes(kCFAllocatorDefault,
																			inDataPtr->emulator.stringAccumulator.c_str(),
																			inDataPtr->emulator.stringAccumulator.size(),
																			inDataPtr->emulator.inputTextEncoding,
																			false/* is external representation */),
													CFRetainRelease::kAlreadyRetained);
				
				
				if (titleCFString.exists())
				{
					if ((kStateSWIT == inDataPtr->emulator.stringAccumulatorState) ||
						(kStateSWT == inDataPtr->emulator.stringAccumulatorState))
					{
						inDataPtr->windowTitleCFString = titleCFString;
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowFrameTitle, inDataPtr->selfRef/* context */);
					}
					if ((kStateSWIT == inDataPtr->emulator.stringAccumulatorState) ||
						(kStateSIT == inDataPtr->emulator.stringAccumulatorState))
					{
						inDataPtr->iconTitleCFString = titleCFString;
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowIconTitle, inDataPtr->selfRef/* context */);
					}
				}
				
				inDataPtr->emulator.stringAccumulator.clear();
				inDataPtr->emulator.stringAccumulatorState = kMy_ParserStateInitial;
			}
			else
			{
				// ignore
				outHandled = false;
			}
			break;
		
		case kStateSetColor:
			if (inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTerm256Color))
			{
				// the accumulated string up to this point actually needs to be parsed; currently,
				// only strings of this form are allowed:
				//		%d;rgb:%x/%x/%x - set color at index, to red, green, blue components in hex
				char const*		stringPtr = REINTERPRET_CAST(inDataPtr->emulator.stringAccumulator.c_str(), char const*);
				int				i = 0;
				int				r = 0;
				int				g = 0;
				int				b = 0;
				int				scanResult = sscanf(stringPtr, "%d;rgb:%x/%x/%x", &i, &r, &g, &b);
				
				
				if (4 == scanResult)
				{
					if ((i > 255) || (r > 255) || (g > 255) || (b > 255) ||
						(i < 16/* cannot overwrite base ANSI colors */) || (r < 0) || (g < 0) || (b < 0))
					{
						Console_Warning(Console_WriteValueFloat4, "one or more illegal indices found in request to set XTerm color: index, red, green, blue", i, r, g, b);
					}
					else
					{
						// success!
						Terminal_XTermColorDescription		colorInfo;
						
						
						bzero(&colorInfo, sizeof(colorInfo));
						colorInfo.screen = inDataPtr->selfRef;
						colorInfo.index = STATIC_CAST(i, UInt8);
						colorInfo.redComponent = STATIC_CAST(r, UInt8);
						colorInfo.greenComponent = STATIC_CAST(g, UInt8);
						colorInfo.blueComponent = STATIC_CAST(b, UInt8);
						
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeXTermColor, &colorInfo/* context */);
						
						//Console_WriteValueFloat4("set color at index to red, green, blue", i, r, g, b);
					}
				}
				else
				{
					Console_Warning(Console_WriteValue, "failed to parse XTerm color string; sscanf() result", scanResult);
					Console_Warning(Console_WriteValueCString, "discarding unrecognized syntax for XTerm color string", stringPtr);
				}
				
				inDataPtr->emulator.stringAccumulator.clear();
				inDataPtr->emulator.stringAccumulatorState = kMy_ParserStateInitial;
			}
			else
			{
				// ignore
				outHandled = false;
			}
			break;
		
		default:
			// ignore
			outHandled = false;
			break;
		}
		break;
	
	default:
		// other state transitions are not handled at all
		outHandled = false;
		break;
	}
	
	// WARNING: Do not call any other full emulator here!  This is a
	//          pre-callback ONLY, which is designed to fall through
	//          to something else (such as a VT100).
	
	return result;
}// My_XTermCore::stateTransition


/*!
Performs various assertions on the current custom scrolling
region range, to make sure all values are valid.

This should be called if the range changes for any reason.

(4.0)
*/
inline void
assertScrollingRegion	(My_ScreenBufferPtr		inDataPtr)
{
	assert(inDataPtr->customScrollingRegion.lastRow >= inDataPtr->customScrollingRegion.firstRow);
	assert(inDataPtr->customScrollingRegion.lastRow < inDataPtr->screenBuffer.size());
	assert(inDataPtr->screenBuffer.size() >= (inDataPtr->customScrollingRegion.lastRow - inDataPtr->customScrollingRegion.firstRow));
}// assertScrollingRegion


/*!
Erases the entire line containing the cursor.

(2.6)
*/
void
bufferEraseCursorLine	(My_ScreenBufferPtr		inDataPtr,
						 My_BufferChanges		inChanges)
{
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	SInt16								postWrapCursorX = 0;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// clear appropriate attributes and text
	bufferEraseLineWithoutUpdate(inDataPtr, inChanges, **cursorLineIterator);
	
	// add the entire row contents to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase line");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->current.returnNumberOfColumnsPermitted();
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseCursorLine


/*!
Erases the specified number of characters on the cursor row,
starting from the cursor position.

(4.0)
*/
void
bufferEraseFromCursorColumn		(My_ScreenBufferPtr		inDataPtr,
								 My_BufferChanges		inChanges,
								 UInt16					inCharacterCount)
{
	TerminalLine_TextIterator						textIterator = nullptr;
	TerminalLine_TextIterator						endText = nullptr;
	TerminalLine_TextAttributesList::iterator		attrIterator;
	TerminalLine_TextAttributesList::iterator		endAttrs;
	My_ScreenBufferLineList::iterator				cursorLineIterator;
	SInt16											postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex								postWrapCursorY = inDataPtr->current.cursorY;
	UInt16											fillDistance = inCharacterCount;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// do not overflow the line buffer
	fillDistance = std::min(fillDistance, STATIC_CAST(inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX, UInt16));
	
	// find the right line offset
	attrIterator = (*cursorLineIterator)->returnMutableAttributeVector().begin();
	std::advance(attrIterator, postWrapCursorX);
	endAttrs = attrIterator;
	std::advance(endAttrs, fillDistance);
	textIterator = (*cursorLineIterator)->textVectorBegin;
	std::advance(textIterator, postWrapCursorX);
	endText = textIterator;
	std::advance(endText, fillDistance);
	
	// change attributes if appropriate; note that since a partial line is
	// being erased, "kMy_BufferChangesResetLineAttributes" does NOT apply
	if (inChanges & kMy_BufferChangesResetCharacterAttributes)
	{
		std::fill(attrIterator, endAttrs, (*cursorLineIterator)->returnGlobalAttributes());
	}
	if (inChanges & kMy_BufferChangesKeepBackgroundColor)
	{
		TerminalLine_TextAttributesList::iterator		tmpAttrIterator;
		
		
		for (tmpAttrIterator = attrIterator; tmpAttrIterator != endAttrs; ++tmpAttrIterator)
		{
			(*tmpAttrIterator).colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
		}
	}
	
	// clear out the specified part of the screen line
	if (inChanges & kMy_BufferChangesEraseAllText)
	{
		std::fill(textIterator, endText, ' ');
	}
	else
	{
		// erase only erasable characters
		for (; textIterator != endText; ++textIterator, ++attrIterator)
		{
			if ((endAttrs == attrIterator) || (false == (*attrIterator).hasAttributes(kTextAttributes_CannotErase)))
			{
				*textIterator = ' ';
			}
		}
	}
	
	// add the remainder of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from cursor column to line end");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = fillDistance;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromCursorColumn


/*!
Erases characters on the cursor row, from the cursor
position to the end of the line.

(2.6)
*/
void
bufferEraseFromCursorColumnToLineEnd	(My_ScreenBufferPtr		inDataPtr,
										 My_BufferChanges		inChanges)
{
	TerminalLine_TextIterator						textIterator = nullptr;
	TerminalLine_TextIterator						endText = nullptr;
	TerminalLine_TextAttributesList::iterator		attrIterator;
	TerminalLine_TextAttributesList::iterator		endAttrs;
	My_ScreenBufferLineList::iterator				cursorLineIterator;
	SInt16											postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex								postWrapCursorY = inDataPtr->current.cursorY;
	
	
	// if the cursor is positioned so as to trigger an erase
	// of the entire top line, and forced saving is enabled,
	// be sure to dump the screen contents into the scrollback
	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->mayNeedToSaveToScrollback))
	{
		inDataPtr->mayNeedToSaveToScrollback = false;
		screenCopyLinesToScrollback(inDataPtr);
	}
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// find the right line offset
	attrIterator = (*cursorLineIterator)->returnMutableAttributeVector().begin();
	std::advance(attrIterator, postWrapCursorX);
	endAttrs = (*cursorLineIterator)->returnMutableAttributeVector().end();
	textIterator = (*cursorLineIterator)->textVectorBegin;
	std::advance(textIterator, postWrapCursorX);
	endText = (*cursorLineIterator)->textVectorEnd;
	
	// change attributes if appropriate; note that since a partial line is
	// being erased, "kMy_BufferChangesResetLineAttributes" does NOT apply
	if (inChanges & kMy_BufferChangesResetCharacterAttributes)
	{
		std::fill(attrIterator, endAttrs, (*cursorLineIterator)->returnGlobalAttributes());
	}
	if (inChanges & kMy_BufferChangesKeepBackgroundColor)
	{
		TerminalLine_TextAttributesList::iterator		tmpAttrIterator;
		
		
		for (tmpAttrIterator = attrIterator; tmpAttrIterator != endAttrs; ++tmpAttrIterator)
		{
			(*tmpAttrIterator).colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
		}
	}
	
	// clear out the specified screen line
	if (inChanges & kMy_BufferChangesEraseAllText)
	{
		std::fill(textIterator, endText, ' ');
	}
	else
	{
		// erase only erasable characters
		for (; textIterator != endText; ++textIterator, ++attrIterator)
		{
			if ((endAttrs == attrIterator) || (false == (*attrIterator).hasAttributes(kTextAttributes_CannotErase)))
			{
				*textIterator = ' ';
			}
		}
	}
	
	// add the remainder of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from cursor column to line end");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromCursorColumnToLineEnd


/*!
Erases characters from the current cursor position
up to the end position (last line, last column).

(2.6)
*/
void
bufferEraseFromCursorToEnd  (My_ScreenBufferPtr		inDataPtr,
							 My_BufferChanges		inChanges)
{
	My_ScreenRowIndex	postWrapCursorY = inDataPtr->current.cursorY + 1;
	
	
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// if the cursor is in the home position, this action will
	// erase the entire visible screen; so, check the preference
	// for saving to scrollback, and copy the data if necessary
 	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->mayNeedToSaveToScrollback))
	{
		inDataPtr->mayNeedToSaveToScrollback = false;
		screenCopyLinesToScrollback(inDataPtr);
	}
	
	// blank out current line from cursor to end, and update
	bufferEraseFromCursorColumnToLineEnd(inDataPtr, inChanges);
	
	// blank out following rows
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		
		// skip cursor line
		++postWrapCursorY;
		++lineIterator;
		for (; lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, inChanges, **lineIterator);
		}
	}
	
	// add all the remaining rows to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from cursor to screen end");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = inDataPtr->screenBuffer.size() - postWrapCursorY + 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromCursorToEnd


/*!
Erases characters from the beginning of the screen
up to and including the current cursor position.
For example, if the cursor is at line 3, column 6,
lines 1 and 2 are erased as well as the first 6
columns of line 3.

(2.6)
*/
void
bufferEraseFromHomeToCursor		(My_ScreenBufferPtr		inDataPtr,
								 My_BufferChanges		inChanges)
{
	My_ScreenRowIndex	postWrapCursorY = 0;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// blank out current line from beginning to cursor
	bufferEraseFromLineBeginToCursorColumn(inDataPtr, inChanges);
	
	// blank out previous rows
	{
		My_ScreenBufferLineList::iterator	lineIterator = inDataPtr->screenBuffer.begin();
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		for (; lineIterator != cursorLineIterator; ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, inChanges, **lineIterator);
		}
	}
	
	// add all the pre-cursor rows to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from home to cursor");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = postWrapCursorY - 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromHomeToCursor


/*!
Erases characters on the cursor row, from the start of
the line up to and including the cursor position.

(2.6)
*/
void
bufferEraseFromLineBeginToCursorColumn  (My_ScreenBufferPtr		inDataPtr,
										 My_BufferChanges		inChanges)
{
	TerminalLine_TextIterator						textIterator = nullptr;
	TerminalLine_TextIterator						endText = nullptr;
	TerminalLine_TextAttributesList::iterator		attrIterator;
	TerminalLine_TextAttributesList::iterator		endAttrs;
	My_ScreenBufferLineList::iterator				cursorLineIterator;
	SInt16											postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex								postWrapCursorY = inDataPtr->current.cursorY;
	UInt16											fillDistance = 0;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	fillDistance = 1 + postWrapCursorX;
	
	// find the right line offset
	attrIterator = (*cursorLineIterator)->returnMutableAttributeVector().begin();
	endAttrs = attrIterator;
	std::advance(endAttrs, fillDistance);
	textIterator = (*cursorLineIterator)->textVectorBegin;
	endText = textIterator;
	std::advance(endText, fillDistance);
	
	// change attributes if appropriate; note that since a partial line is
	// being erased, "kMy_BufferChangesResetLineAttributes" does NOT apply
	if (inChanges & kMy_BufferChangesResetCharacterAttributes)
	{
		std::fill(attrIterator, endAttrs, (*cursorLineIterator)->returnGlobalAttributes());
	}
	if (inChanges & kMy_BufferChangesKeepBackgroundColor)
	{
		TerminalLine_TextAttributesList::iterator		tmpAttrIterator;
		
		
		for (tmpAttrIterator = attrIterator; tmpAttrIterator != endAttrs; ++tmpAttrIterator)
		{
			(*tmpAttrIterator).colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
		}
	}
	
	// clear out the specified part of the screen line
	if (inChanges & kMy_BufferChangesEraseAllText)
	{
		std::fill(textIterator, endText, ' ');
	}
	else
	{
		// erase only erasable characters
		for (; textIterator != endText; ++textIterator, ++attrIterator)
		{
			if ((endAttrs == attrIterator) || (false == (*attrIterator).hasAttributes(kTextAttributes_CannotErase)))
			{
				*textIterator = ' ';
			}
		}
	}
	
	// add the first part of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from line begin to cursor column");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = 0;
		range.columnCount = fillDistance;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromLineBeginToCursorColumn


/*!
Erases an entire line in the screen buffer without triggering
a redraw.  See also bufferEraseCursorLine().

This should generally only be used as a helper to implement
more specific "bufferErase...()" routines.

(2.6)
*/
void
bufferEraseLineWithoutUpdate	(My_ScreenBufferPtr  	inDataPtr,
								 My_BufferChanges		inChanges,
								 My_ScreenBufferLine&	inRow)
{
	TerminalLine_TextIterator						textIterator = nullptr;
	TerminalLine_TextIterator						endText = nullptr;
	TerminalLine_TextAttributesList::iterator		attrIterator;
	TerminalLine_TextAttributesList::iterator		endAttrs;
	
	
	// find the right line offset
	attrIterator = inRow.returnMutableAttributeVector().begin();
	endAttrs = inRow.returnMutableAttributeVector().end();
	textIterator = inRow.textVectorBegin;
	endText = inRow.textVectorEnd;
	
	// 3.0 - line-global attributes are cleared only if clearing an entire line
	inRow.returnMutableGlobalAttributes().clear();
	
	// change attributes if appropriate
	if (inChanges & kMy_BufferChangesResetLineAttributes)
	{
		inRow.returnMutableGlobalAttributes().clear();
	}
	if (inChanges & kMy_BufferChangesResetCharacterAttributes)
	{
		std::fill(attrIterator, endAttrs, inRow.returnGlobalAttributes());
	}
	if (inChanges & kMy_BufferChangesKeepBackgroundColor)
	{
		TerminalLine_TextAttributesList::iterator		tmpAttrIterator;
		
		
		for (tmpAttrIterator = attrIterator; tmpAttrIterator != endAttrs; ++tmpAttrIterator)
		{
			(*tmpAttrIterator).colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
		}
	}
	
	// clear out the screen line
	if (inChanges & kMy_BufferChangesEraseAllText)
	{
		std::fill(textIterator, endText, ' ');
	}
	else
	{
		// erase only erasable characters
		for (; textIterator != endText; ++textIterator, ++attrIterator)
		{
			if ((endAttrs == attrIterator) || (false == (*attrIterator).hasAttributes(kTextAttributes_CannotErase)))
			{
				*textIterator = ' ';
			}
		}
	}
}// bufferEraseLineWithoutUpdate


/*!
Clears the screen, first saving its contents in the scrollback
buffer if that flag is turned on.  A screen redraw is triggered.

(2.6)
*/
void
bufferEraseVisibleScreen	(My_ScreenBufferPtr		inDataPtr,
							 My_BufferChanges		inChanges)
{
	//Console_WriteLine("bufferEraseVisibleScreen");
	
	// save screen contents in scrollback buffer, if appropriate
	if (inDataPtr->saveToScrollbackOnClear)
	{
		screenCopyLinesToScrollback(inDataPtr);
	}
	
	// clear buffer
	for (auto& lineInfo : inDataPtr->screenBuffer)
	{
		bufferEraseLineWithoutUpdate(inDataPtr, inChanges, *lineInfo);
	}
	
	// add the entire visible buffer to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase visible screen");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = inDataPtr->screenBuffer.size();
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseVisibleScreen


/*!
Inserts the specified number of blank lines, scrolling the
remaining ones down and dropping any that fall off the end
of the scrolling region.  The display is updated.

New blank lines normally have cleared attributes.  If however
"inAttributeRule" is set to "kMy_AttributeRuleCopyLast", they
will instead copy the attributes of the given insertion line.

See also bufferRemoveLines().

NOTE:   You cannot use this to alter the scrollback.

WARNING:	The specified line MUST be part of the terminal
			scrolling region.

(3.0)
*/
void
bufferInsertBlankLines	(My_ScreenBufferPtr						inDataPtr,
						 UInt16									inNumberOfLines,
						 My_ScreenBufferLineList::iterator&		inInsertionLine,
						 My_AttributeRule						inAttributeRule)
{
	My_ScreenBufferLineList::iterator	scrollingRegionBegin;
	My_ScreenBufferLineList::iterator	scrollingRegionEnd;
	
	
	locateScrollingRegion(inDataPtr, scrollingRegionBegin, scrollingRegionEnd);
	if (0 != inNumberOfLines)
	{
		// the row index MUST be calculated immediately, since inserting lines into the
		// buffer might make it impossible to find this at the end (where this index is
		// actually needed)
		My_ScreenRowIndex const						kFirstInsertedRow = std::distance(inDataPtr->screenBuffer.begin(), inInsertionLine);
		My_ScreenRowIndex const						kLinesUntilEnd = std::distance(inInsertionLine, scrollingRegionEnd);
		My_ScreenRowIndex const						kMostLines = std::min(STATIC_CAST(inNumberOfLines, My_ScreenRowIndex), kLinesUntilEnd);
		My_ScreenBufferLineList::size_type const	kBufferSize = inDataPtr->screenBuffer.size();
		My_ScreenBufferLineList::iterator			pastLastKeptLine;
		
		
		// insert blank lines
		if ((kMy_AttributeRuleCopyLast == inAttributeRule) && (scrollingRegionEnd != scrollingRegionBegin))
		{
			My_ScreenBufferLinePtr		lineTemplate = createLinePtr();
			
			
			// copy attributes of the insertion line, making a special exception
			// to prevent bitmaps from being copied
			std::copy((*inInsertionLine)->returnAttributeVector().begin(),
						(*inInsertionLine)->returnAttributeVector().end(),
						lineTemplate->returnMutableAttributeVector().begin());
			for (auto& attributeFlags : lineTemplate->returnMutableAttributeVector())
			{
				attributeFlags.removeImageRelatedAttributes();
			}
			lineTemplate->returnMutableGlobalAttributes() = (*inInsertionLine)->returnGlobalAttributes();
			inDataPtr->screenBuffer.insert(inInsertionLine, kMostLines, lineTemplate);
		}
		else if (kMy_AttributeRuleCopyLatentBackground == inAttributeRule)
		{
			My_ScreenBufferLinePtr		lineTemplate = createLinePtr();
			
			
			// the new lines have no attributes EXCEPT for a custom background color
			for (auto& attributeFlags : lineTemplate->returnMutableAttributeVector())
			{
				attributeFlags.colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
			}
			inDataPtr->screenBuffer.insert(inInsertionLine, kMostLines, lineTemplate);
		}
		else
		{
			inDataPtr->screenBuffer.insert(inInsertionLine, kMostLines, createLinePtr());
		}
		
		// delete last lines
		pastLastKeptLine = scrollingRegionEnd;
		std::advance(pastLastKeptLine, -STATIC_CAST(kMostLines, SInt16));
		inDataPtr->screenBuffer.erase(pastLastKeptLine, scrollingRegionEnd);
		
		assert(kBufferSize == inDataPtr->screenBuffer.size());
		
		// redraw the area
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = kFirstInsertedRow;
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = inDataPtr->customScrollingRegion.lastRow - range.firstRow + 1;
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// bufferInsertBlankLines


/*!
Inserts the specified number of blank characters at the current
cursor position, truncating that many characters from the end of
the line when the line is shifted.  The display is NOT updated.

If "inAttributeRule" is "kMy_AttributeRuleCopyLatentBackground",
then the background color attribute of each new blank character
comes from the most recent background color setting.

(2.6)
*/
void
bufferInsertBlanksAtCursorColumnWithoutUpdate	(My_ScreenBufferPtr		inDataPtr,
												 SInt16					inNumberOfBlankCharactersToInsert,
												 My_AttributeRule		inAttributeRule)
{
	UInt16								numBlanksToAdd = inNumberOfBlankCharactersToInsert;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_ScreenBufferLineList::iterator	toCursorLine;
	TextAttributes_Object				copiedAttributes;
	
	
	// wrap cursor
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, toCursorLine);
	
	// since the caller cannot know for sure if the cursor wrapped,
	// do bounds-checking between the screen edge and new location
	if ((postWrapCursorX + numBlanksToAdd) >= inDataPtr->current.returnNumberOfColumnsPermitted())
	{
		numBlanksToAdd = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
	}
	
	// change attributes if necessary
	copiedAttributes = (*toCursorLine)->returnGlobalAttributes();
	if (kMy_AttributeRuleCopyLatentBackground == inAttributeRule)
	{
		copiedAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
	}
	
	// update attributes
	{
		TerminalLine_TextAttributesList::iterator		pastVisibleEnd = (*toCursorLine)->returnMutableAttributeVector().begin();
		TerminalLine_TextAttributesList::iterator		toCursorAttr = (*toCursorLine)->returnMutableAttributeVector().begin();
		TerminalLine_TextAttributesList::iterator		toFirstRelocatedAttr;
		TerminalLine_TextAttributesList::iterator		pastLastPreservedAttr;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastPreservedAttr = pastVisibleEnd;
		
		std::advance(toCursorAttr, postWrapCursorX);
		toFirstRelocatedAttr = toCursorAttr;
		std::advance(toFirstRelocatedAttr, numBlanksToAdd);
		std::advance(pastLastPreservedAttr, -numBlanksToAdd);
		
		std::copy_backward(toCursorAttr, pastLastPreservedAttr, pastVisibleEnd);
		std::fill(toCursorAttr, toFirstRelocatedAttr, copiedAttributes);
	}
	
	// update text
	{
		TerminalLine_TextIterator		pastVisibleEnd = (*toCursorLine)->textVectorBegin;
		TerminalLine_TextIterator		toCursorChar = (*toCursorLine)->textVectorBegin;
		TerminalLine_TextIterator		toFirstRelocatedChar;
		TerminalLine_TextIterator		pastLastPreservedChar;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastPreservedChar = pastVisibleEnd;
		
		std::advance(toCursorChar, postWrapCursorX);
		toFirstRelocatedChar = toCursorChar;
		std::advance(toFirstRelocatedChar, numBlanksToAdd);
		std::advance(pastLastPreservedChar, -numBlanksToAdd);
		
		std::copy_backward(toCursorChar, pastLastPreservedChar, pastVisibleEnd);
		std::fill(toCursorChar, toFirstRelocatedChar, ' ');
	}
}// bufferInsertBlanksAtCursorColumnWithoutUpdate


/*!
Shifts the text cursor based on the number of terminal cells
required to render the given image, updating the attributes
of all affected cells to display individual segments of the
image.  The display is NOT updated.

If "inAllowScrolling" is set to true, the terminal scrolls
if the cursor shifts past the bottom of the screen and will
continue to scroll as long as the image grows that way.

If "inRestoreCursor" is set to true, the cursor returns to
its original position after the image is inserted instead
of having a new location at the end of the image.

(2017.12)
*/
void
bufferInsertInlineImageWithoutUpdate	(My_ScreenBufferPtr		inDataPtr,
										 NSImage*				inCompleteImage,
										 UInt16					inTotalPixelsH,
										 UInt16					inTotalPixelsV,
										 UInt16					inCellPixelsH,
										 UInt16					inCellPixelsV,
										 Boolean				inAllowScrolling,
										 Boolean				inRestoreCursor)
{
	UInt16 const	kCellsCoveredH = std::max< UInt16 >(1, STATIC_CAST(roundf(STATIC_CAST(inTotalPixelsH, Float32) /
																				STATIC_CAST(inCellPixelsH, Float32)),
																		UInt16));
	UInt16 const	kCellsCoveredV = std::max< UInt16 >(1, STATIC_CAST(roundf(STATIC_CAST(inTotalPixelsV, Float32) /
																				STATIC_CAST(inCellPixelsV, Float32)),
																		UInt16));
	NSRect			subImageRect = NSZeroRect; // initialized below
	
	
	if (DebugInterface_LogsSixelDecoderSummary())
	{
		Console_WriteValue("terminal columns covered by image", kCellsCoveredH);
		Console_WriteValue("terminal rows covered by image", kCellsCoveredV);
		if (DebugInterface_LogsSixelDecoderErrors())
		{
			if (kCellsCoveredH > inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
			{
				Console_Warning(Console_WriteLine, "the Sixel image has been clipped on the right side (terminal screen not wide enough)");
			}
			if (kCellsCoveredV > inDataPtr->screenBuffer.size())
			{
				Console_Warning(Console_WriteLine, "the Sixel image has been clipped or scrolled vertically (terminal screen not tall enough)");
			}
		}
	}
	
	// initialize first sub-rectangle (rendered by one terminal cell)
	subImageRect = NSMakeRect(0, inCompleteImage.size.height - inCellPixelsV,
								inCellPixelsH, inCellPixelsV);
	
	// since the image is being rendered inline by the terminal, shift the text
	// cursor location according to the number of text cells that are occupied
	UInt16 const		kOriginalCursorX = inDataPtr->current.cursorX;
	UInt16 const		kOriginalCursorY = inDataPtr->current.cursorY;
	for (UInt16 i = 0; i < kCellsCoveredV; ++i)
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		moveCursorX(inDataPtr, kOriginalCursorX);
		subImageRect.origin.x = 0;
		locateCursorLine(inDataPtr, cursorLineIterator);
		for (UInt16 j = 0; j < kCellsCoveredH; ++j)
		{
			TextAttributes_BitmapID		cellBitmapID;
			TextAttributes_Object		newAttributes;
			
			
			// set attributes on all affected text cells to allow them to render their
			// assigned portion of the overall image
			if (false == defineBitmap(inDataPtr, inCompleteImage, subImageRect, cellBitmapID))
			{
				if (DebugInterface_LogsSixelDecoderErrors())
				{
					Console_Warning(Console_WriteLine, "failed to allocate a cell bitmap");
				}
			}
			else
			{
				newAttributes.bitmapIDSet(cellBitmapID);
			}
			changeLineRangeAttributes(inDataPtr, *(*cursorLineIterator),
										inDataPtr->current.cursorX, 1 + inDataPtr->current.cursorX,
										newAttributes/* attributes to set */,
										TextAttributes_Object(0xFFFFFFFF, 0xFFFFFFFF)/* attributes to clear */);
			
			moveCursorRight(inDataPtr);
			subImageRect.origin.x += subImageRect.size.width;
		}
		
		if (inAllowScrolling)
		{
			moveCursorDownOrScroll(inDataPtr);
		}
		else
		{
			moveCursorDown(inDataPtr);
		}
		subImageRect.origin.y -= subImageRect.size.height;
	}
	moveCursorX(inDataPtr, kOriginalCursorX);
	if (inRestoreCursor)
	{
		moveCursorY(inDataPtr, kOriginalCursorY);
	}
}// bufferInsertInlineImageWithoutUpdate


/*!
Efficiently overwrites the entire buffer for the given
line, such that every character on the line becomes the
given fill character and every character’s attributes
match the given fill attributes.  You may also update
the line global attributes, which define the default
attributes effective regardless of individual character
attributes on the line.

(3.0)
*/
void
bufferLineFill	(My_ScreenBufferPtr			UNUSED_ARGUMENT(inDataPtr),
				 My_ScreenBufferLine&		inRow,
				 UInt8						inFillCharacter,
				 TextAttributes_Object		inFillAttributes,
				 Boolean					inUpdateLineGlobalAttributesAlso)
{
	std::fill(inRow.textVectorBegin, inRow.textVectorEnd, inFillCharacter);
	std::fill(inRow.returnMutableAttributeVector().begin(), inRow.returnMutableAttributeVector().end(), inFillAttributes);
	if (inUpdateLineGlobalAttributesAlso)
	{
		inRow.returnMutableGlobalAttributes() = inFillAttributes;
	}
}// bufferLineFill


/*!
Deletes characters starting at the current cursor position and
moving forwards.  The rest of the line is shifted to the left
as a result.

If "inAttributeRule" is set to "kMy_AttributeRuleCopyLast",
all new blank characters copy the attributes of the last
character on the cursor line.  If the rule is set to
"kMy_AttributeRuleCopyLatentBackground", then the background
color attribute comes from the most recent background color
setting.

(3.0)
*/
void
bufferRemoveCharactersAtCursorColumn	(My_ScreenBufferPtr		inDataPtr,
										 SInt16					inNumberOfCharactersToDelete,
										 My_AttributeRule		inAttributeRule)
{
	UInt16								numCharsToRemove = inNumberOfCharactersToDelete;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_ScreenBufferLineList::iterator	toCursorLine;
	TextAttributes_Object				copiedAttributes;
	
	
	// wrap cursor
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, toCursorLine);
	
	// since the caller cannot know for sure if the cursor wrapped,
	// do bounds-checking between the screen edge and new location
	if ((postWrapCursorX + numCharsToRemove) >= inDataPtr->current.returnNumberOfColumnsPermitted())
	{
		numCharsToRemove = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
	}
	
	// change attributes if necessary
	copiedAttributes = (*toCursorLine)->returnGlobalAttributes();
	if (kMy_AttributeRuleCopyLast == inAttributeRule)
	{
		// copy attributes of the last character, making a special exception
		// to prevent bitmaps from being copied
		copiedAttributes = (*toCursorLine)->returnAttributeVector()[inDataPtr->current.returnNumberOfColumnsPermitted() - 1];
		if (copiedAttributes.hasAttributes(kTextAttributes_ColorIndexIsBitmapID))
		{
			copiedAttributes.removeImageRelatedAttributes();
		}
	}
	else if (kMy_AttributeRuleCopyLatentBackground == inAttributeRule)
	{
		copiedAttributes.colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
	}
	
	// update attributes
	{
		TerminalLine_TextAttributesList::iterator	pastVisibleEnd = (*toCursorLine)->returnMutableAttributeVector().begin();
		TerminalLine_TextAttributesList::iterator	toCursorAttr = (*toCursorLine)->returnMutableAttributeVector().begin();
		TerminalLine_TextAttributesList::iterator	toFirstPreservedAttr;
		TerminalLine_TextAttributesList::iterator	pastLastRelocatedAttr;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastRelocatedAttr = pastVisibleEnd;
		
		std::advance(toCursorAttr, postWrapCursorX);
		toFirstPreservedAttr = toCursorAttr;
		std::advance(toFirstPreservedAttr, numCharsToRemove);
		std::advance(pastLastRelocatedAttr, -numCharsToRemove);
		
		std::copy(toFirstPreservedAttr, pastVisibleEnd, toCursorAttr);
		std::fill(pastLastRelocatedAttr, pastVisibleEnd, copiedAttributes);
	}
	
	// update text
	{
		TerminalLine_TextIterator	pastVisibleEnd = (*toCursorLine)->textVectorBegin;
		TerminalLine_TextIterator	toCursorChar = (*toCursorLine)->textVectorBegin;
		TerminalLine_TextIterator	toFirstPreservedChar;
		TerminalLine_TextIterator	pastLastRelocatedChar;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastRelocatedChar = pastVisibleEnd;
		
		std::advance(toCursorChar, postWrapCursorX);
		toFirstPreservedChar = toCursorChar;
		std::advance(toFirstPreservedChar, numCharsToRemove);
		std::advance(pastLastRelocatedChar, -numCharsToRemove);
		
		std::copy(toFirstPreservedChar, pastVisibleEnd, toCursorChar);
		std::fill(pastLastRelocatedChar, pastVisibleEnd, ' ');
	}
	
	// add the entire line from the cursor to the end
	// to the text-change region; this would trigger
	// things like Terminal View updates
	//Console_WriteLine("text changed event: remove characters at cursor column");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferRemoveCharactersAtCursorColumn


/*!
Deletes lines from the screen buffer, scrolling up the
remainder and inserting new blank lines at the bottom of the
scrolling region.  The display is updated.

New blank lines normally have cleared attributes.  If however
"inAttributeRule" is set to "kMy_AttributeRuleCopyLast", they
will instead copy the attributes of the line that was at the
end prior to new lines being inserted.

See also bufferInsertBlankLines().

NOTE:   You cannot use this to alter the scrollback.

WARNING:	The specified line MUST be part of the terminal
			scrolling region.

(3.0)
*/
void
bufferRemoveLines	(My_ScreenBufferPtr						inDataPtr,
					 UInt16									inNumberOfLines,
					 My_ScreenBufferLineList::iterator&		inFirstDeletionLine,
					 My_AttributeRule						inAttributeRule)
{
	My_ScreenBufferLineList::iterator	scrollingRegionBegin;
	My_ScreenBufferLineList::iterator	scrollingRegionEnd;
	
	
	locateScrollingRegion(inDataPtr, scrollingRegionBegin, scrollingRegionEnd);
	if (0 != inNumberOfLines)
	{
		// the row index MUST be calculated immediately, since removing lines from the
		// buffer might make it impossible to find this at the end (where this index
		// is actually needed)
		My_ScreenRowIndex const						kFirstDeletedRow = std::distance(inDataPtr->screenBuffer.begin(), inFirstDeletionLine);
		My_ScreenRowIndex const						kLinesUntilEnd = std::distance(inFirstDeletionLine, scrollingRegionEnd);
		My_ScreenRowIndex const						kMostLines = std::min(STATIC_CAST(inNumberOfLines, My_ScreenRowIndex), kLinesUntilEnd);
		My_ScreenBufferLineList::size_type const	kBufferSize = inDataPtr->screenBuffer.size();
		My_ScreenBufferLineList::iterator			pastLastDeletionLine;
		
		
		// insert blank lines
		if ((kMy_AttributeRuleCopyLast == inAttributeRule) && (scrollingRegionEnd != scrollingRegionBegin))
		{
			My_ScreenBufferLinePtr				lineTemplate = createLinePtr();
			My_ScreenBufferLineList::iterator	toCopiedLine = scrollingRegionEnd;
			
			
			std::advance(toCopiedLine, -1);
			// copy attributes of the previous last line, making a special
			// exception to prevent bitmaps from being copied
			std::copy((*toCopiedLine)->returnAttributeVector().begin(),
						(*toCopiedLine)->returnAttributeVector().end(),
						lineTemplate->returnMutableAttributeVector().begin());
			for (auto& attributeFlags : lineTemplate->returnMutableAttributeVector())
			{
				attributeFlags.removeImageRelatedAttributes();
			}
			lineTemplate->returnMutableGlobalAttributes() = (*toCopiedLine)->returnGlobalAttributes();
			inDataPtr->screenBuffer.insert(scrollingRegionEnd, kMostLines, lineTemplate);
		}
		else if (kMy_AttributeRuleCopyLatentBackground == inAttributeRule)
		{
			My_ScreenBufferLinePtr		lineTemplate = createLinePtr();
			
			
			// the new lines have no attributes EXCEPT for a custom background color
			for (auto& attributeFlags : lineTemplate->returnMutableAttributeVector())
			{
				attributeFlags.colorIndexBackgroundCopyFrom(inDataPtr->current.latentAttributes);
			}
			inDataPtr->screenBuffer.insert(scrollingRegionEnd, kMostLines, lineTemplate);
		}
		else
		{
			inDataPtr->screenBuffer.insert(scrollingRegionEnd, kMostLines, createLinePtr());
		}
		
		// delete first lines
		pastLastDeletionLine = inFirstDeletionLine;
		std::advance(pastLastDeletionLine, kMostLines);
		inDataPtr->screenBuffer.erase(inFirstDeletionLine, pastLastDeletionLine);
		
		assert(kBufferSize == inDataPtr->screenBuffer.size());
		
		// redraw the area
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = kFirstDeletedRow;
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = inDataPtr->customScrollingRegion.lastRow - range.firstRow + 1;
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// bufferRemoveLines


/*!
Internal version of Terminal_ChangeLineAttributes().

(3.0)
*/
void
changeLineAttributes	(My_ScreenBufferPtr			inDataPtr,
						 My_ScreenBufferLine&		inRow,
						 TextAttributes_Object		inSetTheseAttributes,
						 TextAttributes_Object		inClearTheseAttributes)
{
	changeLineRangeAttributes(inDataPtr, inRow, 0/* first column */, -1/* last column; negative means “very end” */,
								inSetTheseAttributes, inClearTheseAttributes);
}// changeLineAttributes


/*!
Sets attributes that are global to a line.  Unlike
changeLineAttributes() which merely copies attributes
to each character, this routine also remembers the
specified changes as attributes for the entire line
so that every time the cursor enters the line, the
cursor attributes inherit the line’s global attributes.

(3.0)
*/
void
changeLineGlobalAttributes	(My_ScreenBufferPtr			inDataPtr,
							 My_ScreenBufferLine&		inRow,
							 TextAttributes_Object		inSetTheseAttributes,
							 TextAttributes_Object		inClearTheseAttributes)
{
	// first copy the attributes to every character in the line
	changeLineAttributes(inDataPtr, inRow, inSetTheseAttributes, inClearTheseAttributes);
	
	// now remember these changes so that the cursor can inherit them automatically;
	// currently attributes for scrollback lines do not exist, but negative indices
	// are allowed; to avoid array overflow, check for nonnegativity here
	inRow.returnMutableGlobalAttributes().removeAttributes(inClearTheseAttributes);
	inRow.returnMutableGlobalAttributes().addAttributes(inSetTheseAttributes);
}// changeLineGlobalAttributes


/*!
Internal version of Terminal_ChangeLineRangeAttributes().

(3.0)
*/
void
changeLineRangeAttributes	(My_ScreenBufferPtr			inDataPtr,
							 My_ScreenBufferLine&		inRow,
							 UInt16						inZeroBasedStartColumn,
							 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
							 TextAttributes_Object		inSetTheseAttributes,
							 TextAttributes_Object		inClearTheseAttributes)
{
	SInt16		pastTheEndColumn = (inZeroBasedPastTheEndColumnOrNegativeForLastColumn < 0)
									? inDataPtr->text.visibleScreen.numberOfColumnsAllocated
									: inZeroBasedPastTheEndColumnOrNegativeForLastColumn;
	SInt16		i = 0;
	
	
	// update attributes for the specified columns of the given line
	for (i = inZeroBasedStartColumn; i < pastTheEndColumn; ++i)
	{
		inRow.returnMutableAttributeVector()[i].removeAttributes(inClearTheseAttributes);
		inRow.returnMutableAttributeVector()[i].addAttributes(inSetTheseAttributes);
	}
	
	// update current attributes too, if the cursor is in the given range
	if ((inZeroBasedStartColumn <= inDataPtr->current.cursorX) && (inDataPtr->current.cursorX < pastTheEndColumn))
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		if (*cursorLineIterator == &inRow)
		{
			inDataPtr->current.drawingAttributes.removeAttributes(inClearTheseAttributes);
			inDataPtr->current.drawingAttributes.addAttributes(inSetTheseAttributes);
			
			// ...however, do not propagate text highlighting to text rendered from now on
			inDataPtr->current.drawingAttributes.removeAttributes(kTextAttributes_Selected);
			
			// ...do not propagate images either
			inDataPtr->current.drawingAttributes.removeImageRelatedAttributes();
		}
	}
	
	// finally, if the end column is beyond the last character in the line,
	// apply the attributes to EVERY column that does not have a valid character
	// UNIMPLEMENTED
}// changeLineRangeAttributes


/*!
Notifies all listeners for the specified Terminal
state change, passing the given context to the
listener.

IMPORTANT:	The context must make sense for the
			type of change; see "Terminal.h" for
			the type of context associated with
			each terminal change.

(3.0)
*/
void
changeNotifyForTerminal		(My_ScreenBufferConstPtr	inPtr,
							 Terminal_Change			inWhatChanged,
							 void*						inContextPtr)
{
	// invoke listener callback routines appropriately, from the specified terminal’s listener model
	ListenerModel_NotifyListenersOfEvent(inPtr->changeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyForTerminal


/*!
Uniform interface for creating new entries in line-lists.
DO NOT attempt manual memory management, as the scheme
may need to change over time to address performance and
sharing of data.

For instance, later the semantics of this function may be
to return an already-allocated line that is no longer in
use (as opposed to strictly allocating it here).

(4.1)
*/
My_ScreenBufferLinePtr
createLinePtr ()
{
	return My_ScreenBufferLinePtr(); // see TerminalLine_Handle
}// createLinePtr


/*!
Restores the last saved cursor position and
attribute settings.  See cursorSave() to ensure
this function does the opposite.

(2.6)
*/
void
cursorRestore  (My_ScreenBufferPtr		inPtr)
{
	moveCursor(inPtr, inPtr->previous.cursorX, inPtr->previous.cursorY);
	
	// ??? this will clear the current graphics character set too, is that supposed to happen ???
	inPtr->current.drawingAttributes = inPtr->previous.drawingAttributes;
}// cursorRestore


/*!
Saves the current cursor position and attribute
settings.  See cursorRestore() to ensure this
function does the opposite.

(2.6)
*/
void
cursorSave  (My_ScreenBufferPtr  inPtr)
{
	inPtr->previous.cursorX = inPtr->current.cursorX;
	inPtr->previous.cursorY = inPtr->current.cursorY;
	inPtr->previous.drawingAttributes = inPtr->current.drawingAttributes;
}// cursorSave


/*!
Returns the cursor location, first wrapping it to the
following line if it is out of bounds (if a wrap is
pending).

(3.0)
*/
void
cursorWrapIfNecessaryGetLocation	(My_ScreenBufferPtr		inDataPtr,
									 SInt16*				outCursorXPtr,
									 My_ScreenRowIndex*		outCursorYPtr)
{
	if (inDataPtr->current.cursorX >= inDataPtr->current.returnNumberOfColumnsPermitted()) 
	{
		moveCursorX(inDataPtr, 0);
		moveCursorDownOrScroll(inDataPtr);
	}
	*outCursorXPtr = inDataPtr->current.cursorX;
	*outCursorYPtr = inDataPtr->current.cursorY;
}// cursorWrapIfNecessaryGetLocation


/*!
Creates a bitmap and assigns it an ID.  (See the public
Terminal_BitmapGetFromID() API.)  Returns true only if
successful.

As noted in "TextAttributes.h", there is a limit to the
number of possible IDs and they will be reused (oldest
first) when the limit is exceeded.

Currently there is no mechanism for intelligently reusing
bitmap IDs.  It is up to you to avoid calling this method
if you can tell that the resulting bitmap already exists.

Also, ideally you use simple color attributes (or even
“true color” attributes via defineTrueColor()) to display
solid colors, instead of bitmaps.  Keep track of what
your pixel values ultimately are so that all-same-color
bitmap requests can be converted into simpler cells.

A bitmap may be constructed incrementally and grow its
definition according to other coordinate systems prior to
being finalized.  It may also be the case that a much
larger image is constructed first, and then split into an
appropriate number of terminal cell bitmaps.  If bitmaps
are split in this way and the total dimensions are not
even multiples of the cell, it is important to keep all
bitmaps the same size; for instance, include transparent
pixels in edge bitmaps to allow an image to appear to
occupy less space than the bitmaps actually occupy.

(2017.11)
*/
Boolean
defineBitmap	(My_ScreenBufferPtr			inDataPtr,
				 NSImage*					inSourceImage,
				 NSRect						inSubRect,
				 TextAttributes_BitmapID&	outBitmapID)
{
	Boolean		result = false;
	
	
	// define new ID within range of allowed IDs
	outBitmapID = inDataPtr->emulator.bitmapTableNextID;
	if (kTextAttributes_BitmapIDMaximum == inDataPtr->emulator.bitmapTableNextID)
	{
		inDataPtr->emulator.bitmapTableNextID = 0; // start reusing IDs
	}
	else
	{
		++(inDataPtr->emulator.bitmapTableNextID);
	}
	
	// create bitmap storage if necessary
	if (nil == inDataPtr->emulator.bitmapImageTable)
	{
		inDataPtr->emulator.bitmapImageTable = [[NSMutableArray alloc] init];
	}
	if (nil == inDataPtr->emulator.bitmapSegmentTable)
	{
		inDataPtr->emulator.bitmapSegmentTable = [[NSMutableArray alloc] init];
	}
	
	// add objects to storage
	if (nil != inSourceImage)
	{
		NSValue*	rectValue = [NSValue valueWithRect:inSubRect];
		
		
		result = true;
		if (inDataPtr->emulator.bitmapTableNextID >= inDataPtr->emulator.bitmapSegmentTable.count)
		{
			[inDataPtr->emulator.bitmapImageTable
				addObject:inSourceImage];
			[inDataPtr->emulator.bitmapSegmentTable
				addObject:rectValue];
		}
		else
		{
			[inDataPtr->emulator.bitmapImageTable
				replaceObjectAtIndex:inDataPtr->emulator.bitmapTableNextID
										withObject:inSourceImage];
			[inDataPtr->emulator.bitmapSegmentTable
				replaceObjectAtIndex:inDataPtr->emulator.bitmapTableNextID
										withObject:rectValue];
		}
	}
	
	return result;
}// defineBitmap


/*!
Provides the ID for the given RGB combination.  (See the
public Terminal_TrueColorGetFromID() API.)  Returns true
only if successful.

As noted in "TextAttributes.h", there is a limit to the
number of possible IDs and they will be reused (oldest
first) when the limit is exceeded.

The color components are used to find any existing ID
for the same color however, preventing many individual
requests for the same color from being assigned unique
IDs.

(4.1)
*/
Boolean
defineTrueColor		(My_ScreenBufferPtr				inDataPtr,
					 UInt8							inRed,
					 UInt8							inGreen,
					 UInt8							inBlue,
					 TextAttributes_TrueColorID&	outColorID)
{
	Boolean		result = false;
	UInt32		rgbKey = (inBlue | (inGreen << 8) | (inRed << 16));
	auto&		rgbMap = inDataPtr->emulator.trueColorIDsByRGB;
	auto		rgbIter = inDataPtr->emulator.trueColorIDsByRGB.find(rgbKey);
	
	
	if (rgbIter != rgbMap.end())
	{
		// the specified R/G/B combination is currently in use
		// and it has an ID; rather than wasting another ID,
		// return the existing ID
		outColorID = rgbIter->second;
		result = true;
	}
	else if (nullptr != inDataPtr->emulator.trueColorTableReds)
	{
		outColorID = inDataPtr->emulator.trueColorTableNextID;
		
		if (inDataPtr->emulator.trueColorTableReds->size() <= kTextAttributes_TrueColorIDMaximum)
		{
			// table is not yet big enough; allocate more space
			inDataPtr->emulator.trueColorTableReds->push_back(inRed);
			inDataPtr->emulator.trueColorTableGreens->push_back(inGreen);
			inDataPtr->emulator.trueColorTableBlues->push_back(inBlue);
			if (inDataPtr->emulator.trueColorTableNextID == kTextAttributes_TrueColorIDMaximum)
			{
				// table has reached its maximum size; next one will overwrite
				inDataPtr->emulator.trueColorTableNextID = 0;
			}
			else
			{
				// set next ID to match index of next new element
				inDataPtr->emulator.trueColorTableNextID = STATIC_CAST(inDataPtr->emulator.trueColorTableReds->size(), UInt16);
			}
		}
		else
		{
			// table is at maximum size; reuse the already-allocated space
			(*(inDataPtr->emulator.trueColorTableReds))[inDataPtr->emulator.trueColorTableNextID] = inRed;
			(*(inDataPtr->emulator.trueColorTableGreens))[inDataPtr->emulator.trueColorTableNextID] = inGreen;
			(*(inDataPtr->emulator.trueColorTableBlues))[inDataPtr->emulator.trueColorTableNextID] = inBlue;
			if (kTextAttributes_TrueColorIDMaximum == inDataPtr->emulator.trueColorTableNextID)
			{
				inDataPtr->emulator.trueColorTableNextID = 0;
			}
			else
			{
				++(inDataPtr->emulator.trueColorTableNextID);
			}
			
			// remove any previous RGB mapping to the ID that is being reused
			// (otherwise, a future request for the previous key color would
			// continue to return the same ID)
			auto	iterPair = inDataPtr->emulator.trueColorIDsByRGB.begin();
			auto	endPairs = inDataPtr->emulator.trueColorIDsByRGB.end();
			for (; iterPair != endPairs; ++iterPair)
			{
				if (outColorID == iterPair->second)
				{
					break;
				}
			}
			if (iterPair != endPairs)
			{
				inDataPtr->emulator.trueColorIDsByRGB.erase(iterPair);
			}
		}
		
		// create a mapping based on the RGB value back to this entry
		// so that repeated requests for the same color do not exhaust
		// the limited set of color ID entries
		rgbMap[rgbKey] = outColorID;
		
		result = true;
	}
	
	return result;
}// defineTrueColor


/*!
Uniform interface for freeing the entries in line-lists.
DO NOT attempt manual memory management, as the scheme
may need to change over time to address performance and
sharing of data.

For instance, later the semantics of this function may be
to mark an allocated line as available for reuse (as
opposed to destroying it here).

(4.1)
*/
void
deleteLinePtr	(My_ScreenBufferLinePtr&	inoutLinePtr)
{
	inoutLinePtr.reset(); // might be shared
}// deleteLinePtr


/*!
Treats the specified string as “verbatim”, sending the
characters wherever they need to go (any open print jobs
or capture files, the terminal, etc.).

The given string is also analyzed for words in order to
support future auto-completion.

(4.0)
*/
void
echoCFString	(My_ScreenBufferPtr		inDataPtr,
				 CFStringRef			inString)
{
	CFIndex const	kLength = CFStringGetLength(inString);
	Boolean const	kPrinterOnly = (0 != (inDataPtr->printingModes & kMy_PrintingModePrintController));
	
	
	// append to capture file, if one is open; try to avoid conversion,
	// but if necessary convert the bytes into a Unicode format
	if ((nullptr != inDataPtr->captureStream) || (nullptr != inDataPtr->printingStream))
	{
		CFStringEncoding const		kDesiredEncoding = kCFStringEncodingUTF8;
		CFIndex						bytesNeeded = 0;
		UInt8 const*				bufferReadOnly = REINTERPRET_CAST(CFStringGetCStringPtr(inString, kDesiredEncoding),
																		UInt8 const*);
		UInt8*						buffer = nullptr;
		Boolean						freeBuffer = false;
		
		
		if (nullptr != bufferReadOnly)
		{
			bytesNeeded = CPP_STD::strlen(REINTERPRET_CAST(bufferReadOnly, char const*));
		}
		else
		{
			CFIndex		conversionResult = CFStringGetBytes(inString, CFRangeMake(0, kLength),
															kDesiredEncoding, '?'/* loss byte */,
															true/* is external representation */,
															nullptr/* buffer; do not use, just find size */, 0/* buffer size, ignored */,
															&bytesNeeded);
			
			
			if (conversionResult > 0)
			{
				buffer = new UInt8[bytesNeeded];
				freeBuffer = true;
				
				conversionResult = CFStringGetBytes(inString, CFRangeMake(0, kLength),
													kDesiredEncoding, '?'/* loss byte */,
													true/* is external representation */,
													buffer, bytesNeeded, &bytesNeeded);
				assert(conversionResult > 0);
				
				bufferReadOnly = buffer;
			}
		}
		
		if (nullptr != bufferReadOnly)
		{
			if ((false == kPrinterOnly) && (nullptr != inDataPtr->captureStream))
			{
				StreamCapture_WriteUTF8Data(inDataPtr->captureStream, bufferReadOnly, bytesNeeded);
			}
			if (nullptr != inDataPtr->printingStream)
			{
				StreamCapture_WriteUTF8Data(inDataPtr->printingStream, bufferReadOnly, bytesNeeded);
			}
		}
		
		if (freeBuffer)
		{
			delete [] buffer, buffer = nullptr;
		}
	}
	
	// add each character to the terminal at the current cursor position, advancing
	// the cursor each time (and therefore being mindful of wrap settings, etc.)
	if (false == kPrinterOnly)
	{
		__block My_ScreenBufferLineList::iterator	cursorLineIterator;
		__block SInt16								preWriteCursorX = inDataPtr->current.cursorX;
		__block My_ScreenRowIndex					preWriteCursorY = inDataPtr->current.cursorY;
		__block TextAttributes_Object				temporaryAttributes;
		
		
		// WARNING: This is done once here, for efficiency, and is only
		//          repeated below if the cursor actually moves vertically
		//          (as evidenced by some moveCursor...() call that would
		//          affect the cursor row).  Keep this in sync!!!
		locateCursorLine(inDataPtr, cursorLineIterator);
		StringUtilities_ForEachComposedCharacterSequenceInRange
		(inString, CFRangeMake(0, kLength),
		^(CFStringRef	aSubstring,
		  CFRange		UNUSED_ARGUMENT(aRange),
		  Boolean&		UNUSED_ARGUMENT(outStopFlag))
		{
			UnicodeScalarValue		glyphType = StringUtilities_ReturnUnicodeSymbol(aSubstring);
			
			
		#if 0
			// debug
			{
				Console_WriteValueCFString("echo character: glyph", aSubstring);
				Console_WriteValue("echo character: value", glyphType);
			}
		#endif
			
			// if the cursor was about to wrap on the previous
			// write, perform that wrap now
			if (inDataPtr->wrapPending)
			{
				// autowrap to start of next line
				moveCursorLeftToEdge(inDataPtr);
				moveCursorDownOrScroll(inDataPtr);
				locateCursorLine(inDataPtr, cursorLineIterator); // cursor changed rows...
				
				// reset column tracker
				preWriteCursorX = 0;
			}
			
			// write characters on a single line
			if (inDataPtr->modeInsertNotReplace)
			{
				bufferInsertBlanksAtCursorColumnWithoutUpdate(inDataPtr, 1/* number of blank characters */, kMy_AttributeRuleInitialize);
			}
			(*cursorLineIterator)->textVectorBegin[inDataPtr->current.cursorX] = translateCharacter(inDataPtr, glyphType,
																									inDataPtr->current.drawingAttributes,
																									temporaryAttributes);
			(*cursorLineIterator)->returnMutableAttributeVector()[inDataPtr->current.cursorX] = temporaryAttributes;
			
			if (false == inDataPtr->wrapPending)
			{
				if (inDataPtr->current.cursorX < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1))
				{
					// advance the cursor position
					moveCursorRight(inDataPtr);
				}
				else
				{
					// hit right margin
					if (inDataPtr->modeAutoWrap)
					{
						// the cursor just arrived here, so set up a pending
						// wrap-and-scroll; it will only occur the next time
						// data is actually written
						inDataPtr->wrapPending = true;
					}
					else
					{
						// stay at right margin
						moveCursorRightToEdge(inDataPtr);
					}
				}
			}
		});
		
		// end of data; notify of a change (this will cause things like Terminal View updates)
		{
			// add the new line of text to the text-change region;
			// this should trigger things like Terminal View updates
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = preWriteCursorY;
			if (preWriteCursorY != inDataPtr->current.cursorY)
			{
				// more than one line; just draw all lines completely
				range.firstColumn = 0;
				range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
				range.rowCount = inDataPtr->current.cursorY - preWriteCursorY + 1;
			}
			else
			{
				range.firstColumn = preWriteCursorX;
				if (inDataPtr->modeInsertNotReplace)
				{
					// invalidate the rest of the line
					range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - preWriteCursorX;
				}
				else
				{
					range.columnCount = inDataPtr->current.cursorX - preWriteCursorX + 1;
				}
				range.rowCount = 1;
			}
			//Console_WriteValuePair("text changed event: add data starting at row, column", range.firstRow, range.firstColumn);
			//Console_WriteValuePair("text changed event: add data for #rows, #columns", range.rowCount, range.columnCount);
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// echoCFString


/*!
Erases the characters from the halfway point on the
screen to the right edge of the specified line,
clearing all attributes.  NO update events are sent.

This is required when switching to double-width
mode, which specifies that the right half of the
screen data on a double-width line be lost.

(3.0)
*/
void
eraseRightHalfOfLine	(My_ScreenBufferPtr		inDataPtr,
						 My_ScreenBufferLine&	inRow)
{
	SInt16										midColumn = STATIC_CAST(INTEGER_DIV_2
																		(inDataPtr->text.visibleScreen.numberOfColumnsPermitted),
																			SInt16);
	TerminalLine_TextIterator					textIterator = nullptr;
	TerminalLine_TextAttributesList::iterator	attrIterator;
	
	
	// clear from halfway point to end of line
	textIterator = inRow.textVectorBegin;
	std::advance(textIterator, midColumn);
	attrIterator = inRow.returnMutableAttributeVector().begin();
	std::advance(attrIterator, midColumn);
	std::fill(textIterator, inRow.textVectorEnd, ' ');
	std::fill(attrIterator, inRow.returnMutableAttributeVector().end(), inRow.returnGlobalAttributes());
}// eraseRightHalfOfLine


/*!
Returns a pointer to the internal structure, given a
reference to it.

(3.0)
*/
inline My_LineIteratorPtr
getLineIterator		(Terminal_LineRef	inIterator)
{
	return REINTERPRET_CAST(inIterator, My_LineIteratorPtr);
}// getLineIterator


/*!
A convenience function that calls getParametersFromStringRange()
on the "inDataPtr->emulator.stringAccumulator" string’s begin/end,
which is often where parameter data comes from during parsing. 

(2017.12)
*/
void
getParametersFromStringAccumulator	(My_ScreenBufferPtr								inDataPtr,
									 ParameterDecoder_StateMachine&					inoutParamDecoder,
									 std::basic_string< UInt8 >::const_iterator&	outIterChar)
{
	getParametersFromStringRange(inDataPtr->emulator.stringAccumulator.begin(),
									inDataPtr->emulator.stringAccumulator.end(),
									inoutParamDecoder, outIterChar);
}// getParametersFromStringAccumulator


/*!
Iterates over the first few characters of the given
character range until a non-parameter character is found
(typically this means capturing all numerical values
delimited by semicolons and stopping at the first
non-digit, non-semicolon).

Upon return, the state machine object can be used to
see what was found, such as numerical parameters.

For convenience the iterator’s final value is returned,
as this is usually vital for understanding what the
parameters mean.  If this is equal to "inEnd", all the
given bytes were processed without finding a terminator
(otherwise, this can be dereferenced to see the start
of the terminating sequence).

See also getParametersFromStringAccumulator().

(2017.12)
*/
void
getParametersFromStringRange	(std::basic_string< UInt8 >::const_iterator		inBegin,
								 std::basic_string< UInt8 >::const_iterator		inEnd,
								 ParameterDecoder_StateMachine&					inoutParamDecoder,
								 std::basic_string< UInt8 >::const_iterator&	outIterChar)
{
	for (outIterChar = inBegin; outIterChar != inEnd; /* incremented below */)
	{
		ParameterDecoder_StateMachine::State const	kOriginalState = inoutParamDecoder.returnState();
		SInt16										loopGuard = 0;
		Boolean										byteNotUsed = true;
		
		
		while (byteNotUsed)
		{
			inoutParamDecoder.goNextState(*outIterChar, byteNotUsed);
			if ((byteNotUsed) && (kOriginalState == inoutParamDecoder.returnState()))
			{
				// no way to proceed
				break;
			}
			++loopGuard;
			if (loopGuard > 100/* arbitrary */)
			{
				break;
			}
		}
		
		if (byteNotUsed)
		{
			break;
		}
		
		++outIterChar;
	}
}// getParametersFromStringRange


/*!
Returns a pointer to the internal structure, given a
reference to it.

(3.0)
*/
inline My_ScreenBufferPtr
getVirtualScreenData	(TerminalScreenRef		inScreen)
{
	return REINTERPRET_CAST(inScreen, My_ScreenBufferPtr);
}// getVirtualScreenData


/*!
Turns on a specific terminal LED, or turns all LEDs
off (if 0 is given as the LED number).  The meaning of
an LED with a specific number is left to the caller.

Currently, only 4 LEDs are defined.  Providing an LED
number of 5 or greater will have no effect.  These 4
refer to the user-defined LEDs, so although VT100 (for
example) defines 3 other LEDs, you can only set the
user-defined ones with this routine.

Numbers less than zero are reserved.  Probably, they
will one day be used to turn off specific LEDs.

(3.0)
*/
void
highlightLED	(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inOneBasedLEDNumberOrZeroToTurnOffAllLEDs)
{
	switch (inOneBasedLEDNumberOrZeroToTurnOffAllLEDs)
	{
	case 0:
		inDataPtr->litLEDs = kMy_LEDBitsAllOff;
		break;
	
	case 1:
		inDataPtr->litLEDs |= kMy_LEDBitsLight1;
		break;
	
	case 2:
		inDataPtr->litLEDs |= kMy_LEDBitsLight2;
		break;
	
	case 3:
		inDataPtr->litLEDs |= kMy_LEDBitsLight3;
		break;
	
	case 4:
		inDataPtr->litLEDs |= kMy_LEDBitsLight4;
		break;
	
	default:
		// ???
		break;
	}
	
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeNewLEDState, inDataPtr->selfRef/* context */);
}// highlightLED


/*!
Returns a map initialized with keys equal to the function
pointers used as callbacks, and values for constant strings
describing those callbacks.  Used in debugging logs.  See
the internal call gCallbackIDsByFuncPtr().

(2017.12)
*/
My_StringByPointer
initCallbackIDsByFuncPtr ()
{
	My_StringByPointer		result;
	
	
	// insert descriptions of every available callback (for debugging);
	// since the context for use already knows the purpose of the
	// callback, the main important information is the emulator name
	result[REINTERPRET_CAST(My_DefaultEmulator::stateDeterminant, void const*)] = "Default";
	result[REINTERPRET_CAST(My_DefaultEmulator::stateTransition, void const*)] = "Default";
	result[REINTERPRET_CAST(My_DumbTerminal::stateDeterminant, void const*)] = "Dumb";
	result[REINTERPRET_CAST(My_DumbTerminal::stateTransition, void const*)] = "Dumb";
	result[REINTERPRET_CAST(My_VT100::stateDeterminant, void const*)] = "VT100";
	result[REINTERPRET_CAST(My_VT100::stateTransition, void const*)] = "VT100";
	result[REINTERPRET_CAST(My_VT100::VT52::stateDeterminant, void const*)] = "VT100/VT52";
	result[REINTERPRET_CAST(My_VT100::VT52::stateTransition, void const*)] = "VT100/VT52";
	result[REINTERPRET_CAST(My_VT102::stateDeterminant, void const*)] = "VT102";
	result[REINTERPRET_CAST(My_VT102::stateTransition, void const*)] = "VT102";
	result[REINTERPRET_CAST(My_VT220::stateDeterminant, void const*)] = "VT220";
	result[REINTERPRET_CAST(My_VT220::stateTransition, void const*)] = "VT220";
	result[REINTERPRET_CAST(My_XTerm::stateDeterminant, void const*)] = "XTerm";
	result[REINTERPRET_CAST(My_XTerm::stateTransition, void const*)] = "XTerm";
	result[REINTERPRET_CAST(My_XTermCore::stateDeterminant, void const*)] = "XTermCore";
	result[REINTERPRET_CAST(My_XTermCore::stateTransition, void const*)] = "XTermCore";
	result[REINTERPRET_CAST(My_ITermCore::stateDeterminant, void const*)] = "ITermCore";
	result[REINTERPRET_CAST(My_ITermCore::stateTransition, void const*)] = "ITermCore";
	result[REINTERPRET_CAST(My_SixelCore::stateDeterminant, void const*)] = "SixelCore";
	result[REINTERPRET_CAST(My_SixelCore::stateTransition, void const*)] = "SixelCore";
	result[REINTERPRET_CAST(My_UTF8Core::stateDeterminant, void const*)] = "UTF8Core";
	result[REINTERPRET_CAST(My_UTF8Core::stateTransition, void const*)] = "UTF8Core";
	
	return result;
}// initCallbackIDsByFuncPtr


/*!
See the description for "My_EmulatorStateTransitionProcPtr".

(4.0)
*/
inline UInt32
invokeEmulatorStateTransitionProc	(My_EmulatorStateTransitionProcPtr	inProc,
									 My_ScreenBuffer*					inDataPtr,
									 My_ParserStatePair const&			inOldNew,
									 Boolean&							outHandled)
{
	UInt32		result = 0;
	
	
	outHandled = true; // initially...
	result = (*inProc)(inDataPtr, inOldNew, outHandled);
	
	if (outHandled)
	{
		Boolean const	kIsLogged = ((DebugInterface_LogsTerminalEcho() && (kMy_ParserStateAccumulateForEcho == inOldNew.second)) ||
										(DebugInterface_LogsTerminalState() && (kMy_ParserStateAccumulateForEcho != inOldNew.second)));
		
		
		if (kIsLogged)
		{
			inDataPtr->debugStateHandlerSequence.push_front(REINTERPRET_CAST(inProc, void const*));
		}
	}
	
	return result;
}// invokeEmulatorStateTransitionProc


/*!
Locates the screen buffer line that the cursor is on,
providing an iterator into its list (which may be
past-the-end, that is, invalid).

This operation is linear with the size of the visible
screen area.  Using this routine instead of retaining
an iterator is recommended, since it is difficult to
properly synchronize an iterator with cursor movement,
(frequent) scroll activity and other list modifications.
Plus, the cursor line iterator only needs to be found
when data on the cursor line should actually be
manipulated.

(3.0)
*/
void
locateCursorLine	(My_ScreenBufferPtr						inDataPtr,
					 My_ScreenBufferLineList::iterator&		outCursorLine)
{
	My_ScreenBufferLineList::size_type const	kMaximumLines = inDataPtr->screenBuffer.size();
	
	
	//assert(inDataPtr->current.cursorY >= 0);
	assert(inDataPtr->current.cursorY <= kMaximumLines);
	if (inDataPtr->current.cursorY < INTEGER_DIV_2(kMaximumLines))
	{
		// near the top; search from the beginning
		// (NOTE: linear search is still horrible, but this makes it less horrible)
		outCursorLine = inDataPtr->screenBuffer.begin();
		std::advance(outCursorLine, inDataPtr->current.cursorY/* zero-based */);
	}
	else
	{
		// near the bottom; search from the end
		// (NOTE: linear search is still horrible, but this makes it less horrible)
		SInt64 const	kDelta = inDataPtr->current.cursorY/* zero-based */ - kMaximumLines;
		
		
		outCursorLine = inDataPtr->screenBuffer.end();
		std::advance(outCursorLine, kDelta);
	}
}// locateCursorLine


/*!
Locates the screen buffer lines bounding the cursor and
scrolling activity (as defined by the terminal’s current
origin setting), providing iterators into the list (each
of which may be past-the-end, that is, invalid).  See
also locateScrollingRegionTop().

This operation is linear with the size of the visible
screen area.  Using this routine instead of retaining
iterators is recommended, since it is difficult to
properly synchronize an iterator with origin changes,
(frequent) scroll activity and other list modifications.
Plus, the scrolling region iterators only need to be
found rarely, in practice less frequently than most list
modifications that would invalidate retained iterators.

(3.0)
*/
void
locateScrollingRegion	(My_ScreenBufferPtr						inDataPtr,
						 My_ScreenBufferLineList::iterator&		outTopLine,
						 My_ScreenBufferLineList::iterator&		outPastBottomLine)
{
	My_ScreenBufferLineList::size_type const	kMaximumLines = inDataPtr->screenBuffer.size();
	
	
	//assert(inDataPtr->customScrollingRegion.firstRow >= 0);
	assertScrollingRegion(inDataPtr);
	
	// find top line iterator in screen
	outTopLine = inDataPtr->screenBuffer.begin();
	std::advance(outTopLine, inDataPtr->customScrollingRegion.firstRow/* zero-based */);
	
#if 0
	// slow, but definitely correct
	outPastBottomLine = inDataPtr->screenBuffer.begin();
	std::advance(outPastBottomLine, 1 + inDataPtr->customScrollingRegion.lastRow/* zero-based */);
#else
	{
		// note the region boundary is inclusive but past-the-end is exclusive;
		// also, for efficiency, assume this will be much closer to the end and
		// advance backwards to save some pointer iterations
		SInt64 const	kDelta = inDataPtr->customScrollingRegion.lastRow/* zero-based */ - kMaximumLines + 1/* past-end */;
		
		
		outPastBottomLine = inDataPtr->screenBuffer.end();
		std::advance(outPastBottomLine, kDelta);
	}
#endif
}// locateScrollingRegion


/*!
Locates the screen origin line, providing an iterator
into the list (which may be past-the-end, that is,
invalid).  See also locateScrollingRegion().

The origin is usually very close to the top of the
screen buffer, so although this search is technically
linear, in practice it will not be a slow operation.

(3.0)
*/
void
locateScrollingRegionTop	(My_ScreenBufferPtr						inDataPtr,
							 My_ScreenBufferLineList::iterator&		outTopLine)
{
	// find top line iterator in screen
	outTopLine = inDataPtr->screenBuffer.begin();
	std::advance(outTopLine, inDataPtr->customScrollingRegion.firstRow/* zero-based */);
}// locateScrollingRegionTop


/*!
Changes both the row and column of the cursor in
the specified terminal screen at the same time.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursor		(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inNewX,
				 My_ScreenRowIndex		inNewY)
{
	moveCursorX(inDataPtr, inNewX);
	moveCursorY(inDataPtr, inNewY);
	//Console_WriteValuePair("new cursor x, y", inDataPtr->current.cursorX, inDataPtr->current.cursorY);
}// moveCursor


/*!
Moves the cursor to the row below its current row.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorDown		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->current.cursorY + 1);
}// moveCursorDown


/*!
Moves the cursor to the row below its current row,
or scrolls up one line if the cursor is already at
the bottom of the scrolling region.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
void
moveCursorDownOrScroll	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY == inDataPtr->customScrollingRegion.lastRow)
	{
		screenScroll(inDataPtr, +1);
	}
	else if (inDataPtr->current.cursorY < (inDataPtr->screenBuffer.size() - 1))
	{
		moveCursorDown(inDataPtr);
	}
}// moveCursorDownOrScroll


/*!
Moves the cursor to the last row (constrained by
the current scrolling region).

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorDownToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->originRegionPtr->lastRow);
}// moveCursorDownToEdge


/*!
Moves the cursor to the column immediately preceding
its current column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeft		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX - 1);
}// moveCursorLeft


/*!
Moves the cursor to the first column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeftToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, 0);
}// moveCursorLeftToEdge


/*!
Moves the cursor to the middle column, but only if
this is possible by moving left.  In this way, the
cursor stays put unless it is in the right half of
the screen.  Useful for implementing double-width
text mode.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeftToHalf	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		halfway = STATIC_CAST(INTEGER_DIV_2
										(inDataPtr->text.visibleScreen.numberOfColumnsPermitted),
											SInt16);
	
	
	if (inDataPtr->current.cursorX >= halfway)
	{
		moveCursorX(inDataPtr, halfway);
	}
}// moveCursorLeftToHalf


/*!
Moves the cursor in the opposite direction of a
normal tab, to find the stop that is behind it.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeftToNextTabStop	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX - tabStopGetDistanceFromCursor(inDataPtr, false/* forward */));
}// moveCursorLeftToNextTabStop


/*!
Moves the cursor to the column immediately following
its current column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorRight		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX + 1);
}// moveCursorRight


/*!
Moves the cursor to the last column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorRightToEdge		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.returnNumberOfColumnsPermitted() - 1);
}// moveCursorRightToEdge


/*!
Moves the cursor to the next column that has a
tab stop set for it.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorRightToNextTabStop	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX + tabStopGetDistanceFromCursor(inDataPtr, true/* forward */));
}// moveCursorRightToNextTabStop


/*!
Moves the cursor to the row above its current row.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->current.cursorY - 1);
}// moveCursorUp


/*!
Moves the cursor to the row above its current row,
or scrolls down one line if the cursor is already
at the top of the scrolling region.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
void
moveCursorUpOrScroll	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY == inDataPtr->customScrollingRegion.firstRow)
	{
		screenScroll(inDataPtr, -1);
	}
	else
	{
		moveCursorUp(inDataPtr);
	}
}// moveCursorUpOrScroll


/*!
Moves the cursor to the first row (constrained by
the current scrolling region).

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorUpToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->originRegionPtr->firstRow);
}// moveCursorUpToEdge


/*!
Changes the column of the cursor, within the boundaries of
the screen.  All cursor-dependent data is automatically
synchronized if necessary.

This has no effect if print controller mode (VT102 or later)
is active.

IMPORTANT:	ALWAYS use moveCursor...() routines to set the
			cursor value; otherwise, cursor-dependent stuff
			could become out of sync.

(3.0)
*/
inline void
moveCursorX		(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inNewX)
{
	if (0 == (inDataPtr->printingModes & kMy_PrintingModePrintController))
	{
		// arbitrarily check the state of password mode whenever the cursor
		// moves to a different column (TEMPORARY; may need a more intelligent
		// way to check this, such as a periodic timer?)
		{
			Boolean		currentPasswordMode = Session_IsInPasswordMode(inDataPtr->listeningSession);
			
			
			if (currentPasswordMode != inDataPtr->passwordMode)
			{
				inDataPtr->passwordMode = currentPasswordMode;
				// do not notify; change-location notification occurs below
				//changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorState, inDataPtr->selfRef);
			}
		}
		
		if ((inDataPtr->mayNeedToSaveToScrollback) && (inNewX != 0))
		{
			// once the cursor leaves the home position, there is no
			// chance of having to save the top line into scrollback
			inDataPtr->mayNeedToSaveToScrollback = false;
		}
		inDataPtr->current.cursorX = inNewX;
		if ((inDataPtr->current.cursorX == 0) && (inDataPtr->current.cursorY == 0))
		{
			// if the cursor moves into the home position, flag this
			// and prepare to possibly save the line contents if any
			// subsequent erase commands show up
			inDataPtr->mayNeedToSaveToScrollback = true;
		}
		
		// TEMPORARY - the inefficiency of this is ugly, but it
		// is definitely known to work
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			inDataPtr->current.cursorAttributes = (*cursorLineIterator)->returnAttributeVector()[inDataPtr->current.cursorX];
		}
		
		// reset wrap flag, now that the cursor is moving
		inDataPtr->wrapPending = false;
		
		//if (inDataPtr->cursorVisible)
		{
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorLocation, inDataPtr->selfRef);
		}
		
	#if 0
		// DEBUG
		Console_WriteValuePair("cursor trace (dx)", inDataPtr->current.cursorX, inDataPtr->current.cursorY);
	#endif
	}
}// moveCursorX


/*!
Changes the row of the cursor, within the boundaries of the
screen and any current margins.  All cursor-dependent data is
automatically synchronized if necessary.

This has no effect if print controller mode (VT102 or later)
is active.

IMPORTANT:	ALWAYS use moveCursor...() routines to set the
			cursor value; otherwise, cursor-dependent stuff
			could become out of sync.

(3.0)
*/
inline void
moveCursorY		(My_ScreenBufferPtr		inDataPtr,
				 My_ScreenRowIndex		inNewY)
{
	if (0 == (inDataPtr->printingModes & kMy_PrintingModePrintController))
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		
		if ((inDataPtr->mayNeedToSaveToScrollback) && (0 != inNewY))
		{
			// once the cursor leaves the home position, there is no
			// chance of having to save the top line into scrollback
			inDataPtr->mayNeedToSaveToScrollback = false;
		}
		inDataPtr->current.drawingAttributes.removeAttributes((*cursorLineIterator)->returnGlobalAttributes());
		
		// don’t allow the cursor to fall off the screen (in origin
		// mode, it cannot fall outside the scrolling region)
		{
			SInt16		newCursorY = STATIC_CAST(inNewY, SInt16);
			
			
			newCursorY = std::max< SInt16 >(newCursorY, STATIC_CAST(inDataPtr->originRegionPtr->firstRow, SInt16));
			newCursorY = std::min< SInt16 >(newCursorY, STATIC_CAST(inDataPtr->originRegionPtr->lastRow, SInt16));
			inDataPtr->current.cursorY = newCursorY;
		}
		
		// cursor has moved, so find the data for its new row
		locateCursorLine(inDataPtr, cursorLineIterator);
		
		inDataPtr->current.drawingAttributes.addAttributes((*cursorLineIterator)->returnGlobalAttributes());
		if ((0 == inDataPtr->current.cursorY) && (0 == inDataPtr->current.cursorX))
		{
			// if the cursor moves into the home position, flag this
			// and prepare to possibly save the line contents if any
			// subsequent erase commands show up
			inDataPtr->mayNeedToSaveToScrollback = true;
		}
		
		inDataPtr->current.cursorAttributes = (*cursorLineIterator)->returnAttributeVector()[inDataPtr->current.cursorX];
		
		// reset wrap flag, now that the cursor is moving
		inDataPtr->wrapPending = false;
		
		//if (inDataPtr->cursorVisible)
		{
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorLocation, inDataPtr->selfRef);
		}
		
	#if 0
		// DEBUG
		Console_WriteValuePair("cursor trace (dy)", inDataPtr->current.cursorX, inDataPtr->current.cursorY);
	#endif
	}
}// moveCursorY


/*!
Resets terminal modes to defaults, and (for hard resets) clears
the screen and returns all settings to factory defaults.
Also notifies any listeners of reset events.

The effect of a soft and hard reset can vary by terminal type.

(2.6)
*/
void
resetTerminal   (My_ScreenBufferPtr		inDataPtr,
				 Boolean				inSoftReset)
{
	inDataPtr->customScrollingRegion = inDataPtr->visibleBoundary.rows;
	assertScrollingRegion(inDataPtr);
	My_VT100::ansiMode(inDataPtr);
	//inDataPtr->modeAutoWrap = false; // 3.0 - do not touch the auto-wrap setting
	inDataPtr->modeCursorKeysForApp = false;
	inDataPtr->modeApplicationKeys = false;
	inDataPtr->modeOriginRedefined = false; // also requires cursor homing (below), according to manual
	inDataPtr->originRegionPtr = &inDataPtr->visibleBoundary.rows;
	inDataPtr->previous.drawingAttributes = kTextAttributes_Invalid;
	inDataPtr->current.drawingAttributes.clear();
	inDataPtr->current.latentAttributes.clear();
	inDataPtr->modeInsertNotReplace = false;
	inDataPtr->modeNewLineOption = false;
	inDataPtr->emulator.isUTF8Encoding = (kCFStringEncodingUTF8 == inDataPtr->emulator.inputTextEncoding);
	inDataPtr->emulator.lockUTF8 = false;
	inDataPtr->emulator.disableShifts = false;
	inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
	inDataPtr->printingModes = 0;
	inDataPtr->printingEnd();
	
	// reset the base emulator, and then (rare) perform any emulator-specific actions
	inDataPtr->emulator.reset(inSoftReset);
	invokeEmulatorResetProc(inDataPtr->emulator.currentCallbacks.resetHandler, &inDataPtr->emulator, inSoftReset);
	
	unless (inSoftReset)
	{
		moveCursor(inDataPtr, 0, 0);
		bufferEraseVisibleScreen(inDataPtr, inDataPtr->emulator.returnEraseEffectsForNormalUse());
	}
	tabStopInitialize(inDataPtr);
	highlightLED(inDataPtr, 0/* zero means “turn off all LEDs” */);
	
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeReset, inDataPtr->selfRef/* context */);
}// resetTerminal


/*!
Returns the currently attached SessionRef, or nullptr
if none is attached.  This is necessary for a small
number of terminal features that need to report data
back to a requester (e.g. VT100 Device Attributes).

(3.1)
*/
SessionRef
returnListeningSession	(My_ScreenBufferPtr		inDataPtr)
{
	return inDataPtr->listeningSession;
}// returnListeningSession


/*!
Appends the visible screen to the scrollback buffer, usually in
preparation for then blanking the visible screen area.

Returns "true" only if successful.

(3.0)
*/
Boolean
screenCopyLinesToScrollback		(My_ScreenBufferPtr		inDataPtr)
{
	Boolean		result = true;
	
	
	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->customScrollingRegion == inDataPtr->visibleBoundary.rows))
	{
		SInt16 const			kLineCount = STATIC_CAST(inDataPtr->screenBuffer.size(), SInt16);
		My_ScreenBufferLinePtr	templateLine = createLinePtr();
		
		
		inDataPtr->scrollbackBuffer.insert(inDataPtr->scrollbackBuffer.begin(), kLineCount/* number of lines */, templateLine);
		inDataPtr->scrollbackBufferCachedSize += kLineCount;
		std::copy(inDataPtr->screenBuffer.rbegin(), inDataPtr->screenBuffer.rend(), inDataPtr->scrollbackBuffer.begin());
		
		if (inDataPtr->scrollbackBufferCachedSize > inDataPtr->text.scrollback.numberOfRowsPermitted)
		{
			inDataPtr->scrollbackBuffer.resize(inDataPtr->text.scrollback.numberOfRowsPermitted);
			inDataPtr->scrollbackBufferCachedSize = inDataPtr->text.scrollback.numberOfRowsPermitted;
		}
		
		if (result)
		{
			Terminal_ScrollDescription	scrollInfo;
			
			
			bzero(&scrollInfo, sizeof(scrollInfo));
			scrollInfo.screen = inDataPtr->selfRef;
			scrollInfo.rowDelta = -kLineCount;
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
		}
	}
	return result;
}// screenCopyLinesToScrollback


/*!
Modifies only the screen buffer, to include the specified
number of additional lines (at the end).  The new lines are
cleared.

This is rarely required, because scrolling will rotate the
oldest lines to the bottom.  However, if the screen buffer
becomes physically larger, or a screen is being created for
the first time, this can be useful.

The resultant lines may share a large memory block.  So, it
is usually better to invoke this routine once for the number
of lines you’ll ultimately need, than to invoke it many
times to add a single line.

Returns "true" only if successful.

(3.0)
*/
Boolean
screenInsertNewLines	(My_ScreenBufferPtr						inDataPtr,
						 My_ScreenBufferLineList::size_type		inNumberOfElements)
{
	Boolean		result = true;
	
	
	//Console_WriteValue("request to insert new lines", inNumberOfElements);
	if (0 != inNumberOfElements)
	{
		My_ScreenBufferLineList::size_type   kOldSize = inDataPtr->screenBuffer.size();
		
		
		// start by allocating more lines if necessary, or freeing unneeded lines
		inDataPtr->screenBuffer.resize(kOldSize + inNumberOfElements);
		
		// make sure the cursor line doesn’t fall off the end
		if (inDataPtr->current.cursorY >= inDataPtr->screenBuffer.size())
		{
			moveCursorY(inDataPtr, inDataPtr->screenBuffer.size() - 1);
		}
		
		// stop now if the resize failed
		if (inDataPtr->screenBuffer.size() != (kOldSize + inNumberOfElements))
		{
			// failed to resize!
			result = false;
		}
		else
		{
			//Console_WriteValue("append new lines", inNumberOfElements);
			//Console_WriteValue("final size", inDataPtr->screenBuffer.size());
		}
	}
	return result;
}// screenInsertNewLines


/*!
Removes lines from the top of the screen buffer, to the
adjacent part of the scrollback buffer, and replaces the
lost lines with blank ones at the bottom (so that the overall
screen buffer size is unchanged).

The data structures used for the “new” lines may actually
exist already, stripped from the oldest lines of the
scrollback buffer.

Returns "true" only if successful.

See also screenCopyLinesToScrollback().

(3.0)
*/
Boolean
screenMoveLinesToScrollback		(My_ScreenBufferPtr						inDataPtr,
								 My_ScreenBufferLineList::size_type		inNumberOfElements)
{
	Boolean		result = true;
	
	
	//Console_WriteValue("request to move lines to the scrollback", inNumberOfElements);
	if (0 != inNumberOfElements)
	{
		Boolean		recycleLines = false;
		
		
		// scrolling will be done; figure out whether or not to recycle old lines
		recycleLines = (!(inDataPtr->text.scrollback.enabled)) ||
						(!(inDataPtr->scrollbackBuffer.empty()) &&
							(inDataPtr->scrollbackBufferCachedSize >= inDataPtr->text.scrollback.numberOfRowsPermitted));
		
		// adjust screen and scrollback buffers appropriately; new lines
		// will either be rotated in from the oldest scrollback, or
		// allocated anew; in either case, they'll end up initialized
		if (recycleLines)
		{
			//Console_WriteValue("recycled lines", inNumberOfElements);
			
			// get the line destined for the new bottom of the main screen;
			// either extract it from elsewhere, or allocate a new line
			for (My_ScreenBufferLineList::size_type i = 0; i < inNumberOfElements; ++i)
			{
				// extract the oldest line
				assert(!inDataPtr->screenBuffer.empty());
				if (inDataPtr->scrollbackBuffer.empty())
				{
					// make the oldest screen line the newest one
					inDataPtr->screenBuffer.splice(inDataPtr->screenBuffer.end()/* the next newest screen line */,
													inDataPtr->screenBuffer/* the list to move from */,
													inDataPtr->screenBuffer.begin()/* the line to move */);
				}
				else
				{
					My_ScrollbackBufferLineList::iterator	oldestScrollbackLine;
					
					
					// make the oldest screen line the newest scrollback line
					inDataPtr->scrollbackBuffer.splice(inDataPtr->scrollbackBuffer.begin()/* the next oldest scrollback line */,
														inDataPtr->screenBuffer/* the list to move from */,
														inDataPtr->screenBuffer.begin()/* the line to move */);
					++(inDataPtr->scrollbackBufferCachedSize);
					
					// end() points one past the end, so nudge it back
					oldestScrollbackLine = inDataPtr->scrollbackBuffer.end();
					std::advance(oldestScrollbackLine, -1);
					
					// make the oldest scrollback line the newest screen line
					inDataPtr->screenBuffer.splice(inDataPtr->screenBuffer.end()/* the next newest screen line */,
													inDataPtr->scrollbackBuffer/* the list to move from */,
													oldestScrollbackLine/* the line to move */);
					--(inDataPtr->scrollbackBufferCachedSize);
				}
				
				// the recycled line may have data in it, so clear it out
				inDataPtr->screenBuffer.back()->structureInitialize();
			}
			
			//Console_WriteValue("post-recycle scrollback size (actual)", inDataPtr->scrollbackBuffer.size());
			//Console_WriteValue("post-recycle scrollback size (cached)", inDataPtr->scrollbackBufferCachedSize);
		}
		else
		{
			//Console_WriteValue("moved-and-reallocated lines", inNumberOfElements);
			
			My_ScreenBufferLineList::iterator	pastLastLineToScroll = inDataPtr->screenBuffer.begin();
			My_ScreenBufferLineList				movedLines;
			
			
			// find the last line that will remain in the screen buffer
			std::advance(pastLastLineToScroll, inNumberOfElements);
			
			// make the oldest screen lines the newest scrollback lines; since the
			// “front” scrollback line is adjacent to the “front” screen line, a
			// single splice is wrong; the lines have to insert in reverse order
			movedLines.splice(movedLines.begin()/* where to insert */,
								inDataPtr->screenBuffer/* the list to move from */,
								inDataPtr->screenBuffer.begin()/* the first line to move */,
								pastLastLineToScroll/* the first line that will not be moved */);
			movedLines.reverse();
			inDataPtr->scrollbackBuffer.splice(inDataPtr->scrollbackBuffer.begin()/* the next oldest scrollback line */,
												movedLines/* the list to move from */,
												movedLines.begin(), movedLines.end());
			inDataPtr->scrollbackBufferCachedSize += movedLines.size();
			
			if (inDataPtr->scrollbackBufferCachedSize > inDataPtr->text.scrollback.numberOfRowsPermitted)
			{
				inDataPtr->scrollbackBuffer.resize(inDataPtr->text.scrollback.numberOfRowsPermitted);
				inDataPtr->scrollbackBufferCachedSize = inDataPtr->text.scrollback.numberOfRowsPermitted;
			}
			
			//Console_WriteValue("post-move scrollback size (actual)", inDataPtr->scrollbackBuffer.size());
			//Console_WriteValue("post-move scrollback size (cached)", inDataPtr->scrollbackBufferCachedSize);
			
			// allocate new lines
			try
			{
				inDataPtr->screenBuffer.resize(inDataPtr->screenBuffer.size() + inNumberOfElements);
			}
			catch (std::bad_alloc)
			{
				// abort
				result = false;
			}
			
			//Console_WriteValue("post-allocation screen size", inDataPtr->screenBuffer.size());
		}
	}
	return result;
}// screenMoveLinesToScrollback


/*!
Removes the specified positive number of rows from the top of the
scrolling region, or the specified magnitude of rows from the
bottom when the value is negative.

When rows disappear from the top, blank lines appear at the
bottom.  Lines scrolled off the top are lost unless the terminal
is set to save them, in which case the lines are inserted into
the scrollback buffer.

When rows disappear from the bottom, blank lines appear at the
top.  Lines scrolled off the bottom are lost.

The display is updated.

(3.0)
*/
void
screenScroll	(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inLineCount)
{
	if (inLineCount > 0)
	{
		// remove lines from the top of the scrolling region,
		// and possibly save them
		if (inDataPtr->current.cursorY < inDataPtr->screenBuffer.size())
		{
			if ((inDataPtr->text.scrollback.enabled) &&
				(inDataPtr->customScrollingRegion == inDataPtr->visibleBoundary.rows))
			{
				// scrolling region is entire screen, and lines are being saved off the top
				UNUSED_RETURN(Boolean)screenMoveLinesToScrollback(inDataPtr, inLineCount);
				
				// displaying right from top of scrollback buffer; topmost line being shown
				// has in fact vanished; update the display to show this
				//Console_WriteLine("text changed event: scroll terminal buffer");
				{
					Terminal_RangeDescription	range;
					
					
					range.screen = inDataPtr->selfRef;
					range.firstRow = 0;
					range.firstColumn = 0;
					range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
					range.rowCount = inDataPtr->visibleBoundary.rows.lastRow - inDataPtr->visibleBoundary.rows.firstRow + 1;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
				}
				
				// notify about the scrolling amount
				{
					Terminal_ScrollDescription	scrollInfo;
					
					
					bzero(&scrollInfo, sizeof(scrollInfo));
					scrollInfo.screen = inDataPtr->selfRef;
					scrollInfo.rowDelta = -inLineCount;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
				}
			}
			else
			{
				My_ScreenBufferLineList::iterator	scrollingRegionBegin;
				
				
				// region being scrolled is not entire screen; no saving of lines
				locateScrollingRegionTop(inDataPtr, scrollingRegionBegin);
				bufferRemoveLines(inDataPtr, inLineCount, scrollingRegionBegin,
									inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermBCE)
									? kMy_AttributeRuleCopyLatentBackground
									: kMy_AttributeRuleInitialize);
			}
		}
	}
	else if (inLineCount < 0)
	{
		// remove lines from the bottom of the scrolling region (they are lost)
		// and add blank lines at the top
		My_ScreenBufferLineList::iterator	scrollingRegionBegin;
		
		
		locateScrollingRegionTop(inDataPtr, scrollingRegionBegin);
		bufferInsertBlankLines(inDataPtr, -inLineCount, scrollingRegionBegin/* insertion row */,
								inDataPtr->emulator.supportsVariant(My_Emulator::kVariantFlagXTermBCE)
								? kMy_AttributeRuleCopyLatentBackground
								: kMy_AttributeRuleInitialize);
	}
}// screenScroll


/*!
Changes the logical cursor state.  Performed in a
function for consistency in case, for instance,
callbacks are invoked in the future whenever this
flag changes.

(3.0)
*/
inline void
setCursorVisible	(My_ScreenBufferPtr		inDataPtr,
					 Boolean				inIsVisible)
{
	inDataPtr->cursorVisible = inIsVisible;
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorState, inDataPtr->selfRef);
}// setCursorVisible


/*!
Changes the maximum size of the scrollback buffer.  If the
current content is larger, its memory is truncated.

This triggers two events: "kTerminal_ChangeScrollActivity"
to indicate that data has been removed, and also
"kTerminal_ChangeTextRemoved" (given a range consisting of
all previous scrollback lines).

(4.0)
*/
void
setScrollbackSize	(My_ScreenBufferPtr		inDataPtr,
					 UInt32					inLineCount)
{
	My_ScreenBufferLineList::size_type const	kPreviousScrollbackCount = inDataPtr->scrollbackBufferCachedSize;
	
	
	inDataPtr->text.scrollback.numberOfRowsPermitted = inLineCount;
	inDataPtr->text.scrollback.enabled = (inDataPtr->text.scrollback.numberOfRowsPermitted > 0L);
	
	if (kPreviousScrollbackCount > inLineCount)
	{
		// notify listeners of the range of text that has gone away
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = -kPreviousScrollbackCount;
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kPreviousScrollbackCount - inLineCount;
			
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextRemoved, &range/* context */);
		}
	}
	
	inDataPtr->scrollbackBuffer.resize(inLineCount);
	inDataPtr->scrollbackBufferCachedSize = inLineCount;
	
	// notify listeners that scroll activity has taken place,
	// though technically no remaining lines have been affected
	{
		Terminal_ScrollDescription	scrollInfo;
		
		
		bzero(&scrollInfo, sizeof(scrollInfo));
		scrollInfo.screen = inDataPtr->selfRef;
		scrollInfo.rowDelta = 0;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
	}
}// setScrollbackSize


/*!
Changes the number of characters of text per line for a
screen buffer.  This operation is currently trivial
because all line arrays are allocated to maximum size
when a buffer is created, and this simply adjusts the
percentage of the total that should be usable.  Although,
this also forces the cursor into the new region, if
necessary.

IMPORTANT:	This is a low-level routine for internal use;
			send a "kTerminal_ChangeScreenSize" notification
			after all size changes are complete.  Note that
			Terminal_SetVisibleScreenDimensions() will set
			dimensions and automatically notify listeners.

\retval kTerminal_ResultOK
if the terminal is resized without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the given number of columns is too small or too large

\retval kTerminal_ResultNotEnoughMemory
not currently returned because this routine does no memory
reallocation; however a future implementation might decide
to reallocate, and if such reallocation fails, this error
should be returned

(2.6)
*/
Terminal_Result
setVisibleColumnCount	(My_ScreenBufferPtr		inPtr,
						 UInt16					inNewNumberOfCharactersWide)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (nullptr == inPtr) result = kTerminal_ResultInvalidID;
	else
	{
		// move cursor, if necessary
		if (inNewNumberOfCharactersWide <= inPtr->current.cursorX)
		{
			moveCursorX(inPtr, inNewNumberOfCharactersWide - 1);
		}
		
		if (inNewNumberOfCharactersWide > Terminal_ReturnAllocatedColumnCount())
		{
			// flag an error, but set a reasonable value anyway
			result = kTerminal_ResultParameterError;
			inNewNumberOfCharactersWide = Terminal_ReturnAllocatedColumnCount();
		}
		inPtr->text.visibleScreen.numberOfColumnsPermitted = inNewNumberOfCharactersWide;
	}
	return result;
}// setVisibleColumnCount


/*!
Changes the number of lines of text, not including the
scrollback buffer, for a screen buffer.  This non-trivial
operation requires reallocating a series of arrays.

IMPORTANT:	This is a low-level routine for internal use;
			send a "kTerminal_ChangeScreenSize" notification
			after all size changes are complete.  Note that
			Terminal_SetVisibleScreenDimensions() will set
			dimensions and automatically notify listeners.

\retval kTerminal_ResultOK
if the terminal is resized without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the given number of rows is too small or too large

\retval kTerminal_ResultNotEnoughMemory
if it is not possible to allocate the requested number of rows

(2.6)
*/
Terminal_Result
setVisibleRowCount	(My_ScreenBufferPtr		inPtr,
					 UInt16					inNewNumberOfLinesHigh)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	//Console_WriteValue("requested new number of lines", inNewNumberOfLinesHigh);
	if (inNewNumberOfLinesHigh > 200)
	{
		Console_WriteLine("refusing to resize on account of ridiculous line size");
		result = kTerminal_ResultParameterError;
	}
	else if (nullptr == inPtr) result = kTerminal_ResultInvalidID;
	else if (STATIC_CAST(inPtr->screenBuffer.size(), UInt16) != inNewNumberOfLinesHigh)
	{
		// then the requested number of lines is different than the current number; resize!
		SInt16 const	kOriginalNumberOfLines = STATIC_CAST(inPtr->screenBuffer.size(), SInt16);
		SInt16 const	kLineDelta = (inNewNumberOfLinesHigh - kOriginalNumberOfLines);
		
		
		//Console_WriteValue("window line size", kOriginalNumberOfLines);
		//Console_WriteValue("window size change", kLineDelta);
		
		// force view to the top of the bottommost screenful
		// UNIMPLEMENTED
		
		if (kLineDelta > 0)
		{
			// if more lines are in the screen buffer than before,
			// allocate space for them (but don’t scroll them off!)
			Boolean		insertOK = screenInsertNewLines(inPtr, kLineDelta);
			
			
			unless (insertOK) result = kTerminal_ResultNotEnoughMemory;
		}
		else
		{
		#if 0
			// this will move the entire screen buffer into the scrollback
			// beforehand, if desired (having the effect of CLEARING the
			// main screen, which is basically undesirable most of the time)
			if (inPtr->text.scrollback.enabled)
			{
				(Boolean)screenCopyLinesToScrollback(inPtr);
			}
		#endif
			
			// now make sure the cursor line doesn’t fall off the end
			if (inPtr->current.cursorY >= (inPtr->screenBuffer.size() + kLineDelta))
			{
				moveCursorY(inPtr, inPtr->screenBuffer.size() + kLineDelta - 1);
			}
			
			// finally, shrink the buffer
			inPtr->screenBuffer.resize(inPtr->screenBuffer.size() + kLineDelta);
		}
		
		// reset visible region; if the custom margin is also at the bottom,
		// keep it at the new bottom (otherwise, assume there was a good
		// reason that it was not using all the lines, and leave it alone!)
		if (inPtr->customScrollingRegion.lastRow >= inPtr->visibleBoundary.rows.lastRow)
		{
			inPtr->customScrollingRegion.lastRow = inNewNumberOfLinesHigh - 1;
			if (inPtr->customScrollingRegion.firstRow > inPtr->customScrollingRegion.lastRow)
			{
				inPtr->customScrollingRegion.firstRow = inPtr->customScrollingRegion.lastRow - 1;
				assertScrollingRegion(inPtr);
			}
		}
		inPtr->visibleBoundary.rows.firstRow = 0;
		inPtr->visibleBoundary.rows.lastRow = inNewNumberOfLinesHigh - 1;
		
		// add new screen rows to the text-change region; this should trigger
		// things like Terminal View updates
		if (kLineDelta > 0)
		{
			// when the screen is becoming bigger, it is only necessary to
			// refresh the new region; when the screen is becoming smaller,
			// no refresh is necessary at all!
			Terminal_RangeDescription	range;
			
			
			//Console_WriteLine("text changed event: set visible row count");
			range.screen = inPtr->selfRef;
			range.firstRow = kOriginalNumberOfLines;
			range.firstColumn = 0;
			range.columnCount = inPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kLineDelta;
			changeNotifyForTerminal(inPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
	
	return result;
}// setVisibleRowCount


/*!
Returns an autoreleased string that is a substring of the
given string, keeping all leading whitespace but stopping
before all trailing whitespace.

This is useful when searching, for example, as there is no
benefit to scanning beyond the text portion of a line.

TEMPORARY:	Could move this to String Utilities module but
			that file is pure C++ right now.

(4.1)
*/
CFStringRef
stringByStrippingEndWhitespace		(CFStringRef	inSourceString)
{
	CFStringRef		result = inSourceString;
	
	
	if (nullptr != inSourceString)
	{
		CFIndex const		kStringLength = CFStringGetLength(inSourceString);
		NSCharacterSet*		whitespaceSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
		NSString*			asNSString = BRIDGE_CAST(inSourceString, NSString*);
		NSInteger			i = 0;
		
		
		for (i = (kStringLength - 1);
				((i >= 0) && [whitespaceSet characterIsMember:[asNSString characterAtIndex:i]]); --i)
		{
			// empty loop
		}
		
		result = BRIDGE_CAST([asNSString substringToIndex:(i + 1)], CFStringRef);
	}
	
	return result;
}// stringByStrippingEndWhitespace


/*!
Removes all tab stops.  See also tabStopInitialize(),
which sets tabs to reasonable default values.

(3.0)
*/
void
tabStopClearAll		(My_ScreenBufferPtr		inDataPtr)
{
	for (auto& tabStopChar : inDataPtr->tabSettings)
	{
		tabStopChar = kMy_TabClear;
	}
}// tabStopClearAll


/*!
Returns the number of spaces until the next tab stop, based on
the current cursor position of the specified screen.

If "inForwardDirection" is true, the distance is returned
relative to the next stop to the right of the cursor location;
otherwise, the distance refers to the next stop to the left
(that is, a backwards tab).

(2.6)
*/
UInt16
tabStopGetDistanceFromCursor	(My_ScreenBufferConstPtr	inDataPtr,
								 Boolean					inForwardDirection)
{
	UInt16		result = 0;
	
	
	if (inForwardDirection)
	{
		if (inDataPtr->current.cursorX < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1))
		{
			result = inDataPtr->current.cursorX + 1;
			while ((inDataPtr->tabSettings[result] != kMy_TabSet) &&
					(result < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1)))
			{
				++result;
			}
			result = (result - inDataPtr->current.cursorX);
		}
	}
	else
	{
		if (inDataPtr->current.cursorX > 0)
		{
			result = inDataPtr->current.cursorX - 1;
			while ((inDataPtr->tabSettings[result] != kMy_TabSet) && (result > 0))
			{
				--result;
			}
			result = (inDataPtr->current.cursorX - result);
		}
	}
	return result;
}// tabStopGetDistanceFromCursor


/*!
Reset tabs to default stops (one every "kMy_TabStop"
columns starting at the first column, and one at the
last column).

(3.0)
*/
void
tabStopInitialize	(My_ScreenBufferPtr		inDataPtr)
{
	My_TabStopList::size_type	i = 0;
	
	
	for (auto& tabStopChar : inDataPtr->tabSettings)
	{
		if (0 == (i % kMy_TabStop))
		{
			tabStopChar = kMy_TabSet;
		}
		else
		{
			tabStopChar = kMy_TabClear;
		}
		++i;
	}
	assert(!inDataPtr->tabSettings.empty());
	inDataPtr->tabSettings.back() = kMy_TabSet; // also make last column a tab
}// tabStopInitialize


/*!
A POSIX thread (which can be preempted) that handles
dynamic search for a portion of a terminal buffer.

WARNING:	As this is a preemptable thread, you MUST
		NOT use thread-unsafe system calls here.
		On the other hand, you can arrange for
		events to be handled (e.g. with queues).

(4.1)
*/
void*
threadForTerminalSearch		(void*	inSearchThreadContextPtr)
{
	My_SearchThreadContextPtr	contextPtr = REINTERPRET_CAST(inSearchThreadContextPtr, My_SearchThreadContextPtr);
	Boolean const				kIsScreen = (0 == contextPtr->threadNumber); // else scrollback
	SInt32 const				kPastEndRowIndex = (contextPtr->startRowIndex + contextPtr->rowCount);
	SInt32						rowIndex = contextPtr->startRowIndex;
	
	
	for (auto toLine = contextPtr->rangeStart; rowIndex < kPastEndRowIndex; ++toLine, ++rowIndex)
	{
		if (toLine->isDefault())
		{
			// do not even try to search blank lines (initial state);
			// this will save some time, especially in new terminals
			// that have gigantic unused scrollback buffers
			continue;
		}
		
		// find ALL matches; NOTE that this technically will not find words
		// that begin at the end of one line and continue at the start of
		// the next, but that is a known limitation right now (TEMPORARY)
		My_ScreenBufferLine const&	kLine = **toLine;
		CFStringRef const			kCFStringToSearch = stringByStrippingEndWhitespace(kLine.textCFString.returnCFStringRef());
		std::vector<CFRange>		matchRanges;
		
		
		if (contextPtr->isRegularExpression)
		{
			// regular expression string matches
			NSError* /*__autoreleasing*/	error = nil;
			NSRegularExpression*			expressionMatcher = [NSRegularExpression
																	regularExpressionWithPattern:BRIDGE_CAST(contextPtr->queryCFString, NSString*)
																									options:contextPtr->regExOptions
																									error:&error];
			
			
			if (nil == expressionMatcher)
			{
				Console_Warning(Console_WriteValueCFString, "failed to create regex, error", BRIDGE_CAST([error localizedDescription], CFStringRef));
			}
			else
			{
				NSArray<NSTextCheckingResult*>*		matchObjects = [expressionMatcher
																	matchesInString:BRIDGE_CAST(kCFStringToSearch, NSString*)
																					options:(NSMatchingOptions)0
																					range:NSMakeRange(0, CFStringGetLength(kCFStringToSearch))];
				
				
				if (nil != matchObjects)
				{
					matchRanges.reserve(matchObjects.count);
					for (NSTextCheckingResult* aMatch in matchObjects)
					{
						matchRanges.push_back(CFRangeMake(aMatch.range.location, aMatch.range.length));
					}
				}
				else
				{
					Console_Warning(Console_WriteLine, "regular expression not created");
				}
			}
		}
		else
		{
			// literal string matches
			CFRetainRelease		resultsArray(CFStringCreateArrayWithFindResults
												(kCFAllocatorDefault, kCFStringToSearch, contextPtr->queryCFString,
													CFRangeMake(0, CFStringGetLength(kCFStringToSearch)),
													contextPtr->searchFlags),
												CFRetainRelease::kAlreadyRetained);
			CFArrayRef const	kResultsArrayRef = resultsArray.returnCFArrayRef();
			CFIndex const		kNumberOfMatches = ((nullptr == kResultsArrayRef)
													? 0
													: CFArrayGetCount(kResultsArrayRef));
			
			
			matchRanges.reserve(kNumberOfMatches);
			for (CFIndex i = 0; i < kNumberOfMatches; ++i)
			{
				CFRange const*		toRange = REINTERPRET_CAST(CFArrayGetValueAtIndex(kResultsArrayRef, i),
																CFRange const*);
				
				
				matchRanges.push_back(*toRange);
			}
		}
		
		if (false == matchRanges.empty())
		{
			// return the range of text that was found
			
			// extend the size of the results vector
			contextPtr->matchesVectorPtr->reserve(contextPtr->matchesVectorPtr->size() + matchRanges.size());
			
			// figure out where all the matches are on the screen
			for (CFRange aRange : matchRanges)
			{
				SInt32						firstRow = rowIndex;
				UInt16						firstColumn = 0;
				Terminal_RangeDescription	textRegion;
				
				
				// translate all results ranges into external form; the
				// caller understands rows and columns, etc. not offsets
				// into a giant buffer
				firstColumn = STATIC_CAST(aRange.location, UInt16);
				if (false == kIsScreen)
				{
					// translate scrollback into negative coordinates (zero-based)
					firstRow = -firstRow - 1;
				}
				bzero(&textRegion, sizeof(textRegion));
				textRegion.screen = contextPtr->screenBufferPtr->selfRef;
				textRegion.firstRow = firstRow;
				textRegion.firstColumn = firstColumn;
				textRegion.columnCount = STATIC_CAST(aRange.length, UInt16);
				textRegion.rowCount = 1;
				contextPtr->matchesVectorPtr->push_back(textRegion);
			}
		}
		else
		{
			// text was not found
			//Console_WriteLine("string not found");
		}
	}
	
	// since the thread is finished, dispose of dynamically-allocated memory
	Memory_DisposePtrInterruptSafe(REINTERPRET_CAST(&contextPtr, void**));
	
	return nullptr;
}// threadForTerminalSearch


/*!
Translates the given character code using the rules
of the current character set of the specified screen
(character sets G0 or G1, for VT terminals).  The
new code is returned, which may be unchanged.

The attributes are now provided too, which allows
the internal storage of an original character to be
more appropriate; e.g. an ASCII-encoded graphics
character may be stored as the Unicode character
that has the intended glyph.

If the given character needs its attributes changed
(typically, because it is actually graphical), those
new attributes are returned.  The new attributes
should ONLY apply to the given character; they do
not indicate a new default for the character stream.

(4.0)
*/
inline UniChar
translateCharacter	(My_ScreenBufferPtr			inDataPtr,
					 UnicodeScalarValue			inCharacter,
					 TextAttributes_Object		inAttributes,
					 TextAttributes_Object&		outNewAttributes)
{
	UniChar		result = inCharacter;
	
	
	outNewAttributes = inAttributes; // initially...
	switch (inDataPtr->current.characterSetInfoPtr->translationTable)
	{
	case kMy_CharacterSetVT100UnitedStates:
		// this is the default; do nothing
		break;
	
	case kMy_CharacterSetVT100UnitedKingdom:
		// the only difference between ASCII and U.K. is that
		// the pound sign (#) is a British currency symbol (£)
		if (inCharacter == '#') result = 0x00A3;
		break;
	
	default:
		// ???
		break;
	}
	
	// TEMPORARY - the renderer does not handle most Unicode characters,
	// but programs sometimes choose “unnecessarily exotic” variations
	// of characters that would lead to unknown-character renderings when
	// it is pretty easy to choose sensible ASCII equivalents...
	switch (inCharacter)
	{
	case 0x2212: // minus sign
	case 0x2010: // hyphen
		result = '-';
		break;
	
	default:
		break;
	}
	
	if (inAttributes.hasAttributes(kTextAttributes_VTGraphics))
	{
		Boolean const	kIsBold = inAttributes.hasBold();
		Boolean const	kVT52 = (false == inDataPtr->modeANSIEnabled);
		
		
		// if text was originally encoded with graphics attributes, internally
		// store the equivalent Unicode character so that cool stuff like
		// copy and paste of text will do the right thing (the renderer may
		// still choose not to rely on Unicode fonts for rendering them);
		// INCOMPLETE: this might need terminal-emulator-specific code
		// IMPORTANT: old drawing code currently requires that non-graphical
		// symbols below (e.g. degrees, plus-minus, etc.) successfully
		// translate to the Mac Roman encoding
		switch (inCharacter)
		{
		case '`':
			result = 0x25CA; // filled diamond; using hollow (lozenge) for now, Unicode 0x2666 is better
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 'a':
			result = 0x2592; // checkerboard
			break;
		
		case 'b':
			result = 0x21E5; // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
			break;
		
		case 'c':
			result = 0x21DF; // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
			break;
		
		case 'd':
			result = 0x2190; // carriage return (international symbol is an arrow pointing right to left)
			break;
		
		case 'e':
			result = 0x2193; // line feed (international symbol is an arrow pointing top to bottom)
			break;
		
		case 'f':
			result = 0x00B0; // degrees (same in VT52)
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 'g':
			result = 0x00B1; // plus or minus (same in VT52)
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 'h':
			result = 0x21B5; // new line (international symbol is an arrow that hooks from mid-top to mid-left)
			break;
		
		case 'i':
			result = 0x2913; // vertical tab (international symbol is a down-pointing arrow with a terminating line)
			break;
		
		case 'j':
			if (kVT52)
			{
				result = 0x00F7; // division
				inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			}
			else
			{
				result = (kIsBold) ? 0x251B : 0x2518; // hook mid-top to mid-left
			}
			break;
		
		case 'k':
			result = (kIsBold) ? 0x2513 : 0x2510; // hook mid-left to mid-bottom
			break;
		
		case 'l':
			result = (kIsBold) ? 0x250F : 0x250C; // hook mid-right to mid-bottom
			break;
		
		case 'm':
			result = (kIsBold) ? 0x2517 : 0x2514; // hook mid-top to mid-right
			break;
		
		case 'n':
			result = (kIsBold) ? 0x254B : 0x253C; // cross
			break;
		
		case 'o':
			result = 0x23BA; // top line
			break;
		
		case 'p':
			result = 0x23BB; // line between top and middle regions
			break;
		
		case 'q':
			result = (kIsBold) ? 0x2501 : 0x2500; // middle line
			break;
		
		case 'r':
			result = 0x23BC; // line between middle and bottom regions
			break;
		
		case 's':
			result = 0x23BD; // bottom line
			break;
		
		case 't':
			result = (kIsBold) ? 0x2523 : 0x251C; // cross minus the left piece
			break;
		
		case 'u':
			result = (kIsBold) ? 0x252B : 0x2524; // cross minus the right piece
			break;
		
		case 'v':
			result = (kIsBold) ? 0x253B : 0x2534; // cross minus the bottom piece
			break;
		
		case 'w':
			result = (kIsBold) ? 0x2533 : 0x252C; // cross minus the top piece
			break;
		
		case 'x':
			result = (kIsBold) ? 0x2503 : 0x2502; // vertical line
			break;
		
		case 'y':
			result = 0x2264; // less than or equal to
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 'z':
			result = 0x2265; // greater than or equal to
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case '{':
			result = 0x03C0; // pi
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case '|':
			result = 0x2260; // not equal to
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case '}':
			result = 0x00A3; // British pounds (currency) symbol
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case '~':
			if (kVT52)
			{
				result = 0x00B6; // pilcrow (paragraph) sign
				inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			}
			else
			{
				result = 0x2027; // centered dot
				inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			}
			break;
		
		case 159:
			result = 0x0192; // small 'f' with hook
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 224:
			result = 0x03B1; // alpha
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 225:
			result = 0x00DF; // beta
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 226:
			result = 0x0393; // capital gamma
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 227:
			result = 0x03C0; // pi
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 228:
			result = 0x03A3; // capital sigma
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 229:
			result = 0x03C3; // sigma
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 230:
			result = 0x00B5; // mu
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 231:
			result = 0x03C4; // tau
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 232:
			result = 0x03A6; // capital phi
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 233:
			result = 0x0398; // capital theta
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 234:
			result = 0x03A9; // capital omega
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 235:
			result = 0x03B4; // delta
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 237:
			result = 0x03C6; // phi
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 238:
			result = 0x03B5; // epsilon
			inAttributes.removeAttributes(kTextAttributes_VTGraphics); // render normally
			break;
		
		case 251:
			result = 0x221A; // square root left edge
			break;
		
		default:
			break;
		}
	}
	else if ((inCharacter >= 0x2800) && (inCharacter <= 0x28FF))
	{
		// all characters in the Braille range
		outNewAttributes.addAttributes(kTextAttributes_VTGraphics);
	}
	else
	{
		// the original character was not explicitly identified as graphical,
		// but it may still be best to *tag* it as such so that a more
		// advanced rendering can be done (default font renderings for
		// graphics are not always as nice)
		switch (inCharacter)
		{
		// this list should generally match the set of Unicode characters that
		// are handled by the drawVTGraphicsGlyph() internal method in the
		// Terminal View module
		case '=': // equal to
		case 0x2190: // leftwards arrow
		case 0x2191: // upwards arrow
		case 0x2192: // rightwards arrow
		case 0x2193: // downwards arrow
		case 0x21B5: // new line (international symbol is an arrow that hooks from mid-top to mid-left)
		case 0x21DF: // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
		case 0x21E5: // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
		case 0x221A: // square root left edge
		case 0x22EF: // middle ellipsis (three dots, centered)
		case 0x23B2: // large sigma (summation), top half
		case 0x23B3: // large sigma (summation), bottom half
		case 0x23BA: // top line
		case 0x23BB: // line between top and middle regions
		case 0x23BC: // line between middle and bottom regions
		case 0x23BD: // bottom line
		case 0x2500: // middle line
		case 0x2501: // middle line, bold version
		case 0x2502: // vertical line
		case 0x2503: // vertical line, bold version
		case 0x2504: // horizontal triple-dashed line
		case 0x2505: // horizontal triple-dashed line, bold version
		case 0x2506: // vertical triple-dashed line
		case 0x2507: // vertical triple-dashed line, bold version
		case 0x2508: // horizontal quadruple-dashed line
		case 0x2509: // horizontal quadruple-dashed line, bold version
		case 0x250A: // vertical quadruple-dashed line
		case 0x250B: // vertical quadruple-dashed line, bold version
		case 0x250C: // hook mid-right to mid-bottom
		case 0x250D: // hook mid-right to mid-bottom, bold right
		case 0x250E: // hook mid-right to mid-bottom, bold bottom
		case 0x250F: // hook mid-right to mid-bottom, bold version
		case 0x2510: // hook mid-left to mid-bottom
		case 0x2511: // hook mid-left to mid-bottom, bold left
		case 0x2512: // hook mid-left to mid-bottom, bold bottom
		case 0x2513: // hook mid-left to mid-bottom, bold version
		case 0x2514: // hook mid-top to mid-right
		case 0x2515: // hook mid-top to mid-right, bold right
		case 0x2516: // hook mid-top to mid-right, bold top
		case 0x2517: // hook mid-top to mid-right, bold version
		case 0x2518: // hook mid-top to mid-left
		case 0x2519: // hook mid-top to mid-left, bold left
		case 0x251A: // hook mid-top to mid-left, bold top
		case 0x251B: // hook mid-top to mid-left, bold version
		case 0x251C: // cross minus the left piece
		case 0x251D: // cross minus the left piece, bold right
		case 0x251E: // cross minus the left piece, bold top
		case 0x251F: // cross minus the left piece, bold bottom
		case 0x2520: // cross minus the left piece, bold vertical
		case 0x2521: // cross minus the left piece, bold hook mid-top to mid-right
		case 0x2522: // cross minus the left piece, bold hook mid-bottom to mid-right
		case 0x2523: // cross minus the left piece, bold version
		case 0x2524: // cross minus the right piece
		case 0x2525: // cross minus the right piece, bold left
		case 0x2526: // cross minus the right piece, bold top
		case 0x2527: // cross minus the right piece, bold bottom
		case 0x2528: // cross minus the right piece, bold vertical
		case 0x2529: // cross minus the right piece, bold hook mid-top to mid-left
		case 0x252A: // cross minus the right piece, bold hook mid-bottom to mid-left
		case 0x252B: // cross minus the right piece, bold version
		case 0x252C: // cross minus the top piece
		case 0x252D: // cross minus the top piece, bold left
		case 0x252E: // cross minus the top piece, bold right
		case 0x252F: // cross minus the top piece, bold horizontal
		case 0x2530: // cross minus the top piece, bold bottom
		case 0x2531: // cross minus the top piece, bold hook mid-bottom to mid-left
		case 0x2532: // cross minus the top piece, bold hook mid-bottom to mid-right
		case 0x2533: // cross minus the top piece, bold version
		case 0x2534: // cross minus the bottom piece
		case 0x2535: // cross minus the bottom piece, bold left
		case 0x2536: // cross minus the bottom piece, bold right
		case 0x2537: // cross minus the bottom piece, bold horizontal
		case 0x2538: // cross minus the bottom piece, bold top
		case 0x2539: // cross minus the bottom piece, bold hook mid-top to mid-left
		case 0x253A: // cross minus the bottom piece, bold hook mid-top to mid-right
		case 0x253B: // cross minus the bottom piece, bold version
		case 0x253C: // cross
		case 0x253D: // cross, bold left
		case 0x253E: // cross, bold right
		case 0x253F: // cross, bold horizontal
		case 0x2540: // cross, bold top
		case 0x2541: // cross, bold bottom
		case 0x2542: // cross, bold vertical
		case 0x2543: // cross, bold hook mid-top to mid-left
		case 0x2544: // cross, bold hook mid-top to mid-right
		case 0x2545: // cross, bold hook mid-bottom to mid-left
		case 0x2546: // cross, bold hook mid-bottom to mid-right
		case 0x2547: // cross, bold T-up
		case 0x2548: // cross, bold T-down
		case 0x2549: // cross, bold T-left
		case 0x254A: // cross, bold T-right
		case 0x254B: // cross, bold version
		case 0x254C: // horizontal double-dashed line
		case 0x254D: // horizontal double-dashed line, bold version
		case 0x254E: // vertical double-dashed line
		case 0x254F: // vertical double-dashed line, bold version
		case 0x2550: // middle line, double-line version
		case 0x2551: // vertical line, double-line version
		case 0x2552: // hook mid-right to mid-bottom, double-horizontal-only version
		case 0x2553: // hook mid-right to mid-bottom, double-vertical-only version
		case 0x2554: // hook mid-right to mid-bottom, double-line version
		case 0x2555: // hook mid-left to mid-bottom, double-horizontal-only version
		case 0x2556: // hook mid-left to mid-bottom, double-vertical-only version
		case 0x2557: // hook mid-left to mid-bottom, double-line version
		case 0x2558: // hook mid-top to mid-right, double-horizontal-only version
		case 0x2559: // hook mid-top to mid-right, double-vertical-only version
		case 0x255A: // hook mid-top to mid-right, double-line version
		case 0x255B: // hook mid-top to mid-left, double-horizontal-only version
		case 0x255C: // hook mid-top to mid-left, double-vertical-only version
		case 0x255D: // hook mid-top to mid-left, double-line version
		case 0x255E: // cross minus the left piece, double-horizontal-only version
		case 0x255F: // cross minus the left piece, double-vertical-only version
		case 0x2560: // cross minus the left piece, double-line version
		case 0x2561: // cross minus the right piece, double-horizontal-only version
		case 0x2562: // cross minus the right piece, double-vertical-only version
		case 0x2563: // cross minus the right piece, double-line version
		case 0x2564: // cross minus the top piece, double-horizontal-only version
		case 0x2565: // cross minus the top piece, double-vertical-only version
		case 0x2566: // cross minus the top piece, double-line version
		case 0x2567: // cross minus the bottom piece, double-horizontal-only version
		case 0x2568: // cross minus the bottom piece, double-vertical-only version
		case 0x2569: // cross minus the bottom piece, double-line version
		case 0x256A: // cross, double-horizontal-only version
		case 0x256B: // cross, double-vertical-only version
		case 0x256C: // cross, double-line version
		case 0x2320: // integral sign (elongated S), top
		case 0x2321: // integral sign (elongated S), bottom
		case 0x239B: // left parenthesis, upper
		case 0x239C: // left parenthesis extension
		case 0x239D: // left parenthesis, lower
		case 0x239E: // right parenthesis, upper
		case 0x239F: // right parenthesis extension
		case 0x23A0: // right parenthesis, lower
		case 0x23A1: // left square bracket, upper
		case 0x23A2: // left square bracket extension
		case 0x23A3: // left square bracket, lower
		case 0x23A4: // right square bracket, upper
		case 0x23A5: // right square bracket extension
		case 0x23A6: // right square bracket, lower
		case 0x23A7: // left curly brace, upper
		case 0x23A8: // left curly brace, middle
		case 0x23A9: // left curly brace, lower
		case 0x23AA: // curly brace extension
		case 0x23AB: // right curly brace, upper
		case 0x23AC: // right curly brace, middle
		case 0x23AD: // right curly brace, lower
		case 0x23AE: // integral extension
		case 0x23B7: // square root bottom, centered
		case 0x23B8: // left vertical box line
		case 0x23B9: // right vertical box line
		case 0x23D0: // vertical line extension
		case 0x256D: // curved mid-right to mid-bottom
		case 0x256E: // curved mid-left to mid-bottom
		case 0x256F: // curved mid-top to mid-left
		case 0x2570: // curved mid-top to mid-right
		case 0x2571: // diagonal line from top-right to bottom-left
		case 0x2572: // diagonal line from top-left to bottom-right
		case 0x2573: // diagonal lines from each corner crossing in the center
		case 0x2574: // cross, left segment only
		case 0x2575: // cross, top segment only
		case 0x2576: // cross, right segment only
		case 0x2577: // cross, bottom segment only
		case 0x2578: // cross, left segment only, bold version
		case 0x2579: // cross, top segment only, bold version
		case 0x257A: // cross, right segment only, bold version
		case 0x257B: // cross, bottom segment only, bold version
		case 0x257C: // horizontal line, bold right half
		case 0x257D: // vertical line, bold bottom half
		case 0x257E: // horizontal line, bold left half
		case 0x257F: // vertical line, bold top half
		case 0x2580: // upper-half block
		case 0x2581: // 1/8 bottom block
		case 0x2582: // 2/8 (1/4) bottom block
		case 0x2583: // 3/8 bottom block
		case 0x2584: // lower-half block
		case 0x2585: // 5/8 bottom block
		case 0x2586: // 6/8 (3/4) bottom block
		case 0x2587: // 7/8 bottom block
		case 0x2588: // solid block
		case 0x2589: // 7/8 left block
		case 0x258A: // 6/8 (3/4) left block
		case 0x258B: // 5/8 left block
		case 0x258C: // left-half block
		case 0x258D: // 3/8 left block
		case 0x258E: // 2/8 (1/4) left block
		case 0x258F: // 1/8 left block
		case 0x2590: // right-half block
		case 0x2591: // light gray pattern
		case 0x2592: // medium gray pattern
		case 0x2593: // heavy gray pattern or checkerboard
		case 0x2594: // 1/8 top block
		case 0x2595: // 1/8 right block
		case 0x2596: // quadrant lower-left
		case 0x2597: // quadrant lower-right
		case 0x2598: // quadrant upper-left
		case 0x2599: // block minus upper-right quadrant
		case 0x259A: // quadrants upper-left and lower-right
		case 0x259B: // block minus lower-right quadrant
		case 0x259C: // block minus lower-left quadrant
		case 0x259D: // quadrant upper-right
		case 0x259E: // quadrants upper-right and lower-left
		case 0x259F: // block minus upper-left quadrant
		case 0x2699: // gear
		case 0x26A1: // online/offline lightning bolt
		case 0x2713: // check mark
		case 0x2714: // check mark, bold
		case 0x2718: // X mark
		case 0x27A6: // curve-to-right arrow (used for detached-from-head in "powerline")
		case 0x2913: // vertical tab (international symbol is a down-pointing arrow with a terminating line)
		case 0xE0A0: // "powerline" version control branch
		case 0xE0A1: // "powerline" line (LN) marker
		case 0xE0A2: // "powerline" closed padlock
		case 0xE0B0: // "powerline" rightward triangle
		case 0xE0B1: // "powerline" rightward arrowhead
		case 0xE0B2: // "powerline" leftward triangle
		case 0xE0B3: // "powerline" leftward arrowhead
		case 0xFFFD: // replacement character
			outNewAttributes.addAttributes(kTextAttributes_VTGraphics);
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// translateCharacter

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
