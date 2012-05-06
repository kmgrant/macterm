/*!	\file VectorInterpreter.cp
	\brief Takes Tektronix codes as input, and sends the output
	to real graphics devices.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#include <UniversalDefines.h>

// standard-C++ includes
#include <deque>
#include <map>

// library includes
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// application includes
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

UInt16 const	kMy_NumberOfSupportedTargets = 2;	//!< e.g. screen, bitmap, ...

/* temporary states */
#define HIY			0	/* waiting for various pieces of coordinates */
#define EXTRA		1
#define LOY			2
#define HIX			3
#define LOX			4

#define DONE		5	/* not waiting for coordinates */
#define ENTERVEC	6	/* entering vector mode */
#define CANCEL		7	/* done but don't draw a line */
#define RS			8	/* RS - incremental plot mode */
#define ESCOUT		9	/* when you receive an escape char after a draw command */
#define CMD0		50	/* got esc, need 1st cmd letter */
#define SOMEL		51	/* got esc L, need 2nd letter */
#define IGNORE		52	/* ignore next char */
#define SOMEM		53	/* got esc M, need 2nd letter */
#define IGNORE2		54
#define INTEGER		60	/* waiting for 1st integer part */
#define INTEGER1	61	/* waiting for 2nd integer part */
#define INTEGER2	62	/* waiting for 3rd (last) integer part */
#define COLORINT	70
#define GTSIZE0		75
#define GTSIZE1		76
#define	GTEXT		77	/* TEK4105 GraphText			17jul90dsw */
#define MARKER		78	/* TEK4105 Marker select		17jul90dsw */
#define	GTPATH		79	/* TEK4105 GraphText path		17jul90dsw */
#define SOMET		80
#define JUNKARRAY	81
#define STARTDISC	82
#define DISCARDING	83
#define	FPATTERN	84	/* TEK4105 FillPattern			17jul90dsw */
#define	GTROT		85	/* TEK4105 GraphText rotation	17jul90dsw */
#define GTROT1		86
#define	GTINDEX		87	/* TEK4105 GraphText color		17jul90dsw */
#define PANEL		88	/* TEK4105 Begin Panel			23jul90dsw */
#define	SUBPANEL	89	/* TEK4105 Begin^2 Panel		25jul90dsw */
#define TERMSTAT	90	/* TEK4105 Report Term Status	24jul90dsw */
#define	SOMER		91	/* TEK4105 for ViewAttributes	10jan91dsw */
#define	VIEWAT		92	/* TEK4105 ViewAttributes		10jan91dsw */
#define VIEWAT2		93

/* output modes */
#define ALPHA		0
#define DRAW		1
#define MARK		3
#define TEMPDRAW	101
#define TEMPMOVE	102
#define TEMPMARK	103

/* stroked fonts */
#define CHARWIDE	51		/* total horz. size */
#define CHARTALL	76		/* total vert. size */
#define CHARH		10		/* horz. unit size */
#define CHARV		13		/* vert. unit size */

} // anonymous namespace

#pragma mark Types
namespace {

// TEMPORARY - Convert this to a modern data structure.
struct My_Point
{
	My_Point	(): x(0), y(0), next(nullptr) {}
	My_Point	(SInt16 inX,
				 SInt16	inY)
	: x(inX), y(inY), next(nullptr) {}
	
	SInt16		x;
	SInt16		y;
	My_Point*	next;
};
typedef My_Point*	My_PointList;

typedef void		(*My_NoArgReturnVoidProcPtr)();
typedef SInt16		(*My_NoArgReturnIntProcPtr)();
typedef char const*	(*My_NoArgReturnCharConstPtrProcPtr)();
typedef void		(*My_IDArgReturnVoidProcPtr)(VectorInterpreter_ID);
typedef SInt16		(*My_IDArgReturnIntProcPtr)(VectorInterpreter_ID);
typedef SInt16		(*My_IDIntArgsReturnIntProcPtr)(VectorInterpreter_ID, SInt16);
typedef void		(*My_IDIntArgsReturnVoidProcPtr)(VectorInterpreter_ID, SInt16);
typedef SInt16		(*My_ID2IntArgsReturnIntProcPtr)(VectorInterpreter_ID, SInt16, SInt16);
typedef void		(*My_ID2IntArgsReturnVoidProcPtr)(VectorInterpreter_ID, SInt16, SInt16);
typedef SInt16		(*My_ID4IntArgsReturnIntProcPtr)(VectorInterpreter_ID, SInt16, SInt16, SInt16, SInt16);

struct My_VectorCallbacks
{
	// IMPORTANT: This order is relied upon for initialization below.
	My_NoArgReturnIntProcPtr			newwin;
	My_NoArgReturnCharConstPtrProcPtr	devname;
	My_NoArgReturnVoidProcPtr			init;
	My_IDArgReturnIntProcPtr			gin;
	My_IDIntArgsReturnIntProcPtr		pencolor;
	My_IDArgReturnIntProcPtr			clrscr;
	My_IDArgReturnIntProcPtr			close;
	My_ID2IntArgsReturnIntProcPtr		point;
	My_ID4IntArgsReturnIntProcPtr		drawline;
	My_ID2IntArgsReturnVoidProcPtr		info;
	My_IDArgReturnVoidProcPtr			pagedone;
	My_ID2IntArgsReturnVoidProcPtr		dataline;
	My_ID2IntArgsReturnVoidProcPtr		charmode;
	My_NoArgReturnVoidProcPtr			gmode;
	My_NoArgReturnVoidProcPtr			tmode;
	My_NoArgReturnVoidProcPtr			showcur;
	My_NoArgReturnVoidProcPtr			lockcur;
	My_NoArgReturnVoidProcPtr			hidecur;
	My_IDArgReturnVoidProcPtr			bell;
	My_IDArgReturnVoidProcPtr			uncover;
};
typedef My_VectorCallbacks*		My_VectorCallbacksPtr;

/*!
Used to store drawing commands.
*/
typedef std::deque< SInt16 >	My_VectorDB;

/*!
Stores information used to interpret vector graphics
commands and ultimately render a picture.
*/
struct My_VectorInterpreter
{
	My_VectorInterpreter	(VectorInterpreter_ID, VectorInterpreter_Target, VectorInterpreter_Mode);
	
	inline void
	shrinkVectorDB	(My_VectorDB::size_type);
	
