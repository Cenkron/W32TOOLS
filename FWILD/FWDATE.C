/* ----------------------------------------------------------------------- *\
|
|				     FWDATE
|			   Wild Card File Name Server
|			    Return File Date String
|			  for Lattice C on the IBM PC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   28-May-90
|				    9-May-91
|				   17-Dec-94
|				   17-Aug-97
|				    9-Aug-98 (Convert to system calls)
|
|	    char *		Return a pointer to a date string
|	p = fwdate(		Get the date of the current fWild filename
|	    void *hp)	Pointer to the fWild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <time.h>

#define  FWILD_INTERNAL

#include  "fWild.h"


static	char	date [11];	/* The returned date string */

static	char   *month [] =	/* The month string lookup table */
	{
	"",    "Jan", "Feb", "Mar",
	"Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov",
	"Dec", "",    "",    ""
	};

/* ------------------------------------------------------------------------ */
	char *				/* Return a pointer to a date string */
fwdate (hp)				/* Get the date of the current filename */
	FW_HDR   *hp;		/* Pointer to the DTA header */

	{
	int        year;		/* The year number */
	struct tm *ptm;			/* Ptr to the struct tm */
	time_t	   fdt;			// The file datetime

	
	if ((! fwActive(hp))  ||  ((fdt = hp->file_fdt) == 0))
		strcpy(&date[0], " --------");
	else
		{
		ptm  = localtime(&hp->file_fdt);
		year = ptm->tm_year;
		if (year >= 100)
			year -= 100;

		sprintf(date, "%d-%s-%02d", ptm->tm_mday, month[ptm->tm_mon + 1], year);
		}

	return  (&date[0]);
	}

/* ------------------------------------------------------------------------ */
