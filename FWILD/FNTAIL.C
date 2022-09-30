/* ----------------------------------------------------------------------- *\
|
|				    FNTAIL
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    char *		Return pointer to the tail of s
|	p = fntail (s);		Point the tail of a path
|	    char  *s		Pointer to the pathname
|
|	Returns a pointer to the last component of path s
|	The path delimiter is not returned as part of the string.
|	If there is no last component, the pointer points the terminating NUL.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	char *
fntail (				/* Return pointer to the last component of path s */
    char  *s)			/* Pointer to the path string */

	{
	char  *p;			/* Temporary pointer into the path string */

	s = PointPastPrefix(s, TRUE);	/* Skip over the path prefix (drive spec) */

				/* Skip over all path delimiters */

	while ((p = strchr(s, '/')) || (p = strchr(s, '\\')))
		s = p + 1;

	return (s);
	}

/* ----------------------------------------------------------------------- */
