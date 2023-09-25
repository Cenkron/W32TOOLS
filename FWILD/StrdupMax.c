/* ----------------------------------------------------------------------- *\
|
|							     strdupMax
|
|			    Copyright (c) 2023, all rights reserved
|							Brian W Johnson
|							   21-Sep-23
|
|			char *			// Returned pointer to the duplicated pathname
|		strdupMax(			// (must be freed by the caller)
|			char* s)		// Pointer to the allocated pathname string
|
|	Like str4dup() but always allocates MAX_PATH.
|	The returned pointer must be freed by the caller.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <malloc.h>
#include  <string.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	char *					// Returned pointer to the duplicated pathname
strdupMax(					// (must be freed by the caller)
	const char* s)			// Pointer to the pathname string

	{
	char *p = malloc(MAX_PATH);

	if (p)	
		strcpy_s(p, MAX_PATH, s);
	return (p);
	}
	
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
