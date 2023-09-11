/* ----------------------------------------------------------------------- *\
|
|				     FWSIZE
|			   Wild Card File Name Server
|				Return File Size
|			  for Lattice C on the IBM PC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   20-May-90
|				   17-Aug-97
|				   27-Sep-07
|
|	    UINT64		Return the (64 bit) file size
|	size = fwsize(	Get the size of the current fwild filename
|	    void  *hp)	Pointer to the fwild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ------------------------------------------------------------------------ */
	UINT64				/* Return the file size longword */
fwsize (				/* Get the size of the current filename */
    DTA_HDR  *hp)		/* Pointer to the DTA header */

	{
	return  hp->f_size;
	}

/* ------------------------------------------------------------------------ */
