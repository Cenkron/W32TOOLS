/* ------------------------------------------------------------------------ *\
|
	|						    FNDOT
|					  for Lattice C on the IBM PC
|
|			    	Copyright (c) 1985, 1990, all rights reserved
|						Brian W Johnson
|						26-May-90
|					     1-Aug-06 Do not reject all ".*"
|						
|
|	    int				Returns TRUE if s is a "." or ".." filename
|	fndot(
|	    char  *s)		Pointer to the [drive:path/]filename
|
|	Returns TRUE if s is a pathname whose last component is "." or ".."
|
\* ------------------------------------------------------------------------ */

#include  <stdio.h>
#include  <string.h>

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
	int
fndot (					/* Return TRUE if a dot or dotdot directory name */
	const char  *s)		/* Pointer to the filespec string */

	{
	const char  *p = _fntail(s);

	return ((strcmp(p, ".") == 0)  ||  (strcmp(p, "..") == 0));
	}

/* ------------------------------------------------------------------------ */
