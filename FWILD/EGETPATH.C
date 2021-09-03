/* ----------------------------------------------------------------------- *\
|
|				    EGETPATH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   20-May-90
|
|	    char
|	ch = egetpath();	Return the current program path character
|
\* ----------------------------------------------------------------------- */

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	char			/* Return the program path character */
egetpath (void)		/* Determine the correct path character */

	{
	return ('\\');
	}

/* ----------------------------------------------------------------------- */
