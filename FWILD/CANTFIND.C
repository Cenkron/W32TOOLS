/* ----------------------------------------------------------------------- *\
|
|	cantfind () - Report unfindable file
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
cantfind (
	const char *fnp)	/* Input file name */

	{
	fprintf(stderr, "Unable to find file: %s\n", fnp);
	}

/* ----------------------------------------------------------------------- */
