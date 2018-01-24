/* ----------------------------------------------------------------------- *\
|
|	cantopen () - Report unopenable file
|
|	Copyright (c) 1987, 1990, all rights reserved, Brian W Johnson
|
|	16-Jun-90
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
    void
cantopen (char *fnp)			/* Input file name */

    {
    printf("Unable to open input file: %s\n", fnp);
    }

/* ----------------------------------------------------------------------- */
