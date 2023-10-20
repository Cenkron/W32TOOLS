/* ----------------------------------------------------------------------- *\
|
|	fwInitError () - Report FWinit failure
|
|	Copyright (c) 1987, 1990, all rights reserved, Brian W Johnson
|
|	10-Sep-23
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	void
fwInitError (
	const char *fnp)	/* Input file name */

	{
	char *pErrMsg = fwerror();

	fprintf(stderr, "%s\"  -  %s\n", fnp, pErrMsg);
	exit(5);
	}

/* ----------------------------------------------------------------------- */
