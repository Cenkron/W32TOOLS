/* ----------------------------------------------------------------------- *\
|
|				    GETPATH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    char
|	ch = getpath();		Return the DOS system path character
|
\* ----------------------------------------------------------------------- */

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	char				/* Return the DOS path character */
getpath (void)			/* Determine the correct path character */

	{
	return ('\\');
	}

/* ----------------------------------------------------------------------- */
