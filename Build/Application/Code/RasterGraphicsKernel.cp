/*###############################################################

	RasterGraphicsKernel.cp
	
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

#define MASTERDEF

#include "UniversalDefines.h"

// standard-C includes
#include <cstring>

// library includes
#include <MemoryBlocks.h>

// MacTelnet includes
#include "RasterGraphicsKernel.h"
#include "RasterGraphicsScreen.h"



#pragma mark Constants

enum
{
	// raster states
	DONE = 0,			// invocation
	ESCFOUND = 1,		// found escape
	WANTCMD = 2,		// command character wanted
	WANTDEL = 3,		// first delimiter wanted
	IARG = 4,			// integer argument desired
	SARG = 5,			// string argument desired
	CARG = 6,			// count argument desired
	DATA = 7			// found '^' (therefore, all arguments parsed)
};

enum
{
	// commands
	ESCCMD = ' ',		// escape to next level of commands
	DELIM = ';',		// argument delimiter
	CMDTRM = '^',		// terminator, but also a prefix (with ESC)
	WINCMD = 'W',		// “create window” command
	DESCMD = 'D',		// “destroy window” command
	MAPCMD = 'M',		// “change color map entries” command
	RLECMD = 'R',		// “run-length encoded data”
	PIXCMD = 'P',		// “standard pixel data”
	IMPCMD = 'I',		// “IMCOMP compressed data”
	FILCMD = 'F',		// “save to file” command
	CLKCMD = 'C',		// “click the slide camera”
	SAVCMD = 'S'		// “save color map to file”
};

enum
{
	// command parameter types
	MAXARGS = 7,		// maximum arguments to any command
	INT = 1,			// integer argument type
	STRING = 2,			// string (character) argument
	COUNT = 3,			// data count argument
	LINEMAX = 1024		// longest width for a window
};

enum
{
	// flag values
	FL_NORMAL = 1,		// command needs no data
	FL_DATA = 2			// command takes data
};

enum
{
	// decoding states
	FRSKIP = 0,
	FRIN = 1,
	FRSPECIAL = 2,
	FROVER = 3,
	FRDONE = 4
};

enum
{
	// number of commands in "gCommandTable", below
	kRasterGraphicsCommandCount = 9
};

#pragma mark Types

typedef short (*RasterGraphicsCommandInterpreter)(union arg av[], char*);

struct RasterGraphicsCommand
{
	// command table
	SInt16								c_name;
	SInt16								c_flags;
	RasterGraphicsCommandInterpreter	c_func;
	SInt16								c_args[MAXARGS];
};

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	RasterGraphicsCommand	gCommandTable[] =
	{
		{ WINCMD, FL_NORMAL, VRwindow, { INT, INT, INT, INT, INT, STRING, 0 } },
		{ DESCMD, FL_NORMAL, VRdestroy, { STRING, 0, 0, 0, 0, 0, 0 } },
		{ MAPCMD, FL_DATA, VRmap, { INT, INT, COUNT, STRING, 0, 0, 0 } },
		{ FILCMD, FL_NORMAL, VRfile, { INT, INT, INT, INT, INT, STRING, STRING } },
		{ PIXCMD, FL_DATA, VRpixel, { INT, INT, INT, COUNT, STRING, 0, 0 } },
		{ RLECMD, FL_DATA, VRrle, { INT, INT, INT, COUNT, STRING, 0, 0 } },
		{ IMPCMD, FL_DATA, VRimp, { INT, INT, INT, COUNT, STRING, 0, 0 } },
		{ CLKCMD, FL_NORMAL, VRclick, { STRING, 0, 0, 0, 0, 0, 0 } },
		{ SAVCMD, FL_NORMAL, VRmsave, { STRING, STRING, 0, 0, 0, 0, 0 } }
	};
	struct VRwin	VRhead;				// linked list head
	short			VRcmdnum = -1;		// current command
	short			VRargcount = 0;		// number of args for command
	int				VRdatalen = 0;		// length of expected data
	short			VRbufpos = 0;		// current pointer in tempdata
	union arg		VRargs[MAXARGS];	// argument vector
	char			VRtempdata[256];	// temporary storage while parsing
	char*			VRspace = nullptr;	// storage for incoming data
	char*			VRsp2 = nullptr;	// storage for pixel expansion
	char*			VRptr = nullptr;	// pointer for data buffer
	char*			VRptr2 = nullptr;	// copy of above
	short			dstate = FRSKIP;
	short			dspec = 0;
}

#pragma mark Internal Method Prototypes

static short	getNextArgState		(short, short);



/*!
Initializes the interactive color raster
graphics system.  Returns 0 if this fails.

In MacTelnet 3.0, this routine internally
initializes the raster graphics screen
module.

(2.6)
*/
short
RasterGraphicsKernel_Init ()
{
	VRhead.w_next = nullptr;
	VRsp2 = (char*)Memory_NewPtr(INTEGER_QUADRUPLED(LINEMAX) + 10);
	VRspace = (char*)Memory_NewPtr(LINEMAX + 10);
	
	MacRGinit();
	return !!(VRspace);
}// Init


