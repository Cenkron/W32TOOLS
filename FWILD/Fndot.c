/* ----------------------------------------------------------------------- *\
|
|				    FNDOT
|			  for Lattice C on the IBM PC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				    1-Aug-06 Do not reject all ".*"
|
|	    int
|	bool = fndot(s);	Returns TRUE if s is a "." or ".." filename
|	    char  *s;		Pointer to the [drive:path/]filename
|
|	Returns TRUE if s is a pathname whose last component is "." or ".."
|
\* ----------------------------------------------------------------------- */

#include  <string.h>
#include  "fwild.h"

/* ------------------------------------------------------------------------ */
    int
fndot (s)		/* Return TRUE if a dot or dotdot directory name */
    char  *s;		/* Pointer to the filename string */

    {
    char   ch;
    char  *p = s;

    for (p = s; ch = *p; )  /* Point the last component */
	{
	++p;
	if ((ch == ':')
	||  (ch == '/')
	||  (ch == '\\'))
	    s = p;
	}

    return ((strcmp(s, ".") == 0)  ||  (strcmp(s, "..") == 0));
    }

/* ------------------------------------------------------------------------ */
