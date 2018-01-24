/* ----------------------------------------------------------------------- *\
|
|	docprint () - Print a list of text lines
|
|	Copyright (c) 1987, 1990, all rights reserved, Brian W Johnson
|
|	17-Jun-90
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
    void
docprint (char *dp[])		/* Help documentation array pointer */

    {
    while (*dp)
	{
	printf(*(dp++), optswch, optswch, optswch, optswch, optswch, optswch);
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- */