/*
** VRwrite -- parse a string of VR commands
** 
** Arguments:
**
**	char *b;	-- buffer pointer
**	short	len;	-- buffer length
**
** Returns:
**
**	short			-- Number of characters processed.  0 tells
**				-- the upper level to switch out of raster mode;
**				-- usually on error, but also at completion of
**				-- command processing.
**
*/
short
VRwrite		(char*		b,
			 short		len)
{
	short				result = 0;
	char*				p = b;
	char				c = '\0';
	register SInt16		i = 0;
	Boolean				final = false;
	static short		VRstate = DONE; // current state
	
	
	// parse until no more characters are left
	while ((result < len) && (!final))
	{
		c = *p;

		switch (VRstate)
		{
		case DONE:
			if (c == 0x1B/* ESC */)
			{
				VRstate = ESCFOUND;
			}
			else
			{
				final = true;
			}
			break;

		case ESCFOUND:
			if (c == CMDTRM)
			{
				VRstate = WANTCMD;
			}
			else
			{
				VRstate = DONE;
				final = true;
			}
			break;

		case WANTCMD:
			// look for a valid command character
			for (i = 0; i < kRasterGraphicsCommandCount; i++)
			{
				if (gCommandTable[i].c_name == c) break;
			}

			VRcmdnum = i;

			if (VRcmdnum == kRasterGraphicsCommandCount)
			{
				VRstate = DONE;
				result = 0;
				final = true;
			}
			else
			{
				// set up for this command
				VRargcount = 0;
				VRbufpos = 0;
				VRstate = WANTDEL;
			}
			break;

		case WANTDEL:
			// look for a beginning semicolon
			if (c == DELIM)
			{
				VRstate = getNextArgState(VRargcount, VRcmdnum);
			}
			else
			{
				VRstate = DONE;
			}
			break;

		case IARG:
			// look for an integer argument
			switch (c)
			{
			/*	we've found the end of the argument, so
				try to put into the vector for later use */
			case DELIM:
			case CMDTRM:
				VRtempdata[VRbufpos] = '\0';

				/*	copy into argument union */
				(int)CPP_STD::sscanf(VRtempdata,"%d",&VRargs[VRargcount].a_num);
				VRbufpos = 0;

				VRargcount++;
				if (c == DELIM)
				{
					VRstate = getNextArgState(VRargcount, VRcmdnum);
				}
				else
				{
					if (gCommandTable[VRcmdnum].c_flags & FL_DATA)
					{
						VRstate = DATA;
					}
					else
					{
						// run the command
						(*gCommandTable[VRcmdnum].c_func)(VRargs, (char*)nullptr);
						VRstate = DONE;
					}
				}
				break;

			/*	copy over characters for later */
			default:
				VRtempdata[VRbufpos++] = c;
				break;
			}
			break;

		case SARG:
			// look for a string argument
			switch (c)
			{
			/*	put string into argument vector */
			case DELIM:
			case CMDTRM:
				
				VRtempdata[VRbufpos] = '\0';
/*
				VRargs[VRargcount].a_ptr = Memory_NewPtr((unsigned)VRbufpos+1);
				
				if (VRargs[VRargcount].a_ptr == (char*)nullptr)
				{
					VRstate = DONE;
					break;
				}
*/

				(void)CPP_STD::strcpy(VRargs[VRargcount].a_ptr, VRtempdata);
				VRbufpos = 0;
				VRargcount++;
				if (c == DELIM)
				{
					VRstate = getNextArgState(VRargcount, VRcmdnum);
				}
				else
				{
					if (gCommandTable[VRcmdnum].c_flags & FL_DATA)
					{
						VRstate = DATA;
					}
					else
					{
						// run the command
						(*gCommandTable[VRcmdnum].c_func)(VRargs, (char*)nullptr);
						VRstate = DONE;
					}
				}
				break;

			/*	save string for later */
			default:
				VRtempdata[VRbufpos++] = c;
				break;
			}
			break;

		case CARG:
			// look for a count argument
			switch (c)
			{
			/*	we've found the end of the argument, so
				try to put into the vector for later use */
			case DELIM:
			case CMDTRM:
				VRtempdata[VRbufpos] = '\0';

				/*	copy into argument union */
				(int)CPP_STD::sscanf(VRtempdata,"%d",&VRdatalen);
				(int)CPP_STD::sscanf(VRtempdata,"%d",&VRargs[VRargcount].a_num);

				if (VRdatalen > LINEMAX)
					VRdatalen = LINEMAX;
				VRargcount++;
				VRbufpos = 0;

				if (c == DELIM)
				{
					VRstate = getNextArgState(VRargcount, VRcmdnum);
				}
				else
				{
					if (gCommandTable[VRcmdnum].c_flags & FL_DATA)
					{
						VRstate = DATA;
					}
					else
					{
						// run the command
						(*gCommandTable[VRcmdnum].c_func)(VRargs, (char*)nullptr);
						VRstate = DONE;
					}
				}
				
				/*	allocate storage for data */
				VRptr = VRspace;

				if (VRptr == (char*)nullptr)
				{
					VRstate = DONE;
					result = 0;
					final = true;
				}
				else
				{
					VRptr2 = VRptr;
					decode0();			/* reset decoder */
				}
				break;

			/*	copy over characters for later */
			default:
				VRtempdata[VRbufpos++] = c;
				break;
			}
			break;

		case DATA:
			// retrieve a line of data
			if (0 <= (i = decode1(c)))
			{
				*VRptr2++ = i;
				--VRdatalen;
			}

			if (!VRdatalen)
			{	/*	we've got all of the data */
				(*gCommandTable[VRcmdnum].c_func)(VRargs, VRptr);
				VRstate = DONE;
			}
			break;
		}
		
		unless (final)
		{
			p++;
			result++;
		}
	}
	
	return result;
}// VRwrite


