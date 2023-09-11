/* ----------------------------------------------------------------------- *\
|
|  fwfirst.c
|
|  This function uses the fwild system to return a pointer to the first
|  filename found matching the supplied fwild pattern name.
|
|  The fwild system is gracefully terminated with all memory freed.
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	16-Jan-93	Copyright (c) 1993, Brian W. Johnson,
|			All rights reserved
|	17-Dec-94	Buffer increased
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <fwild.h>

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

static  char    buffer [1024] = {0};      /* The filename buffer */

/* ----------------------------------------------------------------------- *\
|  fwfirst () - Find the first filename matching the supplied fwild string
\* ----------------------------------------------------------------------- */
	char  *				/* Return pointer to matching string */
fwfirst (
	char  *s)   		/* Pointer to the time/date search filename */

	{
	void  *hp;  		/* Pointer to the wild file data block */
	char  *p;	  		/* Pointer to the found matching string */

	if ((hp = fwinit(s, FW_FILE)) == NULL)
		fwinitError(s);
	if ((p = fwild(hp)) != NULL)
		{
		strncpy(buffer, p, min(strlen(p), (sizeof(buffer) - 1)));
		p = buffer;
		}

	hp = NULL;
	return (p);
	}

/* ----------------------------------------------------------------------- */
