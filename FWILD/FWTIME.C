/* ----------------------------------------------------------------------- *\
|
|				     FWTIME
|			   Wild Card File Name Server
|			    Return File Time String
|			  for Lattice C on the IBM PC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   28-May-90
|				   17-Dec-94
|				   17-Aug-97
|				    9-Aug-98 (Convert to system calls)
|				    9-Jan-04 (display midnight hour as 12AM)
|
|	    char *		Return a pointer to a time string
|	p = fwtime(		Get the time of the current fWild filename
|	    void *hp)	Pointer to the fWild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <time.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

static	char	timeb [8];	/* The returned time string */

/* ------------------------------------------------------------------------ */
	char *				/* Return a pointer to a time string */
fwtime (				/* Get the time of the current filename */
	FW_HDR  *hp)		/* Pointer to the DTA header */

	{
	int    hour;		/* The corrected hour */
	char   ampm;		/* The am/pm symbol */
	struct tm *ptm;		/* Ptr to the struct tm */


	if ( ! fwActive(hp)  ||  (hp->file_fdt == 0))
		strcpy(&timeb[0], " -----");
	else
		{
		ptm  = localtime(&hp->file_fdt);
		hour = ptm->tm_hour;

		if (hour >= 12)
			{
			if (hour >  12)
				hour -= 12;
			ampm = 'p';
			}
		else
			{
			if (hour ==  0)
				hour =  12;
			ampm = 'a';
			}

		sprintf(timeb, "%2d:%02d%c", hour, ptm->tm_min, ampm);
		}

	return  (&timeb[0]);
	}

/* ------------------------------------------------------------------------ */