/*
** VRwindow -- create a new raster window
**
** Arguments:
**
**	union arg av[];	-- the argument vector
**
**	short		av[0, 1]; 	-- upper left;
**	short		av[2, 3]; 	-- width, height
**	short		av[4];	 	-- window hardware display number
**	char	*av[5];   	-- title
**
** Returns:
**
**	None.  No provision has been made for error returns in any of
**	these routines because I don't know what to do if an error
**	occurs.  Perhaps a better man than I can figure out how to 
**	deal with this.
**	N.B -- these functions are declared as short, in the event
**	that an error return is added.
*/
short
VRwindow		(union arg		av[],
				 char*			UNUSED_ARGUMENT(unused))
{
	//UNUSED_ARG(unused)
	VRW*		w = VRhead.w_next;
	short		result = 0;
	
	
	// see if a suitable virtual device already exists
	while (w)
	{
		if (!CPP_STD::strcmp(w->w_name, av[5].a_ptr)
			&& w->w_width == av[2].a_num 
			&& w->w_height == av[3].a_num)		/* don't re-allocate win */
		{
			result = 1;
			break;
		}
		else
		{
			if (!CPP_STD::strcmp(w->w_name,av[5].a_ptr))		/* duplicate, different size */
				w->w_used = 1;
			w = w->w_next;
		}
	}
	
	if (!result)
	{
		w = (VRW*)Memory_NewPtr(sizeof(VRW));				/* new element for list */
		if (w != nullptr)
		{
			w->w_next = VRhead.w_next;					/* set next equal to current head */
			VRhead.w_next = w;							/* set head of list equal to me */
			
			/*
			** fill in the new window area
			*/
		
			w->w_left = av[0].a_num;
			w->w_top = av[1].a_num;
			w->w_width = av[2].a_num;
			w->w_height = av[3].a_num;
			w->w_display = av[4].a_num;
			w->w_used = 0;
			CPP_STD::strncpy(w->w_name, av[5].a_ptr, 100);
		
			if (w->w_width > LINEMAX)			/* have to be SOME limits */
				w->w_width = LINEMAX;
			
			if (0 <= (result = MacRGnewwindow( w->w_name, w->w_left, w->w_top,
							w->w_left+w->w_width, w->w_top+w->w_height )))
				MacRGsetwindow(result);
		
			w->w_rr.wn = result;
		}
		else result = -1;
	}
	
	return result;
}// VRwindow


