/* ----------------------------------------------------------------------- *\
|
|				    FMALLOC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    char *		Return pointer to the allocated memory
|	s = fmalloc (n);	Same as malloc(), except on failure an
|				error message is printed, and exit(1).
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
    char *
fmalloc (n)			/* Get a string buffer, of length n */
    unsigned int  n;		/* Call for a fatal error on failure */

    {
    char  *s;

    if ((s = malloc(n)) == NULL)
	fatalerr("Memory allocation failure");
    return (s);
    }

/* ----------------------------------------------------------------------- */
