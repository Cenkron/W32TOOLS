/* ----------------------------------------------------------------------- *\
|
|				   FATALERR
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	fatalerr (s);	Prints the message pointed by "s" and exit(1).
|	    char  *s;		Pointer to message string (no '\n')
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
    void
fatalerr (s)
    char  *s;

    {
    fprintf(stderr, "\7%s\n", s);
    exit(1);
    }

/* ----------------------------------------------------------------------- */
