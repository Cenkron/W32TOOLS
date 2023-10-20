/* ----------------------------------------------------------------------- *\
|
|						    FMALLOC
|
|			    Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|					   26-May-90
|					   21-Oct-23 (changed to calloc())
|
|		char *		Return pointer to the allocated memory
|	fmalloc (		Allocate memory buffer, length n
|		unsigned n)	Length of the buffer
|
|	Same as malloc(), except on failure an error message is printed,
|	and the programs performs exit(1)
|	Internals changed to calloc() for better leak detection.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	char *
fmalloc (					/* Get a string buffer, of length n */
	unsigned int n)			/* Call for a fatal error on failure */

	{
	char  *s;

	if ((s = calloc(n, 1)) == NULL)
		fatalerr("Memory allocation failure");
	return (s);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
