/* ----------------------------------------------------------------------- *\
|
|				     FWTYPE
|			   Wild Card File Name Server
|				Return File Type
|			  for Lattice C on the IBM PC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   20-May-90
|				   17-Aug-97
|
|	    int			Return the file type byte (ATT_*)
|	type = fwtype(	Get the type of the current fwild filename
|	    void *hp)	Pointer to the fwild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ------------------------------------------------------------------------ */
	int					/* Return the file type byte */
fwtype (				/* Get the type of the current filename */
	DTA_HDR  *hp)		/* Pointer to the DTA header */

	{
	return  (int)(hp->f_type);
	}

/* ------------------------------------------------------------------------ */
