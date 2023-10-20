/* ----------------------------------------------------------------------- *\
|
|	usage () - Report usage error
|
|	Copyright (c) 1987, 1990, all rights reserved, Brian W Johnson
|
|	17-Jun-90
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	void
usage (void)

	{
	printf(usagedoc[0],
	optswch, optswch, optswch, optswch, optswch, optswch);
	putchar('\n');
	exit(2);
	}

/* ----------------------------------------------------------------------- */
