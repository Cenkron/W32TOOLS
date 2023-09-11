/* ----------------------------------------------------------------------- *\
|
|	fwinitError () - Report FWinit failure
|
|	Copyright (c) 1987, 1990, all rights reserved, Brian W Johnson
|
|	10-Sep-23
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	void
fwinitError (
	char *fnp)			/* Input file name */

	{
	printf("Invalid filespec: %s\n", fnp);
	exit(5);
	}

/* ----------------------------------------------------------------------- */
