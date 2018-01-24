/* ----------------------------------------------------------------------- *\
|
|				    FWGETFDT
|			   Wild Card File Name Server
|		   Return UNIX file timedate for the current file
|			  for Microsoft C on the IBM PC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   19-Aug-97
|
|	    long		Return the long UNIX timedate
|	dt = fwgetfdt (hp);	Get the timedate of the current fwild file
|	    char  *hp;		Pointer to the fwild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ------------------------------------------------------------------------ */
    long			/* Return the long UNIX timedate word */
fwgetfdt (hp)			/* Get the UNIX timedate of the current file */
    DTA_HDR  *hp;		/* Pointer to the DTA header */

    {
    return (hp->link->fdt);
    }

/* ------------------------------------------------------------------------ */
