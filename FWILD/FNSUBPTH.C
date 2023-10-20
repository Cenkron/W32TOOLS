/* ----------------------------------------------------------------------- *\
|
|				   FNSUBPTH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    int				Return TRUE if s1 is subpath of s2
|	bool = fnsubpth(s1, s2);	Check for filename subpath
|	    char  *s1;			Pointer to pathname
|	    char  *s2;			Pointer to pathname
|
|	Returns TRUE if the path "s1" is a subpath of path "s2".
|	The strings must be similar in form; for instance, both absolute.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fWild.h"

#define	  ispath(ch)	(((ch) == '/') || ((ch) == '\\'))

/* ----------------------------------------------------------------------- */
	int
fnsubpth (				/* Check if s1 is a subpath of s2 */
	const char  *s1,	/* Pointer to the filename sub-path */
	const char  *s2)	/* Pointer to the filename super-path */

	{
	int  result = TRUE;	/* Default result for identical strings */

	for (;;)
		{
		if (*s1 != *s2)
			{
			result = ((*s1 == '\0') && (ispath(*s2) || (*s2 == '\0')));
			break;
			}
		if (*s1 == '\0')
			break;
		++s1;
		++s2;
		}
	return (result);
	}

/* ----------------------------------------------------------------------- */
