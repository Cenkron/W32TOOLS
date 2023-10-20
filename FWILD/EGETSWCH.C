/* ----------------------------------------------------------------------- *\
|
|		       Get the Switch Character Function
|			  (considers the environment)
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    char
|	ch = egetswch ();	Get the environment switch character
|
\* ----------------------------------------------------------------------- */

#include  <stdlib.h>
#include  <ctype.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	char
egetswch (void)				/* Get the switch character */

	{
	char  *p;
	char   ch = '-';

	if (p = getenv("SWCH"))
		{
		while (*p)
			{
			if ( ! isspace(*p))
				{
				ch = *p;
				break;
				}
			++p;
			}
		}

	return (ch);
	}

/* ------------------------------------------------------------------------ */