/*!
Destroys an ICR window by name.

(2.6)
*/
short
VRdestroy	(union arg		av[],
			 char*			UNUSED_ARGUMENT(unused))
{
	//UNUSED_ARG(unused)
	VRW*	w = nullptr;
	VRW*	ow = &VRhead;
	
	
	w = ow->w_next;

	while (w != nullptr)
	{
		if (!CPP_STD::strcmp(w->w_name, av[0].a_ptr))
		{
			MacRGremove(w->w_rr.wn);
			ow->w_next = w->w_next;
			Memory_DisposePtr((Ptr*)&w);
		}
		else ow = ow->w_next;
		
		w = ow->w_next;
	}
	
	return 0;
}// VRdestroy


/*!
Destroys an ICR window by name.

(2.6)
*/
void
VRdestroybyName		(char*	name)
{
	union arg		temporaryArgs;
	
	
	CPP_STD::strncpy(temporaryArgs.a_ptr, name, 100);
	VRdestroy(&temporaryArgs, nullptr);
}// VRdestroybyName


/*
** VRmap -- take a color map command and set the palette
**
** Arguments:
**
**   union arg av[]; -- the argument vector
**
**   short 	av[0, 1];  -- start & length of map segment
**	 short	av[2];     -- count of data needed (info only)
**	 char	*av[3];    -- window name
**   char	*data;     -- pointer to the data  
**
** Returns:
**
**   None.
**
*/
short
VRmap		(union arg		av[],
			 char*			data)
{
	VRW*		w = nullptr;
	short		result = 0;
	
	
	w = VRlookup(av[3].a_ptr);

	if (w) result = MacRGmap(av[0].a_num, av[1].a_num, data);
	return result;
}// VRmap


/*
** VRpixel -- display a line of pixel data
** 
** Arugments:
**
**	union arg av[]; -- the argument vector
**
**	short		av[0];  -- x coordinate
**	short	 	av[1];  -- y coordinate
**  short     av[2];  -- pixel expansion factor
**	short	 	av[3];  -- length of data
**	char	*av[4]; -- window name
**	char	*data;  -- pointer to data
**
** Returns:
**
**	None.
**
*/
short
VRpixel		(union arg		av[],
			 char*			data)
{
	VRW*		w = VRlookup(av[4].a_ptr); // find the right window
	short		i = 0,
				lim = 0;
	char*		p = nullptr;
	char*		q = nullptr;
	short		result = 0;
	
	
	if (w != nullptr)
	{
		lim = av[3].a_num * av[2].a_num; // total number of expanded pixels
		if (lim > w->w_width) lim = w->w_width;
	
		if (av[2].a_num > 1)
		{
			p = data;
			q = VRsp2;
			for (i = 0; i < lim; i++)
			{
				*q++ = *p;
				unless ((i + 1) % av[2].a_num) p++;
			}
			for (i = 0; i < av[2].a_num; i++)
			{
				MacRGraster(VRsp2, av[0].a_num, av[1].a_num + i,
								 av[0].a_num + lim, av[1].a_num + i, lim);
			}
			result = 0;
		}
		else result = MacRGraster(data, av[0].a_num, av[1].a_num,
									av[0].a_num + av[3].a_num, av[1].a_num, lim);
	}
	
	return result;
}// VRpixel


