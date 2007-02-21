/*###############################################################

	Terminal.cp
	
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

// standard-C includes
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

// standard-C++ includes
#include <algorithm>
#include <iterator>
#include <list>
#include <string>
#include <vector>

// GNU compiler includes
#include <ext/algorithm>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CFRetainRelease.h>
#include <Console.h>
#include <Cursors.h>
#include <FileSelectionDialogs.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// resource includes
#include "ApplicationVersion.h"
#include "StringResources.h"

// MacTelnet includes
#include "Commands.h"
#include "ConnectionData.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Folder.h"
#include "Preferences.h"
#include "Session.h"
#include "TelnetPrinting.h"
#include "Terminal.h"
#include "TerminalSpeaker.h"
#include "TerminalView.h"
#include "VTKeys.h"



#pragma mark Constants

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	/*!
	All possible states the emulator’s character parser can be in.
	The list starts with generic states that simply document sequences
	of characters, so that the “descript” states can be aliases for
	them.  This approach makes it easier to see overlap (conflicts)
	between emulators, and also lets each code base use the name most
	natural for it: i.e. when determining the next logical state it
	may make sense to use generic names, but when acting on a state
	transition it would be helpful to use the descript name.
	
	IMPORTANT:	This reflects the PARSER ONLY, so DUPLICATES MAKE SENSE.
				This must be combined with any internal flags set in
				terminal data structures (e.g. to know whether a VT100
				is in ANSI or VT52 mode) to understand what should
				actually happen when the parser is in any given state.
				See "My_EmulatorStateTransitionProcPtr".
	*/
	namespace My_Parser
	{
		enum State
		{
			// generic states - these are automatically handled by the default state determinant based on the data stream
			kStateSeenNull						= FOUR_CHAR_CODE('Ctl@'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlA					= FOUR_CHAR_CODE('CtlA'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlB					= FOUR_CHAR_CODE('CtlB'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlC					= FOUR_CHAR_CODE('CtlC'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlD					= FOUR_CHAR_CODE('CtlD'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlE					= FOUR_CHAR_CODE('CtlE'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlF					= FOUR_CHAR_CODE('CtlF'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlG					= FOUR_CHAR_CODE('CtlG'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlH					= FOUR_CHAR_CODE('CtlH'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlI					= FOUR_CHAR_CODE('CtlI'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlJ					= FOUR_CHAR_CODE('CtlJ'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlK					= FOUR_CHAR_CODE('CtlK'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlL					= FOUR_CHAR_CODE('CtlL'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlM					= FOUR_CHAR_CODE('CtlM'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlN					= FOUR_CHAR_CODE('CtlN'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlO					= FOUR_CHAR_CODE('CtlO'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlP					= FOUR_CHAR_CODE('CtlP'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlQ					= FOUR_CHAR_CODE('CtlQ'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlR					= FOUR_CHAR_CODE('CtlR'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlS					= FOUR_CHAR_CODE('CtlS'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlT					= FOUR_CHAR_CODE('CtlT'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlU					= FOUR_CHAR_CODE('CtlU'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlV					= FOUR_CHAR_CODE('CtlV'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlW					= FOUR_CHAR_CODE('CtlW'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlX					= FOUR_CHAR_CODE('CtlX'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlY					= FOUR_CHAR_CODE('CtlY'),	//!< generic state used to define emulator-specific states, below
			kStateSeenControlZ					= FOUR_CHAR_CODE('CtlZ'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESC						= FOUR_CHAR_CODE('cESC'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracket			= FOUR_CHAR_CODE('ESC['),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParams	= FOUR_CHAR_CODE('E[;;'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsA	= FOUR_CHAR_CODE('E[;A'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsB	= FOUR_CHAR_CODE('E[;B'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsc	= FOUR_CHAR_CODE('E[;c'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsC	= FOUR_CHAR_CODE('E[;C'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsD	= FOUR_CHAR_CODE('E[;D'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsf	= FOUR_CHAR_CODE('E[;f'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsg	= FOUR_CHAR_CODE('E[;g'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsh	= FOUR_CHAR_CODE('E[;h'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsH	= FOUR_CHAR_CODE('E[;H'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsJ	= FOUR_CHAR_CODE('E[;J'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsK	= FOUR_CHAR_CODE('E[;K'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsl	= FOUR_CHAR_CODE('E[;l'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsm	= FOUR_CHAR_CODE('E[;m'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsn	= FOUR_CHAR_CODE('E[;n'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsq	= FOUR_CHAR_CODE('E[;q'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsr	= FOUR_CHAR_CODE('E[;r'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftSqBracketParamsx	= FOUR_CHAR_CODE('E[;x'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftParen				= FOUR_CHAR_CODE('ESC('),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftParenA				= FOUR_CHAR_CODE('ES(A'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftParenB				= FOUR_CHAR_CODE('ES(B'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftParen0				= FOUR_CHAR_CODE('ES(0'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftParen1				= FOUR_CHAR_CODE('ES(1'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLeftParen2				= FOUR_CHAR_CODE('ES(2'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCRightParen				= FOUR_CHAR_CODE('ESC)'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCRightParenA			= FOUR_CHAR_CODE('ES)A'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCRightParenB			= FOUR_CHAR_CODE('ES)B'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCRightParen0			= FOUR_CHAR_CODE('ES)0'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCRightParen1			= FOUR_CHAR_CODE('ES)1'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCRightParen2			= FOUR_CHAR_CODE('ES)2'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCA						= FOUR_CHAR_CODE('ESCA'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCB						= FOUR_CHAR_CODE('ESCB'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCC						= FOUR_CHAR_CODE('ESCC'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCc						= FOUR_CHAR_CODE('ESCc'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCD						= FOUR_CHAR_CODE('ESCD'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCE						= FOUR_CHAR_CODE('ESCE'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCF						= FOUR_CHAR_CODE('ESCF'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCG						= FOUR_CHAR_CODE('ESCG'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCH						= FOUR_CHAR_CODE('ESCH'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCI						= FOUR_CHAR_CODE('ESCI'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCJ						= FOUR_CHAR_CODE('ESCJ'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCK						= FOUR_CHAR_CODE('ESCK'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCM						= FOUR_CHAR_CODE('ESCM'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCY						= FOUR_CHAR_CODE('ESCY'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCZ						= FOUR_CHAR_CODE('ESCZ'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESC7						= FOUR_CHAR_CODE('ESC7'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESC8						= FOUR_CHAR_CODE('ESC8'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCPound					= FOUR_CHAR_CODE('ESC#'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCPound3					= FOUR_CHAR_CODE('ES#3'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCPound4					= FOUR_CHAR_CODE('ES#4'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCPound5					= FOUR_CHAR_CODE('ES#5'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCPound6					= FOUR_CHAR_CODE('ES#6'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCPound8					= FOUR_CHAR_CODE('ES#8'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCEquals					= FOUR_CHAR_CODE('ESC='),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCLessThan				= FOUR_CHAR_CODE('ESC<'),	//!< generic state used to define emulator-specific states, below
			kStateSeenESCGreaterThan			= FOUR_CHAR_CODE('ESC>'),	//!< generic state used to define emulator-specific states, below
			
			// key states
			kStateInitial				= FOUR_CHAR_CODE('init'),				//!< the very first state, no characters have yet been entered
			kStateEcho					= FOUR_CHAR_CODE('echo'),				//!< no sense could be made of the input, so let it fall through to display
			
			// VT100 immediates (in order of the corresponding control character’s ASCII code)
			kStateVT100ControlENQ		= FOUR_CHAR_CODE('VAns'),				//!< transmit answerback message
			kStateVT100ControlBEL		= FOUR_CHAR_CODE('VBel'),				//!< audio event
			kStateVT100ControlBS		= FOUR_CHAR_CODE('VBks'),				//!< move cursor left if possible
			kStateVT100ControlHT		= FOUR_CHAR_CODE('VTab'),				//!< move cursor right to tab stop, or margin
			kStateVT100ControlLFVTFF	= FOUR_CHAR_CODE('VLnF'),				//!< newline or line feed, depending on LNM
			kStateVT100ControlCR		= FOUR_CHAR_CODE('VCRt'),				//!< move cursor down and to left margin
			kStateVT100ControlSO		= FOUR_CHAR_CODE('VShO'),				//!< shift out to G1 character set
			kStateVT100ControlSI		= FOUR_CHAR_CODE('VShI'),				//!< shift in to G0 character set
			kStateVT100ControlXON		= FOUR_CHAR_CODE('VXON'),				//!< resume transmission
			kStateVT100ControlXOFF		= FOUR_CHAR_CODE('VXOF'),				//!< suspend transmission
			kStateVT100ControlCANSUB	= FOUR_CHAR_CODE('VCAN'),				//!< terminate control seq. with error char.
			
			// VT100 sequences in VT52 compatibility mode (in the order they appear in the manual) - see VT100 manual for full details
			kStateVT52CU				= kStateSeenESCA,				//!< cursor up
			kStateVT52CD 				= kStateSeenESCB,				//!< cursor down
			kStateVT52CR 				= kStateSeenESCC,				//!< cursor right
			kStateVT52CL 				= kStateSeenESCD,				//!< cursor left
			kStateVT52NGM				= kStateSeenESCF,				//!< enter graphics mode
			kStateVT52XGM				= kStateSeenESCG,				//!< exit graphics mode
			kStateVT52CH				= kStateSeenESCH,				//!< cursor home
			kStateVT52RLF				= kStateSeenESCI,				//!< reverse line feed
			kStateVT52EES				= kStateSeenESCJ,				//!< erase to end of screen
			kStateVT52EEL				= kStateSeenESCK,				//!< erase to end of line
			kStateVT52DCA				= kStateSeenESCY,				//!< direct cursor address
			kStateVT52DCAY				= FOUR_CHAR_CODE('DCAY'),		//!< direct cursor address, seen first follow-up character (Y)
			kStateVT52DCAX				= FOUR_CHAR_CODE('DCAX'),		//!< direct cursor address, seen second follow-up character (X)
			kStateVT52ID				= kStateSeenESCZ,				//!< identify terminal
			kStateVT52NAKM				= kStateSeenESCEquals,			//!< enter alternate keypad mode
			kStateVT52XAKM				= kStateSeenESCGreaterThan,	//!< exit alternate keypad mode
			kStateVT52ANSI				= kStateSeenESCLessThan,		//!< enter ANSI mode
			
			// VT100 sequences (in the order they appear in the manual) - see VT100 manual for full details
			kStateVT100CSI				= kStateSeenESCLeftSqBracket,	//!< control sequence inducer
			kStateVT100CSIParamScan		= kStateSeenESCLeftSqBracketParams,	//!< state of accumulating parameters
			kStateVT100CUB				= kStateSeenESCLeftSqBracketParamsD,	//!< cursor backward
			kStateVT100CUD				= kStateSeenESCLeftSqBracketParamsB,	//!< cursor down
			kStateVT100CUF				= kStateSeenESCLeftSqBracketParamsC,	//!< cursor forward
			kStateVT100CUP				= kStateSeenESCLeftSqBracketParamsH,	//!< cursor position
			kStateVT100CUU				= kStateSeenESCLeftSqBracketParamsA,	//!< cursor up
			kStateVT100DA				= kStateSeenESCLeftSqBracketParamsc,	//!< device attributes
			kStateVT100DECALN			= kStateSeenESCPound8,			//!< screen alignment display
			kStateVT100DECANM			= FOUR_CHAR_CODE('VANM'),				//!< ANSI/VT52 mode
			kStateVT100DECARM			= FOUR_CHAR_CODE('VARM'),				//!< auto repeat mode
			kStateVT100DECAWM			= FOUR_CHAR_CODE('VAWM'),				//!< auto wrap mode
			kStateVT100DECCKM			= FOUR_CHAR_CODE('VCKM'),				//!< cursor keys mode
			kStateVT100DECCOLM			= FOUR_CHAR_CODE('VCLM'),				//!< column mode
			kStateVT100DECDHLT			= kStateSeenESCPound3,			//!< double height line, top half
			kStateVT100DECDHLB			= kStateSeenESCPound4,			//!< double height line, bottom half
			kStateVT100DECDWL			= kStateSeenESCPound6,			//!< double width line
			kStateVT100DECID			= kStateSeenESCZ,				//!< identify terminal
			kStateVT100DECKPAM			= kStateSeenESCEquals,			//!< keypad application mode
			kStateVT100DECKPNM			= kStateSeenESCGreaterThan,	//!< keypad numeric mode
			kStateVT100DECLL			= kStateSeenESCLeftSqBracketParamsq,	//!< load LEDs (keyboard lights)
			kStateVT100DECOM			= FOUR_CHAR_CODE('VOM '),				//!< origin mode
			kStateVT100DECRC			= kStateSeenESC8,				//!< restore cursor
			kStateVT100DECREPTPARM		= FOUR_CHAR_CODE('VRPM'),				//!< report terminal parameters
			kStateVT100DECREQTPARM		= kStateSeenESCLeftSqBracketParamsx,	//!< request terminal parameters
			kStateVT100DECSC			= kStateSeenESC7,				//!< save cursor
			kStateVT100DECSTBM			= kStateSeenESCLeftSqBracketParamsr,	//!< set top and bottom margins
			kStateVT100DECSWL			= kStateSeenESCPound5,			//!< single width line
			kStateVT100DECTST			= FOUR_CHAR_CODE('VTST'),				//!< invoke confidence test
			kStateVT100DSR				= kStateSeenESCLeftSqBracketParamsn,	//!< device status report
			kStateVT100ED 				= kStateSeenESCLeftSqBracketParamsJ,	//!< erase in display
			kStateVT100EL 				= kStateSeenESCLeftSqBracketParamsK,	//!< erase in line
			kStateVT100HTS				= kStateSeenESCH,				//!< horizontal tabulation set
			kStateVT100HVP				= kStateSeenESCLeftSqBracketParamsf,	//!< horizontal and vertical position
			kStateVT100IND				= kStateSeenESCD,				//!< index
			kStateVT100NEL				= kStateSeenESCE,				//!< next line
			kStateVT100RI				= kStateSeenESCM,				//!< reverse index
			kStateVT100RIS				= kStateSeenESCc,				//!< reset to initial state
			kStateVT100RM				= kStateSeenESCLeftSqBracketParamsl,	//!< reset mode
			kStateVT100SCSG0UK			= kStateSeenESCLeftParenA,		//!< select character set for G0, U.K.
			kStateVT100SCSG0ASCII		= kStateSeenESCLeftParenB,		//!< select character set for G0, ASCII
			kStateVT100SCSG0SG			= kStateSeenESCLeftParen0,		//!< select character set for G0, special graphics
			kStateVT100SCSG0ACRStd		= kStateSeenESCLeftParen1,		//!< select character set for G0, alternate character ROM standard set
			kStateVT100SCSG0ACRSG		= kStateSeenESCLeftParen2,		//!< select character set for G0, alternate character ROM special graphics
			kStateVT100SCSG1UK			= kStateSeenESCRightParenA,	//!< select character set for G0, U.K.
			kStateVT100SCSG1ASCII		= kStateSeenESCRightParenB,	//!< select character set for G0, ASCII
			kStateVT100SCSG1SG			= kStateSeenESCRightParen0,	//!< select character set for G0, special graphics
			kStateVT100SCSG1ACRStd		= kStateSeenESCRightParen1,	//!< select character set for G0, alternate character ROM standard set
			kStateVT100SCSG1ACRSG		= kStateSeenESCRightParen2,	//!< select character set for G0, alternate character ROM special graphics
			kStateVT100SGR				= kStateSeenESCLeftSqBracketParamsm,	//!< select graphic rendition
			kStateVT100SM				= kStateSeenESCLeftSqBracketParamsh,	//!< set mode
			kStateVT100TBC				= kStateSeenESCLeftSqBracketParamsg	//!< tabulation clear
		};
	} // namespace My_Parser
	
	UInt16 const					kNumberOfScrollbackRowsToAllocateAtOnce = 100;
	TerminalTextAttributes const	kTerminalTextAttributesAllOff = 0L;
	
	char const		kMy_TabSet		= 'x';	//!< in "tabSettings" field of terminal structure, characters with this value mark tab stops
	char const		kMy_TabClear		= ' ';	//!< in "tabSettings" field of terminal structure, all characters not marking tab stops have this value
	UInt8 const		kMy_TabStop		= 8;	//!< number of characters between normal tab stops
	
	enum
	{
		kMy_NumberOfCharactersPerLineMaximum = 256	//!< maximum number of columns allowed; must be a multiple of "kMy_TabStop"
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
	
	enum
	{
		kMy_MaximumANSIParameters = 16		// when <ESC>'['<param>[;<param>][;<param>]... sequences are encountered,
											// this will be the maximum <param>s allowed
	};
}

#pragma mark Callbacks

struct My_ScreenBuffer;						// declared here because the callback declarations use it (defined later)
typedef std::string		My_TextVector;		// defined here because the callback declarations use it

/*!
Emulator State Determinant Routine

Specifies the next state for the parser, based on the given
data and/or current state.  The number of characters read
is returned; this is generally 1, but may be zero or more
if you used look-ahead to set a more precise next state or
do not wish to consume any characters.

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
state transition, but then restore "inCurrentState" and
seek another next state (with a buffer pointer that has
since advanced).

You do not need to set a new state; the default is the
current state.  However, for clarity it is recommended
that you always set the state precisely on each call.
*/
typedef UInt32 (*My_EmulatorStateDeterminantProcPtr)	(My_ScreenBuffer*		inDataPtr,
														 UInt8 const*			inBuffer,
														 UInt32					inLength,
														 My_Parser::State		inCurrentState,
														 My_Parser::State&		outNewState,
														 Boolean&				outInterrupt);
static inline UInt32
invokeEmulatorStateDeterminantProc	(My_EmulatorStateDeterminantProcPtr		inProc,
									 My_ScreenBuffer*						inDataPtr,
									 UInt8 const*							inBuffer,
									 UInt32									inLength,
									 My_Parser::State						inCurrentState,
									 My_Parser::State&						outNewState,
									 Boolean&								outInterrupt)
{
	return (*inProc)(inDataPtr, inBuffer, inLength, inCurrentState, outNewState, outInterrupt);
}

/*!
Emulator State Transition Routine

Unlike a "My_EmulatorStateDeterminantProcPtr", which determines
what the next state should be, this routine *performs an
action* based on the *current* state.  Some context is given
(like the previous state), in case this matters to you.

The number of characters read is returned; this is generally
0, but may be more if you use look-ahead to absorb data (e.g.
the echo state).
*/
typedef UInt32 (*My_EmulatorStateTransitionProcPtr)	(My_ScreenBuffer*		inDataPtr,
													 UInt8 const*			inBuffer,
													 UInt32					inLength,
													 My_Parser::State		inNewState,
													 My_Parser::State		inPreviousState);
static inline UInt32
invokeEmulatorStateTransitionProc	(My_EmulatorStateTransitionProcPtr	inProc,
									 My_ScreenBuffer*					inDataPtr,
									 UInt8 const*						inBuffer,
									 UInt32								inLength,
									 My_Parser::State					inNewState,
									 My_Parser::State					inPreviousState)
{
	return (*inProc)(inDataPtr, inBuffer, inLength, inNewState, inPreviousState);
}

/*!
Screen Line Operation Routine

This defines a function that can be used as an iterator
over all of the rows of a virtual screen buffer (either
the main screen or the scrollback).  The specified text
buffer (which is read/write) includes the contents of
the current row.

The row number passed into the routine is relative to the
iteration currently being done; for example, if someone
iterates over lines 5 to 8 of a buffer, the line number
is 0 when starting at line 5, and maximizes at 3 when
line 8 is hit.

If your operation modifies the buffer (which is allowed),
return "true" to notify the virtual screen module that
you changed the text.  Otherwise, return "false" if you
leave the text alone.
*/
typedef void (*My_ScreenLineOperationProcPtr)	(My_ScreenBuffer*		inScreen,
												 My_TextVector const&	inLineTextBuffer,
												 UInt16					inZeroBasedRowNumberOrNegativeForScrollbackRow,
												 void*					inContextPtr);
static inline void
invokeScreenLineOperationProc	(My_ScreenLineOperationProcPtr	inUserRoutine,
								 My_ScreenBuffer*				inScreen,
								 My_TextVector const&			inLineTextBuffer,
								 UInt16							inZeroBasedRowNumberRelativeToStartOfIterationRange,
								 void*							inContextPtr)
{
	(*inUserRoutine)(inScreen, inLineTextBuffer, inZeroBasedRowNumberRelativeToStartOfIterationRange, inContextPtr);
}

#pragma mark Types

typedef std::vector< char >						My_TabStopList;
typedef std::vector< TerminalTextAttributes >   My_TextAttributesList;

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
Represents a single line of the screen buffer of a
terminal, as well as attributes of its contents
(special styles, colors, highlighting, double-sized
text, etc.).  Since text and attributes are allocated
many lines at a time, a given line may or may not
point to the start of an allocated block, so flags
are also present to indicate which pointers should be
used to dispose memory blocks later.

The line list is an std::list (as opposed to some other
standard container like a vector or deque) because it
requires quick access to the start, end and middle, and
it requires iterators that remain valid after lines are
mucked with.  It also makes use of the splice() method
which is specific to a linked list.

NOTE:	Traditionally NCSA Telnet has used bits to
		represent the style of every single terminal
		cell.  This is memory-inefficient (albeit
		convenient at times), and also worsens linearly
		as the size of the screen increases.  It may be
		nice to implement a “style run”-based approach
		that sets attributes for ranges of text (which
		is pretty much how they’re defined anyway, when
		VT sequences arrive).  That would greatly
		reduce the number of attribute words in memory!
		The first part of this is implemented, in the
		sense that Terminal Views only see terminal data
		in terms of style runs (see the new routine
		Terminal_ForEachLikeAttributeRunDo()).
*/
struct My_ScreenBufferLine
{
	My_TextVector				textVector;			//!< where characters exist
	My_TextAttributesList		attributeVector;	//!< where character attributes exist
	TerminalTextAttributes		globalAttributes;   //!< attributes that apply to every character (e.g. double-sized text)
	
	My_ScreenBufferLine ()
	:
	textVector(kMy_NumberOfCharactersPerLineMaximum, ' '),
	attributeVector(kMy_NumberOfCharactersPerLineMaximum),
	globalAttributes(kTerminalTextAttributesAllOff)
	{
		structureInitialize();
	}
	
	bool
	operator == (My_ScreenBufferLine const&  inLine)
	const
	{
		return (&inLine == this);
	}
	
	void
	structureInitialize ()
	{
		std::fill(textVector.begin(), textVector.end(), ' ');
		std::fill(attributeVector.begin(), attributeVector.end(), kTerminalTextAttributesAllOff);
		globalAttributes = kTerminalTextAttributesAllOff;
	}
};
typedef std::list< My_ScreenBufferLine >		My_ScreenBufferLineList;
typedef My_ScreenBufferLineList::size_type		My_ScreenRowIndex;

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
	: firstColumn(inFirstColumn), firstRow(inFirstRow), lastColumn(inLastColumn), lastRow(inLastRow)
	{
	}
	
	UInt16				firstColumn;	//!< zero-based column number where range begins
	My_ScreenRowIndex	firstRow;		//!< zero-based row number where range occurs
	UInt16				lastColumn;		//!< zero-based column number of the final column included in the range
	My_ScreenRowIndex	lastRow;		//!< zero-based row number of the final row included in the range
};

/*!
Represents a line of the terminal buffer, either in the
scrollback or one of the main screen (“visible”) lines.
*/
struct My_LineIterator
{
	My_ScreenBufferLineList&				sourceList;				//!< the list that this iterator is pointing into
	My_ScreenBufferLineList::iterator		rowIterator;			//!< points into one of the buffer queues
	My_ScreenBufferLineList::size_type		distanceToBeginning;	//!< helps guard against moving iterators too far back
	My_ScreenBufferLineList::size_type		distanceToEnd;			//!< helps guard against moving iterators too far forward
	
	My_LineIterator		(My_ScreenBufferLineList&				inSourceList,
						 My_ScreenBufferLineList::iterator		inRowIterator)
	:
	sourceList(inSourceList),
	rowIterator(inRowIterator),
	distanceToBeginning(std::distance(inSourceList.begin(), inRowIterator)),
	distanceToEnd(std::distance(inRowIterator, inSourceList.end()))
	{
	}
};
typedef My_LineIterator*	My_LineIteratorPtr;

struct My_ScreenBuffer
{
public:
	My_ScreenBuffer	(SInt16, SInt16, SInt16, Boolean);
	~My_ScreenBuffer ();
	
	SessionRef							listeningSession;			//!< may be nullptr; the currently attached session, where certain terminal reports are sent
	
	TerminalSpeaker_Ref					speaker;					//!< object that emits sound based on this terminal data;
																	//!  TEMPORARY: the speaker REALLY shouldn’t be part of the terminal data model!
	CFRetainRelease						windowTitleCFString;		//!< stores the string that the terminal considers its window title
	CFRetainRelease						iconTitleCFString;			//!< stores the string that the terminal considers its icon title
	
	ListenerModel_Ref					changeListenerModel;		//!< registry of listeners for various terminal events
	Terminal_Emulator					emulation;					//!< VT100, VT220, etc.
	My_EmulatorStateDeterminantProcPtr	stateDeterminant;			//!< figures out what the next state should be
	My_EmulatorStateTransitionProcPtr	transitionHandler;			//!< handles new parser states, driving the terminal; varies based on the emulator
	
	/*!
	IMPORTANT:  It may be useful (and implemented so as) to retain iterators.
				However, lines can move between the two buffers, which screws
				up the iterators, as they assume screen lines; and lines can
				be deleted.  The insertNewLines() routine is the ONLY place
				these manipulations are ever performed, so that it is easy to
				check and resynchronize retained iterators at the same time.
				TO MAKE LIFE EASY, DON’T MESS WITH THE BUFFER LISTS ELSEWHERE.
	*/
	My_ScreenBufferLineList				scrollbackBuffer;			//!< a double-ended queue containing all the scrollback text for the terminal;
																	//!  IMPORTANT: the FRONT of this queue is the scrollback line CLOSEST to the
																	//!  top (FRONT) of the screen buffer line queue; imagine both queues starting
																	//!  at the home line and growing outwards from one another
	My_ScreenBufferLineList				screenBuffer;				//!< a double-ended queue containing all the visible text for the terminal;
																	//!  insertion or deletion from either end is fast, other operations are slow;
																	//!  NOTE you should ONLY modify this using insertNewLines()!
	
	My_TabStopList						tabSettings;				//!< array of characters representing tab stops; values are either kMy_TabClear
																	//!  (for most columns), or kMy_TabSet at tab columns
	
	SInt16								captureFileRefNum;			//!< file reference number of opened capture file
	
	Boolean								bellDisabled;				//!< if true, all bell signals are completely ignored (no audio or visual)
	Boolean								cursorVisible;				//!< if true, cursor state is visible (as opposed to invisible)
	Boolean								reverseVideo;				//!< if true, foreground and background colors are swapped when rendering
	Boolean								windowMinimized;			//!< if true, the window has been *flagged* as being iconified; since this is only
																	//!  a data model, this only means that a terminal sequence has told the window to
																	//!  become iconified; synchronizing this flag with the actual window state is the
																	//!  responsibility of the Terminal Window module
	
	My_CharacterSetInfo					vtG0;						//!< the G0 character set
	My_CharacterSetInfo					vtG1;						//!< the G1 character set
	
	My_RowColumnBoundary				visibleBoundary;			//!< rows and columns forming the perimeter of the buffer’s “visible” region
	My_RowBoundary						scrollingRegion;			//!< rows that define the scrolling area, inclusive; should not fall outside of
																	//!  the visible boundary;
																	//!  WARNING: do not change this except with a setScrollingRegion...() routine
	
	struct
	{
		struct
		{
			My_ScreenRowIndex	numberOfRowsAllocated;  //!< how many lines of scrollback are currently taking up memory
			My_ScreenRowIndex	numberOfRowsPermitted;  //!< maximum lines of scrollback specified by the user
														//!  (NOTE: this is inflexible; cooler scrollback handling schemes are limited by it...)
			Boolean				enabled;				//!< true if scrolled lines should be appended to the buffer automatically
		} scrollback;
		
		struct
		{
			My_ScreenRowIndex	numberOfRowsAllocated;		//!< how many lines of non-scrollback are currently taking up memory; it follows
															//!  that this is the maximum possible value for "numberOfRowsPermitted"
			My_ScreenRowIndex	numberOfRowsPermitted;		//!< maximum lines of non-scrollback specified by the user
			UInt16				numberOfColumnsAllocated;	//!< how many characters per line are currently taking up memory; it follows
															//!  that this is the maximum possible value for "numberOfColumnsPermitted"
			UInt16				numberOfColumnsPermitted;	//!< maximum columns per line specified by the user
		} visibleScreen;
	} text;
	
	My_LEDBits							litLEDs;					//!< highlighted states of terminal LEDs (lights)
	
	Boolean								mayNeedToSaveToScrollback;	//!< if true, the cursor has moved to the home position, and therefore
																	//!  a subsequent attempt to erase to the end of the line or screen
																	//!  should cause the data to be copied to the scrollback first (that is,
																	//!  of course, if the force-save flag is also set)
	Boolean								saveToScrollbackOnClear;	//!< if true, the entire screen contents are saved to the scrollback
																	//!  buffer just before being explicitly erased with a clear command
	
	Boolean								reportOnlyOnRequest;		//!< DECREQTPARM mode: determines response format and timing
	
	Boolean								modeANSIEnabled;			//!< DECANM mode: true only if in ANSI mode (as opposed to VT52 compatibility
																	//!  mode); if the former, parameters are only recognized following the CSI
																	//!  sequence, but if the latter, ESC-character sequences are allowed instead
	Boolean								modeApplicationKeys;		//!< DECKPAM mode: true only if the keypad is in application mode
	Boolean								modeAutoWrap;				//!< DECAWM mode: true only if line wrapping is automatic
	Boolean								modeCursorKeys;				//!< DECCKM mode: true only if the keypad should act as cursor movement arrows
																	//!  (note also that the VT100 manual states this setting has no effect unless
																	//!  the terminal is also in ANSI mode and the keypad is in application mode;
																	//!  see Terminal_KeypadHasMovementKeys())
	Boolean								modeInsertNotReplace;		//!< IRM mode: if true, new characters overwrite the cursor position, otherwise
																	//!  they first shift existing text over by one before appearing
	Boolean								modeNewLineOption;			//!< LNM mode: if true, a line feed causes the cursor to move to the start of
																	//!  the next line, and a Return generates a CR LF sequence; if false, the
																	//!  line feed is just vertical movement of the cursor, and Return is just CR
	Boolean								modeOriginRedefined;		//!< DECOM mode: true only if the origin has been moved from its default place
																	//!  (in which case, various subsequent terminal operations must be relative to
																	//!  the origin instead of the home position)
	
    TerminalPrintingInfo				printing;					//!< data used when spooling data to a printer (via temporary file)
	
	struct
	{
		Terminal_SpeechMode		mode;			//!< restrictions on when the computer may speak
		Str255					buffer;			//!< place to hold text that has not yet been spoken
	} speech;
	
	SInt16		currentEscapeSeqParamEndIndex;							//!< zero-based last parameter position in the "values" array
	SInt16		currentEscapeSeqParamValues[kMy_MaximumANSIParameters];	//!< all values provided for the current escape sequence
	
	struct
	{
		TerminalTextAttributes			attributeBits;			//!< text attributes for the line the cursor is on; these should be updated as the cursor
																//!  moves, or as terminal sequences are processed by the emulator
		SInt16							cursorX;				//!< column number of current cursor position;
																//!  WARNING: do not change this value except with a moveCursor...() routine
		My_ScreenRowIndex				cursorY;				//!< row number of current cursor position;
																//!  WARNING: do not change this value except with a moveCursor...() routine
		My_CharacterSetInfoPtr			characterSetInfoPtr;	//!< pointer to either G0 or G1 character rules, whichever is active
	} current;
	
	struct
	{
		TerminalTextAttributes		attributeBits;	//!< previous bits of corresponding bits in "current" structure
		SInt16						cursorX;		//!< previous value of corresponding value in "current" structure
		SInt16						cursorY;		//!< previous value of corresponding value in "current" structure
	} previous;
	
	My_Parser::State		currentState;				//!< state the terminal input parser is in now
	UInt16					parserStateRepetitions;		//!< to guard against looping; counts repetitions of same state
	
	TerminalScreenRef		selfRef;					//!< opaque reference that would resolve to a pointer to this structure
};
typedef My_ScreenBuffer*			My_ScreenBufferPtr;
typedef My_ScreenBuffer const*		My_ScreenBufferConstPtr;

#pragma mark Internal Method Prototypes

static void						addScreenLineSize						(My_ScreenBufferPtr, My_TextVector const&, UInt16, void*);
static void						appendScreenLineToHandle				(My_ScreenBufferPtr, My_TextVector const&, UInt16, void*);
static void						bufferEraseFromCursorColumnToLineEnd	(My_ScreenBufferPtr);
static void						bufferEraseFromCursorToEnd				(My_ScreenBufferPtr);
static void						bufferEraseFromHomeToCursor				(My_ScreenBufferPtr);
static void						bufferEraseFromLineBeginToCursorColumn  (My_ScreenBufferPtr);
static void						bufferEraseLineWithoutUpdate			(My_ScreenBufferPtr, My_ScreenBufferLine&);
static void						bufferEraseLineWithUpdate				(My_ScreenBufferPtr, SInt16);
static void						bufferEraseVisibleScreenWithUpdate		(My_ScreenBufferPtr);
static void						bufferEraseWithoutUpdate				(My_ScreenBufferPtr);
static void						bufferInsertBlanksAtCursorColumn		(My_ScreenBufferPtr, SInt16);
static void						bufferInsertBlankLines					(My_ScreenBufferPtr, UInt16,
																		 My_ScreenBufferLineList::iterator&);
static void						bufferLineFill							(My_ScreenBufferPtr, My_ScreenBufferLine&, UInt8,
																		 TerminalTextAttributes =
																			kTerminalTextAttributesAllOff,
																		 Boolean = true);
static void						bufferRemoveCharactersAtCursorColumn	(My_ScreenBufferPtr, SInt16);
static void						bufferRemoveLines						(My_ScreenBufferPtr, UInt16,
																		 My_ScreenBufferLineList::iterator&);
static void						changeLineAttributes					(My_ScreenBufferPtr, My_ScreenBufferLine&,
																		 TerminalTextAttributes, TerminalTextAttributes);
static void						changeLineGlobalAttributes				(My_ScreenBufferPtr, My_ScreenBufferLine&,
																		 TerminalTextAttributes, TerminalTextAttributes);
static void						changeLineRangeAttributes				(My_ScreenBufferPtr, My_ScreenBufferLine&, UInt16,
																		 SInt16, TerminalTextAttributes,
																		 TerminalTextAttributes);
static void						changeNotifyForTerminal					(My_ScreenBufferConstPtr, Terminal_Change, void*);
static void						clearEscapeSequenceParameters			(My_ScreenBufferPtr);
static void						cursorRestore							(My_ScreenBufferPtr);
static void						cursorSave								(My_ScreenBufferPtr);
static void						cursorWrapIfNecessaryGetLocation		(My_ScreenBufferPtr, SInt16*, My_ScreenRowIndex*);
static void						echoData								(My_ScreenBufferPtr, UInt8 const*, UInt32);
static UInt32					emulatorANSIBBSStateDeterminant			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State&, Boolean&);
static UInt32					emulatorANSIBBSStateTransition			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State);
static void						emulatorFrontEndOld						(My_ScreenBufferPtr, UInt8 const*, SInt32);
static UInt32					emulatorStandardStateDeterminant		(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State&, Boolean&);
static UInt32					emulatorStandardStateTransition			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State);
static UInt32					emulatorVT100StateDeterminant			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State&, Boolean&);
static UInt32					emulatorVT100StateTransition			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State);
static UInt32					emulatorVT100VT52StateDeterminant		(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State&, Boolean&);
static UInt32					emulatorVT100VT52StateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State);
static UInt32					emulatorVT220StateDeterminant			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State&, Boolean&);
static UInt32					emulatorVT220StateTransition			(My_ScreenBufferPtr, UInt8 const*, UInt32,
																		 My_Parser::State, My_Parser::State);
static void						eraseRightHalfOfLine					(My_ScreenBufferPtr, My_ScreenBufferLine&);
static Terminal_ResultCode		forEachLineDo							(TerminalScreenRef, Terminal_LineRef, UInt16,
																		 My_ScreenLineOperationProcPtr, void*);
static inline My_LineIteratorPtr	getLineIterator						(Terminal_LineRef);
static My_ScreenBufferPtr		getVirtualScreenData					(TerminalScreenRef);
static void						highlightLED							(My_ScreenBufferPtr, SInt16);
static void						initializeParserStateStack				(My_ScreenBufferPtr);
static Boolean					insertNewLines							(My_ScreenBufferPtr, My_ScreenBufferLineList::size_type,
																		 Boolean);
static void						locateCursorLine						(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&);
static void						locateScrollingRegion					(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&,
																		 My_ScreenBufferLineList::iterator&);
static void						locateScrollingRegionTop				(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&);
static Boolean					matchingText							(Ptr, char const*, UInt32, Boolean);
static void						moveCursor								(My_ScreenBufferPtr, SInt16, My_ScreenRowIndex);
static void						moveCursorDown							(My_ScreenBufferPtr);
static void						moveCursorDownOrScroll					(My_ScreenBufferPtr);
static void						moveCursorDownOrStop					(My_ScreenBufferPtr);
static void						moveCursorDownToEdge					(My_ScreenBufferPtr);
static void						moveCursorLeft							(My_ScreenBufferPtr);
static void						moveCursorLeftToEdge					(My_ScreenBufferPtr);
static void						moveCursorLeftToHalf					(My_ScreenBufferPtr);
static void						moveCursorRight							(My_ScreenBufferPtr);
static void						moveCursorRightToEdge					(My_ScreenBufferPtr);
static void						moveCursorRightToNextTabStop			(My_ScreenBufferPtr);
static void						moveCursorUpToEdge						(My_ScreenBufferPtr);
static void						moveCursorUp							(My_ScreenBufferPtr);
static void						moveCursorUpOrScroll					(My_ScreenBufferPtr);
static void						moveCursorX								(My_ScreenBufferPtr, SInt16);
static void						moveCursorY								(My_ScreenBufferPtr, My_ScreenRowIndex);
static void						resetTerminal							(My_ScreenBufferPtr);
static SessionRef				returnListeningSession					(My_ScreenBufferPtr);
static Terminal_EmulatorType	returnTerminalType						(Terminal_Emulator);
static Terminal_EmulatorVariant	returnTerminalVariant					(Terminal_Emulator);
static void						saveToScrollback						(My_ScreenBufferPtr);
static void						scrollTerminalBuffer					(My_ScreenBufferPtr);
static Boolean					searchBufferForPhrase					(Handle, long, char const*, UInt32, Boolean, Boolean,
																		 Boolean, long*, long*);
static void						setCursorVisible						(My_ScreenBufferPtr, Boolean);
static void						setScrollingRegionBottom				(My_ScreenBufferPtr, UInt16);
static void						setScrollingRegionTop					(My_ScreenBufferPtr, UInt16);
// IMPORTANT: Attribute bit manipulation is fully described in "TerminalTextAttributes.typedef.h".
//            Changes must be kept consistent everywhere.  See below, for usage.
inline TerminalTextAttributes	styleOfVTParameter						(UInt8	inPs)
{
	return (1 << (inPs - 1));
}
static void						tabStopClearAll							(My_ScreenBufferPtr);
static UInt16					tabStopGetDistanceFromCursor			(My_ScreenBufferConstPtr);
static void						tabStopInitialize						(My_ScreenBufferPtr);
static char						translateCharacter						(My_ScreenBufferPtr, char);
static void						vt100AlignmentDisplay					(My_ScreenBufferPtr);
static void						vt100ANSIMode							(My_ScreenBufferPtr);
static void						vt100CursorBackward						(My_ScreenBufferPtr);
static void						vt100CursorBackward_vt52				(My_ScreenBufferPtr);
static void						vt100CursorDown							(My_ScreenBufferPtr);
static void						vt100CursorDown_vt52					(My_ScreenBufferPtr);
static void						vt100CursorForward						(My_ScreenBufferPtr);
static void						vt100CursorForward_vt52					(My_ScreenBufferPtr);
static void						vt100CursorUp							(My_ScreenBufferPtr);
static void						vt100CursorUp_vt52						(My_ScreenBufferPtr);
static void						vt100DeviceAttributes					(My_ScreenBufferPtr);
static void						vt100DeviceStatusReport					(My_ScreenBufferPtr);
static void						vt100EraseInDisplay						(My_ScreenBufferPtr);
static void						vt100EraseInLine						(My_ScreenBufferPtr);
static void						vt100Identify_vt52						(My_ScreenBufferPtr);
static void						vt100LoadLEDs							(My_ScreenBufferPtr);
static void						vt100ModeSetReset						(My_ScreenBufferPtr, Boolean);
static UInt32					vt100ReadCSIParameters					(My_ScreenBufferPtr, UInt8 const*, UInt32);
static void						vt100ReportTerminalParameters			(My_ScreenBufferPtr);
static void						vt100SetTopAndBottomMargins				(My_ScreenBufferPtr);
static void						vt100VT52Mode							(My_ScreenBufferPtr);
static void						vt220DeviceAttributes					(My_ScreenBufferPtr);
template < typename src_char_seq_const_iter, typename src_char_seq_size_t,
			typename dest_char_seq_iter, typename dest_char_seq_size_t >
static dest_char_seq_iter		whitespaceSensitiveCopy					(src_char_seq_const_iter, src_char_seq_size_t,
																		 dest_char_seq_iter, dest_char_seq_size_t,
																		 dest_char_seq_size_t*, src_char_seq_size_t);



#pragma mark Public Methods

/*!
Constructor.  See Terminal_NewScreen().

Throws a Terminal_ResultCode if any problems occur.

(3.0)
*/
My_ScreenBuffer::
My_ScreenBuffer	(SInt16		inLineCountScrollbackBuffer,
				 SInt16		inLineCountVisibleRows,
				 SInt16		inMaximumColumnCount,
				 Boolean	inForceLineSaving)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
listeningSession(nullptr),
speaker(nullptr),
windowTitleCFString(),
iconTitleCFString(),
changeListenerModel(ListenerModel_New(kListenerModel_StyleStandard, kConstantsRegistry_ListenerModelDescriptorTerminalChanges)),
emulation(kTerminal_EmulatorDumb),
stateDeterminant(/*tmp*/emulatorVT100StateDeterminant/*emulatorStandardStateDeterminant*/),
transitionHandler(/*tmp*/emulatorVT100StateTransition/*emulatorStandardStateTransition*/),
scrollbackBuffer(),
screenBuffer(),
tabSettings(),
captureFileRefNum(0),
bellDisabled(false),
cursorVisible(true),
reverseVideo(false),
windowMinimized(false),
vtG0(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOff),
vtG1(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOn),
visibleBoundary(0, 0, inMaximumColumnCount - 1, inLineCountVisibleRows - 1),
scrollingRegion(0, 0), // reset below...
// text elements - not initialized
litLEDs(kMy_LEDBitsAllOff),
mayNeedToSaveToScrollback(false),
saveToScrollbackOnClear(true),
reportOnlyOnRequest(false),
modeANSIEnabled(true),
modeApplicationKeys(false),
modeAutoWrap(false),
modeCursorKeys(false),
modeInsertNotReplace(false),
modeNewLineOption(false),
modeOriginRedefined(false),
printing(inMaximumColumnCount),
// speech elements - not initialized
// current elements - not initialized
// previous elements - not initialized
currentState(My_Parser::kStateInitial),
parserStateRepetitions(0),
selfRef(REINTERPRET_CAST(this, TerminalScreenRef))
// TEMPORARY: initialize other members here...
{
	initializeParserStateStack(this);
	
	this->text.visibleScreen.numberOfColumnsAllocated = Terminal_ReturnAllocatedColumnCount(); // always allocate max columns
	
	this->current.cursorX = 0; // initialized because moveCursor() depends on prior values...
	this->current.cursorY = 0; // initialized because moveCursor() depends on prior values...
	
	// now “append” the desired number of main screen lines, which will have
	// the effect of allocating a screen buffer of the right size
	unless (insertNewLines(this, inLineCountVisibleRows, true/* append only */))
	{
		throw kTerminal_ResultCodeNotEnoughMemory;
	}
	assert(!this->screenBuffer.empty());
	
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
		throw kTerminal_ResultCodeNotEnoughMemory;
	}
	
	this->current.characterSetInfoPtr = &this->vtG0; // by definition, G0 is active initially
	this->text.scrollback.numberOfRowsPermitted = inLineCountScrollbackBuffer;
	this->text.scrollback.enabled = ((inForceLineSaving) || (inLineCountScrollbackBuffer > 0));
	this->text.visibleScreen.numberOfColumnsPermitted = inMaximumColumnCount;
	this->current.attributeBits = 0;
	this->previous.attributeBits = kInvalidTerminalTextAttributes; /* initially no saved attribute */
	this->currentEscapeSeqParamEndIndex = 0;
	
	// speech setup
	this->speech.mode = kTerminal_SpeechModeSpeakAlways;
	this->speech.buffer[0] = '\0'; // initially empty
	
	// IMPORTANT: Within constructors, calls to routines expecting a *self reference* should be
	//            *last*; otherwise, there’s no telling whether or not the data that the routine
	//            requires will have been properly initialized yet.
	
	moveCursor(this, 0, 0);
	
	this->scrollingRegion.firstRow = 0; // initialized because setScrollingRegionTop() depends on prior values...
	setScrollingRegionTop(this, 0);
	this->scrollingRegion.lastRow = inLineCountVisibleRows - 1; // initialized because setScrollingRegionBottom() depends on prior values...
	setScrollingRegionBottom(this, inLineCountVisibleRows - 1);
	
	this->speaker = TerminalSpeaker_New(REINTERPRET_CAST(this, TerminalScreenRef));
}// My_ScreenBuffer 4-argument constructor


/*!
Destructor.  See Terminal_DisposeScreen().

(1.1)
*/
My_ScreenBuffer::
~My_ScreenBuffer ()
{
	TerminalSpeaker_Dispose(&this->speaker);
	ListenerModel_Dispose(&this->changeListenerModel);
}// My_ScreenBuffer destructor


/*!
Creates a new terminal screen and initial view according
to the given specifications.

MacTelnet 3.0 is designed so that a terminal screen buffer
can have multiple views, or vice-versa (a view can display
the contents of multiple buffers).  Therefore, although a
single buffer and view is created by this routine, more
could be constructed later.

\retval kTerminal_ResultCodeSuccess
if the screen was created successfully

\retval kTerminal_ResultCodeParameterError
if any of the given storage pointers are nullptr

\retval kTerminal_ResultCodeNotEnoughMemory
if there is a serious problem creating the screen

(3.0)
*/
Terminal_ResultCode
Terminal_NewScreen	(SInt16					inLineCountScrollbackBuffer,
					 SInt16					inLineCountVisibleRows,
					 SInt16					inMaximumColumnCount,
					 Boolean				inForceLineSaving,
					 TerminalScreenRef*		outScreenPtr)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	
	
	if (outScreenPtr == nullptr) result = kTerminal_ResultCodeParameterError;
	else
	{
		try
		{
			*outScreenPtr = REINTERPRET_CAST(new My_ScreenBuffer(inLineCountScrollbackBuffer, inLineCountVisibleRows,
																	inMaximumColumnCount, inForceLineSaving),
												TerminalScreenRef);
		}
		catch (std::bad_alloc)
		{
			*outScreenPtr = nullptr;
		}
		catch (Terminal_ResultCode	inConstructionError)
		{
			result = inConstructionError;
		}
	}
	return result;
}// NewScreen


/*!
Disposes of a screen previously created with
Terminal_NewScreen().  On return, the specified
screen ID is invalid.

For legacy reasons, returns nonzero if there is
an error while disposing of the screen.  In
reality, this only happens if you pass in a
nullptr reference.

(2.6)
*/
SInt16
Terminal_DisposeScreen	(TerminalScreenRef		inRef)
{
	SInt16		result = 0;
	
	
	if (inRef == nullptr) result = -3/* legacy return value from NCSA Telnet; can probably ditch this eventually */;
	else
	{
		My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
		
		
		// dispose screen lines - UNIMPLEMENTED!!!
		delete ptr;
	}
	return result;
}// DisposeScreen


/*!
Creates a special reference to the given line of the given
terminal screen’s VISIBLE SCREEN buffer, or returns nullptr
if the line is out of range or the iterator cannot be
created for some other reason.

Pass 0 to indicate you want the very topmost line (that
is, the one that would be added to the scrollback buffer
first if the screen scrolls), or larger values to ask for
lines below it.  Currently, this routine takes linearly
more time to figure out where the bottommost lines are.

A main line iterator cannot point to a scrollback screen
line; use Terminal_NewScrollbackLineIterator() to locate
scrollback lines.

IMPORTANT:	An iterator is completely invalid once the
			screen it was created for has been destroyed.

(3.0)
*/
Terminal_LineRef
Terminal_NewMainScreenLineIterator	(TerminalScreenRef		inRef,
									 UInt16					inLineNumberZeroForTop)
{
	Terminal_LineRef		result = nullptr;
	My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
	
	
	if (ptr != nullptr)
	{
		// ensure the specified row is in range
		if (inLineNumberZeroForTop < ptr->screenBuffer.size())
		{
			try
			{
				My_ScreenBufferLineList::iterator	startIterator = ptr->screenBuffer.begin();
				My_LineIteratorPtr					iteratorPtr = nullptr;
				
				
				std::advance(startIterator, STATIC_CAST(inLineNumberZeroForTop, My_ScreenBufferLineList::difference_type));
				iteratorPtr = new My_LineIterator(ptr->screenBuffer, startIterator);
				if (iteratorPtr != nullptr)
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
	return result;
}// NewMainScreenLineIterator


/*!
Creates a special reference to the given line of the
given terminal screen’s SCROLLBACK buffer, or returns
nullptr if the line is out of range or the iterator
cannot be created for some other reason.

Pass 0 to indicate you want the very newest line (that
is, the one that most recently scrolled off the top of
the main screen), or larger values to ask for older
lines.  Currently, this routine takes linearly more time
to figure out where old scrollback lines are.

A scrollback iterator cannot point to a main screen
line; use Terminal_NewMainLineIterator() to locate main
screen lines.

IMPORTANT:	An iterator is completely invalid once the
			screen it was created for has been destroyed.
			It can also be invalidated in other ways,
			e.g. a call to Terminal_DeleteAllSavedLines().

(3.0)
*/
Terminal_LineRef
Terminal_NewScrollbackLineIterator	(TerminalScreenRef	inRef,
									 UInt16				inLineNumberZeroForNewest)
{
	Terminal_LineRef		result = nullptr;
	My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
	
	
	if (ptr != nullptr)
	{
		// ensure the specified row is in range
		if (inLineNumberZeroForNewest < ptr->scrollbackBuffer.size())
		{
			try
			{
				My_ScreenBufferLineList::iterator	startIterator = ptr->scrollbackBuffer.begin();
				My_LineIteratorPtr					iteratorPtr = nullptr;
				
				
				std::advance(startIterator, STATIC_CAST(inLineNumberZeroForNewest, My_ScreenBufferLineList::difference_type));
				iteratorPtr = new My_LineIterator(ptr->scrollbackBuffer, startIterator);
				if (iteratorPtr != nullptr)
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
	return result;
}// NewScrollbackLineIterator


/*!
Destroys an iterator created with Terminal_NewLineIterator(),
and sets your copy of the reference to nullptr.

(3.0)
*/
void
Terminal_DisposeLineIterator	(Terminal_LineRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		My_LineIteratorPtr		ptr = getLineIterator(*inoutRefPtr);
		
		
		if (ptr != nullptr)
		{
			delete ptr;
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

\retval kTerminal_ResultCodeSuccess
if the line attributes were changed successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_ResultCode
Terminal_ChangeLineAttributes	(TerminalScreenRef			inRef,
								 Terminal_LineRef			inRow,
								 TerminalTextAttributes		inSetTheseAttributes,
								 TerminalTextAttributes		inClearTheseAttributes)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultCodeInvalidIterator;
	else
	{
		changeLineAttributes(dataPtr, *(iteratorPtr->rowIterator), inSetTheseAttributes, inClearTheseAttributes);
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

\retval kTerminal_ResultCodeSuccess
if the line range attributes were changed successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_ResultCode
Terminal_ChangeLineRangeAttributes	(TerminalScreenRef			inRef,
									 Terminal_LineRef			inRow,
									 UInt16						inZeroBasedStartColumn,
									 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
									 TerminalTextAttributes		inSetTheseAttributes,
									 TerminalTextAttributes		inClearTheseAttributes)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultCodeInvalidIterator;
	else
	{
		changeLineRangeAttributes(dataPtr, *(iteratorPtr->rowIterator), inZeroBasedStartColumn,
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

The row values may be negative, signifying that they
apply to scrollback buffer rows.

If "inConstrainToRectangle" is true, the range is
considered to be exact so no columns outside the range
are filled in (normally, all lines in between the start
and end points are filled completely, from column 0 to
the last column).

You can use this to apply effects to a large number of
characters at once (e.g. when highlighting).

\retval kTerminal_ResultCodeSuccess
if the range attributes were changed successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_ResultCode
Terminal_ChangeRangeAttributes	(TerminalScreenRef			inRef,
								 Terminal_LineRef			inStartRow,
								 UInt16						inNumberOfRowsToConsider,
								 UInt16						inZeroBasedStartColumn,
								 UInt16						inZeroBasedPastTheEndColumn,
								 Boolean					inConstrainToRectangle,
								 TerminalTextAttributes		inSetTheseAttributes,
								 TerminalTextAttributes		inClearTheseAttributes)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
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
	if (inZeroBasedPastTheEndColumn < 1)
	{
		inZeroBasedPastTheEndColumn = 1;
	}
	
	if (inNumberOfRowsToConsider > 0)
	{
		// now apply changes to attributes of all specified cells, and to
		// the “current” attributes (if the cursor is anywhere in the range)
		if (nullptr == dataPtr) result = kTerminal_ResultCodeInvalidID;
		else if (nullptr == iteratorPtr) result = kTerminal_ResultCodeInvalidIterator;
		else
		{
			Boolean const						kSingleLineRange = (1 == inNumberOfRowsToConsider);
			My_ScreenBufferLineList::iterator	currentLine = iteratorPtr->rowIterator;
			
			
			// figure out how to treat the first line
			if (kSingleLineRange)
			{
				// same line - iterate only over certain columns
				changeLineRangeAttributes(dataPtr, *currentLine, inZeroBasedStartColumn, inZeroBasedPastTheEndColumn,
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
				
				
				// fill in first line
				changeLineRangeAttributes(dataPtr, *currentLine, inZeroBasedStartColumn, linePastTheEndColumn,
											inSetTheseAttributes, inClearTheseAttributes);
				++currentLine;
				
				if (iteratorPtr->sourceList.end() != currentLine)
				{
					// fill in remaining lines; the last line is special
					// because it will end “early” at the end anchor
					{
						register SInt16		i = 0;
						SInt16 const		kLineEnd = (inNumberOfRowsToConsider - 1);
						
						
						for (i = 0; i < kLineEnd; ++i, ++currentLine)
						{
							if (currentLine == iteratorPtr->sourceList.end())
							{
								Console_WriteLine("warning, exceeded row boundaries when changing attributes of a range");
								break;
							}
							
							// intermediate lines occupy the whole range; the last line
							// only occupies up to the final anchor point
							if (i < (kLineEnd - 1))
							{
								// fill in intermediate line
								changeLineRangeAttributes(dataPtr, *currentLine, lineStartColumn, linePastTheEndColumn,
															inSetTheseAttributes, inClearTheseAttributes);
							}
							else
							{
								// fill in last line
								changeLineRangeAttributes(dataPtr, *currentLine, lineStartColumn, inZeroBasedPastTheEndColumn,
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
Copies a portion of a single line of text not exceeding the
current visible width.  Trailing whitespace is skipped.

DEPRECATED.  Use Terminal_GetLineRangePossibleCopy().

\retval kTerminal_ResultCodeSuccess
if the data was copied successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultCodeParameterError
if the given line number is too large or the given column
numbers are out of range

\retval kTerminal_ResultCodeNotEnoughMemory
if the specified buffer is too small; a truncated version of
the text will be copied into the buffer

(3.0)
*/
Terminal_ResultCode
Terminal_CopyLineRange	(TerminalScreenRef		inScreen,
						 Terminal_LineRef		inRow,
						 UInt16					inZeroBasedStartColumn,
						 SInt16					inZeroBasedEndColumnOrNegativeForLastColumn,
						 char*					outBuffer,
						 SInt32					inBufferLength,
						 SInt32*				outActualLengthPtrOrNull,
						 UInt16					inNumberOfSpacesPerTabOrZeroForNoSubstitution)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultCodeInvalidIterator;
	else
	{
		UInt16		endColumn = (inZeroBasedEndColumnOrNegativeForLastColumn < 0)
									? iteratorPtr->rowIterator->textVector.size() - 1
									: inZeroBasedEndColumnOrNegativeForLastColumn;
		
		
		if (endColumn >= iteratorPtr->rowIterator->textVector.size())
		{
			result = kTerminal_ResultCodeParameterError;
		}
		else
		{
			My_TextVector::iterator		textStartIter;
			SInt32						copyLength = INTEGER_MINIMUM(inBufferLength, endColumn - inZeroBasedStartColumn + 1);
			
			
			textStartIter = iteratorPtr->rowIterator->textVector.begin();
			std::advance(textStartIter, inZeroBasedStartColumn);
			(char*)whitespaceSensitiveCopy(textStartIter, copyLength, outBuffer, copyLength,
											&copyLength/* returned actual length */,
											STATIC_CAST(inNumberOfSpacesPerTabOrZeroForNoSubstitution, SInt32));
			if (outActualLengthPtrOrNull != nullptr) *outActualLengthPtrOrNull = copyLength;
		}
	}
	
	return result;
}// CopyLineRange


/*!
Copies a portion of text from the specified screen into the
given buffer - excluding trailing whitespace on each line.

IMPORTANT:	This concept is under evaluation.  Probably,
			most interactions with terminal data are not
			necessary or, at a minimum, do not require
			copying data - in the future MacTelnet will
			attempt to avoid duplicating large text blocks.

The end-of-line sequence is inserted at line boundaries.
This is currently assumed to be exactly ONE character long.

NOTE:	You can set the first character of the sequence to
		0, to specify that no character should be used to
		separate lines.  You might do this if you want to
		copy multiple lines of terminal text that “really
		are one line”.

In MacTelnet 3.0, this method now copies text differently
if the coordinates are to define a rectangular selection.
That is, while normal Macintosh text selection includes all
text in the rows between the start and end anchors, a text
selection that is “rectangular” will only include text in
the in-between rows that are part of the columns between
the columns of the two anchor points.

\retval kTerminal_ResultCodeSuccess
if the data was copied successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultCodeNotEnoughMemory
if the specified buffer is too small; a truncated version
of the text will be copied into the buffer

(3.0)
*/
Terminal_ResultCode
Terminal_CopyRange	(TerminalScreenRef			inScreen,
					 Terminal_LineRef			inStartRow,
					 UInt16						inNumberOfRowsToConsider,
					 UInt16						inZeroBasedStartColumnOnFirstRow,
					 UInt16						inZeroBasedEndColumnOnLastRow,
					 char*						outBuffer,
					 SInt32						inBufferLength,
					 SInt32*					outActualLengthPtrOrNull,
					 char const*				inEndOfLineSequence,
					 SInt16						inNumberOfSpacesPerTabOrZeroForNoSubstitution,
					 Terminal_TextCopyFlags		inFlags)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultCodeInvalidIterator;
	else
	{
		char const* const	kOutBufferPastTheEndPtr = outBuffer + inBufferLength;
		SInt32				copiedLength = 0L;
		
		
		// copy text; the exact strategy depends on the range - if
		// a range is fairly small, it may be possible to copy the
		// data more efficiently
		if (1 == inNumberOfRowsToConsider)
		{
			// same line - very simple, just copy a few characters
			result = Terminal_CopyLineRange(inScreen, inStartRow, inZeroBasedStartColumnOnFirstRow, inZeroBasedEndColumnOnLastRow,
											outBuffer, inBufferLength, &copiedLength, inNumberOfSpacesPerTabOrZeroForNoSubstitution);
		}
		else
		{
			// there are multiple lines of text
			UInt16 const						kEOLLength = CPP_STD::strlen(inEndOfLineSequence);
			My_ScreenBufferLineList&			lineList = iteratorPtr->sourceList;
			My_ScreenBufferLineList::iterator	currentLine = iteratorPtr->rowIterator;
			UInt16								lineCount = 0;
			SInt16								currentX = 0;
			SInt16								actualLineLength = 0L;
			char*								destPtr = outBuffer;
			
			
			for (; ((lineCount < inNumberOfRowsToConsider) && (currentLine != lineList.end())); ++currentLine, ++lineCount)
			{
				char const* const	kDestLineStartPtr = destPtr; // pointer into destination buffer for start of current line
				char const*			destLinePastTheEndPtr = nullptr;
				SInt16				lineLength = 0;
				
				
				// set up the extreme values
				if ((inFlags & kTerminal_TextCopyFlagsRectangular) == 0)
				{
					// non-rectangular mode; the first and last lines are irregularly-shaped, but
					// the remainder is rectangular, flushed to the left and right screen edges
					assert(!lineList.empty());
					if (0 == lineCount)
					{
						// first row in range
						currentX = inZeroBasedStartColumnOnFirstRow;
						lineLength = dataPtr->text.visibleScreen.numberOfColumnsPermitted - inZeroBasedStartColumnOnFirstRow;
					}
					else if ((lineCount == (inNumberOfRowsToConsider - 1)) || (*currentLine == lineList.back()))
					{
						// last row in range
						currentX = 0;
						lineLength = inZeroBasedEndColumnOnLastRow + 1;
					}
					else
					{
						// middle row
						currentX = 0;
						lineLength = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
					}
				}
				else
				{
					// rectangular mode; all lines are uniform, and the left and right boundaries
					// exactly match those of the anchor points, not flushing to the screen edges
					currentX = inZeroBasedStartColumnOnFirstRow;
					lineLength = INTEGER_ABSOLUTE(inZeroBasedEndColumnOnLastRow - inZeroBasedStartColumnOnFirstRow) + 1;
				}
				destLinePastTheEndPtr = destPtr + lineLength;
				
				// constrain to boundaries of output buffer, in case there
				// really isn’t enough room for all the lines in the range
				if (destLinePastTheEndPtr > kOutBufferPastTheEndPtr)
				{
					destLinePastTheEndPtr = kOutBufferPastTheEndPtr;
				}
				
				// copy another line into the output buffer, replacing
				// spaces with tabs as needed, omitting trailing whitespace
				{
					My_TextVector::iterator		textStartIter;
					
					
					textStartIter = currentLine->textVector.begin();
					std::advance(textStartIter, currentX);
					destPtr = whitespaceSensitiveCopy(textStartIter, lineLength/* source length */,
														destPtr/* destination start */,
														STATIC_CAST(destLinePastTheEndPtr - kDestLineStartPtr, SInt16)/* destination length */,
														&actualLineLength,
														inNumberOfSpacesPerTabOrZeroForNoSubstitution/* number of spaces that equal one tab */);
				}
				
				// if necessary, add the end-of-line sequence; skip it for the last line, and always
				// skip it for rectangular selections; for regular selections, unless told otherwise
				// skip it for any line where the copy range includes the right margin and the right
				// margin character is non-whitespace (this is an XTerm-style way of allowing long
				// pathnames to join when they are broken at the right margin)
				assert(!lineList.empty());
				if ((kEOLLength) && (&*currentLine != &lineList.back()))
				{
					if ((inFlags & kTerminal_TextCopyFlagsAlwaysNewLineAtRightMargin)/* force new-line? */ ||
						(inFlags & kTerminal_TextCopyFlagsRectangular)/* rectangular selections always have new-lines */ ||
						((currentX + actualLineLength) < dataPtr->text.visibleScreen.numberOfColumnsPermitted/* not at edge? */) ||
						CPP_STD::isspace(*destPtr)/* new-line would always follow a line ending in spaces */)
					{
						*destPtr++ = *inEndOfLineSequence; // assumes it’s only one character!
					}
				}
			}
			copiedLength = (destPtr - outBuffer);
		}
		
		if (outActualLengthPtrOrNull != nullptr) *outActualLengthPtrOrNull = copiedLength;
	}
	return result;
}// CopyRange


/*!
Returns the title assigned to the iconified version of this
terminal.  In MacTelnet this is symbolic, as no assumption is
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
MacTelnet this is symbolic, as no assumption is made that a
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
Creates a data descriptor or object specifier that describes
the given range of text in the given terminal screen.

The ranges are zero-based, but can be negative; 0 is the
first line of the main screen area, and -1 is the first
scrollback line.  The oldest scrollback row is a larger
negative number, the bottommost line of the visible screen
is one less than the number of rows high that the terminal
screen is.

IMPORTANT:	Currently this performs text duplication, when in
			fact a smarter approach would probably be to “copy
			on write” and otherwise return a light-weight
			reference to the required data.

(3.0)
*/
OSStatus
Terminal_CreateContentsAEDesc	(TerminalScreenRef		inRef,
								 Terminal_LineRef		inStartRow,
								 UInt16					inNumberOfRowsToConsider,
								 AEDesc*				outDescPtr)
{
	OSStatus	result = noErr;
	Size		totalSize = 0L;
	
	
	if (forEachLineDo(inRef, inStartRow, inNumberOfRowsToConsider, addScreenLineSize, &totalSize) < 0)
	{
		result = memFullErr;
	}
	else
	{
		Handle		handle = nullptr;
		
		
		handle = Memory_NewHandle(totalSize);
		if (handle == nullptr) result = memFullErr;
		else
		{
			if (forEachLineDo(inRef, inStartRow, inNumberOfRowsToConsider, appendScreenLineToHandle, &handle) < 0)
			{
				// No data?  Out of memory!
				result = memFullErr;
			}
			else result = AECreateDesc(typeChar, (Ptr)*handle, GetHandleSize(handle), outDescPtr);
			Memory_DisposeHandle(&handle);
		}
	}
	
	return result;
}// CreateContentsAEDesc


/*!
Returns the zero-based column and line numbers of the
terminal cursor in the given buffer.  Since the cursor
is not allowed to enter the scrollback buffer, negative
return values are not possible.

Pass nullptr for one of the values if you do not want it.

\retval kTerminal_ResultCodeSuccess
if no error occurred

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

(3.0)
*/
Terminal_ResultCode
Terminal_CursorGetLocation	(TerminalScreenRef		inRef,
							 UInt16*				outZeroBasedColumnPtr,
							 UInt16*				outZeroBasedRowPtr)
{
	Terminal_ResultCode			result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else
	{
		if (outZeroBasedColumnPtr != nullptr) *outZeroBasedColumnPtr = dataPtr->current.cursorX;
		if (outZeroBasedRowPtr != nullptr) *outZeroBasedRowPtr = dataPtr->current.cursorY;
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
	
	
	if (dataPtr != nullptr) result = dataPtr->cursorVisible;
	return result;
}// CursorIsVisible


/*!
Destroys all scrollback buffer lines.  The “visible”
lines are not affected.  This obviously invalidates any
iterators pointing to scrollback lines.

It follows that after invoking this routine, the return
value of Terminal_ReturnInvisibleRowCount() should be zero.

This triggers two events: "kTerminal_ChangeScrollActivity"
to indicate that data has been removed, and also
"kTerminal_ChangeText" (given a range consisting of
all previous scrollback lines).

(3.1)
*/
void
Terminal_DeleteAllSavedLines	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		SInt16 const	kPreviousScrollbackCount = dataPtr->scrollbackBuffer.size();
		
		
		dataPtr->scrollbackBuffer.clear();
		
		// notify listeners of the range of text that has gone away
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = dataPtr->selfRef;
			range.firstRow = -kPreviousScrollbackCount;
			range.firstColumn = 0;
			range.columnCount = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kPreviousScrollbackCount;
			
			changeNotifyForTerminal(dataPtr, kTerminal_ChangeText, &range/* context */);
		}
		
		// notify listeners that scroll activity has taken place
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeScrollActivity, inRef/* context */);
	}
}// DeleteAllSavedLines


/*!
Scans the given data for telltale signs that it may be
“expecting” a certain type of terminal emulator, setting
"outApparentEmulator" to the emulator that seems most
appropriate.

Use this to help avoid getting the emulator into unknown
states (the parser can recover, but gobbletygook doesn’t
help the user much).

\retval kTerminal_ResultCodeSuccess
if the text is processed without errors

\retval kTerminal_ResultCodeInvalidID
if the given terminal screen reference is invalid

(3.1)
*/
Terminal_ResultCode
Terminal_EmulatorDeriveFromCString	(TerminalScreenRef		inRef,
									 char const*			inCString,
									 Terminal_Emulator&		outApparentEmulator)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	
	
	outApparentEmulator = kTerminal_EmulatorVT100;
	if (nullptr == dataPtr) result = kTerminal_ResultCodeInvalidID;
	else
	{
		// INCOMPLETE; besides, this is essentially a heuristic
	}
	
	return result;
}// EmulatorDeriveFromCString


/*!
Returns whether the terminal emulator for the given
terminal is exactly VT100 (and NOT a compatible terminal
such as VT102).

WARNING:	Try not to rely on a routine like this.  It
			is far better to query this module generically
			for terminal features, than to check for a
			specific terminal.  This routine may go away
			in the future.

(3.0)
*/
Boolean
Terminal_EmulatorIsVT100	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) result = (dataPtr->emulation == kTerminal_EmulatorVT100);
	return result;
}// EmulatorIsVT100


/*!
Returns whether the terminal emulator for the given
terminal is exactly VT220 (and NOT a somewhat compatible
terminal such as VT100).

WARNING:	Try not to rely on a routine like this.  It
			is far better to query this module generically
			for terminal features, than to check for a
			specific terminal.  This routine may go away
			in the future.

(3.0)
*/
Boolean
Terminal_EmulatorIsVT220	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) result = (dataPtr->emulation == kTerminal_EmulatorVT220);
	return result;
}// EmulatorIsVT220


/*!
Sends a stream of characters originating in a
Core Foundation string to the specified screen’s
terminal emulator, interpreting escape sequences,
etc. which may affect the terminal display.

USE WITH CARE.  Generally, you send data to this
routine that comes directly from a program that
knows what it’s doing.  There are more elegant
ways to have specific effects on terminal screens
in an emulator-independent fashion; you should
use those routines before hacking up a string as
input to this routine.

\retval kTerminal_ResultCodeSuccess
if the text is processed without errors

\retval kTerminal_ResultCodeInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultCodeNotEnoughMemory
if a text buffer cannot be allocated

(3.1)
*/
Terminal_ResultCode
Terminal_EmulatorProcessCFString	(TerminalScreenRef	inRef,
									 CFStringRef		inString)
{
	// TEMPORARY:	This is being handled backwards, stuffing the CFString data
	//				into an array and processing it that way, ignoring any
	//				special encoding.  This MUST be fixed, but it requires a
	//				lot more CFString-aware code, first.
	Handle					buffer = Memory_NewHandle(CFStringGetLength(inString) * sizeof(UniChar));
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	
	
	if (buffer == nullptr) result = kTerminal_ResultCodeNotEnoughMemory;
	else
	{
		CFIndex		actualSize = 0;
		CFIndex		numberOfCharactersCopied = CFStringGetBytes(inString, CFRangeMake(0, CFStringGetLength(inString)),
																kCFStringEncodingMacRoman/* TEMPORARY - should be terminal-dependent */,
																0/* loss byte, or 0 for no lossy conversion */,
																false/* is external representation */,
																REINTERPRET_CAST(*buffer, UInt8*), GetHandleSize(buffer), &actualSize);
		
		
		if (numberOfCharactersCopied < CFStringGetLength(inString))
		{
			// error...
		}
		if (actualSize > GetHandleSize(buffer))
		{
			// error...
		}
		result = Terminal_EmulatorProcessData(inRef, REINTERPRET_CAST(*buffer, UInt8 const*), GetHandleSize(buffer));
		DisposeHandle(buffer), buffer = nullptr;
	}
	return result;
}// EmulatorProcessCFString


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

\retval kTerminal_ResultCodeSuccess
if the text is processed without errors

\retval kTerminal_ResultCodeInvalidID
if the given terminal screen reference is invalid

(3.0)
*/
Terminal_ResultCode
Terminal_EmulatorProcessCString		(TerminalScreenRef	inRef,
									 char const*		inCString)
{
	return Terminal_EmulatorProcessData(inRef, REINTERPRET_CAST(inCString, UInt8 const*), CPP_STD::strlen(inCString));
}// EmulatorProcessCString


/*!
Sends a stream of characters to the specified
screen’s terminal emulator, interpreting escape
sequences, etc. which may affect the terminal
display.

USE WITH CARE.  Generally, you send data to this
routine that comes directly from a program that
knows what it’s doing.  There are more elegant
ways to have specific effects on terminal screens
in an emulator-independent fashion; you should
use those routines before hacking up a string as
input to this routine.

\retval kTerminal_ResultCodeSuccess
if the text is processed without errors

\retval kTerminal_ResultCodeInvalidID
if the given terminal screen reference is invalid

(3.0)
*/
Terminal_ResultCode
Terminal_EmulatorProcessData	(TerminalScreenRef	inRef,
								 UInt8 const*		inBuffer,
								 UInt32				inLength)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	
	
	if (inLength != 0)
	{
		My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
		
		
		if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
		else
		{
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
			for (register UInt32 i = inLength; i > 0; )
			{
				My_Parser::State	currentState = dataPtr->currentState;
				My_Parser::State	nextState = currentState;
				Boolean				isInterrupt = false;
				
				
				// find a new state, which may or may not interrupt the
				// state that is currently forming
				countRead = invokeEmulatorStateDeterminantProc
							(dataPtr->stateDeterminant, dataPtr, ptr, i, currentState, nextState, isInterrupt);
				assert(countRead <= i);
				//Console_WriteValue("number of characters absorbed by state determinant", countRead);
				i -= countRead; // could be zero (no-op)
				ptr += countRead; // could be zero (no-op)
				
				// LOOPING GUARD: whenever the proposed next state matches the
				// current state, a counter is incremented; if this count
				// exceeds an arbitrary value, the next state is FORCED to
				// return to the initial state (flagging a console error) so
				// that this does not hang the application in an infinite loop
				if (currentState == nextState)
				{
					// exclude the echo data, because this is the one state
					// that can be expected to remain for a long period of
					// time (e.g. long strings of printable text)
					if (nextState != My_Parser::kStateEcho)
					{
						++dataPtr->parserStateRepetitions;
						if (dataPtr->parserStateRepetitions > 100/* arbitrary */)
						{
							Console_WriteHorizontalRule();
							Console_WriteValueFourChars("SERIOUS PARSER ERROR: appears to be stuck, state", currentState);
							if (My_Parser::kStateInitial == currentState)
							{
								// if somehow stuck oddly in the initial state, assume
								// the trigger character is responsible and simply
								// ignore the troublesome sequence of characters
								Console_WriteValueCharacter("FORCING step-over of trigger character", *ptr);
								--i;
								++ptr;
							}
							else
							{
								Console_WriteLine("FORCING a return to the initial state");
								nextState = My_Parser::kStateInitial;
							}
							Console_WriteHorizontalRule();
							dataPtr->parserStateRepetitions = 0;
						}
					}
				}
				else
				{
					dataPtr->parserStateRepetitions = 0;
				}
				
				// perform whatever action is appropriate to enter this state
				countRead = invokeEmulatorStateTransitionProc
							(dataPtr->transitionHandler, dataPtr, ptr, i, nextState, currentState);
				assert(countRead <= i);
				//Console_WriteValue("number of characters absorbed by transition handler", countRead);
				i -= countRead; // could be zero (no-op)
				ptr += countRead; // could be zero (no-op)
				
				if (false == isInterrupt)
				{
					// remember this new state
					dataPtr->currentState = nextState;
				}
			}
			
			// restore cursor
			setCursorVisible(dataPtr, true);
		}
	}
	return result;
}// EmulatorProcessData


/*!
Changes the kind of terminal a virtual terminal
will emulate.

(3.0)
*/
void
Terminal_EmulatorSet	(TerminalScreenRef	inRef,
						 Terminal_Emulator	inEmulationType)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) dataPtr->emulation = inEmulationType;
	switch (inEmulationType)
	{
	case kTerminal_EmulatorANSIBBS:
		dataPtr->stateDeterminant = emulatorVT100StateDeterminant;
		dataPtr->transitionHandler = emulatorVT100StateTransition;
		break;
	
	case kTerminal_EmulatorVT100:
	case kTerminal_EmulatorVT102:
		dataPtr->stateDeterminant = emulatorVT100StateDeterminant;
		dataPtr->transitionHandler = emulatorVT100StateTransition;
		break;
	
	case kTerminal_EmulatorVT220:
		dataPtr->stateDeterminant = emulatorVT220StateDeterminant;
		dataPtr->transitionHandler = emulatorVT220StateTransition;
		break;
	
	case kTerminal_EmulatorDumb:
	case kTerminal_EmulatorANSISCO: // UNIMPLEMENTED
	case kTerminal_EmulatorVT320: // UNIMPLEMENTED
	case kTerminal_EmulatorVT420: // UNIMPLEMENTED
	case kTerminal_EmulatorXTermOriginal: // UNIMPLEMENTED
	case kTerminal_EmulatorXTermColor: // UNIMPLEMENTED
	default:
		// ???
		dataPtr->stateDeterminant = emulatorStandardStateDeterminant;
		dataPtr->transitionHandler = emulatorStandardStateTransition;
		break;
	}
}// EmulatorSet


/*!
Initiates a capture to a file for the specified screen,
notifying listeners of "kTerminal_ChangeFileCaptureBegun"
events if successful.  The callbacks are invoked after
internal file capture state has been set up, so APIs
like Terminal_FileCaptureGetReferenceNumber() can be
used in callbacks to determine the given file reference
number.

If the capture begins successfully, "true" is returned;
otherwise, "false" is returned.

(3.0)
*/
Boolean
Terminal_FileCaptureBegin	(TerminalScreenRef	inRef,
							 SInt16				inOpenWritableFile)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Boolean					result = false;
	
	
	if (dataPtr != nullptr)
	{
		dataPtr->captureFileRefNum = inOpenWritableFile;
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeFileCaptureBegun, inRef);
		result = true;
	}
	
	return result;
}// FileCaptureBegin


/*!
Terminates the file capture for the specified screen,
notifying listeners of "kTerminal_ChangeFileCaptureEnding"
events just prior to clearing internal file capture state
(therefore, Terminal_FileCaptureGetReferenceNumber() will
return a valid value in a callback invoked by this routine,
but would not return a valid value after this routine
returns).

Since the Terminal module does not open the capture file,
you must subsequently close it using FSClose().

(3.0)
*/
void
Terminal_FileCaptureEnd		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeFileCaptureEnding, inRef);
		dataPtr->captureFileRefNum = 0;
	}
}// FileCaptureEnd


/*!
Returns the file reference number of the open capture
file for the specified terminal, if any.  If no
capture is open, returns an invalid number (you can
also use Terminal_FileCaptureInProgress() to ascertain
this).

(3.0)
*/
SInt16
Terminal_FileCaptureGetReferenceNumber	(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	SInt16				result = 0;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->captureFileRefNum;
	}
	return result;
}// FileCaptureInProgress


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
	
	
	if (dataPtr != nullptr)
	{
		result = (dataPtr->captureFileRefNum != 0);
	}
	return result;
}// FileCaptureInProgress


/*!
Presents a dialog box asking the user to specify
the file where captured data should be sent.  If
the user does not cancel and no error occurs,
"true" is returned, otherwise, "false" is returned.

(3.0)
*/
Boolean
Terminal_FileCaptureSaveDialog		(FSSpec*	outFSSpecPtr)
{
	Boolean		result = false;
	
	
	if (outFSSpecPtr != nullptr)
	{
		Str255			fileDefaultName,
						prompt,
						title;
		OSStatus		error = noErr;
		Str32			numString;
		static SInt16	captureNumber = 0;
		
		
		// LOCALIZE THIS WITH STRING SUBSTITUTION (AND USER PREFERENCE?)
		NumToString(++captureNumber, numString);
		PLstrcpy(fileDefaultName, "\puntitled");
		fileDefaultName[++(fileDefaultName[0])] = ' ';
		PLstrcat(fileDefaultName, numString);
		
		GetIndString(prompt, rStringsNavigationServices, siNavPromptSaveCapturedText);
		GetIndString(title, rStringsNavigationServices, siNavDialogTitleSaveCapturedText);
		
		{
			NavReplyRecord		reply;
			OSType				captureFileCreator;
			size_t				actualSize = 0L;
			
			
			// get the user’s Capture File Creator preference, if possible
			unless (Preferences_GetData(kPreferences_TagCaptureFileCreator, sizeof(captureFileCreator),
										&captureFileCreator, &actualSize) == kPreferences_ResultCodeSuccess)
			{
				captureFileCreator = 'ttxt'; // default to SimpleText if a preference can’t be found
			}
			
			Alert_ReportOSStatus(error = FileSelectionDialogs_PutFile
											(prompt, title, fileDefaultName,
												captureFileCreator, 'TEXT', kPreferences_NavPrefKeyGenericSaveFile,
												kNavDefaultNavDlogOptions | kNavDontAddTranslateItems,
												EventLoop_HandleNavigationUpdate, &reply, outFSSpecPtr));
			result = ((error == noErr) && (reply.validRecord));
		}
	}
	return result;
}// FileCaptureSaveDialog


/*!
Writes the specified text to the capture file of the
specified screen.

This functionality has been re-written in MacTelnet 3.0
(like so many other god damned things).  This time, the
impetus was to make file captures safe from buffer
overflows, to use constant pointers, and to recover from
unexpected failures to write all bytes via FSWrite().

(3.0)
*/
void
Terminal_FileCaptureWriteData	(TerminalScreenRef	inRef,
								 UInt8 const*		inBuffer,
								 SInt32				inLength)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if ((dataPtr != nullptr) && (dataPtr->captureFileRefNum != 0))
	{
		size_t const	kCaptureBufferSize = 512; // bytes
		UInt8			chunkBuffer[kCaptureBufferSize];
		UInt8*			chunkBufferIterator = nullptr;
		UInt8 const*	paramBufferIterator = nullptr;
		OSStatus		error = noErr;
		size_t			bytesLeft = inLength;
		SInt32			bytesInCaptureBuffer = 0;
		SInt32			chunkByteCount = 0;
		
		
		// initialize the iterator for the buffer given as input
		paramBufferIterator = inBuffer;
		
		// iterate over the input buffer until every byte has been
		// written, or until FSWrite() returns an error (big problem!)
		while ((bytesLeft > 0) && (error == noErr))
		{
			// initialize the iterator for the chunk buffer (512 bytes maximum)
			chunkBufferIterator = chunkBuffer;
			bytesInCaptureBuffer = 0; // each time through, initially nothing important in the chunk buffer
			chunkByteCount = (bytesLeft < kCaptureBufferSize) ? bytesLeft : kCaptureBufferSize; // only copy data that’s there!!!
			
			// iterate over the chunk buffer until either no bytes
			// remain to be written from the input buffer, or until
			// there is no more capacity in the chunk buffer
			while (chunkByteCount > 0)
			{
				*chunkBufferIterator++ = *paramBufferIterator++;
				++bytesInCaptureBuffer;
				--chunkByteCount;
			}
			
			// write the chunk buffer to disk - the number of bytes
			// actually copied to it were tracked during the iteration;
			// FSWrite() will use that as input, and then on output
			// the value will CHANGE to be the number of bytes that
			// were actually successfully written to the file (so,
			// the mega-count "bytesLeft" takes into account the latter)
			error = FSWrite(dataPtr->captureFileRefNum, &bytesInCaptureBuffer, chunkBuffer);
			bytesLeft -= bytesInCaptureBuffer;
		}
		
		// this REALLY should trigger a user alert of some sort
		if (error != noErr)
		{
			Terminal_FileCaptureEnd(inRef);
			Console_WriteValue("file capture unexpectedly terminated, error", error);
		}
	}
}// FileCaptureWriteData


/*!
Iterates over the given row of the specified terminal screen,
executing a function on each chunk of text for which the
associated attributes are all IDENTICAL.  The context is
defined by you, and is passed directly to the specified
function each time it is invoked.

\retval kTerminal_ResultCodeSuccess
if no error occurred

\retval kTerminal_ResultCodeParameterError
if the screen run operation function is invalid

\retval kTerminal_ResultCodeNotEnoughMemory
if any line buffers are unexpectedly empty

(3.0)
*/
Terminal_ResultCode
Terminal_ForEachLikeAttributeRunDo	(TerminalScreenRef			inRef,
									 Terminal_LineRef			inStartRow,
									 ScreenRunOperationProcPtr	inDoWhat,
									 void*						inContextPtr)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		screenPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if ((nullptr == inDoWhat) || (nullptr == screenPtr) || (nullptr == iteratorPtr))
	{
		result = kTerminal_ResultCodeParameterError;
	}
	else
	{
		// need to search line for style chunks
		My_ScreenBufferLineList::iterator		lineIterator = iteratorPtr->rowIterator;
		My_TextVector::iterator					textIterator;
		My_TextAttributesList::const_iterator	attrIterator = lineIterator->attributeVector.begin();
		TerminalTextAttributes					previousAttributes = 0;
		TerminalTextAttributes					currentAttributes = 0;
		SInt16									runStartCharacterIndex = 0;
		SInt16									characterIndex = 0;
		My_TextVector::difference_type			styleRunLength = 0;
		
		
	#if 0
		// DEBUGGING ONLY: if you suspect a bug in the incremental loop below,
		// try asking the entire line to be drawn without formatting, first
		InvokeScreenRunOperationProc(inDoWhat, inRef, lineIterator->textVector.c_str()/* starting point */,
										lineIterator->textVector.size()/* length */,
										inStartRow, 0/* zero-based start column */,
										lineIterator->globalAttributes, inContextPtr);
	#endif
		
		// TEMPORARY - HIGHLY inefficient to search here, need to change this into a cache
		//             (in fact, attribute bit arrays can probably be completely replaced by
		//             style runs at some point in the future)
		assert(lineIterator->textVector.c_str() != nullptr);
		for (textIterator = lineIterator->textVector.begin(),
					attrIterator = lineIterator->attributeVector.begin();
				(textIterator != lineIterator->textVector.end()) &&
					(attrIterator != lineIterator->attributeVector.end());
				++textIterator, ++attrIterator, ++characterIndex)
		{
			currentAttributes = *attrIterator;
			if ((currentAttributes != previousAttributes) ||
				(characterIndex == STATIC_CAST(lineIterator->textVector.size() - 1, SInt16)) ||
				(characterIndex == STATIC_CAST(lineIterator->attributeVector.size() - 1, SInt16)))
			{
				styleRunLength = characterIndex - runStartCharacterIndex;
				
				// found new style run; so handle the previous run
				if (styleRunLength > 0)
				{
					InvokeScreenRunOperationProc(inDoWhat, inRef, lineIterator->textVector.c_str() + runStartCharacterIndex/* starting point */,
													styleRunLength/* length */, inStartRow,
													runStartCharacterIndex/* zero-based start column */,
													previousAttributes | lineIterator->globalAttributes, inContextPtr);
				}
				
				// reset for next run
				previousAttributes = currentAttributes;
				runStartCharacterIndex = characterIndex;
			}
		}
		
		// ask that the remainder of the line be handled as if it were blank
		if (textIterator != lineIterator->textVector.end())
		{
			styleRunLength = std::distance(textIterator, lineIterator->textVector.end());
			
			// found new style run; so handle the previous run
			if (styleRunLength > 0)
			{
				TerminalTextAttributes		attributesForRemainder = lineIterator->globalAttributes;
				
				
				// the “selected” attribute is special; it persists regardless
				if (previousAttributes & kTerminalTextAttributeSelected)
				{
					attributesForRemainder |= kTerminalTextAttributeSelected;
				}
				
				InvokeScreenRunOperationProc(inDoWhat, inRef, nullptr/* starting point */, styleRunLength/* length */,
												inStartRow, runStartCharacterIndex/* zero-based start column */,
												attributesForRemainder, inContextPtr);
			}
		}
	}
	return result;
}// ForEachLikeAttributeRunDo


/*!
Returns ONLY the attributes that were assigned to the
specified line in a global manner.  For example, double-sized
text is applied to an entire line, never to a single column.

IMPORTANT:	To properly render a line, its global attributes
			must be mixed with the attributes of the current
			cursor location.

(3.0)
*/
Terminal_ResultCode
Terminal_GetLineGlobalAttributes	(TerminalScreenRef			UNUSED_ARGUMENT(inScreen),
									 Terminal_LineRef			inRow,
									 TerminalTextAttributes*	outAttributesPtr)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	//My_ScreenBufferConstPtr	dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if ((iteratorPtr == nullptr) || (outAttributesPtr == nullptr)) result = kTerminal_ResultCodeParameterError;
	else
	{
		*outAttributesPtr = iteratorPtr->rowIterator->globalAttributes;
	}
	
	return result;
}// GetLineGlobalAttributes


/*!
Like Terminal_GetLineRange(), but automatically pulls in the
entire line (from the first column to past the end column).

\retval kTerminal_ResultCodeSuccess
if the data was copied successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

(3.1)
*/
Terminal_ResultCode
Terminal_GetLine	(TerminalScreenRef		inScreen,
					 Terminal_LineRef		inRow,
					 char const*&			outPossibleReferenceStart,
					 char const*&			outPossibleReferencePastEnd)
{
	return Terminal_GetLineRange(inScreen, inRow, 0/* first column */, -1/* last column; negative means “very end” */,
									outPossibleReferenceStart, outPossibleReferencePastEnd);
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

\retval kTerminal_ResultCodeSuccess
if the data was copied successfully

\retval kTerminal_ResultCodeInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultCodeInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultCodeParameterError
if the specified column is out of range and nonnegative

(3.1)
*/
Terminal_ResultCode
Terminal_GetLineRange	(TerminalScreenRef		inScreen,
						 Terminal_LineRef		inRow,
						 UInt16					inZeroBasedStartColumn,
						 SInt16					inZeroBasedPastEndColumnOrNegativeForLastColumn,
						 char const*&			outReferenceStart,
						 char const*&			outReferencePastEnd)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeParameterError;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	outReferenceStart = nullptr;
	outReferencePastEnd = nullptr;
	
	if (nullptr == dataPtr) result = kTerminal_ResultCodeInvalidID;
	else if (nullptr == iteratorPtr) result = kTerminal_ResultCodeInvalidIterator;
	else
	{
		UInt16 const	kPastEndColumn = (inZeroBasedPastEndColumnOrNegativeForLastColumn < 0)
											? iteratorPtr->rowIterator->textVector.size()
											: inZeroBasedPastEndColumnOrNegativeForLastColumn;
		
		
		if (kPastEndColumn > iteratorPtr->rowIterator->textVector.size())
		{
			result = kTerminal_ResultCodeParameterError;
		}
		else
		{
			outReferenceStart = iteratorPtr->rowIterator->textVector.c_str() + inZeroBasedStartColumn;
			outReferencePastEnd = iteratorPtr->rowIterator->textVector.c_str() + kPastEndColumn;
			result = kTerminal_ResultCodeSuccess;
		}
	}
	
	return result;
}// GetLineRange


/*!
Returns (for any non-nullptr parameters) the top,
left, bottom and right boundaries of the entire
screen buffer.

If the top extreme is negative, it means the
screen has a scrollback buffer.

(3.0)
*/
void
Terminal_GetMaximumBounds	(TerminalScreenRef	inRef,
							 SInt16*			outZeroBasedTopScrollbackRowPtrOrNull,
							 SInt16*			outZeroBasedLeftColumnPtrOrNull,
							 SInt16*			outZeroBasedBottomRowPtrOrNull,
							 SInt16*			outZeroBasedRightColumnPtrOrNull)
{
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		if (outZeroBasedTopScrollbackRowPtrOrNull != nullptr)
		{
			*outZeroBasedTopScrollbackRowPtrOrNull = dataPtr->scrollingRegion.firstRow - Terminal_ReturnInvisibleRowCount(inRef);
		}
		if (outZeroBasedLeftColumnPtrOrNull != nullptr)
		{
			*outZeroBasedLeftColumnPtrOrNull = 0;
		}
		if (outZeroBasedBottomRowPtrOrNull != nullptr)
		{
			*outZeroBasedBottomRowPtrOrNull = dataPtr->scrollingRegion.lastRow;
		}
		if (outZeroBasedRightColumnPtrOrNull != nullptr)
		{
			*outZeroBasedRightColumnPtrOrNull = dataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
		}
	}
}// GetMaximumBounds


/*!
Returns "true" only if the specified terminal was told
that its keypad keys are used for application-specific
purposes instead of their ordinary ASCII equivalents.

TEMPORARY:	This API is under evaluation.  Most likely,
			this would only be needed to handle key
			mappings elsewhere, and the whole key handling
			mechanism may change.

(3.1)
*/
Boolean
Terminal_KeypadHasApplicationKeys	(TerminalScreenRef	inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->modeApplicationKeys;
	}
	return result;
}// KeypadHasApplicationKeys


/*!
Returns "true" only if the specified terminal was told
that its keypad numbers are actually equivalent to arrow
keys (for movement).

For example, the VT100 terminal in ANSI mode can have
this state while the keypad is set to application keys.

TEMPORARY:	This API is under evaluation.  Most likely,
			this would only be needed to handle key
			mappings elsewhere, and the whole key handling
			mechanism may change.

(3.1)
*/
Boolean
Terminal_KeypadHasMovementKeys	(TerminalScreenRef	inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		result = (dataPtr->modeCursorKeys && dataPtr->modeApplicationKeys && dataPtr->modeANSIEnabled);
	}
	return result;
}// KeypadHasMovementKeys


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
	
	
	if (dataPtr != nullptr)
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
Changes an iterator to point to a different line, one that is
the specified number of rows later than or earlier than the
row that the iterator currently points to.  Pass a value less
than zero to find a previous row, otherwise positive values
find following rows.

Returns "kTerminal_ResultCodeIteratorCannotAdvance" if the
specified iterator is at the end of its list of lines.
Returns "kTerminal_ResultCodeParameterError" if the iterator
is completely invalid.  Otherwise, returns
"kTerminal_ResultCodeSuccess".

(3.0)
*/
Terminal_ResultCode
Terminal_LineIteratorAdvance	(TerminalScreenRef		inRef,
								 Terminal_LineRef		inRow,
								 SInt16					inHowManyRowsForwardOrNegativeForBackward)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	
	
	if (iteratorPtr == nullptr) result = kTerminal_ResultCodeParameterError;
	else if (((inHowManyRowsForwardOrNegativeForBackward > 0) && (iteratorPtr->rowIterator == iteratorPtr->sourceList.end())) ||
			((inHowManyRowsForwardOrNegativeForBackward < 0) && (iteratorPtr->rowIterator == iteratorPtr->sourceList.begin())))
	{
		// unable to advance!
		result = kTerminal_ResultCodeIteratorCannotAdvance;
	}
	else
	{
		My_ScreenBufferLineList::difference_type	moveDistance = STATIC_CAST(inHowManyRowsForwardOrNegativeForBackward,
																				My_ScreenBufferLineList::difference_type);
		
		
		// IMPLEMENTATION DETAIL: the scrollback is maintained in reverse, so
		// a normal advance would technically move backward instead of forward;
		// for consistency, this routine always advances in a downward direction
		// (from a rendering point of view)
		if (&iteratorPtr->sourceList == &dataPtr->scrollbackBuffer) moveDistance = -moveDistance;
		
		std::advance(iteratorPtr->rowIterator, moveDistance);
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
Terminates a printout for the specified terminal,
closing the temporary file used for print data.

(2.6)
*/
void
Terminal_PrintingEnd	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr == nullptr) Sound_StandardAlert();
	else
	{
		OSStatus	error = noErr;
		FSSpec		temporaryFile;
		
		
		// close the printing file...
		error = FSClose(dataPtr->printing.temporaryFileRefNum), dataPtr->printing.temporaryFileRefNum = -1;
		if (error != noErr) Sound_StandardAlert();
		
		// ...and then destroy it (uses modern File Manager in MacTelnet 3.0)
		error = Folder_GetFSSpec(kFolder_RefMacTemporaryItems, &temporaryFile);
		if (error == noErr)
		{
			error = FSMakeFSSpec(temporaryFile.vRefNum, temporaryFile.parID,
									REINTERPRET_CAST(dataPtr->printing.temporaryFileName, StringPtr),
									&temporaryFile);
			if (error == noErr) error = FSpDelete(&temporaryFile);
		}
		if (error != noErr) Sound_StandardAlert();
		
		dataPtr->printing.enabled = false; // 3.0
	}
}// PrintingEnd


/*!
Returns "true" only if the specified screen’s
text is currently being printed.

(2.6)
*/
Boolean
Terminal_PrintingInProgress		(TerminalScreenRef		inRef)
{
	Boolean				result = false;
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) result = dataPtr->printing.enabled;
	return result;
}// PrintingInProgress


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
	
	
	if (dataPtr != nullptr)
	{
		if (inFlags & kTerminal_ResetFlagsGraphicsCharacters)
		{
			// “rescue” the user by forcing graphics not to be used
			{
				Terminal_RangeDescription	range;
				Terminal_LineRef			lineIterator = nullptr;
				
				
				range.screen = dataPtr->selfRef;
				range.firstRow = 0;
				range.firstColumn = 0;
				range.columnCount = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
				range.rowCount = dataPtr->screenBuffer.size();
				
				lineIterator = Terminal_NewMainScreenLineIterator(inRef, range.firstRow);
				if (lineIterator != nullptr)
				{
					// remove this mode attribute from the current character set
					dataPtr->current.characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
					
					// force all visible characters to no longer use graphics
					(Terminal_ResultCode)Terminal_ChangeRangeAttributes
											(dataPtr->selfRef, lineIterator/* first row */, range.rowCount,
												range.firstColumn/* start */, range.firstColumn + range.columnCount/* past the end */,
												false/* constrain to rectangle */, 0/* attributes to set */,
												kTerminalTextAttributeVTGraphics/* attributes to clear */);
					
					// add the entire visible buffer to the text-change region;
					// this should trigger things like Terminal View updates
					//Console_WriteLine("text changed event: reset terminal");
					changeNotifyForTerminal(dataPtr, kTerminal_ChangeText, &range);
					
					// destroy the iterator
					Terminal_DisposeLineIterator(&lineIterator);
				}
			}
		}
		
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
	return kMy_NumberOfCharactersPerLineMaximum;
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
	
	
	if (dataPtr != nullptr) result = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
	return result;
}// ReturnColumnCount


/*!
Returns the number of saved lines that have scrolled
off the top of the screen.

(3.0)
*/
UInt16
Terminal_ReturnInvisibleRowCount	(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) result = dataPtr->scrollbackBuffer.size();
	return result;
}// ReturnInvisibleRowCount


/*!
Returns the number of spaces until the next tab stop,
based on the current cursor position in the specified
terminal.

(3.0)
*/
UInt16
Terminal_ReturnNextTabDistance		(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) result = tabStopGetDistanceFromCursor(dataPtr);
	return result;
}// ReturnNextTabDistance


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
	
	
	if (dataPtr != nullptr) result = dataPtr->screenBuffer.size();
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
	
	
	if (dataPtr != nullptr) result = dataPtr->speaker;
	return result;
}// ReturnSpeaker


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
	
	
	if (dataPtr != nullptr) result = dataPtr->reverseVideo;
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
	
	
	if (dataPtr != nullptr) result = dataPtr->saveToScrollbackOnClear;
	return result;
}// SaveLinesOnClearIsEnabled


/*!
Performs a search for the specified string, using
the given search constraints and starting from
the current search position for the indicated
screen.

Returns "true" only if a match is found in the
text following the current search pointer.

(3.0)
*/
Boolean
Terminal_SearchForPhrase	(TerminalScreenRef		inRef,
							 char const*			inPhraseBuffer,
							 UInt32					inPhraseBufferLength,
							 Terminal_SearchFlags	inFlags)
{
	UInt16 const		kNumberOfScrollbackRows = Terminal_ReturnInvisibleRowCount(inRef);
	UInt16 const		kNumberOfVisibleRows = Terminal_ReturnRowCount(inRef);
	Terminal_LineRef	scrollbackLineIterator = Terminal_NewScrollbackLineIterator(inRef, 0);
	Terminal_LineRef	screenLineIterator = Terminal_NewMainScreenLineIterator(inRef, 0);
	Size				totalSize = 0L;
	Boolean				result = false;
	
	
	// TEMPORARY; since the search algorithm currently traverses a
	// contiguous buffer, one is created; however, it’s hard to imagine
	// anything less efficient than this!  A future implementation will
	// probably create index entries for each terminal line, so that the
	// search algorithm can be rewritten to use the existing buffer and
	// index and not require the creation of a mega-buffer.
	if ((forEachLineDo(inRef, scrollbackLineIterator, kNumberOfScrollbackRows, addScreenLineSize, &totalSize) < 0) ||
		(forEachLineDo(inRef, screenLineIterator, kNumberOfVisibleRows, addScreenLineSize, &totalSize) < 0))
	{
		result = false;
	}
	else
	{
		Handle		handle = nullptr;
		
		
		handle = Memory_NewHandle(totalSize);
		if (handle == nullptr) result = false;
		else
		{
			// handle “reverse buffer” flag by constructing handle in reverse - unimplemented
			
			if ((forEachLineDo(inRef, scrollbackLineIterator, kNumberOfScrollbackRows, appendScreenLineToHandle, &handle) < 0) ||
				(forEachLineDo(inRef, screenLineIterator, kNumberOfVisibleRows, appendScreenLineToHandle, &handle) < 0))
			{
				// No data?  Out of memory!
				result = false;
			}
			else
			{
				long	foundStringStartOffset = 0L;
				long	foundStringEndOffset = 0L;
				
				
				// search the buffer
				result = searchBufferForPhrase(handle, 0/* offset where searching will start */,
												inPhraseBuffer, inPhraseBufferLength,
												((inFlags & kTerminal_SearchFlagsCaseSensitive) != 0),
												((inFlags & kTerminal_SearchFlagsSearchBackwards) != 0),
												((inFlags & kTerminal_SearchFlagsWrapAround) != 0),
												&foundStringStartOffset, &foundStringEndOffset);
				if (result)
				{
					// select the text that was found
					// UNIMPLEMENTED
					Console_WriteValue("starting offset of found string", foundStringStartOffset);
					Console_WriteValue("ending offset of found string", foundStringEndOffset);
				}
				else
				{
					// text was not found
					//Console_WriteLine("string not found");
				}
			}
			Memory_DisposeHandle(&handle);
		}
	}
	
	return result;
}// SearchForPhrase


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
	}
}// SetBellEnabled


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
Terminal_ResultCode
Terminal_SetListeningSession	(TerminalScreenRef	inRef,
								 SessionRef			inSession)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultCodeInvalidID;
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
	
	
	if (dataPtr != nullptr)
	{
		TerminalSpeaker_SetMuted(dataPtr->speaker, !inIsEnabled);
	}
}// SetSpeechEnabled


/*!
Changes the number of characters of text per line for a
screen buffer.  This operation is currently trivial
because all line arrays are allocated to maximum size
when a buffer is created, and this simply adjusts the
percentage of the total that should be usable.  Although,
this also forces the cursor into the new region, if
necessary.

\retval kTerminal_ResultCodeSuccess
if the terminal is resized without errors

\retval kTerminal_ResultCodeInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultCodeParameterError
if the given number of columns is too small or too large

\retval kTerminal_ResultCodeNotEnoughMemory
not currently returned because this routine does no memory
reallocation; however a future implementation might decide
to reallocate, and if such reallocation fails, this error
should be returned

(2.6)
*/
Terminal_ResultCode
Terminal_SetVisibleColumnCount	(TerminalScreenRef	inRef,
								 UInt16				inNewNumberOfCharactersWide)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else
	{
		// move cursor, if necessary
		if (inNewNumberOfCharactersWide <= dataPtr->current.cursorX)
		{
			moveCursorX(dataPtr, inNewNumberOfCharactersWide - 1);
		}
		
		if (inNewNumberOfCharactersWide > kMy_NumberOfCharactersPerLineMaximum)
		{
			// flag an error, but set a reasonable value anyway
			result = kTerminal_ResultCodeParameterError;
			inNewNumberOfCharactersWide = kMy_NumberOfCharactersPerLineMaximum;
		}
		dataPtr->text.visibleScreen.numberOfColumnsPermitted = inNewNumberOfCharactersWide;
		dataPtr->printing.wrapColumnCount = inNewNumberOfCharactersWide;
	}
	return result;
}// SetVisibleColumnCount


/*!
Changes the number of lines of text, not including the
scrollback buffer, for a screen buffer.  This non-trivial
operation requires reallocating a series of arrays.

\retval kTerminal_ResultCodeSuccess
if the terminal is resized without errors

\retval kTerminal_ResultCodeInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultCodeParameterError
if the given number of rows is too small or too large

\retval kTerminal_ResultCodeNotEnoughMemory
if it is not possible to allocate the requested number of rows

(2.6)
*/
Terminal_ResultCode
Terminal_SetVisibleRowCount		(TerminalScreenRef	inRef,
								 UInt16				inNewNumberOfLinesHigh)
{
//return kTerminal_ResultCodeSuccess; // TMP
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	//Console_WriteValue("requested new number of lines", inNewNumberOfLinesHigh);
	if (inNewNumberOfLinesHigh > 200)
	{
		Console_WriteLine("refusing to resize on account of ridiculous line size");
		result = kTerminal_ResultCodeParameterError;
	}
	else if (dataPtr == nullptr) result = kTerminal_ResultCodeInvalidID;
	else if (dataPtr->screenBuffer.size() != inNewNumberOfLinesHigh)
	{
		// then the requested number of lines is different than the current number; resize!
		SInt16 const	kOriginalNumberOfLines = dataPtr->screenBuffer.size();
		SInt16 const	kLineDelta = (inNewNumberOfLinesHigh - kOriginalNumberOfLines);
		
		
		//Console_WriteValue("window line size", kOriginalNumberOfLines);
		//Console_WriteValue("window size change", kLineDelta);
		
		// force view to the top of the bottommost screenful
		// UNIMPLEMENTED
		
		if (kLineDelta > 0)
		{
			// if more lines are in the screen buffer than before,
			// allocate space for them (but don’t scroll them off!)
			Boolean		insertOK = insertNewLines(dataPtr, kLineDelta, true/* append only */);
			
			
			unless (insertOK) result = kTerminal_ResultCodeNotEnoughMemory;
		}
		else
		{
		#if 0
			// this will move the entire screen buffer into the scrollback
			// beforehand, if desired (having the effect of CLEARING the
			// main screen, which is basically undesirable most of the time)
			if (dataPtr->text.scrollback.enabled)
			{
				(Boolean)insertNewLines(dataPtr, kOriginalNumberOfLines, false/* append only */);
			}
		#endif
			
			// now make sure the cursor line doesn’t fall off the end
			if (dataPtr->current.cursorY >= (dataPtr->screenBuffer.size() + kLineDelta))
			{
				moveCursorY(dataPtr, dataPtr->screenBuffer.size() + kLineDelta - 1);
			}
			
			// finally, shrink the buffer
			dataPtr->screenBuffer.resize(dataPtr->screenBuffer.size() + kLineDelta);
		}
		
		// reset scrolling region
		setScrollingRegionTop(dataPtr, 0);
		setScrollingRegionBottom(dataPtr, inNewNumberOfLinesHigh - 1);
		
		// reset visible region
		dataPtr->visibleBoundary.firstRow = 0;
		dataPtr->visibleBoundary.lastRow = inNewNumberOfLinesHigh - 1;
		
		// add new screen rows to the text-change region; this should trigger
		// things like Terminal View updates
		if (kLineDelta > 0)
		{
			// when the screen is becoming bigger, it is only necessary to
			// refresh the new region; when the screen is becoming smaller,
			// no refresh is necessary at all!
			Terminal_RangeDescription	range;
			
			
			//Console_WriteLine("text changed event: set visible row count");
			range.screen = dataPtr->selfRef;
			range.firstRow = kOriginalNumberOfLines;
			range.firstColumn = 0;
			range.columnCount = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kLineDelta;
			changeNotifyForTerminal(dataPtr, kTerminal_ChangeText, &range);
		}
	}
	
	return result;
}// SetVisibleRowCount


/*!
Speaks the specified text buffer using the same speech
channel as the specified terminal, interrupting any
current speech on that channel.  You might use this to
speak terminal-specific things, such as the current
selection of text.

(3.0)
*/
void
Terminal_SpeakBuffer	(TerminalScreenRef	inRef,
						 void const*		inBuffer,
						 Size				inBufferSize)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if ((dataPtr != nullptr) && API_AVAILABLE(SpeakBuffer))
	{
		TerminalSpeaker_SynthesizeSpeechFromBuffer(dataPtr->speaker, inBuffer, inBufferSize);
	}
}// SpeakBuffer


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
	
	
	if (dataPtr != nullptr)
	{
		result = !TerminalSpeaker_IsMuted(dataPtr->speaker);
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
		OSStatus	error = noErr;
		
		
		// add a listener to the specified target’s listener model for the given setting change
		error = ListenerModel_AddListenerForEvent(dataPtr->changeListenerModel, inForWhatChange, inListener);
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

/*!
A method of standard My_ScreenLineOperationProcPtr form,
this routine adds the data size of the specified line text
buffer to an overall sum, pointed to by "inoutSizePtr"
(assumed to be a Size*).

Since this operation never modifies the buffer, "false"
is always returned.
*/
static void
addScreenLineSize		(My_ScreenBufferPtr		UNUSED_ARGUMENT(inRef),
						 My_TextVector const&	inLineTextBuffer,
						 UInt16					UNUSED_ARGUMENT(inOneBasedLineNumber),
						 void*					inoutSizePtr)
{
	Size*		sumPtr = REINTERPRET_CAST(inoutSizePtr, Size*);
	
	
	(*sumPtr) += (inLineTextBuffer.size() + 1/* for newline */) * sizeof(char);
}// addScreenLineSize


/*!
A method of standard My_ScreenLineOperationProcPtr form,
this routine adds the data from the specified line text
buffer to a handle, pointed to by "inoutHandlePtr"
(assumed to be a Handle*).
*/
static void
appendScreenLineToHandle	(My_ScreenBufferPtr		UNUSED_ARGUMENT(inRef),
							 My_TextVector const&	inLineTextBuffer,
							 UInt16					UNUSED_ARGUMENT(inOneBasedLineNumber),
							 void*					inoutHandlePtr)
{
	Handle const*	handlePtr = REINTERPRET_CAST(inoutHandlePtr, Handle const*);
	char const		newline = '\n';
	
	
	PtrAndHand(inLineTextBuffer.c_str(), *handlePtr, inLineTextBuffer.size());
	PtrAndHand(&newline, *handlePtr, sizeof(newline));
}// appendScreenLineToHandle


/*!
Erases characters on the cursor row, from the cursor
position to the end of the line.

(2.6)
*/
static void
bufferEraseFromCursorColumnToLineEnd	(My_ScreenBufferPtr		inDataPtr)
{
	My_TextVector::iterator				textIterator;
	My_TextAttributesList::iterator		attrIterator;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	
	
	// if the cursor is positioned so as to trigger an erase
	// of the entire top line, and forced saving is enabled,
	// be sure to dump the screen contents into the scrollback
	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->mayNeedToSaveToScrollback))
	{
		inDataPtr->mayNeedToSaveToScrollback = false;
		saveToScrollback(inDataPtr);
	}
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// clear out screen line
	attrIterator = cursorLineIterator->attributeVector.begin();
	std::advance(attrIterator, postWrapCursorX);
	textIterator = cursorLineIterator->textVector.begin();
	std::advance(textIterator, postWrapCursorX);
	std::fill(attrIterator, cursorLineIterator->attributeVector.end(),
				cursorLineIterator->globalAttributes);
	std::fill(textIterator, cursorLineIterator->textVector.end(), ' ');
	
	// add the remainder of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from cursor column to line end");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - postWrapCursorX;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferEraseFromCursorColumnToLineEnd


/*!
Erases characters from the current cursor position
up to the end position (last line, last column).

(2.6)
*/
static void
bufferEraseFromCursorToEnd  (My_ScreenBufferPtr  inDataPtr)
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
		saveToScrollback(inDataPtr);
	}
	
	// blank out current line from cursor to end, and update
	bufferEraseFromCursorColumnToLineEnd(inDataPtr);
	
	// blank out following rows
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		
		// skip cursor line
		++postWrapCursorY;
		++lineIterator;
		for (; lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, *lineIterator);
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
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
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
static void
bufferEraseFromHomeToCursor		(My_ScreenBufferPtr  inDataPtr)
{
	My_ScreenRowIndex	postWrapCursorY = 0;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// blank out current line from beginning to cursor
	bufferEraseFromLineBeginToCursorColumn(inDataPtr);
	
	// blank out previous rows
	{
		My_ScreenBufferLineList::iterator	lineIterator = inDataPtr->screenBuffer.begin();
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		for (; lineIterator != cursorLineIterator; ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, *lineIterator);
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
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferEraseFromHomeToCursor


/*!
Erases characters on the cursor row, from the start of
the line up to and including the cursor position.

(2.6)
*/
static void
bufferEraseFromLineBeginToCursorColumn  (My_ScreenBufferPtr  inDataPtr)
{
	SInt32								fillLength = 0;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	fillLength = 1 + postWrapCursorX;
	
	// clear from beginning of line to cursor
	locateCursorLine(inDataPtr, cursorLineIterator);
	std::fill_n(cursorLineIterator->attributeVector.begin(), fillLength,
				cursorLineIterator->globalAttributes);
	std::fill_n(cursorLineIterator->textVector.begin(), fillLength, ' ');
	
	// add the first part of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from line begin to cursor column");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = fillLength;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferEraseFromLineBeginToCursorColumn


/*!
Erases an entire line in the screen buffer, triggering
redraws.  See also bufferEraseLineWithoutUpdate(), which
modifies the data buffer but does not notify about the
change.

Unlike bufferEraseLineWithoutUpdate(), this routine will
respect pending cursor wraps when asked to erase the
cursor line, and will leave line-global attributes
untouched.  (Its only real purpose is to handle the
VT100 "EL" sequence correctly.)

(2.6)
*/
static void
bufferEraseLineWithUpdate		(My_ScreenBufferPtr		inDataPtr,
								 SInt16					inLineToEraseOrNegativeForCursorLine)
{
	My_ScreenRowIndex					postWrapCursorY = inLineToEraseOrNegativeForCursorLine;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	if (inLineToEraseOrNegativeForCursorLine < 0)
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// clear out line
	// 3.0 - the VT100 spec does not say that EL should erase double-width,
	//       so leave the line-global attributes unchanged in this routine
	locateCursorLine(inDataPtr, cursorLineIterator);
	std::fill(cursorLineIterator->attributeVector.begin(),
				cursorLineIterator->attributeVector.end(),
				cursorLineIterator->globalAttributes);
	std::fill(cursorLineIterator->textVector.begin(), cursorLineIterator->textVector.end(), ' ');
	
	// add the entire row contents to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase line");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferEraseLineWithUpdate


/*!
Erases an entire line in the screen buffer without
triggering a redraw.  See also bufferEraseLineWithUpdate().

(2.6)
*/
static void
bufferEraseLineWithoutUpdate	(My_ScreenBufferPtr  	inDataPtr,
								 My_ScreenBufferLine&	inRow)
{
	// 3.0 - line-global attributes are cleared only if clearing an entire line
	inRow.globalAttributes = kTerminalTextAttributesAllOff;
	
	// fill in line
	bufferLineFill(inDataPtr, inRow, ' ', inRow.globalAttributes, false/* update global attributes */);
}// bufferEraseLineWithoutUpdate


/*!
Clears the screen, first saving its contents in the scrollback
buffer if that flag is turned on.  A screen redraw is triggered.

(2.6)
*/
static void
bufferEraseVisibleScreenWithUpdate		(My_ScreenBufferPtr		inDataPtr)
{
	//Console_WriteLine("bufferEraseVisibleScreenWithUpdate");
	
	// save screen contents in scrollback buffer, if appropriate
	if (inDataPtr->saveToScrollbackOnClear)
	{
		saveToScrollback(inDataPtr);
	}
	
	// clear buffer
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		for (lineIterator = inDataPtr->screenBuffer.begin(); lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, *lineIterator);
		}
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
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferEraseVisibleScreenWithUpdate


/*!
Clears out the text area and attributes buffers for
the specified screen.  All text characters are set to
blanks, and all attribute bytes become 0.  The display
is NOT updated.

(2.6)
*/
static void
bufferEraseWithoutUpdate	(My_ScreenBufferPtr		inDataPtr)
{
	My_ScreenBufferLineList::iterator	lineIterator;
	
	
	for (lineIterator = inDataPtr->screenBuffer.begin(); lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
	{
		std::fill(lineIterator->attributeVector.begin(), lineIterator->attributeVector.end(),
					kTerminalTextAttributesAllOff);
		std::fill(lineIterator->textVector.begin(), lineIterator->textVector.end(), ' ');
	}
}// bufferEraseWithoutUpdate


/*!
Inserts the specified number of blank characters at the
current cursor position, truncating that many characters
from the end of the line when the line is shifted.  The
display is NOT updated.

(2.6)
*/
static void
bufferInsertBlanksAtCursorColumn	(My_ScreenBufferPtr	inDataPtr,
									 SInt16				inNumberOfBlankCharactersToInsert)
{
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_TextVector::iterator				textIterator;
	My_TextVector::iterator				firstCharPastInsertedRangeIterator;
	My_TextAttributesList::iterator		attrIterator;
	My_TextAttributesList::iterator		firstAttrPastInsertedRangeIterator;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	// wrap cursor
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// find cursor position
	attrIterator = cursorLineIterator->attributeVector.begin();
	std::advance(attrIterator, 1 + postWrapCursorX);
	firstAttrPastInsertedRangeIterator = attrIterator;
	std::advance(firstAttrPastInsertedRangeIterator, inNumberOfBlankCharactersToInsert);
	textIterator = cursorLineIterator->textVector.begin();
	std::advance(textIterator, 1 + postWrapCursorX);
	firstCharPastInsertedRangeIterator = textIterator;
	std::advance(firstCharPastInsertedRangeIterator, inNumberOfBlankCharactersToInsert);
	
	// copy text backwards from the to-be-overwritten range to the end; note
	// that this is safe only because the destination range begins later than
	// the source
	std::copy_backward(attrIterator, firstAttrPastInsertedRangeIterator, cursorLineIterator->attributeVector.end());
	std::copy_backward(textIterator, firstCharPastInsertedRangeIterator, cursorLineIterator->textVector.end());
	
	// put blanks in the inserted space somewhere in the middle
	std::fill(attrIterator, firstAttrPastInsertedRangeIterator, cursorLineIterator->globalAttributes);
	std::fill(textIterator, firstCharPastInsertedRangeIterator, ' ');
}// bufferInsertBlanksAtCursorColumn


/*!
Inserts the specified number of blank lines, scrolling
the remaining ones down and dropping any that fall off
the end of the scrolling region.  The display is updated.

See also bufferRemoveLines().

NOTE:   You cannot use this to alter the scrollback.

(3.0)
*/
static void
bufferInsertBlankLines	(My_ScreenBufferPtr						inDataPtr,
						 UInt16									inNumberOfLines,
						 My_ScreenBufferLineList::iterator&		inInsertionLine)
{
	//Console_WriteValue("bufferInsertBlankLines: count", inNumberOfLines);
	
	// “insert” by moving the specified number of lines to the
	// beginning of the insertion region, and then clearing them out
	{
		My_ScreenBufferLineList::iterator	pastLastKeptLine;
		My_ScreenBufferLineList::iterator	scrollingRegionBegin;
		My_ScreenBufferLineList::iterator	scrollingRegionEnd;
		
		
		locateScrollingRegion(inDataPtr, scrollingRegionBegin, scrollingRegionEnd);
		
		pastLastKeptLine = scrollingRegionEnd;
		std::advance(pastLastKeptLine, -inNumberOfLines);
		std::rotate(inInsertionLine, pastLastKeptLine, scrollingRegionEnd);
		std::uninitialized_fill_n(scrollingRegionBegin, inNumberOfLines, My_ScreenBufferLine());
	}
	
	// add all the lines from the current line to the end
	// of the scrolling region to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: insert blank lines");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = std::distance(inDataPtr->screenBuffer.begin(), inInsertionLine);
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = inDataPtr->scrollingRegion.lastRow - range.firstRow + 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferInsertBlankLines


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
static void
bufferLineFill	(My_ScreenBufferPtr			UNUSED_ARGUMENT(inDataPtr),
				 My_ScreenBufferLine&		inRow,
				 UInt8						inFillCharacter,
				 TerminalTextAttributes		inFillAttributes,
				 Boolean					inUpdateLineGlobalAttributesAlso)
{
	std::fill(inRow.textVector.begin(), inRow.textVector.end(), inFillCharacter);
	std::fill(inRow.attributeVector.begin(), inRow.attributeVector.end(), inFillAttributes);
	if (inUpdateLineGlobalAttributesAlso)
	{
		inRow.globalAttributes = inFillAttributes;
	}
}// bufferLineFill


/*!
Deletes characters starting at the current cursor
position and moving forwards.  The rest of the line
is shifted to the left as a result.

(3.0)
*/
static void
bufferRemoveCharactersAtCursorColumn	(My_ScreenBufferPtr		inDataPtr,
										 SInt16					inNumberOfCharactersToDelete)
{
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_TextVector::iterator				textIterator;
	My_TextVector::iterator				firstCharPastDeletedRangeIterator;
	My_TextAttributesList::iterator		attrIterator;
	My_TextAttributesList::iterator		firstAttrPastDeletedRangeIterator;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	// wrap cursor
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// find cursor position
	attrIterator = cursorLineIterator->attributeVector.begin();
	std::advance(attrIterator, 1 + postWrapCursorX);
	firstAttrPastDeletedRangeIterator = attrIterator;
	std::advance(firstAttrPastDeletedRangeIterator, inNumberOfCharactersToDelete);
	textIterator = cursorLineIterator->textVector.begin();
	std::advance(textIterator, 1 + postWrapCursorX);
	firstCharPastDeletedRangeIterator = textIterator;
	std::advance(firstCharPastDeletedRangeIterator, inNumberOfCharactersToDelete);
	
	// copy text from end range to the earlier range; note that this is safe
	// only because the destination range begins earlier than the source
	std::copy(attrIterator, cursorLineIterator->attributeVector.end(), firstAttrPastDeletedRangeIterator);
	std::copy(textIterator, cursorLineIterator->textVector.end(), firstCharPastDeletedRangeIterator);
	
	// put blanks in the revealed space at the end
	std::fill(firstAttrPastDeletedRangeIterator, cursorLineIterator->attributeVector.end(),
				cursorLineIterator->globalAttributes);
	std::fill(firstCharPastDeletedRangeIterator, cursorLineIterator->textVector.end(), ' ');
	
	// add the entire line from the cursor to the end
	// to the text-change region; this would trigger
	// things like Terminal View updates
	//Console_WriteLine("text changed event: remove characters at cursor column");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - postWrapCursorX;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferRemoveCharactersAtCursorColumn


/*!
Deletes lines from the screen buffer, scrolling up the
remainder and inserting new blank lines at the bottom
of the scrolling region.  The display is updated.

See also bufferInsertBlankLines().

NOTE:   You cannot use this to alter the scrollback.

(3.0)
*/
static void
bufferRemoveLines	(My_ScreenBufferPtr						inDataPtr,
					 UInt16									inNumberOfLines,
					 My_ScreenBufferLineList::iterator&		inFirstDeletionLine)
{
	// “remove” by clearing out the specified number of lines at the
	// top and then moving them to the end of the scrolling region
	{
		My_ScreenBufferLineList::iterator	pastLastDeletionLine = inFirstDeletionLine;
		My_ScreenBufferLineList::iterator	scrollingRegionBegin;
		My_ScreenBufferLineList::iterator	scrollingRegionEnd;
		
		
		locateScrollingRegion(inDataPtr, scrollingRegionBegin, scrollingRegionEnd);
		
		std::advance(pastLastDeletionLine, inNumberOfLines);
		std::uninitialized_fill_n(inFirstDeletionLine, inNumberOfLines, My_ScreenBufferLine());
		std::rotate(inFirstDeletionLine, pastLastDeletionLine, scrollingRegionEnd);
	}
	
	// add all the lines from the current line to the end
	// of the scrolling region to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: remove lines");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = std::distance(inDataPtr->screenBuffer.begin(), inFirstDeletionLine);
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = inDataPtr->scrollingRegion.lastRow - range.firstRow + 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
	}
}// bufferRemoveLines


/*!
Internal version of Terminal_ChangeLineAttributes().

(3.0)
*/
static void
changeLineAttributes	(My_ScreenBufferPtr			inDataPtr,
						 My_ScreenBufferLine&		inRow,
						 TerminalTextAttributes		inSetTheseAttributes,
						 TerminalTextAttributes		inClearTheseAttributes)
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
static void
changeLineGlobalAttributes	(My_ScreenBufferPtr			inDataPtr,
							 My_ScreenBufferLine&		inRow,
							 TerminalTextAttributes		inSetTheseAttributes,
							 TerminalTextAttributes		inClearTheseAttributes)
{
	// first copy the attributes to every character in the line
	changeLineAttributes(inDataPtr, inRow, inSetTheseAttributes, inClearTheseAttributes);
	
	// now remember these changes so that the cursor can inherit them automatically;
	// currently attributes for scrollback lines do not exist, but negative indices
	// are allowed; to avoid array overflow, check for nonnegativity here
	inRow.globalAttributes &= ~inClearTheseAttributes;
	inRow.globalAttributes |= inSetTheseAttributes;
}// changeLineGlobalAttributes


/*!
Internal version of Terminal_ChangeLineRangeAttributes().

(3.0)
*/
static void
changeLineRangeAttributes	(My_ScreenBufferPtr			inDataPtr,
							 My_ScreenBufferLine&		inRow,
							 UInt16						inZeroBasedStartColumn,
							 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
							 TerminalTextAttributes		inSetTheseAttributes,
							 TerminalTextAttributes		inClearTheseAttributes)
{
	SInt16				pastTheEndColumn = (inZeroBasedPastTheEndColumnOrNegativeForLastColumn < 0)
											? inDataPtr->text.visibleScreen.numberOfColumnsAllocated
											: inZeroBasedPastTheEndColumnOrNegativeForLastColumn;
	register SInt16		i = 0;
	
	
	// update attributes for the specified columns of the given line
	for (i = inZeroBasedStartColumn; i < pastTheEndColumn; ++i)
	{
		inRow.attributeVector[i] &= ~inClearTheseAttributes;
		inRow.attributeVector[i] |= inSetTheseAttributes;
	}
	
	// update current attributes too, if the cursor is in the given range
	if ((inZeroBasedStartColumn <= inDataPtr->current.cursorX) && (inDataPtr->current.cursorX < pastTheEndColumn))
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		if (inRow == *cursorLineIterator)
		{
			inDataPtr->current.attributeBits &= ~inClearTheseAttributes;
			inDataPtr->current.attributeBits |= inSetTheseAttributes;
			
			// ...however, do not propagate text highlighting to text rendered from now on
			inDataPtr->current.attributeBits &= ~kTerminalTextAttributeSelected;
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
static void
changeNotifyForTerminal		(My_ScreenBufferConstPtr	inPtr,
							 Terminal_Change			inWhatChanged,
							 void*						inContextPtr)
{
	// invoke listener callback routines appropriately, from the specified terminal’s listener model
	ListenerModel_NotifyListenersOfEvent(inPtr->changeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyForTerminal


/*!
Responds to a CSI (control sequence inducer) by reinitializing
all parameter values and resetting the current parameter index
to 0.

(3.0)
*/
static void
clearEscapeSequenceParameters	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		parameterIndex = kMy_MaximumANSIParameters;
	
	
	while (--parameterIndex >= 0)
	{
		inDataPtr->currentEscapeSeqParamValues[parameterIndex] = -1;
	}
	inDataPtr->currentEscapeSeqParamEndIndex = 0;
}// clearEscapeSequenceParameters


/*!
Restores the last saved cursor position and
attribute settings.  See cursorSave() to ensure
this function does the opposite.

(2.6)
*/
static void
cursorRestore  (My_ScreenBufferPtr		inPtr)
{
	moveCursor(inPtr, inPtr->previous.cursorX, inPtr->previous.cursorY);
	
	// ??? this will clear the current graphics character set too, is that supposed to happen ???
	inPtr->current.attributeBits = inPtr->previous.attributeBits;
}// cursorRestore


/*!
Saves the current cursor position and attribute
settings.  See cursorRestore() to ensure this
function does the opposite.

(2.6)
*/
static void
cursorSave  (My_ScreenBufferPtr  inPtr)
{
	inPtr->previous.cursorX = inPtr->current.cursorX;
	inPtr->previous.cursorY = inPtr->current.cursorY;
	inPtr->previous.attributeBits = inPtr->current.attributeBits;
}// cursorSave


/*!
Returns the cursor location, first wrapping it to the
following line if it is out of bounds (if a wrap is
pending).

(3.0)
*/
static void
cursorWrapIfNecessaryGetLocation	(My_ScreenBufferPtr		inDataPtr,
									 SInt16*				outCursorXPtr,
									 My_ScreenRowIndex*		outCursorYPtr)
{
	if (inDataPtr->current.cursorX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted) 
	{
		moveCursorX(inDataPtr, 0);
		moveCursorDownOrScroll(inDataPtr);
	}
	*outCursorXPtr = inDataPtr->current.cursorX;
	*outCursorYPtr = inDataPtr->current.cursorY;
}// cursorWrapIfNecessaryGetLocation


/*!
Treats the specified block of characters as “verbatim”,
sending them wherever they need to go (any open print
jobs or capture files, the terminal, etc.).

(3.1)
*/
static void
echoData	(My_ScreenBufferPtr		inDataPtr,
			 UInt8 const*			inBuffer,
			 UInt32					inLength)
{
	// send to printer, if a job is in progress
	if (inDataPtr->printing.enabled)
	{
		SInt32		bytesToPrintBytesPrinted = inLength;
		
		
		TelnetPrinting_Spool(&inDataPtr->printing, inBuffer, &bytesToPrintBytesPrinted);
	}
	
	// append to capture file, if one is open
	Terminal_FileCaptureWriteData(inDataPtr->selfRef, inBuffer/* data to write */, inLength/* buffer length */);
	
	// add each character to the terminal at the current
	// cursor position, advancing the cursor each time
	// (and therefore being mindful of wrap settings, etc.)
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		register SInt16						preWriteCursorX = inDataPtr->current.cursorX;
		register My_ScreenRowIndex			preWriteCursorY = inDataPtr->current.cursorY;
		
		
		// WARNING: This is done once here, for efficiency, and is only
		//          repeated below if the cursor actually moves vertically
		//          (as evidenced by some moveCursor...() call that would
		//          affect the cursor row).  Keep this in sync!!!
		locateCursorLine(inDataPtr, cursorLineIterator);
		for (UInt8 const* bufferIterator = inBuffer; bufferIterator != (inBuffer + inLength); ++bufferIterator)
		{
			// debug
			//Console_WriteValueCharacter("echo", *bufferIterator);
			
			// write characters on a single line
			if (inDataPtr->modeInsertNotReplace)
			{
				bufferInsertBlanksAtCursorColumn(inDataPtr, 1/* number of blank characters */);
			}
			cursorLineIterator->textVector[inDataPtr->current.cursorX] = translateCharacter(inDataPtr, *bufferIterator);
			cursorLineIterator->attributeVector[inDataPtr->current.cursorX] = inDataPtr->current.attributeBits;
			if (inDataPtr->current.cursorX < (inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1))
			{
				// advance the cursor position
				moveCursorRight(inDataPtr);
			}
			else
			{
				// hit right margin
				
				// move the cursor appropriately, and reset state if necessary
				if (inDataPtr->modeAutoWrap)
				{
					//Console_WriteLine("wrap");
					
					// autowrap to start of next line
					moveCursorLeftToEdge(inDataPtr);
					moveCursorDownOrScroll(inDataPtr);
					locateCursorLine(inDataPtr, cursorLineIterator); // cursor changed rows...
					
					// reset column tracker
					preWriteCursorX = 0;
				}
				else
				{
					//Console_WriteLine("nowrap");
					
					// stay at right margin
					moveCursorRightToEdge(inDataPtr);
				}
			}
		}
		
		// end of data; notify of a change (this will cause things like Terminal View updates)
		{
			// add the new line of text to the text-change region;
			// this should trigger things like Terminal View updates
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = inDataPtr->current.cursorY;
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
				range.columnCount = inDataPtr->current.cursorX - preWriteCursorX + 1;
				range.rowCount = 1;
			}
			//Console_WriteValuePair("text changed event: add data starting at row, column", range.firstRow, range.firstColumn);
			//Console_WriteValuePair("text changed event: add data for #rows, #columns", range.rowCount, range.columnCount);
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
		}
	}
}// echoData


/*!
Old terminal emulator implementation.  (Ugly, huh?)
MacTelnet 3.1 finally scraps this implementation and
moves to the fast, flexible and correct-by-construction
callback-based mechanism.  For more information, see
Terminal_EmulatorProcessData().

(2.6)
*/
static void
emulatorFrontEndOld	(My_ScreenBufferPtr		inDataPtr,
					 UInt8 const*			inBuffer,
					 SInt32					inLength)
{
	enum
	{
		ASCII_ESC	= 0x1B		//!< escape character
	};
	register SInt16				escflg = 0;//inDataPtr->current.escapeSequence.level;
    SInt32						ctr = inLength; // 3.0 - use a COPY so string length parameter’s original value isn’t lost!!!
	UInt8 const*				c = inBuffer;
	My_CharacterSetInfoPtr		characterSetInfoPtr = nullptr; // which character set (G0 or G1) is being modified; used only for shift-in, -out
	
	
	while (ctr > 0)
	{
		if (inDataPtr->printing.enabled)
		{
			TelnetPrinting_Spool(&inDataPtr->printing, c, &ctr/* count can decrease, depending on what is printed */);
		}
		
		while ((escflg == 0) && (ctr > 0) && (*c < 32))
		{
			switch (*c)
			{
			case 0x07: // control-G; sound bell
				unless (inDataPtr->bellDisabled)
				{
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeAudioEvent, inDataPtr->selfRef/* context */);
				}
				break;
			
			case 0x08: // control-H; backspace
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr); // do not extend past margin
				break;
			
			case 0x09: // control-I; horizontal tab
				moveCursorRightToNextTabStop(inDataPtr);
				Terminal_FileCaptureWriteData(inDataPtr->selfRef, c, 1);				
				break;
			
			case 0x0A: // control-J; line feed
			case 0x0B: // control-K; vertical tab
			case 0x0C: // control-L; form feed
				// all of these are interpreted the same for VT100;
				// if LNM was received, this is a regular line feed,
				// otherwise it is actually a new-line operation
				moveCursorDownOrScroll(inDataPtr);
				if (inDataPtr->modeNewLineOption) moveCursorLeftToEdge(inDataPtr);
				break;
			
			case 0x0D: // control-M; carriage return
				moveCursorLeftToEdge(inDataPtr);
				Terminal_FileCaptureWriteData(inDataPtr->selfRef, c, 1);				
				break;
			
			case 0x0E: // control-N; shift-out
				inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG1;
				if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
				{
					// set attribute
					inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics;
				}
				else
				{
					// clear attribute
					inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics;
				}
				break;
			
			case 0x0F: // control-O; shift-in
				inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
				if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
				{
					// set attribute
					inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics;
				}
				else
				{
					// clear attribute
					inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics;
				}
				break;
			
			case 0x13: // control-S; resume transmission (XON)
				break;
			
			case 0x15: // control-U; stop transmission (XOFF) except for XON and XOFF
				break;
			
			case ASCII_ESC: // escapes are significant
				++escflg;
				break;
			
			default:
				// ignore
				break;
			} 
			++c;
			--ctr;
		} 
		if ((escflg == 0) &&
			(ctr > 0) &&
			(*c & 0x80) &&
			(*c < 0xA0) &&
			(inDataPtr->emulation == kTerminal_EmulatorVT220)) // VT220 eightbit starts here
		{											
			switch (*c)								
			{									
			case 0x84: /* ind */		//same as ESC D
				moveCursorDownOrScroll(inDataPtr);		
				goto ShortCut;			
			
			case 0x85: /* nel */		//same as ESC E
				moveCursorLeftToEdge(inDataPtr);
				moveCursorDownOrScroll(inDataPtr);				
				goto ShortCut;			
			
			case 0x88: /* hts */		//same as ESC H 
				inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;	
				goto ShortCut;	
			
			case 0x8D: /* ri */			//same as ESC M
				moveCursorUpOrScroll(inDataPtr);
				goto ShortCut;
			
			case 0x9B: /* csi */		//same as ESC [ 
				clearEscapeSequenceParameters(inDataPtr);			
				escflg = 2;			
				++c;			//CCP			
				--ctr;					
				break;						
			
			case 0x86: /* ssa */			// - same as ESC F */
			case 0x87: /* esa */			// - same as ESC G */
			case 0x8E: /* ss2 */			// - same as ESC N */
			case 0x8F: /* ss3 */			// - same as ESC O */
			case 0x90: /* dcs */			// - same as ESC P */
			case 0x93: /* sts */			// - same as ESC S */
			case 0x96: /* spa */			// - same as ESC V */
			case 0x97: /* epa */			// - same as ESC W */
			case 0x9D: /* osc */			// - same as ESC ] */
			case 0x9E: /* pm */				// - same as ESC ^ */
			case 0x9F: /* apc */			// - same as ESC _ */
			default:
				goto ShortCut;				
			} 					
		}// end if vt220
		
		{
			char*								startPtr = nullptr;
			My_TextVector::iterator				startIterator;
			TerminalTextAttributes				attrib = 0;
			register SInt16						preWriteCursorX = 0;
			SInt16								extra = 0;
			Boolean								wrapped = false;
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			// WARNING: This is done once here, for efficiency, and is only
			//			repeated below if the cursor actually moves vertically
			//			(as evidenced by some moveCursor...() call that would
			//			affect the cursor row).  Keep this in sync!!!
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			while ((escflg == 0) &&
					(ctr > 0) &&
					(*c >= 32) &&
					!((*c & 0x80) && (*c < 0xA0) && (inDataPtr->emulation == kTerminal_EmulatorVT220)))
			{
				// print lines of text one at a time
				attrib = inDataPtr->current.attributeBits; /* current writing attribute */
				wrapped = false; /* wrapped to next line (boolean) */
				extra = 0; /* overwriting last character of line  */
				
				if (inDataPtr->current.cursorX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
				{
					if (inDataPtr->modeAutoWrap)
					{
						// wrap to next line 
						moveCursorLeftToEdge(inDataPtr);
						moveCursorDownOrScroll(inDataPtr);
						locateCursorLine(inDataPtr, cursorLineIterator); // cursor changed rows...
					}
					else
					{
						// stay at right margin
						moveCursorX(inDataPtr, inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1);
					}
				}
				
				startPtr = &(cursorLineIterator->textVector[inDataPtr->current.cursorX]);
				startIterator = cursorLineIterator->textVector.begin();
				std::advance(startIterator, inDataPtr->current.cursorX);
				preWriteCursorX = inDataPtr->current.cursorX;
				while ((ctr > 0) &&
						(*c >= 32) &&
						(!wrapped) &&
						!((*c & 0x80) && (*c < 0xA0) && (inDataPtr->emulation == kTerminal_EmulatorVT220)))
				{
					// write characters on a single line
					if (inDataPtr->modeInsertNotReplace)
					{
						bufferInsertBlanksAtCursorColumn(inDataPtr, 1/* number of blank characters */);
					}
					cursorLineIterator->textVector[inDataPtr->current.cursorX] = translateCharacter(inDataPtr, *c);
					cursorLineIterator->attributeVector[inDataPtr->current.cursorX] = attrib;
					++c;
					--ctr;
					if (inDataPtr->current.cursorX < (inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1))
					{
						// advance the cursor position
						moveCursorRight(inDataPtr);
					}
					else
					{
						// hit right margin
						if (inDataPtr->modeAutoWrap)
						{
							// autowrap to start of next line (past-the-end cursor position triggers this)
							moveCursorRight(inDataPtr);
							wrapped = true; // terminate inner loop 
						}
						else
						{
							// stay at right margin
							moveCursorRightToEdge(inDataPtr);
							extra = 1; // cursor position doesn’t advance 
						} 
					} 
				}
				
				// line has terminated, or invisible character arrived - update the terminal view(s)
				// to display the printable characters received in the preceding loop
				extra += inDataPtr->current.cursorX - preWriteCursorX;
				if (*c == '\015')
				{
					// add the new line of text to the text-change region;
					// this should trigger things like Terminal View updates
					Terminal_RangeDescription	range;
					
					
					//Console_WriteValue("text changed event: add new line, terminated by character", *c);
					range.screen = inDataPtr->selfRef;
					range.firstRow = inDataPtr->current.cursorY;
					range.firstColumn = preWriteCursorX;
					range.columnCount = extra;
					range.rowCount = 1;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
				}
				
				Terminal_FileCaptureWriteData(inDataPtr->selfRef, (UInt8*)startPtr/* data to write */, extra/* buffer length */);
				
			#if 0
				// 3.0 - test speech (this implementation will be greatly enhanced in the near future
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
					
					default:
						doSpeak = false;
						break;
					}
					
					if (doSpeak)
					{
						register SInt16				i = 0;
						Str255&						buffer = inDataPtr->speech.buffer;
						My_TextVector::iterator		speechIterator = startIterator;
						
						
						// accumulate characters in a buffer; the buffer keeps all
						// unspoken text, and is cleared (and speech occurs) as
						// soon as it is full, or when a new line is encountered
						for (i = 0; i < extra; ++i, ++speechIterator)
						{
							*(buffer + buffer[0]) = *speechIterator; // append received character to spoken string
							++buffer[0]; // fix string length
							if ((buffer[0] == 255) || (*speechIterator == '\r'))
							{
								TerminalSpeaker_ResultCode	speakerResult = kTerminalSpeaker_ResultCodeSuccess;
								
								
								// TEMPORARY - spin lock, to keep asynchronous speech from jumbling multi-line text;
								//             this really should be changed to use speech callbacks instead
								do
								{
									// stop and speak when a new line is found, or
									// when there is just no more room for characters
									speakerResult = TerminalSpeaker_SynthesizeSpeechFromBuffer(inDataPtr->speaker, 1 + buffer/* buffer */,
																								buffer[0]/* buffer size */);
								} while (speakerResult == kTerminalSpeaker_ResultCodeSpeechSynthesisTryAgain);
								buffer[0] = '\0'; // clear string
							}
						}
					}
				}
			#endif
			}/* while */
		}
		
		// 3.0 - absorb any number of consecutive escape characters, they are meaningless
		while ((escflg == 1) && (*c == ASCII_ESC) && (ctr > 0))
		{
			++c;
			--ctr;
		}
		
		while ((escflg == 1) && (ctr > 0))
		{
			// Basic escape sequence processing.  This indicates that an isolated
			// ESC character (for a new sequence) has been read, but nothing else.
			//
			// IMPORTANT: Be sure to consult "inDataPtr->modeANSIEnabled" for each
			//            of the following, because ANSI and VT52-compatible modes
			//            are combined in the switch below.  Do not respond to ANSI
			//            sequences while in VT52 mode, and vice-versa.
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case '[':
				if (inDataPtr->modeANSIEnabled)
				{
					// CSI (control sequence inducer)
					clearEscapeSequenceParameters(inDataPtr);
					++escflg;
				}
				break;
			
			case ']':
				if (inDataPtr->modeANSIEnabled)
				{
					//if (ScreenFactory_GetConnectionDataOf(inDataPtr->selfRef)->Xterm) escflg = 6;
					escflg = 6; // TEMP
				}
				else
				{
					// print screen
					Commands_ExecuteByID(kCommandPrintScreen);
				}
				break;
			
			case '7':
				if (inDataPtr->modeANSIEnabled) cursorSave(inDataPtr);
				goto ShortCut;				
			
			case '8':
				if (inDataPtr->modeANSIEnabled) cursorRestore(inDataPtr);
				goto ShortCut;				
			
			case 'c':
				if (inDataPtr->modeANSIEnabled) resetTerminal(inDataPtr);
				break;
			
			case 'A':
				unless (inDataPtr->modeANSIEnabled) vt100CursorUp_vt52(inDataPtr);
				goto ShortCut;
			
			case 'B':
				unless (inDataPtr->modeANSIEnabled) vt100CursorDown_vt52(inDataPtr);
				goto ShortCut;
			
			case 'C':
				unless (inDataPtr->modeANSIEnabled) vt100CursorForward_vt52(inDataPtr);
				goto ShortCut;
			
			case 'D':
				if (inDataPtr->modeANSIEnabled) moveCursorDownOrScroll(inDataPtr);
				else vt100CursorBackward_vt52(inDataPtr);
				goto ShortCut;				
			
			case 'E':
				if (inDataPtr->modeANSIEnabled)
				{
					moveCursorLeftToEdge(inDataPtr);
					moveCursorDownOrScroll(inDataPtr);
				}
				goto ShortCut;
			
			case 'F':
				unless (inDataPtr->modeANSIEnabled) { /* enter graphics mode; unimplemented */ }
				goto ShortCut;
			
			case 'G':
				unless (inDataPtr->modeANSIEnabled) { /* exit graphics mode; unimplemented */ }
				goto ShortCut;
			
			case 'H':
				if (inDataPtr->modeANSIEnabled) inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;
				else moveCursor(inDataPtr, 0, 0); // home cursor in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'I':
				unless (inDataPtr->modeANSIEnabled) moveCursorUpOrScroll(inDataPtr); // reverse line feed in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'J':
				unless (inDataPtr->modeANSIEnabled) bufferEraseFromCursorToEnd(inDataPtr); // erase to end of screen, in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'K':
				unless (inDataPtr->modeANSIEnabled) bufferEraseFromCursorColumnToLineEnd(inDataPtr); // erase to end of line, in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'M':
				if (inDataPtr->modeANSIEnabled) moveCursorUpOrScroll(inDataPtr);
				goto ShortCut;
			
			case 'V':
				// print cursor line
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case 'W':
				// enter printer controller mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case 'X':
				// exit printer controller mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case 'Y':
				unless (inDataPtr->modeANSIEnabled)
				{
					// direct cursor address in VT52 compatibility mode of VT100;
					// new cursor position is encoded as the next two characters
					// (vertical first, then horizontal) offset by the octal
					// value 037 (equal to decimal 31)
					SInt16				newX = 0;
					My_ScreenRowIndex	newY = 0;
					
					
					newY = *(++c) - 32/* - 31 - 1 to convert from one-based to zero-based */;
					if (--ctr > 0)
					{
						newX = *(++c) - 32/* - 31 - 1 to convert from one-based to zero-based */;
						--ctr;
					}
					
					// constrain the value and then change it safely
					if (newX < 0) newX = 0;
					if (newX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
					{
						newX = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
					}
					//if (newY < 0) newY = 0;
					if (newY >= inDataPtr->screenBuffer.size())
					{
						newY = inDataPtr->screenBuffer.size() - 1;
					}
					moveCursor(inDataPtr, newX, newY);
				}
				goto ShortCut;
			
			case 'Z':
				if (inDataPtr->modeANSIEnabled) vt100DeviceAttributes(inDataPtr);
				else vt100Identify_vt52(inDataPtr);
				goto ShortCut;
			
			case '=':
				unless (inDataPtr->modeANSIEnabled) inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (application key sequences)
				goto ShortCut;
			
			case '>':
				unless (inDataPtr->modeANSIEnabled) inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (regular keypad characters)
				goto ShortCut;
			
			case '<':
				// enter ANSI mode (from VT52-compatible mode)
				unless (inDataPtr->modeANSIEnabled) inDataPtr->modeANSIEnabled = true;
				goto ShortCut;
			
			case '^':
				// enter auto-print mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case '_':
				// exit auto-print mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case ' ':
			case '*':
			case '#':
				if (inDataPtr->modeANSIEnabled) escflg = 3;
				break;
			
			case '(':
				if (inDataPtr->modeANSIEnabled)
				{
					characterSetInfoPtr = &inDataPtr->vtG0;
					escflg = 4;
				}
				break;
			
			case ')':
				if (inDataPtr->modeANSIEnabled)
				{
					characterSetInfoPtr = &inDataPtr->vtG1;
					escflg = 4;
				}
				break;
			
			default:
				goto ShortCut;				
			}
			++c;
			--ctr;
		}/* while */
		
		while ((escflg == 2) && (ctr > 0))
		{
			// “control sequence” processing
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				// parse numeric parameter
				{
					// rename this incredibly long expression, since it’s needed a lot here!
					SInt16&		valueRef = inDataPtr->currentEscapeSeqParamValues
											[inDataPtr->currentEscapeSeqParamEndIndex];
					
					
					if (valueRef < 0) valueRef = 0;
					valueRef *= 10;
					valueRef += *c - '0';
				}
				break;
			
		#if 0
			case '=':
				// SCO-ANSI
				escflg = 9;
				break;
		#endif
			
			case '?':
				// DEC-private control sequence (see 'h' and 'l', that respectively do mode set/reset)
				inDataPtr->currentEscapeSeqParamValues[(inDataPtr->currentEscapeSeqParamEndIndex)++] = -2;
				break;
			
			case ';':
				// parameter separator
				++(inDataPtr->currentEscapeSeqParamEndIndex);
				break;
			
			case 'A':
				vt100CursorUp(inDataPtr);
				goto ShortCut;				
			
			case 'B':
				vt100CursorDown(inDataPtr);
				goto ShortCut;				
			
			case 'C':
				vt100CursorForward(inDataPtr);
				goto ShortCut;
			
			case 'D':
				vt100CursorBackward(inDataPtr);
				goto ShortCut;
			
			case 'f':
			case 'H':
				// absolute cursor positioning
				{
					SInt16				newX = (inDataPtr->currentEscapeSeqParamValues[1] != -1)
												? inDataPtr->currentEscapeSeqParamValues[1] - 1
												: 0/* default is home */;
					My_ScreenRowIndex	newY = (inDataPtr->currentEscapeSeqParamValues[0] != -1)
												? inDataPtr->currentEscapeSeqParamValues[0] - 1
												: 0/* default is home */;
					
					
					// in origin mode, offset according to the scrolling region
					if (inDataPtr->modeOriginRedefined) newY += inDataPtr->scrollingRegion.firstRow;
					
					// constrain the value and then change it safely
					if (newX < 0) newX = 0;
					if (newX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
					{
						newX = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
					}
					//if (newY < 0) newY = 0;
					if (newY >= inDataPtr->screenBuffer.size())
					{
						newY = inDataPtr->screenBuffer.size() - 1;
					}
					moveCursor(inDataPtr, newX, newY);
				}
				goto ShortCut;				
		   
			case 'i': // media copy
				if (inDataPtr->currentEscapeSeqParamValues[inDataPtr->currentEscapeSeqParamEndIndex] == 5)
				{		
					TelnetPrinting_Begin(&inDataPtr->printing);
				}
				escflg = 0;
				break;
			
			case 'J':
				vt100EraseInDisplay(inDataPtr);
				goto ShortCut;
			
			case 'K':
				vt100EraseInLine(inDataPtr);
				goto ShortCut;
			
			case 'm':
				// ANSI colors and other character attributes
				{
					SInt16		i = 0;
					
					
					while (i <= inDataPtr->currentEscapeSeqParamEndIndex)
					{
						SInt16		p = 0;
						
						
						if (inDataPtr->currentEscapeSeqParamValues[inDataPtr->currentEscapeSeqParamEndIndex] < 0)
						{
							inDataPtr->currentEscapeSeqParamValues[inDataPtr->currentEscapeSeqParamEndIndex] = 0;
						}
						
						p = inDataPtr->currentEscapeSeqParamValues[i];
						
						// Note that a real VT100 will only understand 0-7 here.
						// Other values are basically recognized because they are
						// compatible with VT100 and are very often used (ANSI
						// colors in particular).
						if (p == 0) inDataPtr->current.attributeBits &= 0xFFFF0000; // all style bits off
						else if (p < 9) inDataPtr->current.attributeBits |= styleOfVTParameter(p); // set attribute
						else if (p == 10) { /* set normal font - unsupported */ }
						else if (p == 11) { /* set alternate font - unsupported */ }
						else if (p == 12) { /* set alternate font, shifting by 128 - unsupported */ }
						else if (p == 22) inDataPtr->current.attributeBits &= ~styleOfVTParameter(1); // clear bold (oddball - 22, not 21)
						else if ((p > 22) && (p < 29)) inDataPtr->current.attributeBits &= ~styleOfVTParameter(p - 20); // clear attribute
						else
						{
							if ((p >= 30) && (p < 38))
							{
								inDataPtr->current.attributeBits = (inDataPtr->current.attributeBits & ~0x0700) | ((p - 30) << 8) | 0x0800;
							}
							else if ((p >= 40) && (p < 48))
							{
								inDataPtr->current.attributeBits = (inDataPtr->current.attributeBits & ~0x7000) | ((p - 40) << 12) | 0x8000;
							}
						}
						++i;
					}
				}
				goto ShortCut;
			
			case 't':
				// XTerm window modification and state reporting
				{
					SInt16 const&	instruction = inDataPtr->currentEscapeSeqParamValues[0];
					
					
					switch (instruction)
					{
					case 1:
						// de-iconify window
						inDataPtr->windowMinimized = false;
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowMinimization, inDataPtr->selfRef/* context */);
						break;
					
					case 2:
						// iconify window
						inDataPtr->windowMinimized = true;
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowMinimization, inDataPtr->selfRef/* context */);
						break;
					
					default:
						// SEE DISABLED BELOW
						break;
					}
				}
			#if 0 // TEMP
				{
					WindowRef		window = TerminalView_GetWindow(inDataPtr->view);
					SInt16 const&	instruction = inDataPtr->currentEscapeSeqParamValues[0];
					
					
					// determine which modifier was received
					switch (instruction)
					{
					//case 1 above
					//case 2 above
					
					case 3:
						// move window (X, Y coordinates are 2nd and 3rd parameters, respectively);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->currentEscapeSeqParamEndIndex == 2)
						{
							SInt16		pixelLeft = inDataPtr->currentEscapeSeqParamValues[1];
							SInt16		pixelTop = inDataPtr->currentEscapeSeqParamValues[2];
							Rect		screenBounds;
							
							
							// first, constrain the values to something reasonable;
							// arbitrarily, use the window positioning boundaries as
							// defined by the Mac OS (excludes the Dock and menu bar,
							// etc.)
							RegionUtilities_GetPositioningBounds(window, &screenBounds);
							if (pixelLeft < screenBounds.left) pixelLeft = screenBounds.left;
							if (pixelLeft > screenBounds.right) pixelLeft = screenBounds.right;
							if (pixelTop < screenBounds.top) pixelTop = screenBounds.top;
							if (pixelTop > screenBounds.bottom) pixelTop = screenBounds.bottom;
							
							// now move the window
							MoveWindow(window, pixelLeft, pixelTop, false/* activate */);
						}
						break;
					
					case 4:
						// disabled due to instability
					#if 0
						// resize terminal area (width and height in *pixels* are 2nd and 3rd parameters, respectively);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->currentEscapeSeqParamEndIndex == 2)
						{
							SInt16		pixelWidth = inDataPtr->currentEscapeSeqParamValues[1];
							SInt16		pixelHeight = inDataPtr->currentEscapeSeqParamValues[2];
							UInt16		columns = 0;
							UInt16		rows = 0;
							
							
							// first, constrain the values to something reasonable;
							// arbitrarily, use the window positioning boundaries as
							// defined by the Mac OS (excludes the Dock and menu bar,
							// etc.)
							{
								Rect	screenBounds;
								
								
								RegionUtilities_GetPositioningBounds(window, &screenBounds);
								if (pixelWidth < 10) pixelWidth = 32; // arbitrary
								if (pixelHeight < 10) pixelHeight = 32; // arbitrary
								if (pixelWidth > (screenBounds.right - screenBounds.left))
								{
									pixelWidth = (screenBounds.right - screenBounds.left);
								}
								if (pixelHeight > (screenBounds.bottom - screenBounds.top))
								{
									pixelHeight = (screenBounds.bottom - screenBounds.top);
								}
							}
							
							// determine the closest terminal size to the requested pixel width and height
							TerminalView_GetTheoreticalScreenDimensions(inDataPtr->view, pixelWidth, pixelHeight,
																		false/* include insets */, &columns, &rows);
							Terminal_SetVisibleColumnCount(inDataPtr->selfRef, columns);
							Terminal_SetVisibleRowCount(inDataPtr->selfRef, rows);
						}
					#endif
						break;
					
					case 5:
						// raise (activate) window
						EventLoop_SelectBehindDialogWindows(window);
						break;
					
					case 6:
						// lower window (become last in Z-order)
						SendBehind(window, kLastWindowOfGroup);
						break;
					
					case 7:
						// refresh (update) window
						RegionUtilities_RedrawWindowOnNextUpdate(window); // TEMPORARY; this changes too much (inefficient)
						break;
					
					case 8:
						// resize terminal area (width and height in *characters* are 2nd and 3rd parameters, respectively);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->currentEscapeSeqParamEndIndex == 2)
						{
							SInt16	columns = inDataPtr->currentEscapeSeqParamValues[1];
							SInt16  rows = inDataPtr->currentEscapeSeqParamValues[2];
							
							
							// first, constrain the values to something reasonable;
							// arbitrarily, each value must be at least 1
							if (columns < 1) columns = 1;
							if (columns > inDataPtr->text.visibleScreen.numberOfColumnsAllocated)
							{
								columns = inDataPtr->text.visibleScreen.numberOfColumnsAllocated;
							}
							if (rows < 1) rows = 1;
							if (rows > 64/* arbitrary */) rows = 64;
							
							// now resize the screen
							Terminal_SetVisibleColumnCount(inDataPtr->selfRef, columns);
							Terminal_SetVisibleRowCount(inDataPtr->selfRef, rows);
						}
						break;
					
					case 9:
						// zoom window (2nd parameter of 0 means return from maximized state, parameter of 1 means Maximize);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->currentEscapeSeqParamEndIndex == 1)
						{
							//Boolean		maximize = (inDataPtr->currentEscapeSeqParamValues[1] == 1);
							//WindowPartCode	inOrOut = (inDataPtr->currentEscapeSeqParamValues[1] == 0)
							//								? inZoomOut : inZoomIn;
							
							
							EventLoop_HandleZoomEvent(window);
						}
						break;
					
					case 11:
						// report window state; <CSI>1t if normal, <CSI>2t if iconified
						{
							SessionRef		session = returnListeningSession(inDataPtr);
							
							
							if (nullptr != session)
							{
								if (IsWindowCollapsed(window)) Session_SendData(session, "\033[2t", 4);
								else Session_SendData(session, "\033[1t", 4);
							}
						}
						break;
					
					case 10:
					case 12:
					case 13:
					case 14:
					case 15:
					case 16:
					case 17:
					case 18:
					case 19:
					case 20:
					case 21:
					case 22:
					case 23:
						// reporting function that is not currently handled
						break;
					
					default:
						// any number greater than or equal to 24 is a request to
						// change the number of terminal lines to that amount
						Terminal_SetVisibleRowCount(inDataPtr->selfRef, inDataPtr->currentEscapeSeqParamValues[0]);
						break;
					}
				}
			#endif // TEMP
				goto ShortCut;
			
			case 'q':
				vt100LoadLEDs(inDataPtr);
				goto ShortCut;
			
			case 'c':
				if (inDataPtr->emulation == kTerminal_EmulatorVT220) vt220DeviceAttributes(inDataPtr);
				else if (inDataPtr->emulation == kTerminal_EmulatorVT100) vt100DeviceAttributes(inDataPtr);
				goto ShortCut;
			
			case 'n':
				vt100DeviceStatusReport(inDataPtr);
				goto ShortCut;
			
			case 'L':
				{
					My_ScreenBufferLineList::iterator	cursorLineIterator;
					
					
					locateCursorLine(inDataPtr, cursorLineIterator);
					if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
					{
						inDataPtr->currentEscapeSeqParamValues[0] = 1;
					}
					bufferInsertBlankLines(inDataPtr, inDataPtr->currentEscapeSeqParamValues[0], cursorLineIterator);
				}
				goto ShortCut;				
			
			case 'M':
				{
					My_ScreenBufferLineList::iterator	cursorLineIterator;
					
					
					locateCursorLine(inDataPtr, cursorLineIterator);
					if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
					{
						inDataPtr->currentEscapeSeqParamValues[0] = 1;
					}
					bufferRemoveLines(inDataPtr, inDataPtr->currentEscapeSeqParamValues[0], cursorLineIterator);
				}
				goto ShortCut;
			
			case '@':
				if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
					inDataPtr->currentEscapeSeqParamValues[0] = 1;
				bufferInsertBlanksAtCursorColumn(inDataPtr, inDataPtr->currentEscapeSeqParamValues[0]/* number of blank characters */);
				goto ShortCut;
			
			case 'P':
				if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
					inDataPtr->currentEscapeSeqParamValues[0] = 1;
				bufferRemoveCharactersAtCursorColumn(inDataPtr, inDataPtr->currentEscapeSeqParamValues[0]);
				goto ShortCut;				
			
			case 'r':
				vt100SetTopAndBottomMargins(inDataPtr);
				goto ShortCut;
			
			case 'h':
			  	// set mode
				vt100ModeSetReset(inDataPtr, true/* set */);
				goto ShortCut;				
			
			case 'l':
				// reset mode
				vt100ModeSetReset(inDataPtr, false/* set */);
				goto ShortCut;				
			
			case 'g':
				if (inDataPtr->currentEscapeSeqParamValues[0] == 3)
				  /* clear all tabs */
					tabStopClearAll(inDataPtr);
				else if (inDataPtr->currentEscapeSeqParamValues[0] <= 0)
				  /* clear tab at current position */
					inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabClear;
				goto ShortCut;				
			
			case '!':
			case '\'':
			case '\"':
				++escflg;					
				break;						
			
			default:			/* Dang blasted strays... */
				goto ShortCut;				
			}
			++c;
			--ctr;
		}/* while */

		while ((escflg == 3) && (ctr > 0))
		{
			// "#" handling
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case '3': // double width and height, top half
				
				goto ShortCut;
			
			case '4': // double width and height, bottom half
				
				goto ShortCut;
			
			case '5': // normal
				
				goto ShortCut;
			
			case '6': // double width, normal height
				
				goto ShortCut;
			
			case '8': // alignment display
				goto ShortCut;
			
			default:
				goto ShortCut;
			}
			++c;
			--ctr;
		}/* while */

		while ((escflg == 4) && (ctr > 0) && (characterSetInfoPtr != nullptr))
		{
			// "(" or ")" handling (respectively, selection of G0 or G1 character sets)
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case 'A':
				// U.K. character set, normal ROM, no graphics
				characterSetInfoPtr->translationTable = kMy_CharacterSetVT100UnitedKingdom;
				characterSetInfoPtr->source = kMy_CharacterROMNormal;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
				inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics; // clear graphics attribute
				goto ShortCut;
			
			case 'B':
				// U.S. character set, normal ROM, no graphics
				characterSetInfoPtr->translationTable = kMy_CharacterSetVT100UnitedStates;
				characterSetInfoPtr->source = kMy_CharacterROMNormal;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
				inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics; // clear graphics attribute
				goto ShortCut;
			
			case '0':
				// normal ROM, graphics mode
				characterSetInfoPtr->source = kMy_CharacterROMNormal;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOn;
				inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics; // set graphics attribute
				goto ShortCut;
			
			case '1':
				// alternate ROM, no graphics
				characterSetInfoPtr->source = kMy_CharacterROMAlternate;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
				inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics; // clear graphics attribute
				goto ShortCut;
			
			case '2':
				// alternate ROM, graphics mode
				characterSetInfoPtr->source = kMy_CharacterROMAlternate;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOn;
				inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics; // set graphics attribute
				goto ShortCut;				
			
			default:
				goto ShortCut;
			}
			++c;
			--ctr;
		}/* while */

        // Handle XTerm rename functions, code contributed by Bill Rausch
        // Modified by JMB to handle ESC]2; case as well.
        // 3.0 - handling many more sequences now
		if ((escflg >= 6) && (ctr > 0))
		{
			static char*	tmp = nullptr;
			static Str255	newname;
			Boolean			changeWindowTitle = false;
			Boolean			changeIconTitle = false;
			
			
			if ((escflg == 6) && ((*c == '0') || (*c == '1') || (*c == '2')))
			{
				if ((*c == '0') || (*c == '1')) changeIconTitle = true;
				if ((*c == '0') || (*c == '2')) changeWindowTitle = true;
				++escflg;
            	++c;
            	--ctr;
			}
			if ((escflg == 7) && (ctr > 0) && (*c == ';'))
			{
				--ctr;
				++c;
				++escflg;
				newname[0] = 0;
				tmp = (char*)&newname[1];
			}
			
			while ((escflg == 8) && (ctr > 0) && (*c != 07) && (*c != 033))
			{
				if (*newname < 255)
				{
            	 	*tmp++ = *c;
            		++(*newname);
            	}
            	++c;
            	--ctr;
			}
        	if ((escflg == 8) && (*c == 07 || *c == 033) && (ctr > 0))
			{
				CFStringRef		titleString = CFStringCreateWithPascalString(kCFAllocatorDefault, newname, CFStringGetSystemEncoding());
				
				
				if (changeWindowTitle)
				{
					inDataPtr->windowTitleCFString.setCFTypeRef(titleString);
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowFrameTitle, inDataPtr->selfRef/* context */);
				}
				if (changeIconTitle)
				{
					inDataPtr->iconTitleCFString.setCFTypeRef(titleString);
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowIconTitle, inDataPtr->selfRef/* context */);
				}
           		if (*c != 07)
            	{
            		// this will be undone after the "goto ShortCut" below
            		--c;
                	++ctr;
				}
				CFRelease(titleString);
            	goto ShortCut;
			}
		}/* if */
		
#if 0
		while ((escflg == 8) && (ctr > 0) && (*c != 07))
		{
			*tmp++ = *c++;
			--ctr;
			++(*newname);
		}
		if ((escflg == 8) && (*c == 07) && (ctr > 0))
		{
			CFStringRef		titleString = CFStringCreateWithPascalString(kCFAllocatorDefault, newname, CFStringGetSystemEncoding());
			
			
			if (nullptr != titleString)
			{
				inDataPtr->windowTitleCFString = titleString;
				changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowFrameTitle, inDataPtr->selfRef/* context */);
				CFRelease(titleString);
			}
			goto ShortCut;
		}
#endif
		
		if ((escflg > 2) && (ctr > 0))
		{
ShortCut:
			escflg = 0;
			++c;
			--ctr;
		}
	}/* while (ctr > 0) */
	
	//inDataPtr->current.escapeSequence.level = escflg;
}// emulatorFrontEndOld


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
default states based on the characters of the given buffer.

This routine can be used to automatically handle a huge
variety of state transitions for your specific terminal
type.  It understands a lot of common encoding schemes,
so you need only define your emulator-specific states to
have generic values (like "kStateSeenESCA") and let your
emulator-specific state determinant use this routine as a
fallback for MOST input values!

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
static UInt32
emulatorStandardStateDeterminant	(My_ScreenBufferPtr		UNUSED_ARGUMENT(inDataPtr),
									 UInt8 const*			inBuffer,
									 UInt32					inLength,
									 My_Parser::State		inCurrentState,
									 My_Parser::State&		outNextState,
									 Boolean&				UNUSED_ARGUMENT(outInterrupt))
{
	using namespace My_Parser;
	
	assert(inLength > 0);
	UInt8 const				kTriggerChar = *inBuffer; // for convenience; usually only first character matters
	// if no specific next state seems appropriate, the character will either
	// be printed (if possible) or be re-evaluated from the initial state
	My_Parser::State const	kDefaultNextState = std::isprint(kTriggerChar) ? kStateEcho : kStateInitial;
	UInt32					result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// by default, the state does not change
	outNextState = inCurrentState;
	
	switch (inCurrentState)
	{
	case kStateInitial:
	case kStateEcho:
		outNextState = kDefaultNextState;
		result = 0; // do not absorb the unknown
		break;
	
	case kStateSeenESC:
		switch (kTriggerChar)
		{
		case '[':
			outNextState = kStateSeenESCLeftSqBracket;
			break;
		
		case '(':
			outNextState = kStateSeenESCLeftParen;
			break;
		
		case ')':
			outNextState = kStateSeenESCRightParen;
			break;
		
		case 'A':
			outNextState = kStateSeenESCA;
			break;
		
		case 'B':
			outNextState = kStateSeenESCB;
			break;
		
		case 'C':
			outNextState = kStateSeenESCC;
			break;
		
		case 'c':
			outNextState = kStateSeenESCc;
			break;
		
		case 'D':
			outNextState = kStateSeenESCD;
			break;
		
		case 'E':
			outNextState = kStateSeenESCE;
			break;
		
		case 'F':
			outNextState = kStateSeenESCF;
			break;
		
		case 'G':
			outNextState = kStateSeenESCG;
			break;
		
		case 'H':
			outNextState = kStateSeenESCH;
			break;
		
		case 'I':
			outNextState = kStateSeenESCI;
			break;
		
		case 'J':
			outNextState = kStateSeenESCJ;
			break;
		
		case 'K':
			outNextState = kStateSeenESCK;
			break;
		
		case 'M':
			outNextState = kStateSeenESCM;
			break;
		
		case 'Y':
			outNextState = kStateSeenESCY;
			break;
		
		case 'Z':
			outNextState = kStateSeenESCZ;
			break;
		
		case '7':
			outNextState = kStateSeenESC7;
			break;
		
		case '8':
			outNextState = kStateSeenESC8;
			break;
		
		case '#':
			outNextState = kStateSeenESCPound;
			break;
		
		case '=':
			outNextState = kStateSeenESCEquals;
			break;
		
		case '<':
			outNextState = kStateSeenESCLessThan;
			break;
		
		case '>':
			outNextState = kStateSeenESCGreaterThan;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape", kTriggerChar);
			outNextState = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kStateSeenESCLeftSqBracket:
		// immediately begin parsing parameters, but do not absorb these characters
		outNextState = kStateSeenESCLeftSqBracketParams;
		result = 0; // do not absorb the unknown
		break;
	
	case kStateSeenESCLeftSqBracketParams:
		switch (kTriggerChar)
		{
		case 'A':
			outNextState = kStateSeenESCLeftSqBracketParamsA;
			break;
		
		case 'B':
			outNextState = kStateSeenESCLeftSqBracketParamsB;
			break;
		
		case 'c':
			outNextState = kStateSeenESCLeftSqBracketParamsc;
			break;
		
		case 'C':
			outNextState = kStateSeenESCLeftSqBracketParamsC;
			break;
		
		case 'D':
			outNextState = kStateSeenESCLeftSqBracketParamsD;
			break;
		
		case 'f':
			outNextState = kStateSeenESCLeftSqBracketParamsf;
			break;
		
		case 'g':
			outNextState = kStateSeenESCLeftSqBracketParamsg;
			break;
		
		case 'h':
			outNextState = kStateSeenESCLeftSqBracketParamsh;
			break;
		
		case 'H':
			outNextState = kStateSeenESCLeftSqBracketParamsH;
			break;
		
		case 'J':
			outNextState = kStateSeenESCLeftSqBracketParamsJ;
			break;
		
		case 'K':
			outNextState = kStateSeenESCLeftSqBracketParamsK;
			break;
		
		case 'l':
			outNextState = kStateSeenESCLeftSqBracketParamsl;
			break;
		
		case 'm':
			outNextState = kStateSeenESCLeftSqBracketParamsm;
			break;
		
		case 'n':
			outNextState = kStateSeenESCLeftSqBracketParamsn;
			break;
		
		case 'q':
			outNextState = kStateSeenESCLeftSqBracketParamsq;
			break;
		
		case 'r':
			outNextState = kStateSeenESCLeftSqBracketParamsr;
			break;
		
		case 'x':
			outNextState = kStateSeenESCLeftSqBracketParamsx;
			break;
		
		default:
			// continue looking for parameters until a known terminator is found
			outNextState = kStateSeenESCLeftSqBracketParams;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kStateSeenESCLeftParen:
		switch (kTriggerChar)
		{
		case 'A':
			outNextState = kStateSeenESCLeftParenA;
			break;
		
		case 'B':
			outNextState = kStateSeenESCLeftParenB;
			break;
		
		case '0':
			outNextState = kStateSeenESCLeftParen0;
			break;
		
		case '1':
			outNextState = kStateSeenESCLeftParen1;
			break;
		
		case '2':
			outNextState = kStateSeenESCLeftParen2;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape-(", kTriggerChar);
			outNextState = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kStateSeenESCRightParen:
		switch (kTriggerChar)
		{
		case 'A':
			outNextState = kStateSeenESCRightParenA;
			break;
		
		case 'B':
			outNextState = kStateSeenESCRightParenB;
			break;
		
		case '0':
			outNextState = kStateSeenESCRightParen0;
			break;
		
		case '1':
			outNextState = kStateSeenESCRightParen1;
			break;
		
		case '2':
			outNextState = kStateSeenESCRightParen2;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape-)", kTriggerChar);
			outNextState = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kStateSeenESCPound:
		switch (kTriggerChar)
		{
		case '3':
			outNextState = kStateSeenESCPound3;
			break;
		
		case '4':
			outNextState = kStateSeenESCPound4;
			break;
		
		case '5':
			outNextState = kStateSeenESCPound5;
			break;
		
		case '6':
			outNextState = kStateSeenESCPound6;
			break;
		
		case '8':
			outNextState = kStateSeenESCPound8;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape-#", kTriggerChar);
			outNextState = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	default:
		// unknown state!
		//Console_WriteValueCharacter("WARNING, terminal entered unknown state; choosing a valid state based on character", kTriggerChar);
		outNextState = kDefaultNextState;
		result = 0; // do not absorb the unknown
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< default in state", inCurrentState);
	//Console_WriteValueFourChars(">>>     default proposes state", outNextState);
	//Console_WriteValueCharacter("        default bases this at least on character", *inBuffer);
	
	return result;
}// emulatorStandardStateDeterminant


/*!
Every "My_EmulatorStateTransitionProcPtr" callback should
default to the result of invoking this routine with its
arguments.  This allows special states to be handled
regardless of the emulator, such as the echo state.

(3.1)
*/
static UInt32
emulatorStandardStateTransition		(My_ScreenBufferPtr		inDataPtr,
									 UInt8 const*			inBuffer,
									 UInt32					inLength,
									 My_Parser::State		inNewState,
									 My_Parser::State		UNUSED_ARGUMENT(inPreviousState))
{
	using namespace My_Parser;
	
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< standard handler transition from state", inPreviousState);
	//Console_WriteValueFourChars(">>>     standard handler transition to state  ", inNewState);
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inNewState)
	{
	case kStateEcho:
		// echo state, where characters are simply taken at face value
		{
			UInt8 const*	bufferIterator = inBuffer;
			
			
			// suck up a contiguous block of “printable” characters
			while ((result < inLength) && (std::isprint(*bufferIterator)))
			{
				++bufferIterator;
				++result;
			}
			
			// send the data wherever it needs to go
			echoData(inDataPtr, inBuffer, result);
			// 3.0 - test speech (this implementation will be greatly enhanced in the near future
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
				
				default:
					doSpeak = false;
					break;
				}
				
				if (doSpeak)
				{
					TerminalSpeaker_ResultCode	speakerResult = kTerminalSpeaker_ResultCodeSuccess;
					
					
					// TEMPORARY - spin lock, to keep asynchronous speech from jumbling multi-line text;
					//             this really should be changed to use speech callbacks instead
					do
					{
						// stop and speak when a new line is found, or
						// when there is just no more room for characters
						speakerResult = TerminalSpeaker_SynthesizeSpeechFromBuffer(inDataPtr->speaker, inBuffer, result);
					} while (speakerResult == kTerminalSpeaker_ResultCodeSpeechSynthesisTryAgain);
				}
			}
		}
		break;
	
	default:
		// ???
		//Console_WriteValueFourChars("WARNING, no known actions associated with new terminal state", inNewState);
		// the trigger character would also be skipped in this case
		break;
	}
	
	return result;
}// emulatorStandardStateTransition


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT100-specific states based on the characters of the given
buffer.

(3.1)
*/
static UInt32
emulatorVT100StateDeterminant	(My_ScreenBufferPtr		inDataPtr,
								 UInt8 const*			inBuffer,
								 UInt32					inLength,
								 My_Parser::State		inCurrentState,
								 My_Parser::State&		outNextState,
								 Boolean&				outInterrupt)
{
	using namespace My_Parser;
	
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	Boolean		isControlCharacter = true;
	
	
	// see if the given character is a control character; if so,
	// it will not contribute to the current sequence and may
	// even reset the parser
	switch (*inBuffer)
	{
	case '\000':
		// ignore this character for the purposes of sequencing
		outNextState = inCurrentState;
		break;
	
	case '\005':
		// send answer-back message
		outNextState = kStateVT100ControlENQ;
		break;
	
	case '\007':
		// audio event
		outNextState = kStateVT100ControlBEL;
		break;
	
	case '\010':
		// backspace
		outNextState = kStateVT100ControlBS;
		break;
	
	case '\011':
		// horizontal tab
		outNextState = kStateVT100ControlHT;
		break;
	
	case '\012':
	case '\013':
	case '\014':
		// line feed
		// all of these are interpreted the same for VT100
		outNextState = kStateVT100ControlLFVTFF;
		break;
	
	case '\015':
		// carriage return
		outNextState = kStateVT100ControlCR;
		break;
	
	case '\016':
		// shift out
		outNextState = kStateVT100ControlSO;
		break;
	
	case '\017':
		// shift in
		outNextState = kStateVT100ControlSI;
		break;
	
	case '\021':
		// resume transmission
		outNextState = kStateVT100ControlXON;
		break;
	
	case '\023':
		// suspend transmission
		outNextState = kStateVT100ControlXOFF;
		break;
	
	case '\030':
	case '\032':
		// abort control sequence (if any) and emit error character
		outNextState = kStateVT100ControlCANSUB;
		break;
	
	case '\177': // DEL
		// ignore this character for the purposes of sequencing
		outNextState = inCurrentState;
		break;
	
	default:
		isControlCharacter = false;
		break;
	}
	
	// all control characters are interrupt-class: they should
	// cause actions, but not “corrupt” any partially completed
	// sequence that may have come before them, i.e. the caller
	// should revert to the state preceding the control character
	if (isControlCharacter)
	{
		outInterrupt = true;
	}
	
	// if no interrupt has occurred, use the current state and
	// the available data to determine the next logical state
	if (false == isControlCharacter)
	{
		switch (inCurrentState)
		{
		case kStateVT100CSI:
			outNextState = kStateVT100CSIParamScan;
			result = 0; // absorb nothing
			break;
		
		case kStateVT100CSIParamScan:
			// look for a terminating character (anything not legal in a parameter)
			switch (*inBuffer)
			{
			case 'A':
				outNextState = kStateSeenESCLeftSqBracketParamsA;
				break;
			
			case 'B':
				outNextState = kStateSeenESCLeftSqBracketParamsB;
				break;
			
			case 'c':
				outNextState = kStateSeenESCLeftSqBracketParamsc;
				break;
			
			case 'C':
				outNextState = kStateSeenESCLeftSqBracketParamsC;
				break;
			
			case 'D':
				outNextState = kStateSeenESCLeftSqBracketParamsD;
				break;
			
			case 'f':
				outNextState = kStateSeenESCLeftSqBracketParamsf;
				break;
			
			case 'g':
				outNextState = kStateSeenESCLeftSqBracketParamsg;
				break;
			
			case 'h':
				outNextState = kStateSeenESCLeftSqBracketParamsh;
				break;
			
			case 'H':
				outNextState = kStateSeenESCLeftSqBracketParamsH;
				break;
			
			case 'J':
				outNextState = kStateSeenESCLeftSqBracketParamsJ;
				break;
			
			case 'K':
				outNextState = kStateSeenESCLeftSqBracketParamsK;
				break;
			
			case 'l':
				outNextState = kStateSeenESCLeftSqBracketParamsl;
				break;
			
			case 'm':
				outNextState = kStateSeenESCLeftSqBracketParamsm;
				break;
			
			case 'n':
				outNextState = kStateSeenESCLeftSqBracketParamsn;
				break;
			
			case 'q':
				outNextState = kStateSeenESCLeftSqBracketParamsq;
				break;
			
			case 'r':
				outNextState = kStateSeenESCLeftSqBracketParamsr;
				break;
			
			case 'x':
				outNextState = kStateSeenESCLeftSqBracketParamsx;
				break;
			
			// continue scanning as long as characters are LEGAL in a parameter sequence
			// (the set below should be consistent with vt100ReadCSIParameters())
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case ';':
			case '?':
				outNextState = kStateVT100CSIParamScan;
				break;
			
			default:
				// this is unexpected data; choose a new state
				Console_WriteValueCharacter("warning, VT100 in CSI parameter mode did not expect character", *inBuffer);
				result = emulatorStandardStateDeterminant(inDataPtr, inBuffer, inLength, inCurrentState,
															outNextState, outInterrupt);
				break;
			}
			break;
		
		default:
			if (*inBuffer == '\033')
			{
				// this character forces any partial sequence that came before it to be ignored
				outNextState = kStateSeenESC;
			}
			else
			{
				result = emulatorStandardStateDeterminant(inDataPtr, inBuffer, inLength, inCurrentState,
															outNextState, outInterrupt);
			}
			break;
		}
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in state", inCurrentState);
	//Console_WriteValueFourChars(">>>     VT100 proposes state", outNextState);
	//Console_WriteValueCharacter("        VT100 bases this at least on character", *inBuffer);
	
	return result;
}// emulatorVT100StateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds
to VT100-specific (ANSI mode) state changes.

See also emulatorVT100VT52StateTransition(), which should
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
static UInt32
emulatorVT100StateTransition	(My_ScreenBufferPtr		inDataPtr,
								 UInt8 const*			inBuffer,
								 UInt32					inLength,
								 My_Parser::State		inNewState,
								 My_Parser::State		inPreviousState)
{
	using namespace My_Parser;
	
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 ANSI transition from state", inPreviousState);
	//Console_WriteValueFourChars(">>>     VT100 ANSI transition to state  ", inNewState);
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inNewState)
	{
	case kStateVT100ControlENQ:
		// send answer-back message
		// UNIMPLEMENTED
		Console_WriteLine("request to send answer-back message; unimplemented");
		break;
	
	case kStateVT100ControlBEL:
		// audio event
		unless (inDataPtr->bellDisabled)
		{
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeAudioEvent, inDataPtr->selfRef/* context */);
		}
		break;
	
	case kStateVT100ControlBS:
		// backspace
		if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
		else moveCursorLeftToEdge(inDataPtr); // do not extend past margin
		break;
	
	case kStateVT100ControlHT:
		// horizontal tab
		moveCursorRightToNextTabStop(inDataPtr);
		Terminal_FileCaptureWriteData(inDataPtr->selfRef, inBuffer, 1);
		break;
	
	case kStateVT100ControlLFVTFF:
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
	
	case kStateVT100ControlCR:
		// carriage return
		moveCursorLeftToEdge(inDataPtr);
	#if 0
		if (inDataPtr->modeNewLineOption)
		{
			moveCursorDownOrScroll(inDataPtr);
		}
	#endif
		Terminal_FileCaptureWriteData(inDataPtr->selfRef, inBuffer, 1);
		break;
	
	case kStateVT100ControlSO:
		// shift out
		inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG1;
		if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
		{
			// set attribute
			inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics;
		}
		else
		{
			// clear attribute
			inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics;
		}
		break;
	
	case kStateVT100ControlSI:
		// shift in
		inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
		if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
		{
			// set attribute
			inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics;
		}
		else
		{
			// clear attribute
			inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics;
		}
		break;
	
	case kStateVT100ControlXON:
		// resume transmission
		// UNIMPLEMENTED
		Console_WriteLine("request to resume transmission; unimplemented");
		break;
	
	case kStateVT100ControlXOFF:
		// suspend transmission
		// UNIMPLEMENTED
		Console_WriteLine("request to suspend transmission (except for XON/XOFF); unimplemented");
		break;
	
	case kStateVT100ControlCANSUB:
		// abort control sequence (if any) and emit error character
		{
			UInt8	errorChar[] = { '?' };
			
			
			echoData(inDataPtr, errorChar, 1/* length */);
		}
		break;
	
	case kStateVT100CSI:
		// each new CSI means a blank slate for parameters
		clearEscapeSequenceParameters(inDataPtr);
		break;
	
	case kStateVT100CSIParamScan:
		// continue to accumulate parameters (this could require multiple passes)
		result += vt100ReadCSIParameters(inDataPtr, inBuffer, inLength);
		break;
	
	case kStateVT100CUB:
		vt100CursorBackward(inDataPtr);
		break;
	
	case kStateVT100CUD:
		vt100CursorDown(inDataPtr);
		break;
	
	case kStateVT100CUF:
		vt100CursorForward(inDataPtr);
		break;
	
	case kStateVT100CUU:
		vt100CursorUp(inDataPtr);
		break;
	
	case kStateVT100CUP:
	case kStateVT100HVP:
		// absolute cursor positioning
		{
			SInt16				newX = (inDataPtr->currentEscapeSeqParamValues[1] != -1)
										? inDataPtr->currentEscapeSeqParamValues[1] - 1
										: 0/* default is home */;
			My_ScreenRowIndex	newY = (inDataPtr->currentEscapeSeqParamValues[0] != -1)
										? inDataPtr->currentEscapeSeqParamValues[0] - 1
										: 0/* default is home */;
			
			
			// in origin mode, offset according to the scrolling region
			if (inDataPtr->modeOriginRedefined) newY += inDataPtr->scrollingRegion.firstRow;
			
			// constrain the value and then change it safely
			if (newX < 0) newX = 0;
			if (newX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
			{
				newX = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
			}
			//if (newY < 0) newY = 0;
			if (newY >= inDataPtr->screenBuffer.size())
			{
				newY = inDataPtr->screenBuffer.size() - 1;
			}
			moveCursor(inDataPtr, newX, newY);
		}
		break;
	
	case kStateVT100DA:
		vt100DeviceAttributes(inDataPtr);
		break;
	
	case kStateVT100DECALN:
		vt100AlignmentDisplay(inDataPtr);
		break;
	
	case kStateVT100DECANM:
		break;
	
	case kStateVT100DECARM:
		break;
	
	case kStateVT100DECAWM:
		break;
	
	case kStateVT100DECCKM:
		break;
	
	case kStateVT100DECCOLM:
		break;
	
	case kStateVT100DECDHLT:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, *cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, kTerminalTextAttributeDoubleHeightTop/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateVT100DECDHLB:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, *cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, kTerminalTextAttributeDoubleHeightBottom/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateVT100DECDWL:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, *cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, kTerminalTextAttributeDoubleWidth/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateVT100DECID:
		vt100DeviceAttributes(inDataPtr);
		break;
	
	case kStateVT100DECKPAM:
		inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (use application key sequences) of VT100
		break;
	
	case kStateVT100DECKPNM:
		inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (restore regular keypad characters) of VT100
		break;
	
	case kStateVT100DECLL:
		vt100LoadLEDs(inDataPtr);
		break;
	
	case kStateVT100DECOM:
		break;
	
	case kStateVT100DECRC:
		cursorRestore(inDataPtr);
		break;
	
	case kStateVT100DECREPTPARM:
		// a parameter report has been received
		// IGNORED
		break;
	
	case kStateVT100DECREQTPARM:
		// a request for parameters has been made; send a response
		vt100ReportTerminalParameters(inDataPtr);
		break;
	
	case kStateVT100DECSC:
		cursorSave(inDataPtr);
		break;
	
	case kStateVT100DECSTBM:
		vt100SetTopAndBottomMargins(inDataPtr);
		break;
	
	case kStateVT100DECSWL:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, 0/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
		}
		break;
	
	case kStateVT100DECTST:
		break;
	
	case kStateVT100DSR:
		vt100DeviceStatusReport(inDataPtr);
		break;
	
	case kStateVT100ED:
		vt100EraseInDisplay(inDataPtr);
		break;
	
	case kStateVT100EL:
		vt100EraseInLine(inDataPtr);
		break;
	
	case kStateVT100HTS:
		// set tab at current position
		inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;
		break;
	
	//case kStateVT100HVP:
	//see above
	
	case kStateVT100IND:
		moveCursorDownOrScroll(inDataPtr);
		break;
	
	case kStateVT100NEL:
		moveCursorLeftToEdge(inDataPtr);
		moveCursorDownOrScroll(inDataPtr);
		break;
	
	case kStateVT100RI:
		moveCursorUpOrScroll(inDataPtr);
		break;
	
	case kStateVT100RIS:
		break;
	
	case kStateVT100RM:
		vt100ModeSetReset(inDataPtr, false/* set */);
		break;
	
	case kStateVT100SCSG0UK:
	case kStateVT100SCSG1UK:
		{
			// U.K. character set, normal ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateVT100SCSG1UK == inNewState) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedKingdom;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics; // clear graphics attribute
		}
		break;
	
	case kStateVT100SCSG0ASCII:
	case kStateVT100SCSG1ASCII:
		{
			// U.S. character set, normal ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateVT100SCSG1ASCII == inNewState) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedStates;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics; // clear graphics attribute
		}
		break;
	
	case kStateVT100SCSG0SG:
	case kStateVT100SCSG1SG:
		{
			// normal ROM, graphics mode
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateVT100SCSG1SG == inNewState) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics; // set graphics attribute
		}
		break;
	
	case kStateVT100SCSG0ACRStd:
	case kStateVT100SCSG1ACRStd:
		{
			// alternate ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateVT100SCSG1ACRStd == inNewState) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMAlternate;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			inDataPtr->current.attributeBits &= ~kTerminalTextAttributeVTGraphics; // clear graphics attribute
		}
		break;
	
	case kStateVT100SCSG0ACRSG:
	case kStateVT100SCSG1ACRSG:
		{
			// normal ROM, graphics mode
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateVT100SCSG1ACRSG == inNewState) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMAlternate;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			inDataPtr->current.attributeBits |= kTerminalTextAttributeVTGraphics; // set graphics attribute
		}
		break;
	
	case kStateVT100SGR:
		// ANSI colors and other character attributes
		{
			SInt16		i = 0;
			
			
			while (i <= inDataPtr->currentEscapeSeqParamEndIndex)
			{
				SInt16		p = 0;
				
				
				if (inDataPtr->currentEscapeSeqParamValues[inDataPtr->currentEscapeSeqParamEndIndex] < 0)
				{
					inDataPtr->currentEscapeSeqParamValues[inDataPtr->currentEscapeSeqParamEndIndex] = 0;
				}
				
				p = inDataPtr->currentEscapeSeqParamValues[i];
				
				// Note that a real VT100 will only understand 0-7 here.
				// Other values are basically recognized because they are
				// compatible with VT100 and are very often used (ANSI
				// colors in particular).
				if (p == 0) inDataPtr->current.attributeBits &= 0xFFFF0000; // all style bits off
				else if (p < 9) inDataPtr->current.attributeBits |= styleOfVTParameter(p); // set attribute
				else if (p == 10) { /* set normal font - unsupported */ }
				else if (p == 11) { /* set alternate font - unsupported */ }
				else if (p == 12) { /* set alternate font, shifting by 128 - unsupported */ }
				else if (p == 22) inDataPtr->current.attributeBits &= ~styleOfVTParameter(1); // clear bold (oddball - 22, not 21)
				else if ((p > 22) && (p < 29)) inDataPtr->current.attributeBits &= ~styleOfVTParameter(p - 20); // clear attribute
				else
				{
					if ((p >= 30) && (p < 38))
					{
						inDataPtr->current.attributeBits = (inDataPtr->current.attributeBits & ~0x0700) | ((p - 30) << 8) | 0x0800;
					}
					else if ((p >= 40) && (p < 48))
					{
						inDataPtr->current.attributeBits = (inDataPtr->current.attributeBits & ~0x7000) | ((p - 40) << 12) | 0x8000;
					}
				}
				++i;
			}
		}
		break;
	
	case kStateVT100SM:
		vt100ModeSetReset(inDataPtr, true/* set */);
		break;
	
	case kStateVT100TBC:
		if (3 == inDataPtr->currentEscapeSeqParamValues[0])
		{
			// clear all tabs
			tabStopClearAll(inDataPtr);
		}
		else if (0 >= inDataPtr->currentEscapeSeqParamValues[0])
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
	case kStateVT52CU:
	case kStateVT52CD:
	case kStateVT52CR:
	//case kStateVT52CL: // this conflicts with a valid VT100 ANSI mode value above
	case kStateVT52NGM:
	case kStateVT52XGM:
	//case kStateVT52CH: // this conflicts with a valid VT100 ANSI mode value above
	case kStateVT52RLF:
	case kStateVT52EES:
	case kStateVT52EEL:
	case kStateVT52DCA:
	//case kStateVT52ID: // this conflicts with a valid VT100 ANSI mode value above
	//case kStateVT52NAKM: // this conflicts with a valid VT100 ANSI mode value above
	//case kStateVT52XAKM: // this conflicts with a valid VT100 ANSI mode value above
	case kStateVT52ANSI:
		break;
	
	default:
		result = emulatorStandardStateTransition(inDataPtr, inBuffer, inLength, inNewState, inPreviousState);
		break;
	}
	
	return result;
}// emulatorVT100StateTransition


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT52-specific states based on the characters of the given
buffer.

(3.1)
*/
static UInt32
emulatorVT100VT52StateDeterminant	(My_ScreenBufferPtr		inDataPtr,
									 UInt8 const*			inBuffer,
									 UInt32					inLength,
									 My_Parser::State		inCurrentState,
									 My_Parser::State&		outNextState,
									 Boolean&				outInterrupt)
{
	using namespace My_Parser;
	
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	switch (inCurrentState)
	{
	case kStateSeenESC:
		switch (*inBuffer)
		{
		case 'A':
			outNextState = kStateSeenESCA;
			break;
		
		case 'B':
			outNextState = kStateSeenESCB;
			break;
		
		case 'C':
			outNextState = kStateSeenESCC;
			break;
		
		case 'D':
			outNextState = kStateSeenESCD;
			break;
		
		case 'F':
			outNextState = kStateSeenESCF;
			break;
		
		case 'G':
			outNextState = kStateSeenESCG;
			break;
		
		case 'H':
			outNextState = kStateSeenESCH;
			break;
		
		case 'I':
			outNextState = kStateSeenESCI;
			break;
		
		case 'J':
			outNextState = kStateSeenESCJ;
			break;
		
		case 'K':
			outNextState = kStateSeenESCK;
			break;
		
		case 'Y':
			outNextState = kStateSeenESCY;
			break;
		
		case 'Z':
			outNextState = kStateSeenESCZ;
			break;
		
		case '=':
			outNextState = kStateSeenESCEquals;
			break;
		
		case '>':
			outNextState = kStateSeenESCGreaterThan;
			break;
		
		case '<':
			outNextState = kStateSeenESCLessThan;
			break;
		
		default:
			// this is unexpected data; choose a new state
			Console_WriteValueCharacter("warning, VT52 did not expect an ESC to be followed by character", *inBuffer);
			result = emulatorVT100StateDeterminant(inDataPtr, inBuffer, inLength, inCurrentState,
													outNextState, outInterrupt);
			break;
		}
		break;
	
	case kStateVT52DCA:
		// the 2 characters after a VT52 DCA are the coordinates (Y first)
		outNextState = kStateVT52DCAY;
		result = 0; // absorb nothing
		break;
	
	case kStateVT52DCAY:
		// the 2 characters after a VT52 DCA are the coordinates (Y first)
		outNextState = kStateVT52DCAX;
		result = 0; // absorb nothing
		break;
	
	default:
		result = emulatorVT100StateDeterminant(inDataPtr, inBuffer, inLength, inCurrentState,
												outNextState, outInterrupt);
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in VT52 mode in state", inCurrentState);
	//Console_WriteValueFourChars(">>>     VT100 in VT52 mode proposes state", outNextState);
	//Console_WriteValueCharacter("        VT100 in VT52 mode bases this at least on character", *inBuffer);
	
	return result;
}// emulatorVT100VT52StateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT100-specific (VT52 mode) state changes.

(3.1)
*/
static UInt32
emulatorVT100VT52StateTransition	(My_ScreenBufferPtr		inDataPtr,
									 UInt8 const*			inBuffer,
									 UInt32					inLength,
									 My_Parser::State		inNewState,
									 My_Parser::State		inPreviousState)
{
	using namespace My_Parser;
	
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 VT52 transition from state", inPreviousState);
	//Console_WriteValueFourChars(">>>     VT100 VT52 transition to state  ", inNewState);
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inNewState)
	{
	case kStateVT52CU:
		vt100CursorUp_vt52(inDataPtr);
		break;
	
	case kStateVT52CD:
		vt100CursorDown_vt52(inDataPtr);
		break;
	
	case kStateVT52CR:
		vt100CursorForward_vt52(inDataPtr);
		break;
	
	case kStateVT52CL:
		vt100CursorBackward_vt52(inDataPtr);
		break;
	
	case kStateVT52NGM:
		// enter graphics mode - unimplemented
		break;
	
	case kStateVT52XGM:
		// exit graphics mode - unimplemented
		break;
	
	case kStateVT52CH:
		moveCursor(inDataPtr, 0, 0); // home cursor in VT52 compatibility mode of VT100
		break;
	
	case kStateVT52RLF:
		moveCursorUpOrScroll(inDataPtr); // reverse line feed in VT52 compatibility mode of VT100
		break;
	
	case kStateVT52EES:
		bufferEraseFromCursorToEnd(inDataPtr); // erase to end of screen, in VT52 compatibility mode of VT100
		break;
	
	case kStateVT52EEL:
		bufferEraseFromCursorColumnToLineEnd(inDataPtr); // erase to end of line, in VT52 compatibility mode of VT100
		break;
	
	case kStateVT52DCA:
		// direct cursor address in VT52 compatibility mode of VT100;
		// new cursor position is encoded as the next two characters
		// (vertical first, then horizontal) offset by the octal
		// value 037 (equal to decimal 31); this is handled by 2
		// other states, "kStateVT52DCAY" and "kStateVT52DCAX" (below)
		break;
	
	case kStateVT52DCAY:
		// VT52 DCA, first character (Y + 31)
		{
			My_ScreenRowIndex	newY = 0;
			
			
			newY = *inBuffer - 32/* - 31 - 1 to convert from one-based to zero-based */;
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
	
	case kStateVT52DCAX:
		// VT52 DCA, second character (X + 31)
		{
			SInt16		newX = 0;
			
			
			newX = *inBuffer - 32/* - 31 - 1 to convert from one-based to zero-based */;
			++result;
			
			// constrain the value and then change it safely
			if (newX < 0) newX = 0;
			if (newX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
			{
				newX = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
			}
			moveCursorX(inDataPtr, newX);
		}
		break;
	
	case kStateVT52ID:
		vt100Identify_vt52(inDataPtr);
		break;
	
	case kStateVT52NAKM:
		inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (use application key sequences) in VT52 compatibility mode of VT100
		break;
	
	case kStateVT52XAKM:
		inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (restore regular keypad characters) in VT52 compatibility mode of VT100
		break;
	
	case kStateVT52ANSI:
		vt100ANSIMode(inDataPtr);
		break;
	
	// ignore all VT100 sequences that are invalid in VT52 mode
	// NONE?
	//case :
	//	break;
	
	default:
		// other state transitions should still basically be handled as if in VT100 ANSI
		// (NOTE: this switch should filter out any ANSI mode states that are supposed to
		// be ignored while in VT52 mode!)
		result = emulatorVT100StateTransition(inDataPtr, inBuffer, inLength, inNewState, inPreviousState);
		break;
	}
	
	return result;
}// emulatorVT100VT52StateTransition


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT220-specific states based on the characters of the given
buffer.

(3.1)
*/
static UInt32
emulatorVT220StateDeterminant	(My_ScreenBufferPtr		inDataPtr,
								 UInt8 const*			inBuffer,
								 UInt32					inLength,
								 My_Parser::State		inCurrentState,
								 My_Parser::State&		outNextState,
								 Boolean&				outInterrupt)
{
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// see if the given character is a control character; if so,
	// it will not contribute to the current sequence and may
	// even reset the parser
	switch (*inBuffer)
	{
	default:
		result = emulatorVT100StateDeterminant(inDataPtr, inBuffer, inLength, inCurrentState,
												outNextState, outInterrupt);
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT220 in state", inCurrentState);
	//Console_WriteValueFourChars(">>>     VT220 proposes state", outNextState);
	//Console_WriteValueCharacter("        VT220 bases this at least on character", *inBuffer);
	
	return result;
}// emulatorVT220StateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT220-specific state changes.

(3.1)
*/
static UInt32
emulatorVT220StateTransition	(My_ScreenBufferPtr		inDataPtr,
								 UInt8 const*			inBuffer,
								 UInt32					inLength,
								 My_Parser::State		inNewState,
								 My_Parser::State		inPreviousState)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT220 transition from state", inPreviousState);
	//Console_WriteValueFourChars(">>>     VT220 transition to state  ", inNewState);
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inNewState)
	{
	default:
		// other state transitions should still basically be handled as if in VT100
		result = emulatorVT100StateTransition(inDataPtr, inBuffer, inLength, inNewState, inPreviousState);
		break;
	}
	
	return result;
}// emulatorVT220StateTransition


/*!
Erases the characters from the halfway point on the
screen to the right edge of the specified line,
clearing all attributes.  NO update events are sent.

This is required when switching to double-width
mode, which specifies that the right half of the
screen data on a double-width line be lost.

(3.0)
*/
static void
eraseRightHalfOfLine	(My_ScreenBufferPtr		inDataPtr,
						 My_ScreenBufferLine&	inRow)
{
	SInt16								midColumn = INTEGER_HALVED(inDataPtr->text.visibleScreen.numberOfColumnsPermitted);
	My_TextVector::iterator				textIterator;
	My_TextAttributesList::iterator		attrIterator;
	
	
	// clear from halfway point to end of line
	textIterator = inRow.textVector.begin();
	std::advance(textIterator, midColumn);
	attrIterator = inRow.attributeVector.begin();
	std::advance(attrIterator, midColumn);
	std::fill(textIterator, inRow.textVector.end(), ' ');
	std::fill(attrIterator, inRow.attributeVector.end(), inRow.globalAttributes);
}// eraseRightHalfOfLine


/*!
Iterates over the given range of lines on the specified
terminal screen, executing a function on each of them.

Since iterators can only be created exclusively for the
main screen or exclusively for the scrollback, it follows
that the iteration will only cover one of those buffers.
To iterate over a region that spans both the screen and
the scrollback, you will need to invoke this routine twice
with two different kinds of iterators and the appropriate
row counts for each.

The ranges are zero-based, but can be negative; 0 is the
first line of the main screen area, and -1 is the first
scrollback line.  The oldest scrollback row is a larger
negative number, the bottommost line of the visible screen
is one less than the number of rows high that the terminal
screen is.

The context is defined by you, and is passed directly to
the specified function each time it is invoked.

\retval kTerminal_ResultCodeSuccess
if no error occurred

\retval kTerminal_ResultCodeParameterError
if the screen line operation function, screen reference or
line iterator reference is invalid

\retval kTerminal_ResultCodeNotEnoughMemory
if any line buffers are unexpectedly empty

(3.0)
*/
static Terminal_ResultCode
forEachLineDo	(TerminalScreenRef				inRef,
				 Terminal_LineRef				inStartRow,
				 UInt16							inNumberOfRowsToConsider,
				 My_ScreenLineOperationProcPtr	inDoWhat,
				 void*							inContextPtr)
{
	Terminal_ResultCode		result = kTerminal_ResultCodeSuccess;
	My_ScreenBufferPtr		screenPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if ((inDoWhat == nullptr) || (screenPtr == nullptr) || (iteratorPtr == nullptr))
	{
		result = kTerminal_ResultCodeParameterError;
	}
	else
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		SInt16								lineNumber = 0;
		
		
		// iterate over all lines in the range
		for (lineIterator = iteratorPtr->rowIterator;
				((lineNumber < inNumberOfRowsToConsider) && (iteratorPtr->rowIterator != iteratorPtr->sourceList.end()));
				++lineIterator, ++lineNumber)
		{
			invokeScreenLineOperationProc(inDoWhat, screenPtr, lineIterator->textVector.c_str(), lineNumber, inContextPtr);
		}
	}
	return result;
}// forEachLineDo


/*!
Returns a pointer to the internal structure, given a
reference to it.

(3.0)
*/
static inline My_LineIteratorPtr
getLineIterator		(Terminal_LineRef	inIterator)
{
	return REINTERPRET_CAST(inIterator, My_LineIteratorPtr);
}// getLineIterator


/*!
Returns a pointer to the internal structure, given a
reference to it.

(3.0)
*/
static inline My_ScreenBufferPtr
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
static void
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
Discards all state history in the screen’s parser and creates
a single, initial state.

This is obviously done when a screen is first created, but
should also be done whenever the state determinant and state
transition routines are changed (e.g. to change the emulation
type).

(3.1)
*/
static void
initializeParserStateStack	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->currentState = My_Parser::kStateInitial;
}// initializeParserStateStack


/*!
Modifies the screen and/or scrollback buffers according to
the given parameters.

If "inAppendOnly" is true, the new lines will expand the
screen buffer *without* affecting the scrollback buffer at
all; if false, the screen buffer size remains effectively
unchanged, as the topmost (first) screen line will be moved
to the scrollback before each new line is appended.

The resultant lines may share a large memory block.  So, it
is usually better to invoke this routine once for the number
of lines you’ll ultimately need, than to invoke it many
times to add a single line.

Returns "true" only if successful.

(3.0)
*/
static Boolean
insertNewLines	(My_ScreenBufferPtr						inDataPtr,
				 My_ScreenBufferLineList::size_type		inNumberOfElements,
				 Boolean								inAppendOnly)
{
	Boolean		result = true;
	
	
	//Console_WriteValue("request to insert new lines", inNumberOfElements);
	//Console_WriteValue("append lines only", inAppendOnly);
	if (inNumberOfElements != 0)
	{
		Boolean		recycleLines = false;
		
		
		if (inAppendOnly)
		{
			My_ScreenBufferLineList::size_type   kOldSize = inDataPtr->screenBuffer.size();
			
			
			// start by allocating more lines if necessary, or freeing unneeded lines
			if (inNumberOfElements != 0)
			{
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
		}
		else
		{
			register My_ScreenBufferLineList::size_type		i = 0;
			
			
			// scrolling will be done; figure out whether or not to recycle old lines
			recycleLines = (!(inDataPtr->text.scrollback.enabled)) ||
							(!(inDataPtr->scrollbackBuffer.empty()) &&
								(inDataPtr->scrollbackBuffer.size() >= inDataPtr->text.scrollback.numberOfRowsPermitted));
			
			// adjust screen and scrollback buffers appropriately; new lines
			// will either be rotated in from the oldest scrollback, or
			// allocated anew; in either case, they'll end up initialized
			if (recycleLines)
			{
				//Console_WriteValue("recycled lines", inNumberOfElements);
				
				// get the line destined for the new bottom of the main screen;
				// either extract it from elsewhere, or allocate a new line
				for (i = 0; i < inNumberOfElements; ++i)
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
						My_ScreenBufferLineList::iterator	oldestScrollbackLine;
						
						
						// make the oldest screen line the newest scrollback line
						inDataPtr->scrollbackBuffer.splice(inDataPtr->scrollbackBuffer.begin()/* the next oldest scrollback line */,
															inDataPtr->screenBuffer/* the list to move from */,
															inDataPtr->screenBuffer.begin()/* the line to move */);
						
						// end() points one past the end, so nudge it back
						oldestScrollbackLine = inDataPtr->scrollbackBuffer.end();
						std::advance(oldestScrollbackLine, -1);
						
						// make the oldest scrollback line the newest screen line
						inDataPtr->screenBuffer.splice(inDataPtr->screenBuffer.end()/* the next newest screen line */,
														inDataPtr->scrollbackBuffer/* the list to move from */,
														oldestScrollbackLine/* the line to move */);
					}
					
					// the recycled line may have data in it, so clear it out
					inDataPtr->screenBuffer.back().structureInitialize();
				}
				
				//Console_WriteValue("post-recycle scrollback size", inDataPtr->scrollbackBuffer.size());
			}
			else
			{
				//Console_WriteValue("moved-and-reallocated lines", inNumberOfElements);
				
				My_ScreenBufferLineList::iterator	pastLastLineToScroll = inDataPtr->screenBuffer.begin();
				
				
				// find the first line that will remain in the screen buffer
				std::advance(pastLastLineToScroll, inNumberOfElements);
				
				// make the oldest screen lines the newest scrollback lines
				inDataPtr->scrollbackBuffer.splice(inDataPtr->scrollbackBuffer.begin()/* the next oldest scrollback line */,
													inDataPtr->screenBuffer/* the list to move from */,
													inDataPtr->screenBuffer.begin()/* the first line to move */,
													pastLastLineToScroll/* the first line that will remain on screen */);
				
				//Console_WriteValue("post-move scrollback size", inDataPtr->scrollbackBuffer.size());
				
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
	}
	
	return result;
}// insertNewLines


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
static void
locateCursorLine	(My_ScreenBufferPtr						inDataPtr,
					 My_ScreenBufferLineList::iterator&		outCursorLine)
{
	//assert(inDataPtr->current.cursorY >= 0);
	assert(inDataPtr->current.cursorY <= inDataPtr->screenBuffer.size());
	outCursorLine = inDataPtr->screenBuffer.begin();
	std::advance(outCursorLine, inDataPtr->current.cursorY/* zero-based */);
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
Plus, the scrolling region iterators only needs to be
found rarely, in practice less frequently than most list
modifications that would invalidate retained iterators.

(3.0)
*/
static void
locateScrollingRegion	(My_ScreenBufferPtr						inDataPtr,
						 My_ScreenBufferLineList::iterator&		outTopLine,
						 My_ScreenBufferLineList::iterator&		outPastBottomLine)
{
	//assert(inDataPtr->scrollingRegion.firstRow >= 0);
	assert(inDataPtr->scrollingRegion.lastRow <= inDataPtr->screenBuffer.size());
	assert(inDataPtr->screenBuffer.size() >= (inDataPtr->scrollingRegion.lastRow - inDataPtr->scrollingRegion.firstRow));
	
	// find top line iterator in screen
	outTopLine = inDataPtr->screenBuffer.begin();
	std::advance(outTopLine, inDataPtr->scrollingRegion.firstRow/* zero-based */);
	
#if 1
	// slow, but definitely correct
	outPastBottomLine = inDataPtr->screenBuffer.begin();
	std::advance(outPastBottomLine, 1 + inDataPtr->scrollingRegion.lastRow/* zero-based */);
#else
	// note the region boundary is inclusive but past-the-end is exclusive;
	// also, for efficiency, assume this will be much closer to the end and
	// advance backwards to save some pointer iterations
	outPastBottomLine = inDataPtr->screenBuffer.end();
	std::advance(outPastBottomLine, -(inDataPtr->screenBuffer.size() - 1 - inDataPtr->scrollingRegion.lastRow/* zero-based */));
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
static void
locateScrollingRegionTop	(My_ScreenBufferPtr						inDataPtr,
							 My_ScreenBufferLineList::iterator&		outTopLine)
{
	//assert(inDataPtr->scrollingRegion.firstRow >= 0);
	
	// find top line iterator in screen
	outTopLine = inDataPtr->screenBuffer.begin();
	std::advance(outTopLine, inDataPtr->scrollingRegion.firstRow/* zero-based */);
}// locateScrollingRegionTop


/*!
Returns "true" only if the specified string is the same
as the text at the given memory location, taking into
account the text comparison rules of the current system
script and (if desired) case sensitivity.

Based directly on the SimpleText text search mechanism.

(3.0)
*/
static Boolean
matchingText	(Ptr			inBufferPtr,
				 char const*	inPhraseBuffer,
				 UInt32			inPhraseBufferLength,
				 Boolean		inIsCaseSensitive)
{
	Handle		itl2Table = nullptr;
	long		offset = 0L,
				length = 0L;
	short		script = 0;
	Boolean		result = false;
	
	
	script = FontToScript(GetSysFont());
	GetIntlResourceTable(script, smWordSelectTable, &itl2Table, &offset, &length);
	if (itl2Table != nullptr)
	{
		if (inIsCaseSensitive) result = (CompareText(inBufferPtr, inPhraseBuffer, inPhraseBufferLength, inPhraseBufferLength, itl2Table) == 0);
		else result = (IdenticalText(inBufferPtr, inPhraseBuffer, inPhraseBufferLength, inPhraseBufferLength, itl2Table) == 0);
	}
	return result;
}// matchingText


/*!
Changes both the row and column of the cursor in
the specified terminal screen at the same time.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
static inline void
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
static inline void
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
static void
moveCursorDownOrScroll	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY == inDataPtr->scrollingRegion.lastRow)
	{
		scrollTerminalBuffer(inDataPtr);
	}
	else if (inDataPtr->current.cursorY < (inDataPtr->screenBuffer.size() - 1))
	{
		moveCursorDown(inDataPtr);
	}
}// moveCursorDownOrScroll


/*!
Moves the cursor to the row below its current row,
or does nothing if the cursor is already at the
bottom of the scrolling region.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.1)
*/
static void
moveCursorDownOrStop	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY < (inDataPtr->screenBuffer.size() - 1))
	{
		moveCursorDown(inDataPtr);
	}
}// moveCursorDownOrStop


/*!
Moves the cursor to the last row (constrained by
the current scrolling region).

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
static inline void
moveCursorDownToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->scrollingRegion.lastRow);
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
static inline void
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
static inline void
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
static inline void
moveCursorLeftToHalf	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		halfway = INTEGER_HALVED(inDataPtr->text.visibleScreen.numberOfColumnsPermitted);
	
	
	if (inDataPtr->current.cursorX >= halfway)
	{
		moveCursorX(inDataPtr, halfway);
	}
}// moveCursorLeftToHalf


/*!
Moves the cursor to the column immediately following
its current column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
static inline void
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
static inline void
moveCursorRightToEdge		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1);
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
static inline void
moveCursorRightToNextTabStop	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX + tabStopGetDistanceFromCursor(inDataPtr));
}// moveCursorRightToNextTabStop


/*!
Moves the cursor to the row above its current row.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
static inline void
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
static void
moveCursorUpOrScroll	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY == inDataPtr->scrollingRegion.firstRow)
	{
		My_ScreenBufferLineList::iterator	scrollingRegionBegin;
		
		
		locateScrollingRegionTop(inDataPtr, scrollingRegionBegin);
		bufferInsertBlankLines(inDataPtr, 1/* number of blank lines */, scrollingRegionBegin/* insertion row */);
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
static inline void
moveCursorUpToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->scrollingRegion.firstRow);
}// moveCursorUpToEdge


/*!
Changes the horizontal column of the cursor in
the specified terminal screen.  All cursor-dependent
data is automatically synchronized if necessary.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
static inline void
moveCursorX		(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inNewX)
{
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
	
	//if (inDataPtr->cursorVisible)
	{
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorLocation, inDataPtr->selfRef);
	}
}// moveCursorX


/*!
Changes the horizontal column of the cursor in the
specified terminal screen.  All cursor-dependent
data is automatically synchronized if necessary.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
static inline void
moveCursorY		(My_ScreenBufferPtr		inDataPtr,
				 My_ScreenRowIndex		inNewY)
{
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	if ((inDataPtr->mayNeedToSaveToScrollback) && (0 != inNewY))
	{
		// once the cursor leaves the home position, there is no
		// chance of having to save the top line into scrollback
		inDataPtr->mayNeedToSaveToScrollback = false;
	}
	inDataPtr->current.attributeBits &= ~(cursorLineIterator->globalAttributes);
	
	// don’t allow the cursor to fall off the screen
	{
		SInt16		newCursorY = inNewY;
		
		
		newCursorY = std::max< SInt16 >(newCursorY, 0);
		newCursorY = std::min< SInt16 >(newCursorY, inDataPtr->screenBuffer.size() - 1);
		inDataPtr->current.cursorY = newCursorY;
	}
	
	// cursor has moved, so find the data for its new row
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	inDataPtr->current.attributeBits |= cursorLineIterator->globalAttributes;
	if ((0 == inDataPtr->current.cursorY) && (0 == inDataPtr->current.cursorX))
	{
		// if the cursor moves into the home position, flag this
		// and prepare to possibly save the line contents if any
		// subsequent erase commands show up
		inDataPtr->mayNeedToSaveToScrollback = true;
	}
	
	//if (inDataPtr->cursorVisible)
	{
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorLocation, inDataPtr->selfRef);
	}
}// moveCursorY


/*!
Resets terminal modes to defaults, clears the screen
and notifies any listeners of reset events.

(2.6)
*/
static void
resetTerminal   (My_ScreenBufferPtr  inDataPtr)
{
	setScrollingRegionTop(inDataPtr, 0);
	setScrollingRegionBottom(inDataPtr, inDataPtr->screenBuffer.size() - 1);
	inDataPtr->currentEscapeSeqParamEndIndex = 0;
	vt100ANSIMode(inDataPtr);
	//inDataPtr->modeAutoWrap = false; // 3.0 - do not touch the auto-wrap setting
	inDataPtr->modeCursorKeys = false;
	inDataPtr->modeApplicationKeys = false;
	inDataPtr->modeOriginRedefined = false;
	inDataPtr->previous.attributeBits = kInvalidTerminalTextAttributes;
	inDataPtr->current.attributeBits = 0;
	inDataPtr->modeInsertNotReplace = false;
	inDataPtr->modeNewLineOption = false;
	moveCursor(inDataPtr, 0, 0);
	inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
	inDataPtr->printing.bufferTail = 0x00000000;
	if (Terminal_PrintingInProgress(inDataPtr->selfRef))
	{
		Terminal_PrintingEnd(inDataPtr->selfRef); // 3.0
	}
	bufferEraseVisibleScreenWithUpdate(inDataPtr);
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
static SessionRef
returnListeningSession	(My_ScreenBufferPtr		inDataPtr)
{
	return inDataPtr->listeningSession;
}// returnListeningSession


/*!
Utility to extract a generic terminal type
from a more specific terminal emulation type.
For example, the VT family of terminals may
share many common emulation behaviors, so in
some cases you may only care that you are
currently using *some* kind of VT terminal,
you may not care which specific VT terminal
(e.g. VT220 versus VT100).

(3.0)
*/
static inline Terminal_EmulatorType
returnTerminalType		(Terminal_Emulator	inEmulator)
{
	return ((inEmulator & kTerminal_EmulatorTypeMask) >> kTerminal_EmulatorTypeByteShift);
}// returnTerminalType


/*!
Utility to extract a generic terminal variant
from a more specific terminal emulation type.
For example, if you extracted the terminal type
and found you have a VT terminal, this routine
could tell you if the terminal is specifically
a VT100 or VT220.

(3.0)
*/
static inline Terminal_EmulatorVariant
returnTerminalVariant		(Terminal_Emulator	inEmulator)
{
	return ((inEmulator & kTerminal_EmulatorVariantMask) >> kTerminal_EmulatorVariantByteShift);
}// returnTerminalVariant


/*!
Appends the contents of the visible screen to the scrollback
buffer, usually in preparation for then blanking the visible
screen area.

(3.0)
*/
static void
saveToScrollback	(My_ScreenBufferPtr		inDataPtr)
{
	//Console_WriteLine("saveToScrollback");
	
	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->scrollingRegion.firstRow == 0) &&
		(inDataPtr->scrollingRegion.lastRow == (inDataPtr->screenBuffer.size() - 1)))
	{
		if (insertNewLines(inDataPtr, inDataPtr->screenBuffer.size(), false/* append only */))
		{
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, inDataPtr->selfRef/* context */);
		}
	}
}// saveToScrollback


/*!
Moves the scrolling region up one line.

(3.0)
*/
static void
scrollTerminalBuffer	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY < inDataPtr->screenBuffer.size())
	{
		if ((inDataPtr->text.scrollback.enabled) &&
			(inDataPtr->scrollingRegion.firstRow == 0) &&
			((inDataPtr->scrollingRegion.lastRow == (inDataPtr->screenBuffer.size() - 1)) ||
				inDataPtr->text.scrollback.enabled))
		{
			// scrolling region is entire screen, and lines are being saved off the top
			(Boolean)insertNewLines(inDataPtr, 1/* number of lines */, false/* append only */);
			
			// displaying right from top of scrollback buffer; topmost line being shown
			// has in fact vanished; update the display to show this
			//Console_WriteLine("text changed event: scroll terminal buffer");
			{
				Terminal_RangeDescription	range;
				
				
				range.screen = inDataPtr->selfRef;
				range.firstRow = 0;
				range.firstColumn = 0;
				range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
				range.rowCount = inDataPtr->visibleBoundary.lastRow - inDataPtr->visibleBoundary.firstRow + 1;
				changeNotifyForTerminal(inDataPtr, kTerminal_ChangeText, &range);
			}
		}
		else
		{
			My_ScreenBufferLineList::iterator	scrollingRegionBegin;
			
			
			// region being scrolled is not entire screen; no saving of lines
			locateScrollingRegionTop(inDataPtr, scrollingRegionBegin);
			bufferRemoveLines(inDataPtr, 1/* number of lines */, scrollingRegionBegin);
		}
		
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, inDataPtr->selfRef/* context */);
	}
}// scrollTerminalBuffer


/*!
Performs a search on the supplied handle, starting at
the provided offset.  Returns the new selection start
and end values, and "true" only if the search is
successful.

Based directly on the SimpleText text search mechanism.

(3.0)
*/
static Boolean
searchBufferForPhrase	(Handle			inBufferHandle,
						 long			inStartOffset,
						 char const*	inPhraseBuffer,
						 UInt32			inPhraseBufferLength,
						 Boolean		inIsCaseSensitive,
						 Boolean		inIsBackwards,
						 Boolean		inIsWraparound,
						 long*			outFoundStringStartOffset,
						 long*			outFoundStringEndOffsetOrNull)
{
	SInt8		oldHandleState = '\0';
	long		startOffset = inStartOffset;
	Ptr			startPtr = nullptr;
	Ptr			endPtr = nullptr;
	Ptr			searchPtr = nullptr;
	Boolean		result = false;
	
	
	oldHandleState = HGetState(inBufferHandle);
	HLock(inBufferHandle);
	
	// back up one when searching backwards, to avoid hitting
	// the current character
	if (inIsBackwards)
	{
		if (startOffset != 0)
		{
			--startOffset;
		}
		else
		{
			if (inIsWraparound) startOffset = GetHandleSize(inBufferHandle);
			else return(false);
		}
	}
	
	// determine the bounds of the search
	startPtr = (*inBufferHandle) + startOffset;
	if (inIsWraparound)
	{
		if (inIsBackwards)
		{
			// go backwards until just after the start, or to the
			// beginning of the document if the start offset is
			// already at the end
			if (startOffset == GetHandleSize(inBufferHandle)) endPtr = *inBufferHandle;
			else endPtr = startPtr + 1;
		}
		else
		{
			// go forwards until just before the start, or to the
			// end of the document if the start offset is already
			// at the beginning
			if (startOffset == 0) endPtr = *inBufferHandle + GetHandleSize(inBufferHandle);
			else endPtr = startPtr - 1;
		}
	}
	else
	{
		if (inIsBackwards)
		{
			// go back until the beginning of the document is reached
			endPtr = *inBufferHandle - 1;
		}
		else
		{
			// go forward until the end of the document is reached
			endPtr = *inBufferHandle + GetHandleSize(inBufferHandle);
		}
	}
	
	searchPtr = startPtr;
	while (searchPtr != endPtr)
	{
		short	byteType = 0;
		
		
		byteType = CharacterByteType(startPtr, searchPtr - startPtr, smCurrentScript);
		
		if (((byteType == smSingleByte) || (byteType == smFirstByte)) &&
			matchingText(searchPtr, inPhraseBuffer, inPhraseBufferLength, inIsCaseSensitive))
		{
			result = true;
			*outFoundStringStartOffset = searchPtr - *inBufferHandle;
			if (outFoundStringEndOffsetOrNull != nullptr) *outFoundStringEndOffsetOrNull = *outFoundStringStartOffset + inPhraseBufferLength;
			break;
		}
		
		if (inIsBackwards) --searchPtr;
		else ++searchPtr;
		
		if (inIsWraparound)
		{
			if (searchPtr < *inBufferHandle) searchPtr = *inBufferHandle + GetHandleSize(inBufferHandle);
			if (searchPtr > *inBufferHandle + GetHandleSize(inBufferHandle)) searchPtr = *inBufferHandle;
		}
	}
	
	HSetState(inBufferHandle, oldHandleState);
	
	return result;
}// searchBufferForPhrase


/*!
Changes the logical cursor state.  Performed in a
function for consistency in case, for instance,
callbacks are invoked in the future whenever this
flag changes.

(3.0)
*/
static inline void
setCursorVisible	(My_ScreenBufferPtr		inDataPtr,
					 Boolean				inIsVisible)
{
	inDataPtr->cursorVisible = inIsVisible;
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorState, inDataPtr->selfRef);
}// setCursorVisible


/*!
Sets the “logical bottom” of the main screen, which is
initially the very last screen row.  A VT command can
change this, affecting many subsequent operations such
as scrolling and cursor movement.

IMPORTANT:	ALWAYS use setScrollingRegion...() routines
			to set the scrolling region, because it is
			kept in sync with various other things such
			as key row iterators.

(3.0)
*/
static void
setScrollingRegionBottom	(My_ScreenBufferPtr		inDataPtr,
							 UInt16					inNewBottomRow)
{
	inDataPtr->scrollingRegion.lastRow = inNewBottomRow;
}// setScrollingRegionBottom


/*!
Sets the “logical top” of the main screen, which is
initially row zero (home position).  A VT command can
change this, affecting many subsequent operations such
as scrolling and cursor movement.

IMPORTANT:	ALWAYS use setScrollingRegion...() routines
			to set the scrolling region, because it is
			kept in sync with various other things such
			as key row iterators.

(3.0)
*/
static void
setScrollingRegionTop	(My_ScreenBufferPtr		inDataPtr,
						 UInt16					inNewTopRow)
{
	inDataPtr->scrollingRegion.firstRow = inNewTopRow;
}// setScrollingRegionTop


/*!
Removes all tab stops.  See also tabStopInitialize(),
which sets tabs to reasonable default values.

(3.0)
*/
static void
tabStopClearAll		(My_ScreenBufferPtr		inDataPtr)
{
	My_TabStopList::iterator	tabStopIterator;
	
	
	for (tabStopIterator = inDataPtr->tabSettings.begin(); tabStopIterator != inDataPtr->tabSettings.end(); ++tabStopIterator)
	{
		*tabStopIterator = kMy_TabClear;
	}
}// tabStopClearAll


/*!
Returns the number of spaces until the next tab
stop, based on the current cursor position of
the specified screen.

(2.6)
*/
static UInt16
tabStopGetDistanceFromCursor	(My_ScreenBufferConstPtr	inDataPtr)
{
	UInt16		result = 0;
	
	
	if (inDataPtr->current.cursorX < (inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1))
	{
		result = inDataPtr->current.cursorX + 1;
		while ((inDataPtr->tabSettings[result] != kMy_TabSet) &&
				(result < (inDataPtr->text.visibleScreen.numberOfColumnsPermitted - 1)))
		{
			++result;
		}
		result = (result - inDataPtr->current.cursorX);
	}
	return result;
}// tabStopGetDistanceFromCursor


/*!
Reset tabs to default stops (one every "kMy_TabStop"
columns starting at the first column, and one at the
last column).

(3.0)
*/
static void
tabStopInitialize	(My_ScreenBufferPtr		inDataPtr)
{
	My_TabStopList::iterator	tabStopIterator;
	
	
	tabStopClearAll(inDataPtr);
	
	// IMPORTANT: This works only because the list is initialized to be a multiple
	//            of the tab stop distance.  If this were not true, std::advance()
	//            would jump the iterator to an invalid value, causing this to crash.
	for (tabStopIterator = inDataPtr->tabSettings.begin(); tabStopIterator != inDataPtr->tabSettings.end();
			std::advance(tabStopIterator, STATIC_CAST(kMy_TabStop, My_TabStopList::difference_type)/* tab distance */))
	{
		*tabStopIterator = kMy_TabSet;
	}
	assert(!inDataPtr->tabSettings.empty());
	inDataPtr->tabSettings.back() = kMy_TabSet; // also make last column a tab
}// tabStopInitialize


/*!
Translates the given character code using the rules
of the current character set of the specified screen
(character sets G0 or G1, for VT terminals).  The
new code is returned, which may be unchanged.

(3.0)
*/
static inline char
translateCharacter	(My_ScreenBufferPtr		inDataPtr,
					 char					inCharacter)
{
	char	result = inCharacter;
	
	
	switch (inDataPtr->current.characterSetInfoPtr->translationTable)
	{
	case kMy_CharacterSetVT100UnitedStates:
		// this is the default; do nothing
		break;
	
	case kMy_CharacterSetVT100UnitedKingdom:
		// the only difference between ASCII and U.K. is that
		// the pound sign (#) is a British currency symbol (£)
		if (inCharacter == '#') result = '£';
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// translateCharacter


/*!
Handles the VT100 'DECALN' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static void
vt100AlignmentDisplay	(My_ScreenBufferPtr		inDataPtr)
{
	// first clear the buffer, saving it to scrollback if appropriate
	bufferEraseVisibleScreenWithUpdate(inDataPtr);
	
	// now fill the lines with letter-E characters; technically this
	// also will reset all attributes and this may not be part of the
	// VT100 specification (but it seems reasonable to get rid of any
	// special colors or oversized text when doing screen alignment)
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		for (lineIterator = inDataPtr->screenBuffer.begin(); lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferLineFill(inDataPtr, *lineIterator, 'E', kTerminalTextAttributesAllOff, true/* change line global attributes to match */);
		}
	}
	
	// update the display - UNIMPLEMENTED
}// vt100AlignmentDisplay


/*!
Switches a VT100 terminal to ANSI mode, which means it
no longer accepts VT52 sequences.

(3.1)
*/
static void
vt100ANSIMode	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->modeANSIEnabled = true;
	inDataPtr->stateDeterminant = emulatorVT100StateDeterminant;
	inDataPtr->transitionHandler = emulatorVT100StateTransition;
	initializeParserStateStack(inDataPtr);
}// vt100ANSIMode


/*!
Handles the VT100 'CUB' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100CursorBackward		(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
	{
		if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
		else moveCursorLeftToEdge(inDataPtr);
	}
	else
	{
		SInt16		newValue = inDataPtr->current.cursorX - inDataPtr->currentEscapeSeqParamValues[0];
		
		
		if (newValue < 0) newValue = 0;
		moveCursorX(inDataPtr, newValue);
	}
}// vt100CursorBackward


/*!
Handles the VT100 VT52-compatibility sequence 'ESC D'.
See the VT100 manual for complete details.

(3.0)
*/
static inline void
vt100CursorBackward_vt52	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
	else moveCursorLeftToEdge(inDataPtr);
}// vt100CursorBackward_vt52


/*!
Handles the VT100 'CUD' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100CursorDown		(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
	{
		if (inDataPtr->current.cursorY < inDataPtr->scrollingRegion.lastRow) moveCursorDown(inDataPtr);
		else moveCursorDownToEdge(inDataPtr);
	}
	else
	{
		My_ScreenRowIndex	newValue = inDataPtr->current.cursorY +
										inDataPtr->currentEscapeSeqParamValues[0];
		
		
		if (newValue > inDataPtr->scrollingRegion.lastRow)
		{
			newValue = inDataPtr->scrollingRegion.lastRow;
		}
		if (newValue >= inDataPtr->screenBuffer.size())
		{
			newValue = inDataPtr->screenBuffer.size() - 1;
		}
		moveCursorY(inDataPtr, newValue);
	}
}// vt100CursorDown


/*!
Handles the VT100 VT52-compatibility sequence 'ESC B'.
See the VT100 manual for complete details.

(3.0)
*/
static inline void
vt100CursorDown_vt52	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY < inDataPtr->scrollingRegion.lastRow) moveCursorDown(inDataPtr);
	else moveCursorDownToEdge(inDataPtr);
}// vt100CursorDown_vt52


/*!
Handles the VT100 'CUF' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100CursorForward		(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		rightLimit = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - ((inDataPtr->modeAutoWrap) ? 0 : 1);
	
	
	// the default value is 1 if there are no parameters
	if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
	{
		if (inDataPtr->current.cursorX < rightLimit) moveCursorRight(inDataPtr);
		else moveCursorRightToEdge(inDataPtr);
	}
	else
	{
		SInt16		newValue = inDataPtr->current.cursorX + inDataPtr->currentEscapeSeqParamValues[0];
		
		
		if (newValue > rightLimit) newValue = rightLimit;
		moveCursorX(inDataPtr, newValue);
	}
}// vt100CursorForward


/*!
Handles the VT100 VT52-compatibility sequence 'ESC C'.
See the VT100 manual for complete details.

(3.0)
*/
static inline void
vt100CursorForward_vt52	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		rightLimit = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - ((inDataPtr->modeAutoWrap) ? 0 : 1);
	
	
	if (inDataPtr->current.cursorX < rightLimit) moveCursorRight(inDataPtr);
	else moveCursorRightToEdge(inDataPtr);
}// vt100CursorForward_vt52


/*!
Handles the VT100 'CUU' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100CursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->currentEscapeSeqParamValues[0] < 1)
	{
		if (inDataPtr->current.cursorY > inDataPtr->scrollingRegion.firstRow)
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
		SInt16				newValue = inDataPtr->current.cursorY - inDataPtr->currentEscapeSeqParamValues[0];
		My_ScreenRowIndex	rowIndex = 0;
		
		
		if (newValue < 0)
		{
			newValue = 0;
		}
		
		rowIndex = STATIC_CAST(newValue, My_ScreenRowIndex);
		if (rowIndex < inDataPtr->scrollingRegion.firstRow)
		{
			rowIndex = inDataPtr->scrollingRegion.firstRow;
		}
		moveCursorY(inDataPtr, rowIndex);
	}
}// vt100CursorUp


/*!
Handles the VT100 VT52-compatibility sequence 'ESC A'.
See the VT100 manual for complete details.

(3.0)
*/
static inline void
vt100CursorUp_vt52		(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY > inDataPtr->scrollingRegion.firstRow)
	{
		moveCursorUp(inDataPtr);
	}
	else
	{
		moveCursorUpToEdge(inDataPtr);
	}
}// vt100CursorUp_vt52


/*!
Handles the VT100 'DA' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100DeviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// support GPO (graphics processor option) and AVO (advanced video option)
		Session_SendData(session, "\033[?1;6c", 7);
	}
}// vt100DeviceAttributes


/*!
Handles the VT100 'DSR' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static void
vt100DeviceStatusReport		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->currentEscapeSeqParamValues[0])
	{
	case 5:
		// report status using a DSR control sequence
		{
			SessionRef	session = returnListeningSession(inDataPtr);
			
			
			if (nullptr != session)
			{
				Session_SendData(session, "\033[0n"/* 0 means “ready, no malfunctions detected”; see VT100 manual on DSR for details */,
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
			
			// TEMPORARY (3.0) - I think there is a bug here, DECOM (origin mode) must
			//                   be respected, offsetting the reported cursor location
			//                   relative to the topmost scrolling row if applicable
			
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
					Session_SendData(session, reportBufferString.c_str(), reportBufferString.size());
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// vt100DeviceStatusReport


/*!
Handles the VT100 'ED' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100EraseInDisplay		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->currentEscapeSeqParamValues[0])
	{
	case -1: // -1 means no parameter was given; the default value is 0
	case 0:
		bufferEraseFromCursorToEnd(inDataPtr);
		break;
	
	case 1:
		bufferEraseFromHomeToCursor(inDataPtr);
		break;
	
	case 2:
		bufferEraseVisibleScreenWithUpdate(inDataPtr);
		break;
	
	default:
		// ???
		break;
	}
}// vt100EraseInDisplay


/*!
Handles the VT100 'EL' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100EraseInLine	(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->currentEscapeSeqParamValues[0])
	{
	case -1: // -1 means no parameter was given; the default value is 0
	case 0:
		bufferEraseFromCursorColumnToLineEnd(inDataPtr);
		break;
	
	case 1:
		bufferEraseFromLineBeginToCursorColumn(inDataPtr);
		break;
	
	case 2:
		bufferEraseLineWithUpdate(inDataPtr, -1/* line number, or negative for cursor line */);
		break;
	
	default:
		// ???
		break;
	}
}// vt100EraseInLine


/*!
Handles the VT100 'ESC Z' sequence, which should only
be recognized in VT52 compatibility mode.  See the
VT100 manual for complete details.

(3.0)
*/
static inline void
vt100Identify_vt52	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		Session_SendData(session, "\033/Z", 3);
	}
}// vt100Identify_vt52


/*!
Handles the VT100 'DECLL' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100LoadLEDs	(My_ScreenBufferPtr		inDataPtr)
{
	register SInt16		i = 0;
	
	
	for (i = 0; i <= inDataPtr->currentEscapeSeqParamEndIndex; ++i)
	{
		if (inDataPtr->currentEscapeSeqParamValues[i] == -1)
		{
			// no value; default is “all off”
			highlightLED(inDataPtr, 0);
		}
		else if (inDataPtr->currentEscapeSeqParamValues[i] == 137)
		{
			// could decide to emulate the “ludicrous repeat rate” bug
			// of the VT100, here; in combination with key click, this
			// should basically make each key press play a musical note :)
		}
		else
		{
			// a parameter of 1 means “LED 1 on”, 2 means “LED 2 on”,
			// 3 means “LED 3 on”, 4 means “LED 4 on”; 0 means “all off”
			highlightLED(inDataPtr, inDataPtr->currentEscapeSeqParamValues[i]/* LED # */);
		}
	}
}// vt100LoadLEDs


/*!
Handles the VT100 'SM' sequence (if "inIsModeEnabled" is true)
or the VT100 'RM' sequence (if "inIsModeEnabled" is false).
See the VT100 manual for complete details.

(2.6)
*/
static void
vt100ModeSetReset	(My_ScreenBufferPtr		inDataPtr,
					 Boolean				inIsModeEnabled)
{
	switch (inDataPtr->currentEscapeSeqParamValues[0])
	{
	case -2: // DEC-private control sequence
		{
			register SInt16		i = 0;
			Boolean				emulateDECOMBug = false;
			
			
			for (i = 0; i <= inDataPtr->currentEscapeSeqParamEndIndex; ++i)
			{
				switch (inDataPtr->currentEscapeSeqParamValues[i])
				{
				case 1:
					// DECCKM (cursor-key mode)
					inDataPtr->modeCursorKeys = inIsModeEnabled;
					break;
				
				case 2:
					// DECANM (ANSI/VT52 mode); this is only possible to reset, not set
					// (the set is accomplished in a different way)
					if (false == inIsModeEnabled)
					{
						vt100VT52Mode(inDataPtr);
					}
					break;
				
				case 3:
					// DECCOLM (80/132 column switch)
					{
						(Boolean)Commands_ExecuteByIDUsingEvent((inIsModeEnabled)
																? kCommandLargeScreen
																: kCommandSmallScreen);
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
						// ALL subsequent mode changes are ignored; since technically MacTelnet
						// is emulating a *real* VT100 and not the ideal manual, you could enable
						// this flag to replicate VT100 behavior instead of what the manual says
						emulateDECOMBug = true;
					#endif
						
						inDataPtr->modeOriginRedefined = inIsModeEnabled;
						
						// home the cursor, but relative to the new top margin
						moveCursor(inDataPtr, 0, (inIsModeEnabled) ? inDataPtr->scrollingRegion.firstRow : 0);
					}
					break;
				
				case 7: // DECAWM (auto-wrap mode)
					inDataPtr->modeAutoWrap = inIsModeEnabled;
					break;
				
				case 4: // DECSCLM (if enabled, scrolling is smooth at 6 lines per second; otherwise, instantaneous)
				case 8: // DECARM (auto-repeating)
				case 9: // DECINLM (interlace)
				case 0: // error, ignored
				case -1: // no value given (set and reset do not have default values)
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
}// vt100ModeSetReset


/*!
Scans the specified buffer for parameter-like data
(e.g. ;0;05;21;2) and saves all the parameters it
finds.  The number of characters “used” is returned
(you can use this to offset your original buffer
pointer appropriately).

Typically this is done immediately after a VT100
control sequence inducer (CSI, a.k.a. ESC-[) is
received.  That way, any terminal sequence which
has a CSI in it (i.e. anything that needs parameters)
will have all defined parameters available to it.

(3.1)
*/
static UInt32
vt100ReadCSIParameters	(My_ScreenBufferPtr		inDataPtr,
						 UInt8 const*			inBuffer,
						 UInt32					inLength)
{
	UInt32			result = 0;
	UInt8 const*	bufferIterator = inBuffer;
	Boolean			done = false;
	SInt16&			terminalEndIndexRef = inDataPtr->currentEscapeSeqParamEndIndex;
	
	
	for (; (false == done) && (bufferIterator != (inBuffer + inLength)); ++bufferIterator, ++result)
	{
		//Console_WriteValueCharacter("scan", *bufferIterator);
		switch (*bufferIterator)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			// parse numeric parameter
			{
				// rename this incredibly long expression, since it’s needed a lot here!
				SInt16&		valueRef = inDataPtr->currentEscapeSeqParamValues[terminalEndIndexRef];
				
				
				if (valueRef < 0) valueRef = 0;
				valueRef *= 10;
				valueRef += *bufferIterator - '0';
			}
			break;
		
		case ';':
			// parameter separator
			if (terminalEndIndexRef < kMy_MaximumANSIParameters) ++terminalEndIndexRef;
			break;
		
		case '?':
			// manual says the FIRST character must be this to enter
			// DEC-private parameter mode; otherwise, ignore (stop here)
			if (0 == result)
			{
				// DEC-private parameters
				inDataPtr->currentEscapeSeqParamValues[terminalEndIndexRef++] = -2;
			}
			else
			{
				done = true;
			}
			break;
		
		default:
			// not part of a parameter sequence
			done = true;
			break;
		}
	}
	
	// ignore final character if the loop was broken prematurely
	if (done) --result;
	
	// debug - write results to console
	//Console_WriteValue("parameters found", terminalEndIndexRef + 1);
	//for (register SInt16 i = 0; i <= terminalEndIndexRef; ++i)
	//{
	//	Console_WriteValue("found post-CSI parameter", inDataPtr->currentEscapeSeqParamValues[i]);
	//}
	
	return result;
}// vt100ReadCSIParameters


/*!
Handles the VT100 'DECREQT' sequence.  See the VT100
manual for complete details.

(3.1)
*/
static void
vt100ReportTerminalParameters	(My_ScreenBufferPtr		inDataPtr)
{
	UInt16 const	kRequestType = (inDataPtr->currentEscapeSeqParamValues[0] != -1)
									? inDataPtr->currentEscapeSeqParamValues[0]
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
			Session_SendData(session, reportBufferString.c_str(), reportBufferString.size());
		}
	}
}// vt100ReportTerminalParameters


/*!
Handles the VT100 'DECSTBM' sequence.  See the VT100
manual for complete details.

(3.0)
*/
static inline void
vt100SetTopAndBottomMargins		(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->currentEscapeSeqParamValues[0] < 0)
	{
		// no top parameter given; default is top of screen
		setScrollingRegionTop(inDataPtr, 0);
	}
	else
	{
		// parameter is the line number of the new first line of the scrolling region; the
		// input is 1-based but internally it is a zero-based array index, so subtract one
		setScrollingRegionTop(inDataPtr, inDataPtr->currentEscapeSeqParamValues[0] - 1);
	}
	
	if (inDataPtr->currentEscapeSeqParamValues[1] < 0)
	{
		// no bottom parameter given; default is bottom of screen
		setScrollingRegionBottom(inDataPtr, inDataPtr->screenBuffer.size() - 1);
	}
	else
	{
		// parameter is the line number of the new last line of the scrolling region; the
		// input is 1-based but internally it is a zero-based array index, so subtract one
		setScrollingRegionBottom(inDataPtr, inDataPtr->currentEscapeSeqParamValues[1] - 1);
	}
	
	// ensure the region is not outside the maximum range
	//if (inDataPtr->scrollingRegion.firstRow < 0)
	//{
	//	setScrollingRegionTop(inDataPtr, 0);
	//}
	if (inDataPtr->scrollingRegion.firstRow >= inDataPtr->screenBuffer.size())
	{
		setScrollingRegionTop(inDataPtr, inDataPtr->screenBuffer.size() - 1);
	}
	if (inDataPtr->scrollingRegion.lastRow < 1)
	{
		setScrollingRegionBottom(inDataPtr, inDataPtr->screenBuffer.size() - 1);
	}
	if (inDataPtr->scrollingRegion.lastRow >= inDataPtr->screenBuffer.size())
	{
		setScrollingRegionBottom(inDataPtr, inDataPtr->screenBuffer.size() - 1);
	}
	
	// VT100 requires that the range be 2 lines minimum
	if (inDataPtr->scrollingRegion.firstRow >= inDataPtr->scrollingRegion.lastRow)
	{
		if (inDataPtr->scrollingRegion.lastRow >= 1)
		{
			setScrollingRegionTop(inDataPtr, inDataPtr->scrollingRegion.lastRow - 1);
		}
		else
		{
			setScrollingRegionBottom(inDataPtr, inDataPtr->scrollingRegion.firstRow + 1);
		}
	}
	
	// home the cursor, but relative to any current top margin
	moveCursor(inDataPtr, 0, (inDataPtr->modeOriginRedefined) ? inDataPtr->scrollingRegion.firstRow : 0);
}// vt100SetTopAndBottomMargins


/*!
Switches a VT100 terminal to VT52 mode, which means it
starts to accept VT52 sequences.

(3.1)
*/
static void
vt100VT52Mode	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->modeANSIEnabled = false;
	inDataPtr->stateDeterminant = emulatorVT100VT52StateDeterminant;
	inDataPtr->transitionHandler = emulatorVT100VT52StateTransition;
	initializeParserStateStack(inDataPtr);
}// vt100VT52Mode


/*!
Handles the VT220 'DA' sequence.  See the VT220
manual for complete details.

(3.0)
*/
static inline void
vt220DeviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// support GPO (graphics processor option) and AVO (advanced video option)
		Session_SendData(session, "\033[?62;1;6c", 10);
	}
}// vt220DeviceAttributes


/*!
Copies the specified number of characters from the
source to the destination, dropping trailing blanks.
If table is nonzero, then every series of this many
consecutive spaces will be replaced with a single
tab character.

Returns an iterator just past the end of the copied
data in the output buffer.  This allows you to use
the iterator to insert a character immediately after
the text that this routine copies.

Also, the actual number of bytes copied is provided.
It follows that if you were to advance the original
buffer iterator by this amount, the result should be
the returned iterator.

NOTE:   This routine is generic.  It therefore works
		with something as simple as an array of
		characters, or as complex as an STL container
		of characters - your choice!

(3.0)
*/
template < typename src_char_seq_const_iter, typename src_char_seq_size_t,
			typename dest_char_seq_iter, typename dest_char_seq_size_t >
static dest_char_seq_iter
whitespaceSensitiveCopy		(src_char_seq_const_iter	inSourceBuffer,
							 src_char_seq_size_t		inSourceBufferLength,
							 dest_char_seq_iter			outDestinationBuffer,
							 dest_char_seq_size_t		inDestinationBufferLength,
							 dest_char_seq_size_t*		outDestinationCopiedLengthPtr,
							 src_char_seq_size_t		inNumberOfSpacesPerTabOrZeroForNoSubstitution)
{
	dest_char_seq_iter  result = outDestinationBuffer;
	
	
	if (inSourceBufferLength > 0)
	{
		src_char_seq_const_iter		kPastSourceStartIterator;
		src_char_seq_const_iter		pastSourceEndIterator;
		src_char_seq_const_iter		sourceEndIterator;
		src_char_seq_size_t			sourceLength = inSourceBufferLength;
		
		
		// initialize “constant” past-source-start iterator
		kPastSourceStartIterator = inSourceBuffer;
		std::advance(kPastSourceStartIterator, -1);
		
		// initialize past-source-end iterator
		pastSourceEndIterator = inSourceBuffer;
		std::advance(pastSourceEndIterator, inSourceBufferLength);
		
		// initialize source-end iterator
		sourceEndIterator = inSourceBuffer;
		std::advance(sourceEndIterator, inSourceBufferLength - 1);
		
		// skip trailing blanks
		while ((*sourceEndIterator == ' ') && (sourceEndIterator != kPastSourceStartIterator))
		{
			--sourceEndIterator;
			--pastSourceEndIterator;
			--sourceLength;
		}
		
		// if the string contains non-blanks, continue
		if (sourceEndIterator != kPastSourceStartIterator)
		{
			if (inNumberOfSpacesPerTabOrZeroForNoSubstitution == 0)
			{
				// straight character copy
				src_char_seq_size_t		copiedLength = INTEGER_MINIMUM(sourceLength, inDestinationBufferLength);
				
				
				__gnu_cxx::copy_n(inSourceBuffer, copiedLength, outDestinationBuffer);
				result = outDestinationBuffer;
				std::advance(result, copiedLength);
			}
			else
			{
				src_char_seq_const_iter		srcIter = inSourceBuffer;
				dest_char_seq_iter			destIter = outDestinationBuffer;
				dest_char_seq_iter			kPastDestinationEndIterator;
				
				
				// initialize “constant” past-destination-end iterator
				kPastDestinationEndIterator = outDestinationBuffer;
				std::advance(kPastDestinationEndIterator, inDestinationBufferLength);
				
				// tab-replacement copy
				while (srcIter != pastSourceEndIterator)
				{
					// direct copy for non-whitespace
					while ((*srcIter != ' ') && (srcIter != pastSourceEndIterator) && (destIter != kPastDestinationEndIterator))
					{
						*destIter++ = *srcIter++;
					}
					
					// for whitespace, replace with tabs when appropriate
					if (srcIter != pastSourceEndIterator)
					{
						src_char_seq_size_t		numberOfConsecutiveSpaces = 0;
						
						
						while ((*srcIter == ' ') && (srcIter != pastSourceEndIterator) && (destIter != kPastDestinationEndIterator))
						{
							*destIter++ = *srcIter++;
							++numberOfConsecutiveSpaces;
							if (numberOfConsecutiveSpaces == inNumberOfSpacesPerTabOrZeroForNoSubstitution)
							{
								// replace series of spaces with a tab
								std::advance(destIter, -inNumberOfSpacesPerTabOrZeroForNoSubstitution);
								*destIter++ = '\011';
								numberOfConsecutiveSpaces = 0;
							}
						}
					}
				}
				result = destIter;
			}
		}
	}
	
	*outDestinationCopiedLengthPtr = std::distance(outDestinationBuffer, result);
	return result;
}// whitespaceSensitiveCopy

// BELOW IS REQUIRED NEWLINE TO END FILE
