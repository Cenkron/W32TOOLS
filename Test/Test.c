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
|				   10-Sep-23 Compatibility with ne fnreduce.c
|
|	    int			Returns TRUE if the string "s" is the
|	bool = fnchkfil (s);	name of an existing disk file
|	    char  *s;		Pointer to the string
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

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	int						/* Return TRUE if the file exists */
fnchkfil (					/* Verify the existence of a disk file */
	char  *s)				/* Pointer to the filename string */

	{
	int      result;			/* The returned result */
	DTA_BLK  dta;				/* The DTA_BLK for _findf() */
	char     temp [MAX_PATH];	/* Copy of the input string */


	if ((strlen(s) >= MAX_PATH)
	||  (strpbrk(s, "?*")))		/* Wild is disallowed */
		return (FALSE);

	strcpy(temp, s);			/* Perform reduction */
	if (fnreduce(temp) < 0)
		return (FALSE);

	/* Search for the file */

	result = (_findf(&dta, &temp[0], (ATT_HIDDEN | ATT_SYSTEM)) == 0);
	if (result)
		_findc(&dta);
	return (result);
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