/*
** VRimp -- display a line of IMPCOMP encoded data
**   One line of IMPCOMP data gives 4 lines output.
** 
** Arugments:
**
**	union arg av[]; -- the argument vector
**
**	short		av[0];  -- x coordinate
**	short	 	av[1];  -- y coordinate
**  short     av[2];  -- pixel expansion
**	short	 	av[3];  -- length of data
**	char	*av[4]; -- window name
**	char	*data;  -- pointer to data
**
** Returns:
**
**	None.
**
*/
short
VRimp		(union arg		av[],
			 char*			data)
{
	VRW*		w = VRlookup(av[4].a_ptr); // find the right window
	short		i = 0,
				lim = 0,
				j = 0;
	char*		p = nullptr;
	char*		q = nullptr;
	short		result = 0;
	
	
	if (w != nullptr)
	{
		// decompress
		unimcomp((UInt8*)data, (UInt8*)VRsp2, av[3].a_num, w->w_width);
		
		// this gives four lines in the VRsp2 buffer; now, pixel expand
		i = av[3].a_num;
	
		lim = i * av[2].a_num; // total number of expanded pixels on a line
		if (lim > w->w_width) lim = w->w_width;
		if (i > w->w_width) i = w->w_width;
		
		p = VRsp2; // from this buffer
		
		for (j = 0; j < 4; j++)
		{
			if (av[2].a_num > 1)
			{
				q = VRspace; // to here
				for (i = 0; i < lim; i++)
				{
					*q++ = *p;
					unless ((i + 1) % av[2].a_num) p++;
				}
				p++;
				for (i = 0; i < av[2].a_num; i++)
				{
					MacRGraster(VRspace,
								av[0].a_num,
								av[1].a_num + i + j * av[2].a_num,
								av[0].a_num + av[3].a_num,
								av[1].a_num + i + j * av[2].a_num,
								lim);
				}
			}
			else
			{
				MacRGraster(p,
							av[0].a_num,
							av[1].a_num + j,
							av[0].a_num + av[3].a_num,
							av[1].a_num + j, i);
				p += av[3].a_num; // increment to next line
			}
		}
		result = 0;
	}
	
	return result;
}// VRimp


/************************************************************************/
/*  Function	: unimcomp						*/
/*  Purpose	: 'Decompresses' the compressed image			*/
/*  Parameter	:							*/
/*    xdim       - x dimensions of image				*/
/*    lines      - number of lines of compressed image                  */
/*    in, out    - Input buffer and output buffer. Size of input buffer */
/*		   is xdim*lines. Size of output buffer is 4 times      */
/*		   that. It 'restores' images into seq-type files       */
/*  Returns  	: none							*/
/*  Called by   : External routines					*/
/*  Calls       : none							*/
/************************************************************************/
void
unimcomp	(UInt8		in[],
			 UInt8		out[],
			 short		xdim,
			 short		xmax)
{
  short				bitmap = 0,
  					temp = 0;
  register short 	i = 0,
  					j = 0,
					k = 0,
					x = 0;
  UInt8				hi_color = '\0',
  					lo_color = '\0';
	
	
	if (xmax < xdim)		/* don't go over */
		xdim = xmax;

  /* go over the compressed image */
    for (x=0; x<xdim; x=x+4)
    {
      k = x;
      hi_color = in[k+2]; 
      lo_color = in[k+3];

      bitmap = (in[k] << 8) | in[k+1];

      /* store in out buffer */
      for (i=0; i<4; i++)
      {
        temp = bitmap >> (3 - i)*4;
        for (j=x; j<(x+4); j++)
        {
 	  if ((temp & 8) == 8)
	    out[i*xdim+j] = hi_color;
	  else
	    out[i*xdim+j] = lo_color;
	  temp = temp << 1;
	}
      }
    } /* end of for x */
}// unimcomp