	VectorInterpreter_ID	selfRef;			// the ID given to this structure at construction time
	VectorInterpreter_Mode	commandSet;			// how data is interpreted
	Boolean					pageClears;			// true if PAGE clears the screen, false if it opens a new window
	VectorCanvas_Ref		canvas;				// contains commands to create a picture or paint to a window; must be initialized last
	char	mode,modesave;					/* current output mode */
	char	loy,hiy,lox,hix,ex,ey;			/* current graphics coordinates */
	char	nloy,nhiy,nlox,nhix,nex,ney;	/* new coordinates */
	short	curx,cury;						/* current composite coordinates */
	short	savx,savy;						/* save the panel's x,y */
	short	winbot,wintop,winleft,winright,wintall,winwide; 
		/* position of window in virtual space */
	short	textcol;						/* text starts in 0 or 2048 */
	short	intin;							/* integer parameter being input */
	short	pencolor;						/* current pen color */
	short	fontnum,charx,chary;			/* char size */
	short	count;							/* for temporary use in special state loops */
	char	TEKMarker;						/* 4105 marker type 17jul90dsw */
	char	TEKOutline;						/* 4105 panel outline boolean */
	short	TEKPath;						/* 4105 GTPath */
	short	TEKPattern;						/* 4105 Panel Fill Pattern */
	short	TEKIndex;						/* 4105 GTIndex */
	short	TEKRot;							/* 4105 GTRotation */
	short	TEKSize;						/* 4105 GTSize */
	short	TEKBackground;					/* 4105 Background color */
	My_PointList	TEKPanel;					/* 4105 Panel's list of points */
	My_PointList	current;					/* current point in the list */
	char	state;
	char	savstate;
	// WARNING: shrinkVectorDB() is the recommended way to reduce the size of "commandList", to keep iterators in sync
	My_VectorDB				commandList;		// list of commands
	My_VectorDB::iterator	toCurrentCommand;	// used to track drawing
};
typedef My_VectorInterpreter*			My_VectorInterpreterPtr;
typedef My_VectorInterpreter const*		My_VectorInterpreterConstPtr;

typedef std::map< VectorInterpreter_ID, My_VectorInterpreterPtr >	My_InterpreterPtrByID;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void			clipvec						(My_VectorInterpreterPtr, short, short, short, short,
											 VectorCanvas_PathTarget = kVectorCanvas_PathTargetPrimary);
short			drawc						(My_VectorInterpreterPtr, short);
short			fontnum						(My_VectorInterpreterPtr, UInt16);
Boolean			isValidID					(VectorInterpreter_ID);
short			joinup						(short, short, short);
void			linefeed					(My_VectorInterpreterPtr);
void			newcoord					(My_VectorInterpreterPtr);
void			storexy						(My_VectorInterpreterPtr, short, short);
void			VGclrstor					(My_VectorInterpreterPtr);
void			VGdraw						(My_VectorInterpreterPtr, char);

} // anonymous namespace

#pragma mark Variables
namespace {

char const*		gTEK4014Font[95] =
{
	"", /* space */
	/* ! thru / */
	/* ! " */
	"4d1t1h1b1f3w5t",
	"4w4t4d4b",
	/* # $ % */
	"2w8h4w8f2e8b4d8t",
	"4d8t1x3d6f1v1b1n6h1n1b1v6f",
	"8y6a2f2b2h2t6c2f2b2h2t",
	/* & */
	"8d6r2f2t4h4v4b4h4y",
	/* ' */
	"2w3e3y",
	/* ( ) */
	"4d2r4t2y",
	"4d2y4t2r",
	/* * + , - */
	"4d8t1x3d6v6w6n",
	"4d2w4t2x3a6h",
	"2c3y",
	"3w8h",
	/* . */
	"2d2h2t2f2b",
	"8y",
	/* 0-9 */
	"2d4h2y4t2r4f2v4b2n2a8y",
	"2d4h2a8t2v",
	"8w6h2n2v4f2v2b8h",
	"6h2y2r4f4d2y2r6f",
	"6e2w8b2e8f6y",
	"6h2y2r6f4t8h",
	"2w2y4h2n2v4f2r4t2y4h",
	"8y8f",
	"2d4h2y2r4f2v2n4w2r2y4h2n2v",
	"2d4h2y4t2r4f2v2n4h2y",
	/* : thru @ */
	/* : */
	"2d2h2t2f2b4w2h2t2f2b",
	/* ; */
	"2c4y2w2t2f2b2h",
	"4d4r4y",
	"1w8h3w8f",
	"4y4r",
	"4d1h1t1f1b2w2t3h1y2t1r6f1v",
	"7w1y6h1n6b1v2f4t1x1r3f1v2b1n3h1y",
	/* A-Z */
	"4t4y4n4b2w8f",
	"6h2y2r6f6d2y2r6f8b",
	"6d2e2v4f2r4t2y4h2n",
	"6h2y4t2r6f8b",
	"8d8f8t8h4x8f",
	"8t8h4x8f",
	"7e1d1r5f2v4b2n4h2y2t3f",
	"8t4x8h4w8b",
	"8h4a8t4a8h",
	"2w2n4h2y6t",
	/* K */
	"8t4x4h4y4z4n",
	"8d8f8t",
	"8t4n4y8b",
	"8t8n8t",
	"2d4h2y4t2r4f2v4b2n",
	"8t7h1n2b1v7f",
	"2d2r4t2y4h2n4b2v4f2e4n",
	/* R */
	"8t7h1n2b1v7f4d4n",
	"6h2y2r4f2r2y6h",
	"4d8t4a8h",
	"8w6b2n4h2y6t",
	"8w4b4n4y4t",
	"8w8b4y4n8t",
	"8y8a8n",
	"4d4t4r4c4y",
	"8w8h8v8h",
	/* [ thru ` */
	"6d4f8t4h",
	"8w8n",
	"2d4h8t4f",
	"4w4y4n",
	"8h",
	"8w2d3n",
	/* a */
	"1w2t1y6h1n1t1x2b1n1q1v6f1r",
	"4w7h1n2b1v7f8t",
	"8d7f1r2t1y7h",
	"8e8b7f1r2t1y7h",
	"2w8h1t1r6f1v2b1n6h",
	/* f */
	"4w7h1d3w1r4f1v7b",
	"1d6h1y2t1r6f1v2b1n7h4w5b1v6f1r",
	"8t5x1y6h1n3b",
	"4d4t3w1t1h1b1f",
	"2n4h2y4t3w1t1f1b1h",
	/* k */
	"2d8w8b2w1h2y2z2n",
	/* l */
	"5d1f1r7t",
	"4t1x1y2h1n3b3w1y2h1n3b",
	"4w1h4b3w1y5h1n3b",
	"1d6h1y2t1r6f1v2b1n",
	/* p */
	"2x6t1x1y6h1n2b1v6f1r",
	"1d6h1y2t1r6f1v2b1n7d4w6b",
	"4w1h4b3w1y5h1n",
	"1d6h1y1r6f1r1y6h",
	"4w7h4w5a7b1n4h1y",
	"4w3b1n5h1y1c1f4t",
	/* v */
	"4w4n4y",
	"4w2b2n2y2n2y2t",
	"2d4y4a4n",
	"2x2d6y4z4r",
	"2w2e4h4v4h",
	/* end of lower-case letters */
	/* { thru ~ */
	"8d3f1r2t1r2f2d1y2t1y3h",
	"4d8t",
	"3h1y2t1y2h2a1r2t1r3f",
	"6w2y2n2y"
};

char const*		gTEK4105Font[106] =
{
	"", /* space */
	/* ! thru / */
	/* ! " */
	"2d0t2w4t",
	"1d6w2b2h2t",
	/* # $ % */
	"1d6t2d6b1h2w4f2w4h",
	"1w3h1y1r2f1r1y3h1w2a6b",
	"1w4y3a1f1t1h6x2d1h1t1f",
	/* & */
	"4d4r1t1y1n1b2v1b2n1h2y",
	/* ' */
	"1d4w2y",
	/* ( ) */
	"3d1r4t1y",
	"1d1y4t1r",
	/* * + , - */
	"1w4y4a4n1x2a6t",
	"3w4h2r4b",
	"1d2x2y1t",
	"3w4h",
	/* . */
	"2d1h1t1f",
	"1w4y",
	/* 0-9 */
	"1w4t1y2h1n4b1v2f1r4y",
	"1d2h1a6t1x1f",
	"6w1y2h1n1b1v2f1v2b4h",
	"1w1n2h1y1t1r1f1d1y1t1r2f1v",
	"3d6t3v1x4h",
	"1w1n2h1y2t1r3f2t4h",
	"3d6w1f2v3b1n2h1y1t1r3f",
	"1d2t3y1t4f",
	"1d2h1y1t1r1f1d1y1t1r2f1v1b1n1h1a1v1b1n",
	"1d1h2y3t1r2f1v1b1n3h",
	/* : thru @ */
	/* : */
	"1d1h1t1f2w1h1t1r",
	/* ; */
	"1x1c2y1t2w2t",
	"3d3r3y",
	"2w4h2w4f",
	"3y3r",
	"2d0t2w2y1t1r2f1v",
	"4d3f1r5t1y2h1n4b2f1t2y",
	/* A-Z */
	"4t2y2n4b2w4f",
	"6t3h1n1b1v3f3d1n1b1v3f",
	"4d1w1v2f1r4t1y2h1n",
	"6t2h2n2b2v2f",
	"4d4f6t4h1z2x3f",
	"6t4h3x1a3f",
	"3d3w1h3b3f1r4t1y2h1n",
	"6t4d6b3w4f",
	"1d2h1a6t1d2f",
	"1w1n2h1y5t",
	/* K */
	"6t4d3v3n",
	"6w6b4h",
	"6t2n1b1w2y6b",
	"6t4n2x6t",
	"1w1n2h1y4t1r2f1v4b",
	"6t3h1n1b1v3f",
	"1w1n2h1y4t1r2f1v4b2d1b1d1b1h",
	/* R */
	"6t3h1n1b1v3f1d3n",
	"1w1n2h1y1t1r2f1r1t1y2h1n",
	"2d6t2a4h",
	"6w5b1n2h1y5t",
	"6w4b2n2y4t",
	"6w6b2y1t1x2n6t",
	"1t4y1t4a1b4n1b",
	"2d3t2y1t4a1b2n",
	"6w4h1b4v1b4h",
	/* [ thru ` */
	"2d2f6t2h",
	"5w4n",
	"2d2h6t2f",
	"4w2y2n",
	"3x4h",
	"5w4n",
	/* a */
	"1d4w2h1n3b3f1r1y3h",
	"6t4x2y1h1n2b1v1f2r",
	"4d3f1r2t1y3h",
	"4d6t4b2r1f1v2b1n1h2y",
	"3d2f1r2t1y2h1n1b4f",
	/* f */
	"1d5t1y1h1n2x2a2f",
	"1x1n2h1y5t1z1r1f1v1b1n1h1y1t",
	"6t4x2y1h1n3b",
	"1d2h1a4t1f1e1w0h",
	"1x1n1h1y4t2w0h",
	/* k */
	"6t4x2h2y4x2r",
	/* l */
	"1d2h1a6t1f",
	"4t1h1n2b2w1y1n3b",
	"4t2x2y1h1n3b",
	"1w1n2h1y2t1r2f1v2b",
	/* p */
	"2x6t2x2y1h1n2b1v1f2r",
	"2x4d6t2x2r1f1v2b1n1h2y",
	"4t2x2y1h1n",
	"3h1y1r2f1r1y2h",
	"1d4w2h1f2w5b1n1y",
	"4w3b1n1h2y2x4t",
	/* v */
	"4w2b2n2y2t",
	"4w3b1n1y2t2x1n1y3t",
	"4y4a4n",
	"4w2b1n1h2y1w5b1v2f1r",
	"4w4h4v4h",
	/* end of lower-case letters */
	/* { thru ~ */
	"3d1r1t1r1y1t1y",
	"2d6t",
	"1d1y1t1y1r1t1r",
	"5w1y2n1y",
	/* marker definitions */
	"0t",
	"1x2t1c2f",
	"2x4t2c4f",
	"3x6t2d1x4v4d4r",
	"3x1a2h1e4t1q2f1z4b",
	"3x2a1t4y1t4a1b4n1b",
	"2z4t4h4b4f",
	"2a2y2n2v2r",
	"0t2z4t4h4b4f",
	"0t2x2r2y2n2v",
	"2z4t4h4b4f4y4x4r"
};

My_InterpreterPtrByID	VGwin;

VectorInterpreter_ID	gIDCounter = 0;

} // anonymous namespace



