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

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	void
cantopen (
	const char *fnp)	/* Input file name */

	{
	fprintf(stderr, "Unable to open input file: %s\n", fnp);
	}

/* ----------------------------------------------------------------------- */
