/*!	\file tekdefs.h
	\brief All TEK vector graphics related stuff.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

#ifndef __TEKDEFS__
#define __TEKDEFS__

// Mac includes
#include <Carbon/Carbon.h>

// MacTelnet includes
#include "SessionRef.typedef.h"
#include "TerminalScreenRef.typedef.h"



#define TEK_DEVICE_MAX 2
#define SPLASH_SQUARED	4	/* NCSA: sb - used by tekrgmac.c */

#define MAXWIND 20
#define WINXMAX 4095
#define WINYMAX 3139
#define INXMAX 4096
#define INYMAX 4096

#define PREDCOUNT 50

/* MORE vgtek specific stuff */
#define MAXVG		20 /* maximum number of VG windows */

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



#ifndef REZ

/*--------------------------------------------------------------------------*/
/* Next come the typedefs for the various tek structures					*/
/*--------------------------------------------------------------------------*/

typedef struct TPOINT *pointlist;

typedef	struct TPOINT {
	short		x,y;
	pointlist	next;
} point/*,*pointlist*/;

typedef struct {
	short
		(*newwin)();
	char const *
		(*devname)();
	void 
		(*init)();
	short
		(*gin)(short),
		(*pencolor)(short, short),
		(*clrscr)(short),
		(*close)(short),
		(*point)(short, short, short),
		(*drawline)(short, short, short, short, short);
	void
		(*info)(short, short, short, short, short, short),
		(*pagedone)(short),
		(*dataline)(short, short, short), 
		(*charmode)(short, short, short), 
		(*gmode)(),
		(*tmode)(),
		(*showcur)(),
		(*lockcur)(),
		(*hidecur)(),
		(*bell)(short),
		(*uncover)(short);
} RGLINK;

typedef struct {
	long	thiselnum;	/* number of currently-viewing element */
	Handle	dataHandle;	/* Handle to the data */
	}	TEKSTORE, *TEKSTOREP;


#endif /* REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