#pragma mark Public Methods

/*!
Constructs a new interpreter object that will ultimately
render in the specified way, respecting the given command set.
Returns "kVectorInterpreter_InvalidID" on failure.

Release this object with VectorInterpreter_Dispose().

(3.1)
*/
VectorInterpreter_ID
VectorInterpreter_New	(VectorInterpreter_Target	inTarget,
						 VectorInterpreter_Mode		inCommandSet)
{
	VectorInterpreter_ID		result = gIDCounter++;
	
	
	try
	{
		My_VectorInterpreterPtr		ptr = new My_VectorInterpreter(result, inTarget, inCommandSet);
		
		
		fontnum(ptr, 0);
		storexy(ptr, 0, 3071);
		
		(VectorCanvas_Result)VectorCanvas_SetPenColor(ptr->canvas, 1);
	#if 1
		VectorInterpreter_Zoom(result, 0, 0, 4095, 3119); // important!
	#else
		VectorInterpreter_Zoom(result, 0, 0, kVectorInterpreter_MaxX - 1, kVectorInterpreter_MaxY - 1);
	#endif
	}
	catch (std::bad_alloc)
	{
		result = kVectorInterpreter_InvalidID;
	}
	return result;
}// VectorInterpreter_New


/*!
Destroys a graphic created with VectorInterpreter_New(),
and sets your copy of the ID to kVectorInterpreter_InvalidID.

(2.6)
*/
void
VectorInterpreter_Dispose	(VectorInterpreter_ID*		inoutGraphicIDPtr)
{
	if (isValidID(*inoutGraphicIDPtr))
	{
		My_VectorInterpreterPtr		ptr = VGwin[*inoutGraphicIDPtr];
		
		
		VectorCanvas_Release(&ptr->canvas);
		delete ptr, ptr = nullptr;
		VGwin.erase(*inoutGraphicIDPtr);
	}
	*inoutGraphicIDPtr = kVectorInterpreter_InvalidID;
}// Dispose


/*!
Sets zoom/pan borders for the destination to match the source.
The destination should be redrawn.

(3.1)
*/
void
VectorInterpreter_CopyZoom	(VectorInterpreter_ID	inDestinationGraphicID,
							 VectorInterpreter_ID	inSourceGraphicID)
{
	VectorInterpreter_Zoom(inDestinationGraphicID, VGwin[inSourceGraphicID]->winleft, VGwin[inSourceGraphicID]->winbot,
							VGwin[inSourceGraphicID]->winright, VGwin[inSourceGraphicID]->wintop);
}// CopyZoom


/*!
Converts graphics-screen-local coordinates for the mouse
into a cursor position report ("GIN") that can be sent to
a session in TEK mode.  Returns 0 only if successful.

You must allocate space for at least 5 characters into
which the final report is written.

The position (0, 0) represents the bottom-left corner, and
(4095, 4095) represents the upper-right corner.

(2.6)
*/
SInt16
VectorInterpreter_FillInPositionReport	(VectorInterpreter_ID	inGraphicID,
										 UInt16					inX,
										 UInt16					inY,
										 char					inKeyPress,
										 char*					outPositionReportLength5)
{
	SInt16		result = -1;
	
	
	if (isValidID(inGraphicID))
	{
		My_VectorInterpreterPtr		ptr = VGwin[inGraphicID];
		UInt32						x2 = 0;
		UInt32						y2 = 0;
		
		
		x2 = ((inX * ptr->winwide) / kVectorInterpreter_MaxX + ptr->winleft) >> 2;
		y2 = ((inY * ptr->wintall) / kVectorInterpreter_MaxY + ptr->winbot) >> 2;
		
		outPositionReportLength5[0] = inKeyPress;
		outPositionReportLength5[1] = 0x20 | ((x2 & 0x03e0) >> 5);
		outPositionReportLength5[2] = 0x20 | (x2 & 0x001f);
		outPositionReportLength5[3] = 0x20 | ((y2 & 0x03e0) >> 5);
		outPositionReportLength5[4] = 0x20 | (y2 & 0x001f);
		
		result = 0;
	}
	return result;
}// FillInPositionReport


