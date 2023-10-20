/* ----------------------------------------------------------------------- *\
|
|				   FNCHKFIL
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   20-Oct-93
|				   17-Dec-94
|				   17-Aug-97
|				   30-Sep-23 (Major overhaul)
|
|		int			Returns TRUE if the string "s" is the
|	fnchkfil (		  name of an existing disk file
|		char  *s)	Pointer to the string
|
|	The file will be reported existing even if it is hidden or system.
|	The path/filename must not be wild.
|	The test avoids being fooled by path joining utilities.
|
|	NOTE: Returns TRUE for a Novell directory
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	int						/* Return TRUE if the file exists */
fnchkfil (					/* Verify the existence of a disk file */
	const char  *s)			/* Pointer to the filename string */

	{
	if (s == NULL)			// NULL is not a file
		return (FALSE);

	if (strpbrk(s, "?*"))	// Wild is disallowed
		return (FALSE);

	return (isFile(s));		// Test the file
	}

/* ----------------------------------------------------------------------- */
#ifdef  TEST
main (
	int    argc,
	char  *argv [])

	{
	char  *p;

	if (argc > 1)
		p = *++argv;
	else
		p = "";
	if (fnchkfil(p))
		printf("TRUE\n");
	else
		printf("FALSE\n");
	}
#endif
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