/*!
Decompresses run-length-encoded data.

(2.6)
*/
short
unrleit		(UInt8*			buf,
			 UInt8*			bufto,
			 short			inlen,
			 short			outlen)
{
	short				result = 0;
	register SInt16		cnt = 0;
	register UInt8*		p = nullptr;
	register UInt8*		q = nullptr;
	UInt8*				endp = nullptr;
	UInt8*				endq = nullptr;
	
	
	p = buf;
	endp = buf + inlen;
	q = bufto;
	endq = bufto + outlen;
	
	// continue until either p or q hits its end
	while (p < endp && q < endq)
	{
		cnt = *p++; // count field
		if (!(cnt & 128))
		{
			// is a set of uniques
			while (cnt-- && q < endq) *q++ = *p++; // copy unmodified
		}
		else
		{
			// a repeater of the form “frequency, character”
			cnt &= 127; // strip high bit
			while (cnt-- && q < endq) *q++ = *p; // generate repeats of the character
			p++; // skip over the character
		}
	}
	result = (short)(q - bufto);
	
	return result;
}// unrleit


/***************************************************************************/
/*
** VRrle -- display a line of run-length encoded data
** 
** Arugments:
**
**	union arg av[]; -- the argument vector
**
**	short		av[0];  -- x coordinate
**	short	 	av[1];  -- y coordinate
**  short     av[2];  -- pixel expansion
**	short	 	av[3];  -- length of data
**	char	*av[4]; -- window name
**	char	*data;  -- pointer to data
**
** Returns:
**
**	None.
**
*/
short	VRrle(union arg av[], char *data)
{
	VRW*		w = nullptr;
	short		i = 0,
				lim = 0;
	char*		p = nullptr;
	char*		q = nullptr;
	short		result = 0;
	
	
	// find the right window
	w = VRlookup(av[4].a_ptr);
	
	if (w != (VRW*)nullptr)
	{
		// decompress
		i = unrleit((UInt8*)data, (UInt8*)VRsp2, av[3].a_num, w->w_width);
		
		lim = i*av[2].a_num;				/* total number of expanded pixels */
		if (lim > w->w_width) lim = w->w_width;
		
		if (av[2].a_num > 1)
		{
			p = VRsp2;						/* from this buffer */
			q = VRspace;					/* to here */
			for (i = 0; i < lim; i++)
			{
				*q++ = *p;
				if (!((i+1) % av[2].a_num)) p++;
			}
			for (i = 0; i < av[2].a_num; i++)
			{
				MacRGraster(VRspace, av[0].a_num, av[1].a_num + i,
							av[0].a_num+lim, av[1].a_num+i, lim);
			}
		}
		else result = MacRGraster(VRsp2, av[0].a_num, av[1].a_num, av[0].a_num + i, av[1].a_num, i);
	}
	
	return result;
}// VRrle


/*
** VRfile	-- cause the named window to be dumped to a file
**
** Arguments:
**
**	union arg av[];		-- the argument vector
**
**	short		av[0];		-- start x coordinate
**	short		av[1];		-- start y coordinate
**	short		av[2];		-- width of region to dump
**	short		av[3];		-- height of region to dump
**	short		av[4];		-- format of file -- machine dependent
**	char	*av[5];		-- file name to use
**	char	*av[6];		-- window name to dump
**
** Returns:
**
**	short;				-- 1 if success, 0 if failure
**
*/
short
VRfile	(union arg		av[],
		 char*			UNUSED_ARGUMENT(unused))
{
#ifdef IMPLEMENTED
	#pragma unused (unused)
	VRW*	w = VRlookup(av[6].a_ptr); // look up the window
	short	result = 0;

	
	if (w != nullptr)
	{
		// call the low-level file routine
		result = RRfile(w, av[0].a_num, av[1].a_num, av[2].a_num, av[3].a_num,
						av[4].a_num, av[5].a_ptr);
	}
	return result;
#else 
	//UNUSED_ARG(av)
	//UNUSED_ARG(unused)
	return 0;
#endif
}// VRfile


/*
** VRclick	-- Click the Slide Camera -- very machine dependent
**
** Arguments:
**
**	union arg	av[];		-- the argument vector
**
**	char	*av[0];			-- window name
**
** Returns:
**
**	short;					-- 1 if success, 0 if failure
*/
short
VRclick		(union arg		av[],
			 char*			UNUSED_ARGUMENT(unused))
{
#ifdef IMPLEMENTED
	#pragma unused (unused)
	VRW*	w = VRlookup(av[0].a_ptr); // look up the window
	short	result = 0;
	
	
	if (w != nullptr)
	{
		result = RRclick(w);
	}
	return result;
#else 
	//UNUSED_ARG(av)
	//UNUSED_ARG(unused)
	return 0;
#endif
}// VRclick