/*!
Standard TEK PAGE command; clears the screen, homes the cursor
and switches to alpha mode.

(2.6)
*/
void
VectorInterpreter_PageCommand	(VectorInterpreter_ID	inGraphicID)
{
	My_VectorInterpreterPtr		ptr = VGwin[inGraphicID];
	
	
	if (nullptr != ptr->canvas)
	{
		VectorCanvas_InvalidateView(ptr->canvas);
		(VectorCanvas_Result)VectorCanvas_SetPenColor(ptr->canvas, 1);
	}
	VGclrstor(ptr);
	ptr->mode = ALPHA;
	ptr->state = DONE;
	ptr->textcol = 0;
	fontnum(ptr, 0);
	storexy(ptr, 0, 3071);
}// PageCommand


/*!
Redraws the whole graphic.  Clear the screen before invoking
a redraw.

(2.6)
*/
void
VectorInterpreter_Redraw	(VectorInterpreter_ID	inGraphicID,
							 VectorInterpreter_ID	inDestinationGraphicID)
{
	My_VectorInterpreterPtr		ptr = VGwin[inGraphicID];
	My_VectorInterpreterPtr		destPtr = VGwin[inDestinationGraphicID];
	
	
	for (ptr->toCurrentCommand = ptr->commandList.begin();
			ptr->commandList.end() != ptr->toCurrentCommand; ++(ptr->toCurrentCommand))
	{
		VGdraw(destPtr, *(ptr->toCurrentCommand));
	}
}// Redraw


/*!
Returns the current background color index, as defined by
TEK (from 0 to 7), suitable for clearing the screen in a
renderer.

(3.1)
*/
SInt16
VectorInterpreter_ReturnBackgroundColor		(VectorInterpreter_ID	inGraphicID)
{
	My_VectorInterpreterConstPtr	ptr = VGwin[inGraphicID];
	SInt16							result = (kVectorInterpreter_ModeTEK4105 == ptr->commandSet)
												? ptr->TEKBackground
												: 0;
	
	
	return result;
}// ReturnBackgroundColor


/*!
Returns the renderer for this interpreter.

(3.1)
*/
VectorCanvas_Ref
VectorInterpreter_ReturnCanvas		(VectorInterpreter_ID	inGraphicID)
{
	My_VectorInterpreterConstPtr	ptr = VGwin[inGraphicID];
	VectorCanvas_Ref				result = ptr->canvas;
	
	
	return result;
}// ReturnCanvas


/*!
Returns the command set of the specified graphic.

(3.1)
*/
VectorInterpreter_Mode
VectorInterpreter_ReturnMode	(VectorInterpreter_ID	inGraphicID)
{
	My_VectorInterpreterConstPtr	ptr = VGwin[inGraphicID];
	VectorInterpreter_Mode			result = ptr->commandSet;
	
	
	return result;
}// ReturnMode


/*!
This is the main entry point for rendering any vector graphics!

Stores and renders the specified data, returning the number of
bytes accepted before possible cancellation.  The data should
use the command set specified by VectorInterpreter_ReturnMode().

(3.1)
*/
size_t
VectorInterpreter_ProcessData	(VectorInterpreter_ID	inGraphicID,
								 UInt8 const*			inDataPtr,
								 size_t					inDataSize)
{
	size_t		result = 0;
	
	
	if (isValidID(inGraphicID))
	{
		UInt8 const* const			kPastEnd = inDataPtr + inDataSize;
		My_VectorInterpreterPtr		ptr = VGwin[inGraphicID];
		UInt8 const*				charPtr = nullptr;
		
		
		for (charPtr = inDataPtr;
				((kPastEnd != charPtr) && (24/* CAN(CEL) character */ != *charPtr)); ++charPtr)
		{
			ptr->commandList.push_back(*charPtr);
		#if 1
			VGdraw(ptr, *charPtr);
		#endif
		}
	#if 0
		VectorCanvas_InvalidateView(ptr->canvas);
	#endif
		result = charPtr - inDataPtr;
	}
	return result;
}// ProcessData


/*!
Specifies whether a PAGE command clears the screen, or
opens a new window.

(3.1)
*/
void
VectorInterpreter_SetPageClears		(VectorInterpreter_ID	inGraphicID,
									 Boolean				inTrueClearsFalseNewWindow)
{
	My_VectorInterpreterPtr		ptr = VGwin[inGraphicID];
	
	
	ptr->pageClears = inTrueClearsFalseNewWindow;
}// SetPageClears


/*	Set new borders for zoom/pan region.
 *	x0,y0 is lower left; x1,y1 is upper right.
 *	User should redraw after calling this.
 */
void
VectorInterpreter_Zoom	(VectorInterpreter_ID	inGraphicID,
						 SInt16					inX0,
						 SInt16					inY0,
						 SInt16					inX1,
						 SInt16					inY1)
{
	if (isValidID(inGraphicID))
	{
		My_VectorInterpreterPtr		ptr = VGwin[inGraphicID];
		
		
		ptr->winbot = inY0;
		ptr->winleft = inX0;
		ptr->wintop = inY1;
		ptr->winright = inX1;
		ptr->wintall = inY1 - inY0 + 1;
		ptr->winwide = inX1 - inX0 + 1;
	}
}// Zoom


