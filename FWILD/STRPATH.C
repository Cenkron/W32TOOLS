/* ----------------------------------------------------------------------- *\
|
|				    STRPATH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    char		Return the appropriate path character for
|	ch = strpath (s);	parsing the string "s"
|	    char  *s;		Pointer to the pathname
|
|	If the string contains any path characters, the first one is
|	returned, else the current environment switch character is checked,
|	else DOS is checked, to set an appropriate path character.
|
\* ----------------------------------------------------------------------- */

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	char				/* Return the path character */
strpath (				/* Determine the correct path character */
	char  *s)			/* Pointer to the tested string */

	{
	char  ch;

	for ( ; ch = *s; ++s)
		{
		if ((ch == '/') || (ch == '\\'))
			return (ch);
		}
	return (egetpath());
	}

/* ----------------------------------------------------------------------- */