/*
** VRmsave	-- Save the named colormap to the file -- saves the whole thing
**
** Arguments:
**
**	union arg	av[];		-- the argument vector
**
**	char	*av[0];			-- file name
**	char	*av[1];			-- window name
**
** Returns:
**
**	short;					-- 1 if success, 0 if failure
*/
short
VRmsave		(union arg		av[],
			 char*			UNUSED_ARGUMENT(unused))
{
#ifdef IMPLEMENTED
	#pragma unused (unused)
	VRW*	w = nullptr;
	
	
	/*	look up the window */
	w = VRlookup(av[1].a_ptr);

	if (w == (VRW*)nullptr)
		return 0;

	return RRmsave(w, av[0].a_ptr);
#else 
	//UNUSED_ARG(av)
	//UNUSED_ARG(unused)
	return 0;
#endif
}// VRmsave


/*
** VRlookup			-- find an entry in the list by name
**
** Arguments:
**
**	char	*name;	-- the name of the window
**
** Returns:
**
**	VRW	*w;			-- pointer to window structure, (VRW*)0 if not found
**
*/
VRW*
VRlookup	(char*	name)
{
	VRW*		w = VRhead.w_next;
	
	
	while (w)
	{
		if (!CPP_STD::strcmp(w->w_name, name) && !w->w_used)
		{	/* same name, not old dup */
			if (w->w_rr.wn < 0)							/* maybe window don't work */
				return(nullptr);
			MacRGsetwindow(w->w_rr.wn);
			return(w);
		}
		w = w->w_next;
	}

	return(nullptr);
}// VRlookup


/*!
Removes all windows from the screen.  Returns
1 if successful.

(2.6)
*/
short
VRcleanup ()
{
	VRW*	w = VRhead.w_next;
	VRW*	x = nullptr;
	
	
	while (w != nullptr)
	{
		x = w->w_next;
		MacRGremove(w->w_rr.wn);
		w = x;
	}
	
	return 1;
}// VRcleanup


/*!
Initializes the decoding state.

(2.6)
*/
void
decode0 ()
{
	dstate = FRIN;
}// decode0


/*!
Handles the special ASCII printable character
encoding of data bytes.  Continues decoding
begun by decode0().  Returns real characters,
or 0 if in the middle of an escape sequence.

Decodes as follows:
	123 precedes #'s 0-63 
	124 precedes #'s 64-127
	125 precedes #'s 128-191
	126 precedes #'s 192-255
	overall:	realchar = (specialchar - 123) * 64 + (char - 32)
				specialchar = r div 64 + 123
				char = r mod 64 + 32

(2.6)
*/
short
decode1		(char 	c)
{
	short	result = 0;
	
	
	switch (dstate)
	{
	case FRSKIP:
		result = -1;
		break;

	case FRIN: // decoding
		if (c > 31 && c < 123) result = c;
		else
		{
			// doing special character
			dspec = c; // save special character
			dstate = FRSPECIAL;
			result = -1;
		}
		break;
		
	case FRSPECIAL:
		switch (dspec)
		{
			case 123:
			case 124:
			case 125:
			case 126: // encoded character
				dstate = FRIN;
				result = (((dspec - 123) << 6) - 32 + c);
				break;
				
			default: // mistaken character in stream
				dstate = FRIN; // assume it’s not special...
				result = decode1(c); // ...but be sure!
				break;
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// decode1


#pragma mark Internal Methods

/*!
Returns the next state based on the current one.

(2.6)
*/
static short
getNextArgState		(short		inArgumentCount,
					 short		inCommandIndex)
{
	short		result = DONE;
	
	
	switch (gCommandTable[inCommandIndex].c_args[inArgumentCount])
	{
	case INT:
		result = IARG;
		break;
	
	case STRING:
		result = SARG;
		break;
	
	case COUNT:
		result = CARG;
		break;
	
	case 0:
		result = DATA;
		break;
	
	default:
		break;
	}
	return result;
}// getNextArgState

// BELOW IS REQUIRED NEWLINE TO END FILE
