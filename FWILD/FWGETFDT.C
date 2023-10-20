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
|	    time_t		Return the UNIX timedate
|	dt = fwgetfdt(	Get the timedate of the current fWild file
|	    void *hp)	Pointer to the fWild header
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
	time_t				// Return the UNIX timedate word, or 0 if error
fwgetfdt (hp)			// Get the UNIX timedate of the current file
	FW_HDR  *hp;		// Pointer to the DTA header

	{
	return  (fwActive(hp) ? (hp->file_fdt) : ((time_t)(0)));
	}

/* ------------------------------------------------------------------------ */
