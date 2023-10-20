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
|	type = fwtype(	Get the type of the current fWild filename
|	    void *hp)	Pointer to the fWild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
	int					/* Return the file type byte */
fwtype (				/* Get the type of the current filename */
	FW_HDR  *hp)		/* Pointer to the DTA header */

	{
	return  (fwActive(hp) ? ((int)(hp->file_type)) : 0);
	}

/* ------------------------------------------------------------------------ */
	int					/* Return the file type byte */
fwdepth (				/* Get the directory depth of the current filename */
	FW_HDR  *hp)		/* Pointer to the DTA header */

	{
	return  (fwActive(hp) ? ((int)(hp->xItem.depth)) : 0);
	}

/* ------------------------------------------------------------------------ */
