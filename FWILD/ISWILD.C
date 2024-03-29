/* ----------------------------------------------------------------------- *\
|
|				    ISWILD
|
|		    Copyright (c) 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   24-Sep-23 (protect against NULL ptr)
|
\* ----------------------------------------------------------------------- */

#include  <string.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- *\
|  Test a pathname for wildness
\* ----------------------------------------------------------------------- */
	int
isWild (
	const char  *s)

	{
	int result = FALSE;

	if (s)
		result = ((strchr(s, '?') || strchr(s, '*')));
	return (result);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