#pragma mark Internal Methods
namespace {

/*!
Constructor.

TEMPORARY:	This is horrible as a class, but it is leftover from
			the ancient TEK implementation.  For now, this just
			initializes all the old fields.  Eventually, all of
			this will be redesigned.

(4.0)
*/
My_VectorInterpreter::
My_VectorInterpreter	(VectorInterpreter_ID		inID,
						 VectorInterpreter_Target	inTarget,
						 VectorInterpreter_Mode		inCommandSet)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef(inID),
commandSet(inCommandSet),
pageClears(false),
canvas(nullptr),
mode(ALPHA),
modesave(0),
loy(0),
hiy(0),
lox(0),
hix(0),
ex(0),
ey(0),
nloy(0),
nhiy(0),
nlox(0),
nhix(0),
nex(0),
ney(0),
curx(0),
cury(0),
savx(0),
savy(0),
winbot(0),
wintop(0),
winleft(0),
winright(0),
wintall(0),
winwide(0),
textcol(0),
intin(0),
pencolor(0),
fontnum(0),
charx(0),
chary(0),
count(0),
TEKMarker(0),
TEKOutline(0),
TEKPath(0),
TEKPattern(0),
TEKIndex(0),
TEKRot(0),
TEKSize(0),
TEKBackground(0),
TEKPanel(nullptr),
current(nullptr),
state(DONE),
savstate(0),
commandList(),
toCurrentCommand(commandList.begin())
{
	VGwin[inID] = this;
	// the canvas should be initialized last, because it will trigger
	// rendering that depends on all the initializations above
	this->canvas = VectorCanvas_New(inID, (kVectorInterpreter_TargetQuickDrawPicture == inTarget)
											? kVectorCanvas_TargetQuickDrawPicture
											: kVectorCanvas_TargetScreenPixels);
}// My_VectorInterpreter default constructor


/*!
This is the recommended way to shrink the command vector,
because it keeps the current command iterator in sync!!!

(3.1)
*/
void
My_VectorInterpreter::
shrinkVectorDB	(My_VectorDB::size_type		inByHowMany)
{
	if (this->commandList.size() < inByHowMany)
	{
		// removing all remaining items
		this->toCurrentCommand = this->commandList.begin();
		this->commandList.clear();
	}
	else
	{
		// remove specified number of items; back up the
		// iterator if it is at the end
		for (My_VectorDB::size_type i = 0; i < inByHowMany; ++i)
		{
			if (this->commandList.end() == this->toCurrentCommand)
			{
				--(this->toCurrentCommand);
			}
			this->commandList.resize(this->commandList.size() - 1);
		}
	}
}// shrinkVectorDB


/*
 *	Draw a vector in vw's window from x0,y0 to x1,y1.
 *	Zoom the vector to the current visible window,
 *	and clip it before drawing it.
 *	Uses Liang-Barsky algorithm from ACM Transactions on Graphics,
 *		Vol. 3, No. 1, January 1984, p. 7.
 *
 *  Note: since QuickDraw on the Mac already handles clipping, we
 *		  will not do any processing here.
 *  14may91dsw
 *
 */
void
clipvec		(My_VectorInterpreterPtr		vp,
			 short		xa,
			 short		ya,
			 short		xb,
			 short		yb,
			 VectorCanvas_PathTarget	inTarget)
{
	short				t = 0,
						b = 0,
						l = 0,
						r = 0;
	long				hscale = 0L,
						vscale = 0L;
	
	
	hscale = kVectorInterpreter_MaxX / (long) vp->winwide;
	vscale = kVectorInterpreter_MaxY / (long) vp->wintall;
	
	t = vp->wintop;
	b = vp->winbot;
	l = vp->winleft;
	r = vp->winright;

	(VectorCanvas_Result)VectorCanvas_DrawLine(vp->canvas,
		(short) ((long)(xa - l) * kVectorInterpreter_MaxX / (long) vp->winwide),
		(short) ((long)(ya- b) * kVectorInterpreter_MaxY / (long) vp->wintall),
		(short) ((long)(xb - l) * kVectorInterpreter_MaxX / (long) vp->winwide),
		(short) ((long)(yb- b) * kVectorInterpreter_MaxY / (long) vp->wintall),
		inTarget);
}// clipvec


/*
 *	Draw a stroked character at the current cursor location.
 *	Uses simple 8-directional moving, 8-directional drawing.
 *
 *	Modified 17jul90dsw: TEK4105 character set added.
 */
short
drawc		(My_VectorInterpreterPtr	inPtr,
			 short		c) /* character to draw */
{
	short		x = 0,
				y = 0,
				savex = 0,
				savey = 0;
	short		strokex = 0,
				strokey = 0;
	short		n = 0;						/* number of times to perform command */
	char const*	pstroke = nullptr;				/* pointer into stroke data */
	short		hmag = 0,
				vmag = 0;
	short		xdir = 1,
				ydir = 0;
	short		height = 0;
	
	
	if (c == 10)
	{
		linefeed(inPtr);
		return(0);
	}

	if (c == 7)
	{
		VectorCanvas_AudioEvent(inPtr->canvas);
		inPtr->shrinkVectorDB(1);
		return(0);
	}

	savey = y = inPtr->cury;
	savex = x = inPtr->curx;

	if (c == 8)
	{
		if (savex <= inPtr->textcol) return(0);
		savex -= inPtr->charx;
		if (savex < inPtr->textcol) savex = inPtr->textcol;
		inPtr->cury = savey;
		inPtr->curx = savex;
		return(0);
	}

	if (kVectorInterpreter_ModeTEK4105 == inPtr->commandSet)
	{
		height = inPtr->TEKSize;
		if (c > 126)
		{
			height = 1;
			(VectorCanvas_Result)VectorCanvas_SetPenColor(inPtr->canvas,inPtr->pencolor);
		}
		else
			(VectorCanvas_Result)VectorCanvas_SetPenColor(inPtr->canvas,inPtr->TEKIndex);
		hmag = (height*8);
		vmag = (height*8);
		
		xdir = 0;
		switch(inPtr->TEKRot)
		{
			case 0:
				xdir = 1;
				break;
			case 90:
				ydir = 1;
				break;
			case 180:
				xdir = -1;
				break;
			case 270:
				ydir = -1;
				break;
		}
	}
	else
	{
		hmag = inPtr->charx / 10;
		vmag = inPtr->chary / 10;
	}

	if ((c < 32) || (c > 137))
		return(0);					// Is this return value correct?
	c -= 32;
	pstroke = (kVectorInterpreter_ModeTEK4105 == inPtr->commandSet) ? gTEK4105Font[c] : gTEK4014Font[c];
	while (*pstroke)
	{
		strokex = x;
		strokey = y;
		n = (*(pstroke++) - 48);	/* run length */
		c = *(pstroke++);			/* direction code */

		switch(c) 	/* horizontal movement: positive = right */
		{
		case 'e': case 'd': case 'c': case 'y': case 'h': case 'n':
			x += (n * hmag) * xdir;
			y += (n * hmag) * ydir;
			break;
		case 'q': case 'a': case 'z': case 'r': case 'f': case 'v':
			x -= (n * hmag) * xdir;
			y -= (n * hmag) * ydir;
		}

		switch(c)	/* vertical movement: positive = up */
		{
		case 'q': case 'w': case 'e': case 'r': case 't': case 'y':
			x -= (n * vmag) * ydir;
			y += (n * vmag) * xdir;
			break;
		case 'z': case 'x': case 'c': case 'v': case 'b': case 'n':
			x += (n * vmag) * ydir;
			y -= (n * vmag) * xdir;
		}

		switch(c)	/* draw or move */
		{
		case 'r': case 't': case 'y': case 'f': case 'h':
		case 'v': case 'b': case 'n':
			clipvec (inPtr,strokex,strokey,x,y);
			break;
		}
	
	} /* end while not at end of string */

	/* Update cursor location to next char position */
	savex += inPtr->charx * xdir;
	savey += inPtr->charx * ydir;
	if ((savex < 0) || (savex > 4095) || (savey < 0) || (savey > 3119))
	{
		savex = savex < 0 ? 0 : savex > 4095 ? 4095 : savex;
		savey = savey < 0 ? 0 : savey > 3119 ? 3119 : savey;
	}

	if (kVectorInterpreter_ModeTEK4105 == inPtr->commandSet)
		(VectorCanvas_Result)VectorCanvas_SetPenColor(inPtr->canvas,inPtr->pencolor);

	inPtr->cury = savey;
	inPtr->curx = savex;
	
	return(0);
}// drawc


/*
 *	Set font for window 'vw' to size 'n'.
 *	Sizes are 0..3 in Tek 4014 standard.
 *	Sizes 4 & 5 are used internally for Tek 4105 emulation.
 */
short
fontnum		(My_VectorInterpreterPtr	inPtr,
			 UInt16		n)
{
	size_t const	kCharSizeCount = 6;
	short			result = 0;
	
	
	if (n >= kCharSizeCount) result = -1;
	else
	{
		static SInt16	characterSetX[kCharSizeCount] = { 56, 51, 34, 31, 112, 168 };
		static SInt16	characterSetY[kCharSizeCount] = { 88, 82, 53, 48, 176, 264 };
		
		
		inPtr->fontnum = n;
		inPtr->charx = characterSetX[n];
		inPtr->chary = characterSetY[n];
	}
	return result;
}// fontnum


/*!
Returns true only if the specified ID corresponds to
an existing interpreter.

(3.1)
*/
Boolean
isValidID	(VectorInterpreter_ID	inID)
{
	return (VGwin.end() != VGwin.find(inID));
}// isValidID


short
joinup		(short		hi,
			 short		lo,
			 short		e)
/* returns the number represented by the 3 pieces */
{
#if 1
	return (((hi & 31) << 7) | ((lo & 31) << 2) | (e & 3));
#else
	return (((hi /* & 31 */ ) << 7) | ((lo /* & 31 */ ) << 2) | (e /* & 3 */));
#endif
}// joinup


void
linefeed	(My_VectorInterpreterPtr	inPtr) 
/* 
 *	Perform a linefeed & cr (CHARTALL units) in specified window.
 */
{
/*	short y = joinup(inPtr->hiy,inPtr->loy,inPtr->ey);*/
	short y = inPtr->cury;
	short x;

	if (y > inPtr->chary) y -= inPtr->chary;
	else
	{
		y = 3119 - inPtr->chary;
		inPtr->textcol = 2048 - inPtr->textcol;
	}
	x = inPtr->textcol;
	storexy(inPtr,x,y);
}// linefeed


void
newcoord	(My_VectorInterpreterPtr	inPtr)
/*
 *	Replace x,y with nx,ny
 */
{
	inPtr->hiy = inPtr->nhiy;
	inPtr->hix = inPtr->nhix;
	inPtr->loy = inPtr->nloy;
	inPtr->lox = inPtr->nlox;
	inPtr->ey  = inPtr->ney;
	inPtr->ex  = inPtr->nex;
	
	inPtr->curx = joinup(inPtr->nhix,inPtr->nlox,inPtr->nex);
	inPtr->cury = joinup(inPtr->nhiy,inPtr->nloy,inPtr->ney);
}


void
storexy		(My_VectorInterpreterPtr	inPtr,
			 short		x,
			 short		y)
/* set graphics x and y position */
{
	inPtr->curx = x;
	inPtr->cury = y;
}// storexy


/*	Clear the store associated with window vw.  
 *	All contents are lost.
 *	User program can call this whenever desired.
 *	Automatically called after receipt of Tek page command. */
void	VGclrstor(My_VectorInterpreterPtr	inPtr)
{
	inPtr->commandList.clear();
	inPtr->toCurrentCommand = inPtr->commandList.begin();
}


/*	This is the main Tek emulator process.  Pass it the window and
 *	the latest input character, and it will take care of the rest.
 *	Calls RG functions as well as local zoom and character drawing
 *	functions.
 *
 *	Modified 16jul90dsw:
 *		Added 4105 support.
 */
void VGdraw(My_VectorInterpreterPtr vp, char c)			/* the latest input char */
{
	char		cmd;
	char		value;
	char		goagain;	/* true means go thru the function a second time */
	char		temp[80];
	My_PointList	temppoint;

	temp[0] = c;
	temp[1] = (char) 0;

	/*** MAIN LOOP ***/
 	do
	{
		c &= 0x7f;
 		cmd = (c >> 5) & 0x03;
		value = c & 0x1f;
		goagain = false;

		switch(vp->state)
		{
		case HIY: /* beginning of a vector */
			vp->nhiy = vp->hiy;
			vp->nhix = vp->hix;
			vp->nloy = vp->loy;
			vp->nlox = vp->lox;
			vp->ney  = vp->ey;
			vp->nex  = vp->ex;
			
			switch(cmd)
			{
			case 0:
				if (value == 27)		/* escape sequence */
				{
					vp->state = ESCOUT;
					vp->savstate = HIY;
				}
				else if (value < 27)	/* ignore */
				{
					break;
				}
				else
				{
					vp->state = CANCEL;
					goagain = true;
				}
				break;
			case 1:						/* hiy */
				vp->nhiy = value;
				vp->state = EXTRA;
				break;
			case 2:						/* lox */
				vp->nlox = value;
				vp->state = DONE;
				break;
			case 3:						/* extra or loy */
				vp->nloy = value;
				vp->state = LOY;
				break;
			}
			break;		
		case ESCOUT:
			if ((value != 13) && (value != 10) && (value != 27) && (value != '~'))
			{
				vp->state = vp->savstate;		/* skip all EOL-type characters */
				goagain = true;
			}
			break;
		case EXTRA:	/* got hiy; expecting extra or loy */
			switch(cmd)
			{
			case 0:
				if (value == 27)		/* escape sequence */
				{
					vp->state = ESCOUT;
					vp->savstate = EXTRA;
				}
				else if (value < 27)	/* ignore */
				{
					break;
				}
				else
				{
					vp->state = DONE;
					goagain = true;
				}
				break;
			case 1:						/* hix */
				vp->nhix = value;
				vp->state = LOX;
				break;
			case 2:						/* lox */
				vp->nlox = value;
				vp->state = DONE;
				break;
			case 3:						/* extra or loy */
				vp->nloy = value;
				vp->state = LOY;
				break;
			}
			break;
		case LOY: /* got extra or loy; next may be loy or something else */
			switch(cmd)
			{
			case 0:
				if (value == 27)		/* escape sequence */
				{
					vp->state = ESCOUT;
					vp->savstate = LOY;
				}
				else if (value < 27)	/* ignore */
				{
					break;
				}
				else
				{
					vp->state = DONE;
					goagain = true;
				}
				break;
			case 1: /* hix */
				vp->nhix = value;
				vp->state = LOX;
				break;
			case 2: /* lox */
				vp->nlox = value;
				vp->state = DONE;
				break;
			case 3: /* this is loy; previous loy was really extra */
				vp->ney = (vp->nloy >> 2) & 3;
				vp->nex = vp->nloy & 3;
				vp->nloy = value;
				vp->state = HIX;
				break;
			}
			break;
		case HIX:						/* hix or lox */
			switch(cmd)
			{
			case 0:
				if (value == 27)		/* escape sequence */
				{
					vp->state = ESCOUT;
					vp->savstate = HIX;
				}
				else if (value < 27)	/* ignore */
				{
					break;
				}
				else
				{
					vp->state = DONE;
					goagain = true;
				}
				break;
			case 1:						/* hix */
				vp->nhix = value;
				vp->state = LOX;
				break;
			case 2:						/* lox */
				vp->nlox = value;
				vp->state = DONE;
				break;
			}
		 	break;
	
		case LOX:						/* must be lox */
			switch(cmd)
			{
			case 0:
				if (value == 27)		/* escape sequence */
				{
					vp->state = ESCOUT;
					vp->savstate = LOX;
				}
				else if (value < 27)	/* ignore */
				{
					break;
				}
				else
				{
					vp->state = DONE;
					goagain = true;
				}
				break;
			case 2:
				vp->nlox = value;
				vp->state = DONE;
				break;
			}
			break;
	
		case ENTERVEC:
			if (c == 7) vp->mode = DRAW;
			if (c < 32)
			{
				vp->state = DONE;
				goagain = true;
				vp->mode = DONE;
				break;
			}
			vp->state = HIY;
			vp->mode = TEMPMOVE;
			vp->modesave = DRAW;
			goagain = true;
			break;
		case RS:
			switch (c)
			{
			case ' ':				/* pen up */
				vp->modesave = vp->mode;
				vp->mode = TEMPMOVE;
				break;
			case 'P':				/* pen down */
				vp->mode = DRAW;
				break;
			case 'D':				/* move up */
				vp->cury++;
				break;
			case 'E':
				vp->cury++;
				vp->curx++;
				break;
			case 'A':
				vp->curx++;
				break;
			case 'I':
				vp->curx++;
				vp->cury--;
				break;
			case 'H':
				vp->cury--;
				break;
			case 'J':
				vp->curx--;
				vp->cury--;
				break;
			case 'B':
				vp->curx--;
				break;
			case 'F':
				vp->cury++;
				vp->curx--;
				break;
			case 27:
				vp->savstate = RS;
				vp->state = ESCOUT;
				break;
			default:
/*				storexy(vw,vp->curx,vp->cury);*/
				vp->state = CANCEL;
				goagain = true;
				break;
			}
			if (vp->mode == DRAW)
				clipvec(vp,vp->curx,vp->cury,vp->curx,vp->cury);
			break;
		case CMD0: /* *->CMD0: get 1st letter of cmd */
			switch(c)
			{
			case 29:					/* GS, start draw */
				vp->state = DONE;
				goagain = true;
				break;
			case '%':
				vp->state = TERMSTAT;
				break;
			case '8':
				fontnum(vp,0);
				vp->state = DONE;
				break;
			case '9':
				fontnum(vp,1);
				vp->state = DONE;
				break;
			case ':':
				fontnum(vp,2);
				vp->state = DONE;
				break;
			case ';':
				fontnum(vp,3);
				vp->state = DONE;
				break;
			case 12: /* form feed = clrscr */
				if (vp->pageClears)
				{
					VectorInterpreter_PageCommand(vp->selfRef);
				}
				break;
			case 'L':
				vp->state = SOMEL;
				break;
			case 'K':
				vp->state = IGNORE;
				break;
			case 'M':
				vp->state = SOMEM;
				break;
			case 'R':
				vp->state = SOMER;
				break;
			case 'T':
				vp->state = SOMET;
				break;
			case 26:
				VectorCanvas_MonitorMouse(vp->canvas);
				vp->shrinkVectorDB(2);
				break;
			case 10:
			case 13:
			case 27:
			case '~':
				vp->savstate = DONE;
				vp->state = ESCOUT;
				break;			/* completely ignore these after ESC */
			default:
				vp->state = DONE;
			}
			break;
		case TERMSTAT:
			switch(c)
			{
				case '!':
					vp->state = INTEGER;		/* Drop the next integer */
					vp->savstate = DONE;
					break;
			}
			break;
		case SOMER:
			switch(c)
			{
				case 'A':
					vp->state = INTEGER;
					vp->savstate = VIEWAT;
					break;
				default:
					vp->state = DONE;
			}
			break;
		case VIEWAT:
			vp->state = INTEGER;
			vp->savstate = VIEWAT2;
			goagain = true;
			break;
		case VIEWAT2:
			vp->TEKBackground = vp->intin < 0 ? 0 : vp->intin > 7 ? 7 : vp->intin;
			vp->state = INTEGER;
			vp->savstate = DONE;
			goagain = true;
			break;
		case SOMET:				/* Got ESC T; now handle 3rd char. */
			switch(c)
			{
			case 'C':			/* GCURSOR */
				vp->intin = 3;
				vp->state = STARTDISC;
				break;
			case 'D':
				vp->intin = 2;
				vp->state = STARTDISC;
				break;
			case 'F':			/* set dialog area color map */
				vp->state = JUNKARRAY;
				break;
			case 'G':			/* set surface color map */
				vp->state = INTEGER;
				vp->savstate = JUNKARRAY;
				break;
			default:
				vp->state = DONE;
			}			
			break;
		case JUNKARRAY:			/* This character is the beginning of an integer
									array to be discarded.  Get array size. */
			vp->savstate = STARTDISC;
			vp->state = INTEGER;
			break;					
		case STARTDISC:			/* Begin discarding integers. */
			vp->count = vp->intin + 1;
			goagain = true;
			vp->state = DISCARDING;
			break;
		case DISCARDING:
			/* We are in the process of discarding an integer array. */
			goagain = true;
			if (!(--(vp->count))) vp->state = DONE;
			else if (vp->count == 1)
			{
				vp->state = INTEGER;
				vp->savstate = DONE;
			}
			else
			{
				vp->state = INTEGER;
				vp->savstate = DISCARDING;
			}
			break;
		case INTEGER:
			if (c & 0x40)
			{
				vp->intin = c & 0x3f;
				vp->state = INTEGER1;
			}
			else
			{
				vp->intin = c & 0x0f;
				if (!(c & 0x10)) vp->intin *= -1;
				vp->state = vp->savstate;
			}
			break;
		case INTEGER1:
			if (c & 0x40)
			{
				vp->intin = (vp->intin << 6) | (c & 0x3f);
				vp->state = INTEGER2;
			}
			else
			{
				vp->intin = (vp->intin << 4) | (c & 0x0f);
				if (!(c & 0x10)) vp->intin *= -1;
				vp->state = vp->savstate;
			}
			break;
		case INTEGER2:
			vp->intin = (vp->intin << 4) | (c & 0x0f);
			if (!(c & 0x10)) vp->intin *= -1;
			vp->state = vp->savstate;
			break;
		case IGNORE:			/* ignore next char; it's not supported */
			vp->state = DONE;
			break;
		case IGNORE2:			/* ignore next 2 chars */
			vp->state = IGNORE;
			break;
		case SOMEL:				/* now process 2nd letter */
			switch(c)
			{
			case 'E':					/* END PANEL 25jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					if (vp->TEKPanel)
					{
						if ((vp->current->x != vp->savx) ||
							(vp->current->y != vp->savy))
						{
							temppoint = new My_Point(vp->savx, vp->savy);
							vp->current->next = temppoint;
						}
						temppoint = vp->current = vp->TEKPanel;
						vp->savx = vp->curx = vp->current->x;
						vp->savy = vp->cury = vp->current->y;
						vp->current = vp->current->next;
						delete temppoint;
						(VectorCanvas_Result)VectorCanvas_ScrapPathReset(vp->canvas);
						while (vp->current)
						{
							clipvec(vp,vp->curx,vp->cury,
									vp->current->x,vp->current->y, kVectorCanvas_PathTargetScrap);
							temppoint = vp->current;
							vp->curx = vp->current->x;
							vp->cury = vp->current->y;
							vp->current = vp->current->next;
							delete temppoint;
						}
						VectorCanvas_ScrapPathFill(vp->canvas, (vp->TEKPattern <= 0) ? -vp->TEKPattern : vp->pencolor,
													(vp->TEKOutline) ? 1.0 : 0.0);
						vp->TEKPanel = (My_PointList) nullptr;
						vp->curx = vp->savx;
						vp->cury = vp->savy;
					}
				}
				vp->state = DONE;
				break;
			case 'F':					/* MOVE */
				vp->modesave = vp->mode;
				vp->mode = TEMPMOVE;
				vp->state = HIY;
				break;
			case 'G':					/* DRAW */
				vp->modesave = vp->mode;
				vp->mode = TEMPDRAW;
				vp->state = HIY;
				break;
			case 'H':					/* MARKER */
				vp->modesave = vp->mode;
				vp->mode = TEMPMARK;
				vp->state = HIY;
				break;
			case 'I':					/* DAINDEX 24jul90dsw*/
				vp->state = STARTDISC;
				vp->intin = 3;
				break;
			case 'L':
				vp->state = INTEGER;
				vp->savstate = DONE;
				break;
			case 'P':					/* BEGIN PANEL 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)		/* 4105 only */
				{
					vp->state = HIY;
					vp->mode = PANEL;
				}
				else
					vp->state = DONE;
				break;
			case 'T':					/* GTEXT 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)		/* 4105 only */
				{
					vp->savstate = GTEXT;
					vp->state = INTEGER;
				}
				else
					vp->state = DONE;
				break;
			default:
				vp->state = DONE;
			}
			break;
		case SOMEM:
			switch(c)
			{
			case 'C':					/* set graphtext size */
				vp->savstate = GTSIZE0;
				vp->state = INTEGER;
				break;
			case 'L':					/* set line index */
				vp->savstate = COLORINT;
				vp->state = INTEGER;
				break;
			case 'M':					/* MARKERTYPE 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					vp->savstate = MARKER;
					vp->state = INTEGER;
				}
				else
					vp->state = DONE;
				break;
			case 'N':					/* GTPATH 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					vp->savstate = GTPATH;
					vp->state = INTEGER;
				}
				else
					vp->state = DONE;
				break;
			case 'P':					/* FillPattern 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					vp->savstate = FPATTERN;
					vp->state = INTEGER;
				}
				else
					vp->state = DONE;
				break;
			case 'R':					/* GTROT 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					vp->savstate = GTROT;
					vp->state = INTEGER;
				}
				else
					vp->state = DONE;
				break;
			case 'T':					/* GTINDEX 17jul90dsw */
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					vp->savstate = GTINDEX;
					vp->state = INTEGER;
				}
				else
					vp->state = DONE;
				break;
			case 'V':
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
				{
					vp->state = INTEGER;
					vp->savstate = DONE;
				}
				else
					vp->state = DONE;
				break;
			default:
				vp->state = DONE;
			}
			break;
		case COLORINT:				/* set line index; have integer */
			vp->pencolor = vp->intin;
			(VectorCanvas_Result)VectorCanvas_SetPenColor(vp->canvas,vp->intin);
			vp->state = CANCEL;
			goagain = true;			/* we ignored current char; now process it */
			break;
		case GTSIZE0:				/* discard the first integer; get the 2nd */
			vp->state = INTEGER;	/* get the important middle integer */
			vp->savstate = GTSIZE1;
			goagain = true;
			break;
		case GTSIZE1:				/* integer is the height */
			if (kVectorInterpreter_ModeTEK4105 == vp->commandSet)
			{
				if (vp->intin < 88) vp->TEKSize = 1;
				else if ((vp->intin > 87) && (vp->intin < 149)) vp->TEKSize = 2;
				else if ((vp->intin > 148) && (vp->intin < 209)) vp->TEKSize = 3;
				else if (vp->intin > 208) vp->TEKSize = vp->intin / 61;
				vp->charx = (vp->TEKSize * 52);
				vp->chary = (vp->TEKSize * 64);
			}
			else
			{
				if (vp->intin < 88)
					fontnum(vp,0);
				else if (vp->intin < 149)
					fontnum(vp,4);
				else
					fontnum(vp,5);
			}
			vp->state = INTEGER;	/* discard last integer */
			vp->savstate = DONE;
			goagain = true;
			break;
		case GTEXT:					/* TEK4105 GraphText output.  17jul90dsw */
			if (vp->intin > 0)
			{
				drawc(vp,(short) c);	/* Draw the character */
				vp->intin--;		/* One less character in the string... */
			}
			else
			{
				goagain = true;
				vp->state = DONE;
				newcoord(vp);
			}
			break;
		case MARKER:				/* TEK4105 Set marker type.  17jul90dsw */
			vp->TEKMarker = vp->intin;
			if (vp->TEKMarker > 10) vp->TEKMarker = 10;
			if (vp->TEKMarker <  0) vp->TEKMarker = 0;
			vp->state = DONE;
			goagain = true;
			break;
		case GTPATH:
			vp->TEKPath = vp->intin;
			vp->state = DONE;
			goagain = true;
			break;
		case FPATTERN:
			vp->TEKPattern = (vp->intin <  -7) ?  -7 :
							 (vp->intin > 149) ? 149 : vp->intin;
			vp->state = DONE;
			goagain = true;
			break;
		case GTROT:
			vp->TEKRot = vp->intin;
			vp->state = INTEGER;
			vp->savstate = GTROT1;
			goagain = true;
			break;
		case GTROT1:
			vp->TEKRot = (vp->TEKRot) << (vp->intin);
			vp->TEKRot = ((vp->TEKRot + 45) / 90) * 90;
			vp->state = DONE;
			goagain = true;
			break;
		case GTINDEX:
			vp->TEKIndex = (vp->intin < 0) ? 0 : (vp->intin > 7) ? 7 : vp->intin;
			vp->state = DONE;
			goagain = true;
			break;
		case PANEL:
			vp->TEKOutline = (vp->intin == 0) ? 0 : 1;
			temppoint = new My_Point;
			if (vp->TEKPanel)
			{
				if ((vp->current->x != vp->savx) && (vp->current->y != vp->savy))
				{
					temppoint->x = vp->savx;
					temppoint->y = vp->savy;
					vp->current->next = temppoint;
					vp->current = temppoint;
					temppoint = new My_Point;
				}
				vp->current->next = temppoint;
			}
			else vp->TEKPanel = temppoint;
			vp->current = temppoint;
			vp->current->x = vp->savx = joinup(vp->nhix,vp->nlox,vp->nex);
			vp->current->y = vp->savy = joinup(vp->nhiy,vp->nloy,vp->ney);
			vp->current->next = (My_PointList) nullptr;
			vp->state = INTEGER;
			vp->savstate = PANEL;
			vp->mode = DONE;
			newcoord(vp);
			vp->state = DONE;
			goagain = true;
			break;
		case DONE:					/* ready for anything */
			switch(c)
			{
			case 31:				/* US - enter ALPHA mode */
				vp->mode = ALPHA; 
				vp->state = CANCEL;
				break;
			case 30:
				vp->state = RS;
				break;
			case 28:
					vp->mode = MARK;
					vp->state = HIY;
				break;
			case 29:				/* GS - enter VECTOR mode */
				vp->state = ENTERVEC;
				break;
			case 27:
				vp->state = CMD0;
				break;
			default:
				if (vp->mode == ALPHA)
				{
					vp->state = DONE;
					if (kVectorInterpreter_ModeTEK4014 == vp->commandSet)
						drawc(vp,(short) c);
					else
					{
						//UInt8		uc = c;
						// TEMPORARY - FIX THIS
						//Session_TerminalWrite(vp->theVS, &uc, 1);
						vp->shrinkVectorDB(1);
					}
					return;
				}
				else if ((vp->mode == DRAW) && cmd)
				{
					vp->state = HIY;
					goagain = true;
				}
				else if ((vp->mode == MARK) && cmd)
				{
					vp->state = HIY;
					goagain = true;
				}
				else if ((vp->mode == DRAW) && ((c == 13) || (c == 10)))
				{
					/* break drawing mode on CRLF */
					vp->mode = ALPHA; 
					vp->state = CANCEL;
				}
				else
				{
					vp->state = DONE;			/* do nothing */
					return;
				}
			}
		}
	
		if (vp->state == DONE)
		{
			if (vp->mode == PANEL)
			{
				vp->mode = DONE;
				vp->state = INTEGER;
				vp->savstate = PANEL;
			}
			else if ((vp->TEKPanel) && ((vp->mode == DRAW) || (vp->mode == TEMPDRAW)
					|| (vp->mode == MARK) || (vp->mode == TEMPMARK) ||
						(vp->mode == TEMPMOVE)))
			{
				temppoint = new My_Point;
				vp->current->next = temppoint;
				vp->current = temppoint;
				vp->current->x = joinup(vp->nhix,vp->nlox,vp->nex);
				vp->current->y = joinup(vp->nhiy,vp->nloy,vp->ney);
				vp->current->next = (My_PointList) nullptr;
				if ((vp->mode == TEMPDRAW) || (vp->mode == TEMPMOVE) ||
					(vp->mode == TEMPMARK))
					vp->mode = vp->modesave;
				newcoord(vp);
			}
			else if (vp->mode == TEMPMOVE)
			{
				vp->mode = vp->modesave;
				newcoord(vp);
			}
			else if ((vp->mode == DRAW) || (vp->mode == TEMPDRAW))
			{
				clipvec(vp,vp->curx,vp->cury,
					joinup(vp->nhix,vp->nlox,vp->nex),
					joinup(vp->nhiy,vp->nloy,vp->ney));
				newcoord(vp);
				if (vp->mode == TEMPDRAW) vp->mode = vp->modesave;
			}
			else if ((vp->mode == MARK) || (vp->mode == TEMPMARK))
			{
				newcoord(vp);
				if (kVectorInterpreter_ModeTEK4105 == vp->commandSet) drawc(vp,127 + vp->TEKMarker);
				newcoord(vp);
				if (vp->mode == TEMPMARK) vp->mode = vp->modesave;
			}
		}

		if (vp->state == CANCEL) vp->state = DONE;
	} while (goagain);
	return;
}

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
