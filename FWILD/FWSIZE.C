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
|	size = fwsize(	Get the size of the current fWild filename
|	    void  *hp)	Pointer to the fWild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
	UINT64				/* Return the file size longword */
fwsize (				/* Get the size of the current filename */
    FW_HDR  *hp)		/* Pointer to the DTA header */

	{
	return  (fwActive(hp) ? (hp->file_size) : (0));
	}

/* ------------------------------------------------------------------------ */
